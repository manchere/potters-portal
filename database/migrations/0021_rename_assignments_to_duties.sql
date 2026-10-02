-- Rename scheduling terms to "duty": the admin-managed list of duty kinds
-- (assignment_roles) becomes duty_types, and each Sunday's scheduled entry
-- (assignments) becomes duties. Pure renames -- no data changes.

ALTER TABLE assignment_roles RENAME TO duty_types;
ALTER TABLE assignments RENAME TO duties;

ALTER TABLE duties RENAME COLUMN role_id TO duty_type_id;
ALTER TABLE non_availability_requests RENAME COLUMN assignment_id TO duty_id;

-- Indexes (primary keys and unique constraints are indexes too, so this
-- also renames those constraints).
ALTER INDEX IF EXISTS assignment_roles_pkey RENAME TO duty_types_pkey;
ALTER INDEX IF EXISTS assignment_roles_name_key RENAME TO duty_types_name_key;
ALTER INDEX IF EXISTS assignments_pkey RENAME TO duties_pkey;
ALTER INDEX IF EXISTS assignments_service_date_idx RENAME TO duties_service_date_idx;
ALTER INDEX IF EXISTS assignments_member_id_idx RENAME TO duties_member_id_idx;
ALTER INDEX IF EXISTS assignments_support_member_id_idx RENAME TO duties_support_member_id_idx;
ALTER INDEX IF EXISTS assignments_role_id_idx RENAME TO duties_duty_type_id_idx;
ALTER INDEX IF EXISTS non_availability_requests_assignment_id_idx RENAME TO non_availability_requests_duty_id_idx;

-- Sequences behind the BIGSERIAL ids (column defaults follow the rename
-- automatically).
ALTER SEQUENCE IF EXISTS assignment_roles_id_seq RENAME TO duty_types_id_seq;
ALTER SEQUENCE IF EXISTS assignments_id_seq RENAME TO duties_id_seq;

-- Foreign keys, renamed only where they still carry the default names.
DO $$
DECLARE
    renames TEXT[][] := ARRAY[
        ['duties', 'assignments_member_id_fkey', 'duties_member_id_fkey'],
        ['duties', 'assignments_support_member_id_fkey', 'duties_support_member_id_fkey'],
        ['duties', 'assignments_role_id_fkey', 'duties_duty_type_id_fkey'],
        ['non_availability_requests', 'non_availability_requests_assignment_id_fkey', 'non_availability_requests_duty_id_fkey']
    ];
    r TEXT[];
BEGIN
    FOREACH r SLICE 1 IN ARRAY renames LOOP
        IF EXISTS (SELECT 1 FROM pg_constraint WHERE conname = r[2]) THEN
            EXECUTE format('ALTER TABLE %I RENAME CONSTRAINT %I TO %I', r[1], r[2], r[3]);
        END IF;
    END LOOP;
END $$;
