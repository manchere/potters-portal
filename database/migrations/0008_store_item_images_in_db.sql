-- Switch from filesystem-path storage to storing image bytes directly in
-- Postgres. A hosted server's local disk isn't guaranteed to survive a
-- restart/redeploy, but the database is the one thing that's always there.
ALTER TABLE items DROP COLUMN image_path;
ALTER TABLE items ADD COLUMN image_data BYTEA;
ALTER TABLE items ADD COLUMN image_mime TEXT NOT NULL DEFAULT '';
