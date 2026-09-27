-- Replaces the fixed TEXT+CHECK "role" column (0017_assignments_role_enum.sql)
-- with a role_id FK into assignment_roles (0018), so assignments reference
-- an admin-manageable role type instead of a hardcoded enum value.
ALTER TABLE assignments ADD COLUMN role_id BIGINT REFERENCES assignment_roles(id);

UPDATE assignments a
SET role_id = ar.id
FROM assignment_roles ar
WHERE lower(ar.name) = a.role;

ALTER TABLE assignments ALTER COLUMN role_id SET NOT NULL;
ALTER TABLE assignments DROP CONSTRAINT assignments_role_check;
ALTER TABLE assignments DROP COLUMN role;

CREATE INDEX assignments_role_id_idx ON assignments(role_id);
