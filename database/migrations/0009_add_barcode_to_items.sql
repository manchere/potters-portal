-- Optional barcode/QR value scanned on a physical item, used by the mobile
-- app's "scan to identify" flow to look an item up without typing its name.
-- Nullable + partial unique index since most items won't have one yet and
-- multiple items can share a NULL barcode.
ALTER TABLE items ADD COLUMN barcode TEXT;

CREATE UNIQUE INDEX items_barcode_key ON items(barcode) WHERE barcode IS NOT NULL;
