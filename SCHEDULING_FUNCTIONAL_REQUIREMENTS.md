# Scheduling / Member Duties — Functional Requirements

Scope: adds Member profiles, Sunday duties, and non-availability
requests on top of the existing Portal backend. Targets the
desktop app (`src/`, primarily Admin workflows) and the mobile
app (`mobile/`, primarily Member workflows), sharing the C++ REST backend.

## Actors

- **Member** — a person with a profile who can be assigned to serve.
- **Admin** — manages members, duties, and approvals; an Admin account
  may also hold the Member role and be assignable to serve like any other
  Member.

## 1. Authentication

- **FR-0.1** The desktop app shall be usable read-only without logging in
  — there is no login gate at startup.
- **FR-0.2** An Admin shall be able to unlock Admin mode from a single lock
  icon in the title bar. Unlocking requires a **password only** (no
  username/email field): the entered password is checked against every
  Admin account, and whichever one matches becomes the active Admin for
  attribution (e.g. who approved a request).
- **FR-0.3** Clicking the same icon while unlocked shall log the Admin back
  out, hiding every Admin-only action again.
- **FR-0.4** Every Admin-only action (creating/editing/deleting a Member or
  duty, approving/denying a request) shall be hidden — not merely
  disabled — until Admin mode is unlocked.
- **FR-0.5** The mobile app is Member-facing and keeps its own
  email + password login (FR-1.1), separate from the desktop Admin unlock.

## 2. Member Profile Management

- **FR-1.1** A Member profile shall be creatable in two ways:
  a) **self-service**, via the mobile app (name, email, password), or
  b) **by an Admin**, via the "+ Add Member" button on the desktop Date tab
  (name, email, password — the Admin sets initial credentials and shares
  them with the Member).
