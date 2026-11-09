import { env } from "cloudflare:workers";

// Anonymous, consented diagnostics only. Mint a short-lived capability scoped
// to one immutable report and its digest. No report-read/list route is exposed.
const maximumBytes = 12288;
const retentionSeconds = 90 * 86400;
const json = (value: unknown, status = 200) => Response.json(value, { status, headers: { "Cache-Control": "no-store" } });
const hex = (buffer: ArrayBuffer) => Array.from(new Uint8Array(buffer), b => b.toString(16).padStart(2, "0")).join("");
const digest = async (text: string) => hex(await crypto.subtle.digest("SHA-256", new TextEncoder().encode(text)));
async function boundedBody(request: Request) {
  if (!request.body) throw new Error("empty");
  const reader = request.body.getReader();
  const chunks: Uint8Array[] = []; let total = 0;
  for (;;) {
    const { done, value } = await reader.read(); if (done) break;
    total += value.length;
    if (total > maximumBytes) { await reader.cancel(); throw new Error("large"); }
    chunks.push(value);
  }
  const bytes = new Uint8Array(total); let offset = 0;
  for (const part of chunks) { bytes.set(part, offset); offset += part.length; }
  return new TextDecoder("utf-8", { fatal: true }).decode(bytes);
}
export async function POST(request: Request) {
  const db = env.DB;
  if (!db) return json({ error: "unavailable" }, 503);
  try {
    const now = Math.floor(Date.now() / 1000);
    const raw = await boundedBody(request);
    const body = JSON.parse(raw);
    const path = new URL(request.url).pathname;
    if (path.endsWith("/tickets")) {
      if (!/^[a-f0-9]{32}$/.test(body.id) || !/^[a-f0-9]{64}$/.test(body.hash)) return json({ error: "invalid" }, 400);
      // Store no raw address. The platform supplies CF-Connecting-IP; fail closed
      // if absent. Per-hour hash expires with the rate bucket.
      const ip = request.headers.get("cf-connecting-ip");
      if (!ip) return json({ error: "client unavailable" }, 503);
      const bucket = await digest(`${Math.floor(now / 3600)}:${ip}`);
      await db.batch([
        db.prepare("DELETE FROM quotas WHERE expires < ?").bind(now),
        db.prepare("DELETE FROM tickets WHERE expires < ?").bind(now),
        db.prepare("DELETE FROM reports WHERE received_at < ?").bind(now - retentionSeconds),
      ]);
      const quota = await db.prepare("INSERT INTO quotas (key,count,expires) VALUES (?,1,?) ON CONFLICT(key) DO UPDATE SET count=count+1 RETURNING count").bind(bucket, now + 3600).first<{count:number}>();
      if (!quota || quota.count > 10) return json({ error: "Try again in an hour" }, 429);
      const size = await db.prepare("SELECT (SELECT count(*) FROM reports) AS reports, (SELECT count(*) FROM tickets) AS tickets").first<{reports:number;tickets:number}>();
      if (!size || size.reports >= 1000 || size.tickets >= 1000) return json({ error: "Inbox full" }, 503);
      const token = hex(crypto.getRandomValues(new Uint8Array(32)).buffer);
      await db.prepare("INSERT INTO tickets (token_hash,report_id,hash,expires) VALUES (?,?,?,?)").bind(await digest(token), body.id, body.hash, now + 300).run();
      return json({ token });
    }
    if (!path.endsWith("/reports")) return json({ error: "not found" }, 404);
    const token = request.headers.get("authorization")?.replace(/^Bearer /, "") || "";
    if (!/^[a-f0-9]{64}$/.test(token)) return json({ error: "unauthorized" }, 401);
    const hash = await digest(raw);
    const ticket = await db.prepare("SELECT report_id,hash FROM tickets WHERE token_hash=? AND expires>=?").bind(await digest(token), now).first<{report_id:string;hash:string}>();
    if (!ticket || ticket.report_id !== body.id || ticket.hash !== hash) return json({ error: "unauthorized" }, 401);
    if (body.schema !== 1 || !/^[a-f0-9]{32}$/.test(body.id) || typeof body.context !== "string" || body.context.length > 2048 ||
        typeof body.firmware !== "string" || !/^\d+\.\d+\.\d+$/.test(body.firmware) || !Array.isArray(body.events) || body.events.length > 20 ||
        !body.events.every((e: {at: unknown;message:unknown}) => Number.isSafeInteger(e.at) && Number(e.at) >= 0 && typeof e.message === "string" && e.message.length <= 256)) return json({ error: "invalid report" }, 400);
    // Idempotent after a lost HTTP response; a report ID never overwrites data.
    await db.prepare("INSERT OR IGNORE INTO reports (id,hash,received_at,body) VALUES (?,?,?,?)").bind(body.id, hash, now, raw).run();
    const saved = await db.prepare("SELECT hash FROM reports WHERE id=?").bind(body.id).first<{hash:string}>();
    if (saved?.hash !== hash) return json({ error: "ID conflict" }, 409);
    await db.prepare("DELETE FROM tickets WHERE token_hash=?").bind(await digest(token)).run();
    return json({ id: body.id, received: true }, 201);
  } catch { return json({ error: "Invalid report or service unavailable" }, 400); }
}
