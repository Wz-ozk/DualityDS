; DualityDS installer script (Inno Setup 6)
; Build:  "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" dist\DualityDS.iss
; Run from repo root (melonDS-src). Source = lean portable folder.

#define MyAppName "DualityDS"
#define MyAppVersion "0.1"
#define MyAppPublisher "Weziliey"
#define MyAppURL "https://github.com/wesleywtchang-jpg/DualityDS"
#define MyAppExeName "DualityDS.exe"
#define SourceDir "DualityDS-portable-lean"

[Setup]
AppId={{8F3A1C9E-DUAL-4D2B-9A77-DUALITYDSNDS}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
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
Name: "ndsassoc"; Description: "Associate .nds ROM files with DualityDS"; GroupDescription: "File associations:"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; .nds association (opt-in via ndsassoc task)
Root: HKA; Subkey: "Software\Classes\.nds\OpenWithProgids"; ValueType: string; ValueName: "DualityDS.nds"; ValueData: ""; Flags: uninsdeletevalue; Tasks: ndsassoc
Root: HKA; Subkey: "Software\Classes\DualityDS.nds"; ValueType: string; ValueName: ""; ValueData: "Nintendo DS ROM"; Flags: uninsdeletekey; Tasks: ndsassoc
Root: HKA; Subkey: "Software\Classes\DualityDS.nds\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"; Tasks: ndsassoc
Root: HKA; Subkey: "Software\Classes\DualityDS.nds\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: ndsassoc

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
