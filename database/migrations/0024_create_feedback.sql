-- Requests sent from the desktop Feedback tab: a bug report, a feature
-- request, or a change wanted on someone's member profile. Anyone can send
-- one; Admins read them and mark them done. member_id is who it's from
-- (optional -- the app has no login for ordinary members), kept as NULL if
-- that member is later deleted.
CREATE TABLE feedback (
    id          BIGSERIAL PRIMARY KEY,
    kind        TEXT NOT NULL CHECK (kind IN ('bug', 'feature', 'profile')),
    member_id   BIGINT REFERENCES users(id) ON DELETE SET NULL,
    subject     TEXT NOT NULL,
    details     TEXT NOT NULL DEFAULT '',
    status      TEXT NOT NULL DEFAULT 'open' CHECK (status IN ('open', 'done')),
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX feedback_status_idx ON feedback(status, created_at DESC);
