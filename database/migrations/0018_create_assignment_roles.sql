-- Assignment "roles" become an admin-manageable list of duty types (name +
-- icon) instead of a fixed 5-value enum, so an Admin can define new ones
-- from the desktop Taxonomy tab's "+ Add Assignment" button (which creates
-- a role type, not a scheduled assignment -- scheduling a Member against a
-- role on a given Sunday still only happens on the Date tab).
CREATE TABLE assignment_roles (
    id          BIGSERIAL PRIMARY KEY,
    name        TEXT NOT NULL UNIQUE,
    icon        TEXT NOT NULL DEFAULT '📋',
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

-- Seed with the original 5 fixed roles (same icons as the old
-- RoleDisplay.cpp) so existing assignments have somewhere to point once
-- 0019 migrates assignments.role -> assignments.role_id.
INSERT INTO assignment_roles (name, icon) VALUES
    ('Singing', '🎤'),
    ('Translating', '🌐'),
    ('Preaching', '📖'),
    ('Offering', '💰'),
    ('Announcement', '📢');
