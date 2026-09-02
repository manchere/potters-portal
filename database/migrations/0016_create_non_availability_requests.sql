-- Formal non-availability requests tied to a specific assignment
-- (FR-4.1-4.5), requiring a message and going through Admin approve/deny
-- (FR-5.1/5.2). Status is TEXT+CHECK rather than a native ENUM so adding a
-- new status later is a simple migration instead of an
-- ALTER TYPE ... ADD VALUE, which can't run inside a transaction (see
-- 0004_create_items.sql for the same reasoning on items.status).
CREATE TABLE non_availability_requests (
    id             BIGSERIAL PRIMARY KEY,
    assignment_id  BIGINT NOT NULL REFERENCES assignments(id) ON DELETE CASCADE,
    user_id        BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    message        TEXT NOT NULL,
    status         TEXT NOT NULL DEFAULT 'pending'
                   CHECK (status IN ('pending', 'approved', 'denied')),
    decided_by     BIGINT REFERENCES users(id) ON DELETE SET NULL,
    decided_at     TIMESTAMPTZ,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at     TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX non_availability_requests_assignment_id_idx ON non_availability_requests(assignment_id);
CREATE INDEX non_availability_requests_user_id_idx ON non_availability_requests(user_id);
CREATE INDEX non_availability_requests_status_idx ON non_availability_requests(status);
