-- Assignments now carry a fixed set of service roles (singing, translating,
-- preaching, offering, announcement) instead of a freeform title, so the
-- UI can show a distinct icon per role. TEXT+CHECK rather than a native
-- ENUM, same reasoning as items.status (see 0004_create_items.sql) --
-- adding a role later is a simple migration instead of an
-- ALTER TYPE ... ADD VALUE, which can't run inside a transaction.
ALTER TABLE assignments RENAME COLUMN title TO role;
ALTER TABLE assignments ADD CONSTRAINT assignments_role_check
    CHECK (role IN ('singing', 'translating', 'preaching', 'offering', 'announcement'));
