-- Tracks which migrations have been applied, in order. The migration runner
-- (database/migrate.py) reads and writes this table; never edit rows by hand.
CREATE TABLE IF NOT EXISTS schema_migrations (
    version     TEXT PRIMARY KEY,
    applied_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);
