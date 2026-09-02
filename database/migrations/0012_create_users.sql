-- Users double as Member profiles (FR-1.1/FR-1.3 in
-- SCHEDULING_FUNCTIONAL_REQUIREMENTS.md): a login identity and a
-- scheduling profile are the same row, since a profile is just a name plus
-- an optional Admin flag beyond credentials. avatar_seed drives a
-- DiceBear avatar URL built client-side (defaults to name, user can
-- shuffle it) -- no photo/image storage needed for this.
--
-- password_hash/password_salt back a hand-rolled PBKDF2-HMAC-SHA256 check
-- (see PottersInventory/Auth/PasswordAuth.*) since no password-hashing
-- library is available anywhere in this repo/vcpkg install.
CREATE TABLE users (
    id             BIGSERIAL PRIMARY KEY,
    name           TEXT NOT NULL,
    email          TEXT NOT NULL,
    password_hash  TEXT NOT NULL,
    password_salt  TEXT NOT NULL,
    is_admin       BOOLEAN NOT NULL DEFAULT false,
    avatar_seed    TEXT NOT NULL,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at     TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE UNIQUE INDEX users_email_key ON users(LOWER(email));
