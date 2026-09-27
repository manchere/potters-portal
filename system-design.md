# System Design

## Overview

A desktop application for managing a church's inventory — sound system equipment, microphones,
music instruments, computer items, etc.

## User Interface Requirements

### Items

- Full CRUD: create, view, update, delete.
- **Creation flow**: done manually through the desktop app by adding a photo. Groq AI then
  auto-fills the item's fields based on the photo.
- Items can be assigned.
- Items can be assigned tags (many-to-many).

### Tags

- Full CRUD: create, view, update, delete.
- Fields: name, color, description.

### Categories

- Full CRUD: create, view, update, delete.

### Members / Scheduling

Adds Member profiles and Sunday duty assignments (Admin scheduling,
non-availability requests). See
[SCHEDULING_FUNCTIONAL_REQUIREMENTS.md](SCHEDULING_FUNCTIONAL_REQUIREMENTS.md)
for the full spec.

### Songs

- Desktop **Songs** tab: a searchable song library (title, artist, key,
  play link, lyrics). Selecting a song shows its details; **Play** opens the
  link in the browser.
- Anyone can view songs; adding, editing, and deleting are Admin-only.
- `GET /api/songs` exposes the library read-only for the mobile app.

<!-- Add more UI requirements here. -->
