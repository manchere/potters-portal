# Potters Inventory — Mobile

React Native (Expo) app for iOS/Android. It talks to `PottersInventoryServer`
over HTTP — the same REST API the `web/` frontend uses — so it shares one
Postgres-backed source of truth with the desktop app and browser UI. It does
not embed any business logic; every read/write goes through the server's
`Controllers/`.

See [FUNCTIONAL_REQUIREMENTS.md](FUNCTIONAL_REQUIREMENTS.md) for the
detailed requirements and acceptance criteria behind each camera/scan flow.

## Screens

- **Item list** — browse inventory, pull to refresh.
- **Add item** (FR1) — opens the camera, takes a photo, sends it to
  `POST /api/vision/describe-item` (Groq vision) to pre-fill a name/description,
  then lets the member review/edit before saving. The photo is attached via
  `POST /api/items/:id/image` after the item is created.
- **Scan** (FR2) — opens the camera in barcode/QR mode, looks the code up via
  `GET /api/items/barcode/:code`, and jumps to that item's detail. An
  unmatched code offers "Add as New Item" instead of dead-ending.
- **Identify by photo** (FR3) — opens the camera, takes a photo, gets a
  vision suggestion, then ranks it against every existing item's name to
  show "possible matches" the member can pick from. No match carries the
  photo + suggestion forward into Add Item instead of dead-ending.
- **Item detail** — shows the stored photo, status, quantity, location, and
  barcode.

## Backend prerequisites

This app needs the corresponding backend changes, already applied in this
branch:

- `database/migrations/0009_add_barcode_to_items.sql` adds a nullable,
  unique `items.barcode` column.
- `Controllers/ItemController` gained `itemByBarcode()`.
- `Server/ServerMain.cpp` gained `GET /api/items/barcode/<code>`.

Run migrations and start `PottersInventoryServer` before using the app.

## Setup

```
cd mobile
npm install
npx expo start
```

Then press `a` for an Android emulator, `i` for an iOS simulator, or scan the
QR code with Expo Go on a physical device.

### Pointing the app at your server

`src/config.ts` defaults to `http://localhost:8080` (iOS simulator/web) or
`http://10.0.2.2:8080` (Android emulator — its alias for the host machine).
A physical device can't reach either; set your machine's LAN IP instead:

```
EXPO_PUBLIC_API_URL=http://192.168.1.50:8080 npx expo start
```

The server must be reachable from the phone's network (same Wi-Fi, or a
tunnel) — `localhost` on the phone means the phone itself.

## Structure

```
mobile/
  App.tsx                  # entry point, mounts navigation
  src/
    config.ts               # API_BASE_URL resolution
    api/
      client.ts             # fetch wrapper, one method per endpoint
      types.ts               # Item/Tag/Category, mirrors Server/Json.cpp
    navigation/
      index.tsx              # stack navigator + route param types
    screens/
      ItemListScreen.tsx
      ItemDetailScreen.tsx
      AddItemScreen.tsx       # camera capture -> vision suggest -> save (FR1)
      ScanScreen.tsx           # barcode scan -> lookup -> detail (FR2)
      IdentifyScreen.tsx        # photo -> vision suggest -> ranked matches (FR3)
```

## Not yet implemented

- Editing/deleting items, tag and category management (list-only against
  those endpoints today — `api.tags.list()` / `api.categories.list()` are
  wired up but no screens use them yet).
- Offline queueing — every action requires a live connection to the server.
- Auth — the REST API currently has none; add it at the server before
  shipping this beyond a trusted local network.
# pottersinventory
