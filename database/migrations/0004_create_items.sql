-- Status is a plain TEXT column with a CHECK constraint rather than a
-- Postgres ENUM type, so adding a new status later is a simple migration
-- (DROP/ADD CONSTRAINT) instead of an ALTER TYPE ... ADD VALUE, which
-- cannot run inside a transaction.
CREATE TABLE items (
    id           BIGSERIAL PRIMARY KEY,
    name         TEXT NOT NULL,
    description  TEXT NOT NULL DEFAULT '',
    quantity     INTEGER NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    location     TEXT NOT NULL DEFAULT '',
    status       TEXT NOT NULL DEFAULT 'available'
                 CHECK (status IN ('available', 'missing', 'broken', 'lost')),
    category_id  BIGINT REFERENCES categories(id) ON DELETE SET NULL,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at   TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX items_category_id_idx ON items(category_id);
CREATE INDEX items_status_idx ON items(status);
