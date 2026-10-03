# Builds the Windows x64 installer: dist\PottersPortal-Setup-x64.exe
#
#   powershell -ExecutionPolicy Bypass -File installer\build-installer.ps1 [-Version 1.0.0]
#
# Needs: the configured build folder (see docs/installing.md), Qt 6
# (windeployqt runs as part of the build), Visual Studio's C++
# redistributables, and Inno Setup 6 (winget install JRSoftware.InnoSetup).

param(
    [string]$Version = "1.0.0"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$release = Join-Path $root "build\src\Release"
$stage = Join-Path $root "dist\stage"

# 1. Release build. Its post-build steps already run windeployqt and copy
#    libpq + OpenSSL next to the exe.
cmake --build (Join-Path $root "build") --target PottersPortal --config Release
if ($LASTEXITCODE -ne 0) { throw "Release build failed" }

# 2. Stage a clean copy of the app folder.
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item -Recurse -Force (Join-Path $release "*") $stage

# Only the Postgres driver is used; drop the other database drivers.
Get-ChildItem (Join-Path $stage "sqldrivers") -Filter "*.dll" |
    Where-Object { $_.Name -ne "qsqlpsql.dll" } |
    Remove-Item -Force

# 3. The Microsoft C++ runtime, app-local, so it runs on PCs that don't have
#    the Visual C++ Redistributable installed. Newest CRT found wins.
$vsRoots = @("C:\Program Files\Microsoft Visual Studio", "C:\Program Files (x86)\Microsoft Visual Studio")
$crt = Get-ChildItem $vsRoots -Recurse -Directory -Filter "Microsoft.VC14*.CRT" -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "\\x64\\" -and $_.FullName -notmatch "onecore" } |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $crt) { throw "Couldn't find the Visual C++ runtime (Microsoft.VC14*.CRT) under Visual Studio." }
Copy-Item (Join-Path $crt.FullName "*.dll") $stage
Write-Host "C++ runtime from $($crt.FullName)"

# 4. Compile the installer.
$iscc = @(
    (Join-Path $env:LOCALAPPDATA "Programs\Inno Setup 6\ISCC.exe"),
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe",
    "C:\Program Files\Inno Setup 6\ISCC.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { throw "Inno Setup 6 not found. Install it with: winget install JRSoftware.InnoSetup" }
& $iscc "/DAppVersion=$Version" (Join-Path $PSScriptRoot "PottersPortal.iss")
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed" }

Write-Host "Installer: $(Join-Path $root 'dist\PottersPortal-Setup-x64.exe')"
