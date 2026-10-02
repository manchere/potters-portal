-- Teams group Members (e.g. Choir, Ushers, Media), managed by an Admin
-- from the desktop Taxonomy tab. A Member belongs to at most one team;
-- deleting a team leaves its Members without one rather than deleting them.
CREATE TABLE teams (
    id           BIGSERIAL PRIMARY KEY,
    name         TEXT NOT NULL,
    description  TEXT NOT NULL DEFAULT '',
    created_at   TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at   TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE UNIQUE INDEX teams_name_key ON teams(LOWER(name));

ALTER TABLE users ADD COLUMN team_id BIGINT REFERENCES teams(id) ON DELETE SET NULL;
CREATE INDEX users_team_id_idx ON users(team_id);
