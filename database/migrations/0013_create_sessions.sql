-- Opaque bearer-token sessions for the mobile client (FR-0.1/FR-0.2). The
-- desktop app authenticates in-process against users directly and never
-- creates a row here. Only the token's SHA-256 is stored, never the raw
-- token, matching a standard bearer-token-at-rest practice.
CREATE TABLE sessions (
    id          BIGSERIAL PRIMARY KEY,
    user_id     BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    token_hash  TEXT NOT NULL,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    expires_at  TIMESTAMPTZ NOT NULL
);

CREATE UNIQUE INDEX sessions_token_hash_key ON sessions(token_hash);
CREATE INDEX sessions_user_id_idx ON sessions(user_id);
