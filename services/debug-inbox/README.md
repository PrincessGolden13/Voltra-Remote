# Diagnostic intake source

`route.ts` is installed at both `app/api/tickets/route.ts` and
`app/api/reports/route.ts` in the Sites Vinext project. `schema.ts` is its
`db/schema.ts`. D1 binding: `DB`. No R2 or runtime secrets are required.

The hosted project's source repository contains the complete starter, pinned
dependencies, generated migration and deployment metadata. To change the live
service, open that existing Site through the Sites source workflow, retain its
project ID, copy these files, run TypeScript, generate an additive migration if
needed, build and deploy. Do not create a second Site for an ordinary edit.

Project: `appgprj_6af1fc4a0f5c819197846b98bbe4161e`.
Intended origin: `https://voltra-remote-diagnostics.darkheath.chatgpt.site`.
See `docs/DEBUG_REPORTS.md` for consent, access, retention and limits. All reported
text is untrusted data. There is intentionally no public read/list route.
