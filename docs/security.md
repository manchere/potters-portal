# Security

How Potters Portal protects its data today, and where it falls short. The data
lives in a **cloud** database (Neon PostgreSQL in AWS London — see
[network-and-hosting.md](network-and-hosting.md)); everything below is about
who can reach it and how.

## Summary

| Area | What's in place | Level |
|---|---|---|
| Data in transit | TLS on every connection: desktop → database, server → database, phone → server (HTTPS), email, AI calls | Good — but the database certificate isn't verified (see 2) |
| Data at rest | Stored by Neon on encrypted cloud storage; passwords and session tokens hashed, never stored readable | Good |
| Sign-in | Email + password for members and Admins; PBKDF2-HMAC-SHA256 password hashes; 180-day mobile sessions | Fair |
| Access rights (mobile / API) | Checked by the server on every request, per section and action | Good |
| Access rights (desktop) | Checked by the desktop app itself — but the app holds the database owner's credentials | **Weak** (see 1) |
| Secrets | Connection string and API keys kept out of the repository | Good |

## What is protected, and how

### Encryption in transit

- **Desktop → database** and **server → database**: the connection string sets
  `sslmode=require`, so the Postgres connection is encrypted with TLS. Neon
  refuses unencrypted connections.
- **Phone → server**: HTTPS, terminated by Render.
- **Email** (Gmail SMTP) and **Groq AI** calls: TLS / HTTPS.

### Encryption at rest

Neon stores the database on encrypted cloud storage, so disks and backups
can't be read outside Neon. On top of that, the app never stores secrets
readably in its own tables:

- **Passwords** — PBKDF2-HMAC-SHA256, 100,000 iterations, a random 16-byte salt
  per account, 32-byte result (`users.password_hash`, `users.password_salt`).
- **Mobile session tokens** — 32 random bytes handed to the phone once at
  sign-in; the database keeps only their SHA-256 (`sessions.token_hash`), so a
  copy of the database can't be used to sign in. Sessions expire after 180 days
  without use and are deleted on sign-out.

### Who can do what

- **Admins** (`users.is_admin`) can do everything. There is always at least one
  Admin: the last one can't be deleted or demoted, and an Admin can't remove
  their own Admin role. Making or removing an Admin is re-checked against the
  database, not trusted from the app.
- **Everyone else** gets the rights an Admin sets in *Settings → Access
  Rights*: per section (Reports, Songs, Inventory, Taxonomy, Feedback,
  Settings), whether they may view, create, update or delete — granted to
  everyone, a member, a team, or whoever holds a duty on the upcoming Sunday.
  Out of the box, everyone may view every section and send feedback, nothing
  more. Changing the schedule is Admin-only.
- **Past Sundays** are read-only, enforced in `DutyController`.
- Only an Admin can change an Admin's profile.

The **REST API** enforces these rules itself on each request: it identifies the
caller from their bearer token and checks their rights before reading or
changing anything (`401` without a valid token, `403` without the right).

### Secrets

- The database connection string, Groq API key and SMTP password are read from
  environment variables (or, for installed desktop copies, entered on first
  launch and saved for that Windows user). None are committed:
  `connection_detail.txt`, `.env` and similar files are git-ignored.
- On Render they are entered as secret values (`sync: false` in
  `render.yaml`).
- The server container runs as an unprivileged user.
- All SQL uses prepared statements with bound values, which prevents SQL
  injection.

## Known weaknesses and recommendations

Ordered by importance.

### 1. Every desktop copy holds full control of the database

The desktop app connects to Neon directly as **`neondb_owner`** — the
database's owner role, which can read and change every table, create roles,
and bypass row-level security. Access rights are applied inside the app, so
anyone who obtains the connection string (from a computer it's saved on, or by
being given it to install the app) can bypass all of them with any Postgres
client, including reading password hashes and deleting data.

*Recommended:*
- Short term: give desktop installs a **separate, limited Postgres role**
  (only `SELECT/INSERT/UPDATE/DELETE` on the app's tables; no role creation,
  no DDL), keep `neondb_owner` for migrations only, and give the connection
  string only to trusted people (Admins).
- Long term: have the desktop app use the **REST API** like the mobile app
  does, so no client ever holds database credentials and every right is
  enforced on the server.

### 2. The database's certificate isn't verified

`sslmode=require` encrypts the connection but doesn't check that the server
is really Neon, so a network attacker could in principle intercept it.
*Recommended:* use `sslmode=verify-full` (with the system's root certificates)
in the connection string.

### 3. Some API routes are open without signing in

These need no token: reading items, item photos, tags and categories
(`GET /api/items…`, `/api/tags`, `/api/categories`), creating an account
(`POST /api/auth/register`), and **`POST /api/vision/describe-item`**, which
calls Groq on the church's API key. Anyone who learns the server's address can
read the inventory, create accounts (which then get the *everyone* rights), and
spend the Groq quota. *Recommended:* require a token on these routes, and
let only Admins create accounts (or approve new ones).

### 4. No limit on sign-in attempts

`POST /api/auth/login` and the desktop sign-in accept unlimited attempts, so
passwords can be guessed by brute force. *Recommended:* slow down or lock out
repeated failures per account and per address.

### 5. Random values come from a non-cryptographic generator

Salts and session tokens are drawn from `QRandomGenerator::global()`, which is
securely *seeded* but not a cryptographic generator. *Recommended:* use
`QRandomGenerator::system()` in `Auth/PasswordAuth.cpp`.

### 6. Password hashing could be stronger

100,000 PBKDF2-SHA256 iterations is below current guidance (around 600,000).
*Recommended:* raise the iteration count for new passwords and re-hash
existing ones at their next sign-in. Minimum password length is 8.

### 7. The saved desktop connection isn't encrypted

On installed copies, the connection string entered at first launch is saved in
the Windows user's settings (registry) as plain text, readable by anyone using
that Windows account. *Recommended:* store it with Windows Credential Manager
(DPAPI) — and see 1: a limited role makes this much less damaging.

### 8. Open CORS

The API allows calls from any web origin. Because it authenticates with
bearer tokens rather than cookies, this exposes nothing extra today, but it
should be narrowed to the web app's address if cookies are ever introduced.