- **FR-1.2** Creating a profile shall generate an avatar for the Member
  automatically, keyed to a seed derived from their name — implemented via
  [DiceBear](https://www.dicebear.com/), a free third-party avatar API
  requiring no key and no photo upload. The avatar is a consistent
  generated picture for that Member, not a literal photo likeness.
- **FR-1.3** A user account may hold both the Admin and Member roles
  simultaneously; an Admin with the Member role shall have their own
  profile, appear in scheduling, and be assignable to duties the same
  as any other Member.

- **FR-1.4** Only a current Admin shall be able to change a Member's role
  (Member ↔ Admin), from the Members list on the Taxonomy tab ("Make
  Admin" / "Remove Admin", shown only in Admin mode). The acting account's
  Admin status is re-checked in the database at the moment of the change.
  An Admin cannot remove their own Admin role or delete their own account,
  and the last remaining Admin can never be removed or deleted. Editing a
  Member's profile never changes their role.

## 3. Member — Duty Visibility

- **FR-2.1** A Member shall be able to view their own duties for
  upcoming Sundays.

## 4. Member — General Availability Calendar

- **FR-3.1** A Member shall be able to use a calendar view to indicate days
  they expect not to be present at church.
- **FR-3.2** Marking a date as unavailable on this calendar is informational
  only and does not, by itself, require a message or trigger Admin approval.

## 5. Member — Non-Availability Requests (duty-specific)

- **FR-4.1** A non-availability request is only applicable when the Admin
  has already assigned the Member to a duty on that date.
- **FR-4.2** If the Admin assigns a Member to a duty on a date the
  Member has already marked unavailable in their general calendar (FR-3.1),
  the system shall auto-flag the conflict and prompt the Member to submit a
  formal non-availability request.
- **FR-4.3** A Member shall be able to submit a non-availability request
  against a specific duty they've been assigned to, whether prompted
  by an auto-flag (FR-4.2) or initiated manually after being assigned.
- **FR-4.4** A non-availability request shall require a message explaining
  the reason before it can be submitted.
- **FR-4.5** A Member shall be able to view the status
  (pending/approved/denied) of their submitted requests.

## 6. Admin — Non-Availability Approval

> Not currently implemented: the desktop approval screen was removed, so
> requests submitted from mobile stay pending.

- **FR-5.1** An Admin shall be able to view all pending non-availability
  requests.
- **FR-5.2** An Admin shall be able to approve or deny a non-availability
  request.

## 7. Admin — Duty Management (CRUD)

- **FR-6.1** An Admin shall be able to create a new duty by picking a
  **duty type** from an Admin-managed list (e.g. Singing, Translating,
  Preaching, Offering, Announcement) rather than typing a freeform title.
- **FR-6.2** Each duty type shall have a distinct icon shown wherever it
  appears in the UI (the duty list, the duty type picker, the all-dates
  overview, etc.).
- **FR-6.3** An Admin shall be able to update an existing duty,
  including reassigning it to a different Member.
- **FR-6.4** An Admin shall be able to delete a duty.
- **FR-6.5** An Admin shall be able to view every duty, both for a
  single Sunday (Date tab, FR-8) and as a full all-dates list (FR-9).

## 8. Admin — Scheduling

- **FR-7.1** An Admin shall be able to assign a Member to a duty for a
  given Sunday.
- **FR-7.2** A Member may hold **more than one duty on the same
  Sunday** (e.g. Singing and also Offering) — duties are independent
  records, not a one-duty-per-Member-per-Sunday limit.
- **FR-7.3** An Admin shall be able to assign a second, support/backup
  Member to a duty, to cover the primary assignee's absence.
- **FR-7.4** Every Member shown in scheduling views shall display their
  avatar (FR-1.2) alongside their name; when a duty has a support
  Member, the primary and support Member shall be shown together on the
  same line (e.g. "Grace Adeyemi · Support: Ruth Mensah").

## 9. Desktop UI — Date Tab

- **FR-8.1** The interface shall add a new tab, positioned first
  (leftmost), listing Sundays to navigate between — as a scrollable list,
  not a calendar grid — with each entry formatted like `Sun 6 Sep 2026`.
  Only Sundays are listed, from the upcoming Sunday through the end of
  October next year; no other day of the week appears. Past Sundays are
  looked up on the Reports tab (FR-8a).
- **FR-8.2** Selecting a Sunday in this list shall display every Member
  scheduled for it (FR-7.4), with Sundays that already have at least one
  duty visually distinguished in the list itself.
- **FR-8.3** Double-clicking a Member in the results shall open a
  read-only "responsibilities" summary for that Member: an avatar, a count
  of duties by duty type, and their full duty history (not only the
  selected Sunday).
- **FR-8.4** Creating, editing, and deleting a duty (FR-6, FR-7)
  shall happen via buttons on this same Date tab — an "+ Assign Duty"
  button plus Edit/Delete for the selected row — there is no separate
  Duties tab.
- **FR-8.5** A "+ Add Member" button on this same tab shall let an Admin
  create a new Member profile (FR-1.1b) without leaving the Date tab.
- **FR-8.6** FR-8.4 and FR-8.5's buttons are Admin-only (FR-0.4).
- **FR-8.7** An Admin shall be able to copy one Sunday's whole schedule
  ("Copy Schedule" / Ctrl+C) and paste it onto another Sunday ("Paste
  Schedule" / Ctrl+V). Duty type, Member, support Member, and notes are copied.
  If the target Sunday already has duties, the Admin chooses to
  **add to them** (a duty the same Member already holds that day is
  skipped) or **replace them** (deleting the existing duties and any
  time-off requests filed against them). After pasting, the Admin is told
  which pasted Members marked themselves unavailable that day (FR-3.1).

## 9a. Desktop UI — Reports Tab

- **FR-8a.1** A Reports tab shall let anyone look up past Sunday schedules
  by date range (presets: last month, last 3 months, last year), optionally
  filtered to one Member.
- **FR-8a.2** Selecting a Sunday shall show who did what that day: each
  duty, who served, the backup Member, and notes. A small tag appears next
  to a Member's name only when they had asked for time off or marked
  themselves away that day. The reason given in a time-off request is
  never shown.
- **FR-8a.3** An "All Sundays in range" entry shall show every Sunday in
  the range in turn, newest first, each with its own who-did-what list.
- **FR-8a.4** The current report shall be savable as HTML (printable) or
  CSV (one row per duty).

## 10. Desktop UI — Tags, Categories & Duties Tab

- **FR-9.1** The interface shall provide one tab for managing inventory
  Tags and Categories together with a read-only, all-dates overview of
  every scheduling duty — replacing the earlier separate "Tags &
  Categories" tab.
- **FR-9.2** This tab, including the duties overview, is visible to
  everyone (not Admin-gated), consistent with Tags/Categories management
  never having required a login. Creating, editing, or deleting a
  duty still only happens on the Date tab (FR-8.4), which stays
  Admin-gated.
- **FR-9.3** Tags and Categories in this tab apply to **inventory items
  only** — they are unrelated to Members, duties, or scheduling, and no
  scheduling data should be taggable or categorized through them.

## 11. Desktop UI — Modal Presentation

- **FR-10.1** Every modal dialog in the app shall be frameless (no native
  title bar, minimize, maximize, or close controls), centered over the
  main window when shown, and rendered as a rounded card with a gray
  background — a consistent look across Login, Assign Duty, Add Member,
  Member responsibilities, Category, and Item dialogs alike.

## Platform split

- **Desktop (Qt/C++)**: Admin-facing — the Date tab is the primary
  scheduling surface (duty CRUD, scheduling, support-Member
  duty, Member creation, avatars, duty type icons), plus non-availability
  approval and the combined Tags/Categories/Duties overview tab.
- **Mobile (React Native)**: Member-facing — self-service profile
  creation, avatar, own-duty view, availability calendar,
  non-availability request submission and status.
- **Backend (`src/Server`)**: authentication, Members,
  Duties, and Non-Availability Requests REST endpoints shared by both
  clients.
