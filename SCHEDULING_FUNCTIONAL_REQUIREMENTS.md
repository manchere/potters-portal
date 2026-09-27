# Scheduling / Member Assignments — Functional Requirements

Scope: adds Member profiles, Sunday duty assignments, and non-availability
requests on top of the existing PottersInventory backend. Targets the
desktop app (`PottersInventory/`, primarily Admin workflows) and the mobile
app (`mobile/`, primarily Member workflows), sharing the C++ REST backend.

## Actors

- **Member** — a person with a profile who can be assigned to serve.
- **Admin** — manages members, assignments, and approvals; an Admin account
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
  assignment, approving/denying a request) shall be hidden — not merely
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
  profile, appear in scheduling, and be assignable to assignments the same
  as any other Member.

- **FR-1.4** Only a current Admin shall be able to change a Member's role
  (Member ↔ Admin), from the Members list on the Taxonomy tab ("Make
  Admin" / "Remove Admin", shown only in Admin mode). The acting account's
  Admin status is re-checked in the database at the moment of the change.
  An Admin cannot remove their own Admin role or delete their own account,
  and the last remaining Admin can never be removed or deleted. Editing a
  Member's profile never changes their role.

## 3. Member — Assignment Visibility

- **FR-2.1** A Member shall be able to view their own assignments for
  upcoming Sundays.

## 4. Member — General Availability Calendar

- **FR-3.1** A Member shall be able to use a calendar view to indicate days
  they expect not to be present at church.
- **FR-3.2** Marking a date as unavailable on this calendar is informational
  only and does not, by itself, require a message or trigger Admin approval.

## 5. Member — Non-Availability Requests (assignment-specific)

- **FR-4.1** A non-availability request is only applicable when the Admin
  has already assigned the Member to an assignment on that date.
- **FR-4.2** If the Admin assigns a Member to an assignment on a date the
  Member has already marked unavailable in their general calendar (FR-3.1),
  the system shall auto-flag the conflict and prompt the Member to submit a
  formal non-availability request.
- **FR-4.3** A Member shall be able to submit a non-availability request
  against a specific assignment they've been assigned to, whether prompted
  by an auto-flag (FR-4.2) or initiated manually after being assigned.
- **FR-4.4** A non-availability request shall require a message explaining
  the reason before it can be submitted.
- **FR-4.5** A Member shall be able to view the status
  (pending/approved/denied) of their submitted requests.

## 6. Admin — Non-Availability Approval

- **FR-5.1** An Admin shall be able to view all pending non-availability
  requests.
- **FR-5.2** An Admin shall be able to approve or deny a non-availability
  request.

## 7. Admin — Assignment Management (CRUD)

- **FR-6.1** An Admin shall be able to create a new assignment by picking a
  **role** from a fixed set — Singing, Translating, Preaching, Offering,
  Announcement — rather than typing a freeform title.
- **FR-6.2** Each role shall have a distinct icon shown wherever the role
  appears in the UI (the assignment list, the role picker, the all-dates
  overview, etc.).
- **FR-6.3** An Admin shall be able to update an existing assignment,
  including reassigning it to a different Member.
- **FR-6.4** An Admin shall be able to delete an assignment.
- **FR-6.5** An Admin shall be able to view every assignment, both for a
  single Sunday (Date tab, FR-8) and as a full all-dates list (FR-9).

## 8. Admin — Scheduling

- **FR-7.1** An Admin shall be able to assign a Member to a role for a
  given Sunday.
- **FR-7.2** A Member may hold **more than one role assignment on the same
  Sunday** (e.g. Singing and also Offering) — assignments are independent
  records, not a one-role-per-Member-per-Sunday limit.
- **FR-7.3** An Admin shall be able to assign a second, support/backup
  Member to an assignment, to cover the primary assignee's absence.
- **FR-7.4** Every Member shown in scheduling views shall display their
  avatar (FR-1.2) alongside their name; when an assignment has a support
  Member, the primary and support Member shall be shown together on the
  same line (e.g. "Grace Adeyemi · Support: Ruth Mensah").

## 9. Desktop UI — Date Tab

- **FR-8.1** The interface shall add a new tab, positioned first
  (leftmost), listing Sundays to navigate between — as a scrollable list,
  not a calendar grid — with each entry formatted like `Sun 6 Sep 2026`.
  Only Sundays are listed; no other day of the week appears.
- **FR-8.2** Selecting a Sunday in this list shall display every Member
  scheduled for it (FR-7.4), with Sundays that already have at least one
  assignment visually distinguished in the list itself.
- **FR-8.3** Double-clicking a Member in the results shall open a
  read-only "responsibilities" summary for that Member: an avatar, a count
  of assignments by role, and their full assignment history (not only the
  selected Sunday).
- **FR-8.4** Creating, editing, and deleting a role assignment (FR-6, FR-7)
  shall happen via buttons on this same Date tab — an "+ Assign Role"
  button plus Edit/Delete for the selected row — there is no separate
  Assignments tab.
- **FR-8.5** A "+ Add Member" button on this same tab shall let an Admin
  create a new Member profile (FR-1.1b) without leaving the Date tab.
- **FR-8.6** FR-8.4 and FR-8.5's buttons are Admin-only (FR-0.4).

## 10. Desktop UI — Tags, Categories & Assignments Tab

- **FR-9.1** The interface shall provide one tab for managing inventory
  Tags and Categories together with a read-only, all-dates overview of
  every scheduling assignment — replacing the earlier separate "Tags &
  Categories" tab.
- **FR-9.2** This tab, including the assignments overview, is visible to
  everyone (not Admin-gated), consistent with Tags/Categories management
  never having required a login. Creating, editing, or deleting an
  assignment still only happens on the Date tab (FR-8.4), which stays
  Admin-gated.
- **FR-9.3** Tags and Categories in this tab apply to **inventory items
  only** — they are unrelated to Members, roles, or scheduling, and no
  scheduling data should be taggable or categorized through them.

## 11. Desktop UI — Modal Presentation

- **FR-10.1** Every modal dialog in the app shall be frameless (no native
  title bar, minimize, maximize, or close controls), centered over the
  main window when shown, and rendered as a rounded card with a gray
  background — a consistent look across Login, Assign Role, Add Member,
  Member responsibilities, Category, and Item dialogs alike.

## Platform split

- **Desktop (Qt/C++)**: Admin-facing — the Date tab is the primary
  scheduling surface (assignment CRUD, scheduling, support-Member
  assignment, Member creation, avatars, role icons), plus non-availability
  approval and the combined Tags/Categories/Assignments overview tab.
- **Mobile (React Native)**: Member-facing — self-service profile
  creation, avatar, own-assignment view, availability calendar,
  non-availability request submission and status.
- **Backend (`PottersInventory/Server`)**: authentication, Members,
  Assignments, and Non-Availability Requests REST endpoints shared by both
  clients.
