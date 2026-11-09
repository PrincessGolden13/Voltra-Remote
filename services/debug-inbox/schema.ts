import { sqliteTable, text, integer } from "drizzle-orm/sqlite-core";
export const reports = sqliteTable("reports", {
  id: text("id").primaryKey(), hash: text("hash").notNull(),
  receivedAt: integer("received_at").notNull(), body: text("body").notNull(),
});
export const tickets = sqliteTable("tickets", {
  tokenHash: text("token_hash").primaryKey(), reportId: text("report_id").notNull(),
  hash: text("hash").notNull(), expires: integer("expires").notNull(),
});
export const quotas = sqliteTable("quotas", {
  key: text("key").primaryKey(), count: integer("count").notNull(), expires: integer("expires").notNull(),
});
