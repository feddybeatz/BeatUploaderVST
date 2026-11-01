#define MyAppName "BeatUploader"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Feddy Beatz"
#define MyAppExeName "BeatUploader.exe"

[Setup]
AppId={{8E7C35C0-1B9A-48B5-91EE-FD66FA4315E7}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
ArchitecturesAllowed=x64os
ArchitecturesInstallIn64BitMode=x64os
PrivilegesRequired=admin
OutputDir=Output
OutputBaseFilename=BeatUploader-Installer
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Types]
Name: "full"; Description: "Standalone app and VST3 plugin"
Name: "custom"; Description: "Choose components"; Flags: iscustom

[Components]
Name: "standalone"; Description: "BeatUploader standalone app"; Types: full custom
Name: "vst3"; Description: "BeatUploader VST3 plugin"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked; Components: standalone

[Files]
; Release payloads currently staged at the project root.
Source: "..\BeatUploader.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone
; In 64-bit install mode, {commoncf} resolves to C:\Program Files\Common Files.
Source: "..\BeatUploader.vst3\*"; DestDir: "{commoncf}\VST3\BeatUploader.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3

[Icons]
Name: "{group}\BeatUploader"; Filename: "{app}\{#MyAppExeName}"; Components: standalone
Name: "{autodesktop}\BeatUploader"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; Components: standalone
