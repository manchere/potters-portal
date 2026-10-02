-- Access rights, set by an Admin in Settings > Access Rights. Each row
-- grants one subject what it can do in one section of the desktop app:
-- open it (can_view) and create / update / delete there.
--
-- subject_kind says who the row is for:
--   'everyone' -- anyone using the app, signed in or not (subject_id 0)
--   'member'   -- one member (users.id), permanently
--   'team'     -- every member of a team (teams.id), permanently
--   'duty'     -- whoever has that duty (duty_types.id) on the upcoming
--                 Sunday, only for as long as they have it
-- A person gets everything any of their rows allows. Admins always have
-- full access, whatever is stored here.
CREATE TABLE access_rules (
    subject_kind  TEXT NOT NULL CHECK (subject_kind IN ('everyone', 'member', 'team', 'duty')),
    subject_id    BIGINT NOT NULL DEFAULT 0,
    section       TEXT NOT NULL
                  CHECK (section IN ('reports', 'songs', 'inventory', 'taxonomy', 'feedback', 'settings')),
    can_view      BOOLEAN NOT NULL DEFAULT false,
    can_create    BOOLEAN NOT NULL DEFAULT false,
    can_update    BOOLEAN NOT NULL DEFAULT false,
    can_delete    BOOLEAN NOT NULL DEFAULT false,
    updated_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    PRIMARY KEY (subject_kind, subject_id, section)
);

-- Everyone can open every section and send feedback; any changing beyond
-- that has to be granted.
INSERT INTO access_rules (subject_kind, subject_id, section, can_view, can_create) VALUES
    ('everyone', 0, 'reports', true, false),
    ('everyone', 0, 'songs', true, false),
    ('everyone', 0, 'inventory', true, false),
    ('everyone', 0, 'taxonomy', true, false),
    ('everyone', 0, 'feedback', true, true),
    ('everyone', 0, 'settings', true, false);
