# Installing and using Potters Portal (Windows)

Potters Portal runs on **Windows 10 or 11, 64-bit**. A macOS version isn't
available yet.

## 1. Download

Download **`PottersPortal-Setup-x64.exe`** (about 24 MB) from wherever your
Admin shared it (for example the project's GitHub *Releases* page).

## 2. Install

1. Double-click `PottersPortal-Setup-x64.exe`.
2. Windows may show **"Windows protected your PC"**, because the installer
   isn't signed with a paid certificate. Click **More info → Run anyway**.
3. Choose the language for the installer, then follow the steps. You can
   tick **Create a desktop shortcut**.
4. No administrator rights are needed: it installs for your Windows account
   only (in `%LOCALAPPDATA%\Programs\Potters Portal`). If you run it as an
   administrator you can choose to install it for all users instead.
5. Leave **Launch Potters Portal** ticked and click **Finish**.

## 3. First launch: connect to the database

The first time it starts, Potters Portal asks for the **database connection
string** — the address and password of the church's online database. Your
Admin gives you this; it looks like:

```
postgresql://user:password@host/neondb?sslmode=require
```

Paste it and click **Connect**. Once it connects, it's saved for your Windows
account, so you won't be asked again. If it ever stops working (for example
the password was changed), the app asks again on its next start and shows
what went wrong.

> Treat the connection string like a password: anyone who has it can read
> and change all of the church's data. Don't email it around or paste it in
> group chats — see [security.md](security.md).

The app needs an internet connection. Networks that block outgoing database
connections (port 5432) — some company or school Wi-Fi — will stop it from
loading data.

## 4. Using it

- **Browse** — without signing in, you can open the tabs everyone is allowed
  to see (by default: all of them) and send feedback.
- **Sign in** — click the 🔒 lock at the top right and enter your email and
  password. You then get whatever your Admin has allowed you (adding items,
  editing songs, …). Admins can do everything, including the schedule. Click
  the 🔓 lock again to sign out.
- **Language and look** — *Settings* switches between English and French
  (the app restarts) and between the Light, Black and Navy & Gold themes.
- **Something wrong?** — use the *Feedback* tab to report a bug or ask for a
  feature or a change to your profile.

For Admins, *Settings → Access rights* decides who can open and change what.

## 5. Updating

Run the newer `PottersPortal-Setup-x64.exe` over the existing install. Your
saved connection, language and theme are kept.

## 6. Uninstalling

*Settings → Apps → Installed apps → Potters Portal → Uninstall* (or the
*Uninstall Potters Portal* shortcut in the Start menu). This removes the
program; the saved connection, language and theme stay in your Windows
account's settings
(`HKEY_CURRENT_USER\Software\PottersPortal`) unless you delete them.

---

## For maintainers: building the installer

On a Windows PC set up for development (Visual Studio with C++, Qt 6.9
`msvc2022_64`, the `build` folder configured — see the project setup):

1. Install Inno Setup once:

   ```
   winget install JRSoftware.InnoSetup
   ```

2. From the repository root:

   ```
   powershell -ExecutionPolicy Bypass -File installer\build-installer.ps1 -Version 1.0.0
   ```

This builds the Release version, gathers Qt, the PostgreSQL driver
(`libpq`, OpenSSL) and the Microsoft C++ runtime into `dist\stage`, and
compiles [`installer/PottersPortal.iss`](../installer/PottersPortal.iss) into
**`dist\PottersPortal-Setup-x64.exe`**. Nothing secret is packaged — the
database connection is entered on first launch. `dist\` is git-ignored;
attach the `.exe` to a GitHub Release (or share it another way) for people
to download.
