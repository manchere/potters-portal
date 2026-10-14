-- Live updates. Members change things through the API server while the
-- desktop app writes to the database directly, so the database is the one
-- place that sees every change. Each insert / update / delete on a watched
-- table is recorded in change_log and announced on the 'changes' channel
-- (pg_notify), so open apps can refresh without polling every table:
--
--   * listeners (LISTEN changes) hear about it immediately;
--   * anyone whose connection dropped (Neon suspending an idle compute, the
--     API server sleeping) catches up by reading change_log rows newer than
--     the last id they saw.
--
-- The notification payload is JSON:
--   {"id": <change_log.id>, "table": "duties", "op": "UPDATE", "row": "42"}
-- It only says what changed; clients re-read the data the usual way.
--
-- backend_pid is the Postgres process that made the change, so an app can
-- skip changes it made itself (it has already refreshed for those).
CREATE TABLE change_log (
    id           BIGSERIAL PRIMARY KEY,
    table_name   TEXT NOT NULL,
    op           TEXT NOT NULL CHECK (op IN ('INSERT', 'UPDATE', 'DELETE')),
    row_id       TEXT,  -- the row's id; NULL for tables without one (item_tags)
    backend_pid  INTEGER NOT NULL DEFAULT pg_backend_pid(),
    changed_at   TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX change_log_changed_at_idx ON change_log(changed_at);

-- Row trigger. TG_ARGV[0], when given, names the table's id column; it's
-- read on its own so tables with large columns (items.image) aren't
-- converted to JSON on every change.
CREATE FUNCTION log_change() RETURNS trigger
LANGUAGE plpgsql AS $$
DECLARE
    rid     TEXT;
    log_id  BIGINT;
BEGIN
    IF TG_NARGS > 0 THEN
        IF TG_OP = 'DELETE' THEN
            EXECUTE format('SELECT ($1).%I::text', TG_ARGV[0]) INTO rid USING OLD;
        ELSE
            EXECUTE format('SELECT ($1).%I::text', TG_ARGV[0]) INTO rid USING NEW;
        END IF;
    END IF;

    INSERT INTO change_log (table_name, op, row_id)
    VALUES (TG_TABLE_NAME, TG_OP, rid)
    RETURNING id INTO log_id;

    PERFORM pg_notify('changes', json_build_object(
        'id', log_id, 'table', TG_TABLE_NAME, 'op', TG_OP, 'row', rid)::text);

    -- Keep the log short: now and then, drop what's older than 30 days.
    -- Anyone offline for longer just reloads everything.
    IF random() < 0.01 THEN
        DELETE FROM change_log WHERE changed_at < now() - interval '30 days';
    END IF;

    RETURN NULL;  -- AFTER trigger; return value ignored
END;
$$;

DO $$
DECLARE
    t TEXT;
BEGIN
    FOREACH t IN ARRAY ARRAY[
        'items', 'categories', 'tags', 'songs', 'users', 'teams',
        'duties', 'duty_types', 'availability_marks',
        'non_availability_requests', 'feedback'
    ] LOOP
        EXECUTE format(
            'CREATE TRIGGER %I AFTER INSERT OR UPDATE OR DELETE ON %I '
            'FOR EACH ROW EXECUTE FUNCTION log_change(''id'')',
            t || '_log_change', t);
    END LOOP;
END;
$$;

-- No id column: composite primary keys.
CREATE TRIGGER item_tags_log_change AFTER INSERT OR UPDATE OR DELETE ON item_tags
    FOR EACH ROW EXECUTE FUNCTION log_change();
CREATE TRIGGER access_rules_log_change AFTER INSERT OR UPDATE OR DELETE ON access_rules
    FOR EACH ROW EXECUTE FUNCTION log_change();
