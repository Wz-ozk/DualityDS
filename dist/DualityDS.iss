; DualityDS installer script (Inno Setup 6)
; Build:  "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" dist\DualityDS.iss
; Run from repo root (melonDS-src). Source = full-audio portable set (117 DLLs + multimedia + ffmpeg).

#define MyAppName "DualityDS"
#define MyAppVersion "0.1.2"
#define MyAppPublisher "Weziliey"
#define MyAppURL "https://github.com/wesleywtchang-jpg/DualityDS"
#define MyAppExeName "DualityDS.exe"
#define SourceDir "DualityDS-portable"

[Setup]
AppId={{8F3A1C9E-DUAL-4D2B-9A77-DUALITYDSNDS}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
; Per-user install (no admin). {autopf} -> %LOCALAPPDATA%\Programs, which is
; writable, so the portable-mode config/saves next to the exe work fine.
PrivilegesRequired=lowest
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#MyAppExeName}
SetupIconFile=..\res\duality.ico
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=.
OutputBaseFilename=DualityDS-Setup-v{#MyAppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
