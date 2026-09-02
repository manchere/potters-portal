-- General availability calendar (FR-3.1/3.2): a Member marks days they
-- expect not to be present at church. This is informational only and
-- distinct from non_availability_requests (0016), which are formal,
-- assignment-specific, and go through Admin approval -- a mark here only
-- becomes a request once the Admin actually assigns the member on that
-- date (FR-4.1/4.2).
CREATE TABLE availability_marks (
    id          BIGSERIAL PRIMARY KEY,
    user_id     BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    date        DATE NOT NULL,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE UNIQUE INDEX availability_marks_user_id_date_key ON availability_marks(user_id, date);
