-- Stores a relative path (e.g. "uploads/3.jpg") under the server's upload
-- directory, not the image bytes themselves. Served back via GET /uploads/:file.
ALTER TABLE items ADD COLUMN image_path TEXT NOT NULL DEFAULT '';
