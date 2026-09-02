-- Sunday duty assignments (FR-6, FR-7). member_id is the primary assignee;
-- support_member_id is an optional backup covering for the primary in case
-- they're not present (FR-7.2). Both are ON DELETE SET NULL rather than
-- CASCADE so deleting a user doesn't silently delete the church's
-- scheduling record of that Sunday's assignment.
CREATE TABLE assignments (
    id                 BIGSERIAL PRIMARY KEY,
    title              TEXT NOT NULL,
    service_date       DATE NOT NULL,
    member_id          BIGINT REFERENCES users(id) ON DELETE SET NULL,
    support_member_id  BIGINT REFERENCES users(id) ON DELETE SET NULL,
    notes              TEXT NOT NULL DEFAULT '',
    created_at         TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at         TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX assignments_service_date_idx ON assignments(service_date);
CREATE INDEX assignments_member_id_idx ON assignments(member_id);
CREATE INDEX assignments_support_member_id_idx ON assignments(support_member_id);
