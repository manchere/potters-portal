# Scheduling / Member Assignments — Functional Requirements

Scope: adds Member profiles, Sunday duty assignments, and non-availability
requests on top of the existing PottersInventory backend. Targets the
desktop app (`PottersInventory/`, primarily Admin workflows) and the mobile
app (`mobile/`, primarily Member workflows), sharing the C++ REST backend.

## Actors

- **Member** — a person with a profile who can be assigned to serve.
- **Admin** — manages assignments and approvals; an Admin account may also
  hold the Member role and be assignable to serve like any other Member.

## 1. Authentication

- **FR-0.1** A user shall log in to access the system; the REST API shall
  require authentication for all endpoints introduced by this feature.
- **FR-0.2** A logged-in user's role(s) (Member and/or Admin) determine
  which actions are available to them.

## 2. Profile Management

- **FR-1.1** A Member shall be able to create a profile using their name.
- **FR-1.2** The system shall generate an avatar for the Member using a
  third-party avatar API, keyed to a seed derived from their name (no photo
  upload) — implemented via [DiceBear](https://www.dicebear.com/), which is
  free and needs no API key. The avatar is a generated character
  consistent for that Member, not a literal photo likeness.
- **FR-1.3** A user account may hold both the Admin and Member roles
  simultaneously; an Admin with the Member role shall have their own
  profile, appear in scheduling, and be assignable to assignments the same
  as any other Member.

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
  appears in the UI (the assignment list, the role picker, etc.).
- **FR-6.3** An Admin shall be able to update an existing assignment.
- **FR-6.4** An Admin shall be able to delete an assignment.
- **FR-6.5** An Admin shall be able to view all assignments.

## 8. Admin — Scheduling

- **FR-7.1** An Admin shall be able to assign a Member profile to a role for
  an upcoming Sunday.
- **FR-7.2** An Admin shall be able to assign a support/backup profile to an
  assignment, to cover the primary assignee's absence.
- **FR-7.3** An Admin shall be able to view every Member's assignment(s) for
  upcoming Sundays across the whole roster.
- **FR-7.4** Every Member shown in scheduling views shall display their
  avatar (FR-1.2) alongside their name.

## 9. UI — Date Navigation Tab

- **FR-8.1** The interface shall add a new tab, positioned first
  (leftmost), providing a date picker/filter for navigation.
- **FR-8.2** Selecting a date in this tab shall display every Member
  scheduled for that Sunday together with their role (icon + label),
  avatar, and support member (if any).
- **FR-8.3** Creating, editing, and deleting a role assignment (FR-6, FR-7)
  shall happen via buttons on this same Date tab (an "Assign Role" button,
  plus Edit/Delete for the selected row) — there is no separate Assignments
  tab.

## Platform split

- **Desktop (Qt/C++)**: Admin-facing — the Date tab is the primary
  scheduling surface (assignment CRUD, scheduling, support-profile
  assignment, avatars, role icons), plus non-availability approval and a
  roster-wide view.
- **Mobile (React Native)**: Member-facing — profile creation, avatar,
  own-assignment view, availability calendar, non-availability request
  submission and status.
- **Backend (`PottersInventory/Server`)**: authentication, Members,
  Assignments, and Non-Availability Requests REST endpoints shared by both
  clients.
