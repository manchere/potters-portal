-- Worship song library, managed from the desktop Songs tab. link is a
-- play link (YouTube, Spotify, ...) opened in the browser; song_key is the
-- musical key the team sings it in (free text, e.g. "G" or "Bb"). Both are
-- optional, so empty strings rather than NULLs, matching tags/categories.
CREATE TABLE songs (
    id          BIGSERIAL PRIMARY KEY,
    title       TEXT NOT NULL,
    artist      TEXT NOT NULL DEFAULT '',
    song_key    TEXT NOT NULL DEFAULT '',
    link        TEXT NOT NULL DEFAULT '',
    lyrics      TEXT NOT NULL DEFAULT '',
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX songs_title_idx ON songs(LOWER(title));
