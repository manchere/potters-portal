; Inno Setup script for the Potters Portal desktop app (Windows x64).
; Built by build-installer.ps1, which first puts the Release build and
; everything it needs (Qt, the Postgres driver, the C++ runtime) in
; ..\dist\stage. Output: ..\dist\PottersPortal-Setup-x64.exe
;
; Installs per user (no administrator rights needed), adds Start menu and
; optional desktop shortcuts, and registers an uninstaller. Nothing secret
; is inside: the app asks for the database connection on first launch.

#define AppName "Potters Portal"
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif

[Setup]
AppId={{6F2B8C4E-2B7A-4C61-9E0B-5B1D7A3C9F21}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Potters House
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=PottersPortal-Setup-x64
SetupIconFile=..\src\pottershouse.ico
UninstallDisplayIcon={app}\PottersPortal.exe
UninstallDisplayName={#AppName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "..\dist\stage\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\PottersPortal.exe"
Name: "{group}\{cm:UninstallProgram,{#AppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\PottersPortal.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\PottersPortal.exe"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent
