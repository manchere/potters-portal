CREATE TABLE item_tags (
    item_id  BIGINT NOT NULL REFERENCES items(id) ON DELETE CASCADE,
    tag_id   BIGINT NOT NULL REFERENCES tags(id) ON DELETE CASCADE,
    PRIMARY KEY (item_id, tag_id)
);

CREATE INDEX item_tags_tag_id_idx ON item_tags(tag_id);
