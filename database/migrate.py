#!/usr/bin/env python3
"""Applies pending SQL migrations from migrations/ to a Postgres database,
in filename order, recording each applied file in schema_migrations so it
never runs twice. Run: python migrate.py [--database-url <url>]

Connection string defaults to the DATABASE_URL environment variable.
"""
import argparse
import os
import sys
from pathlib import Path

import psycopg2

MIGRATIONS_DIR = Path(__file__).parent / "migrations"


def applied_versions(cur) -> set[str]:
    cur.execute("CREATE TABLE IF NOT EXISTS schema_migrations ("
                "version TEXT PRIMARY KEY, "
                "applied_at TIMESTAMPTZ NOT NULL DEFAULT now())")
    cur.execute("SELECT version FROM schema_migrations")
    return {row[0] for row in cur.fetchall()}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database-url", default=os.environ.get("DATABASE_URL"))
    args = parser.parse_args()

    if not args.database_url:
        print("error: no database URL. Pass --database-url or set DATABASE_URL.", file=sys.stderr)
        return 1

    migration_files = sorted(MIGRATIONS_DIR.glob("*.sql"))
    if not migration_files:
        print(f"no migration files found in {MIGRATIONS_DIR}")
        return 0

    conn = psycopg2.connect(args.database_url)
    conn.autocommit = False
    try:
        with conn.cursor() as cur:
            already_applied = applied_versions(cur)
        conn.commit()

        pending = [f for f in migration_files if f.name not in already_applied]
        if not pending:
            print("database is up to date, no pending migrations")
            return 0

        for path in pending:
            sql = path.read_text(encoding="utf-8")
            with conn.cursor() as cur:
                print(f"applying {path.name} ...", end=" ", flush=True)
                cur.execute(sql)
                cur.execute(
                    "INSERT INTO schema_migrations (version) VALUES (%s)",
                    (path.name,),
                )
            conn.commit()
            print("ok")
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
