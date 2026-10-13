; Inno Setup 6.x Script for Video Merger Pro
; Target: Windows 10 / 11 (64-bit)
; Location: C:\installer.iss

#define MyAppName "Video Merger Pro"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "MediaTools"
#define MyAppExeName "VideoMerger.exe"
; Direct path to your staged application directory:
#define AppSourceDir "C:\VideoMergerApp"

[Setup]
; Unique application GUID
AppId={{C47F8B2E-318C-4D27-B9C0-8E490D588B41}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}

; Icon used for the installer setup executable itself:
SetupIconFile=C:\app_icon.ico

; Ensure native 64-bit installation in "Program Files"
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

DisableProgramGroupPage=yes
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=commandline
; Installer output directory (creates C:\Output):
OutputDir="C:\Output"
OutputBaseFilename=VideoMergerPro_Setup_v{#MyAppVersion}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Main executable
Source: "{#AppSourceDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; Deployed Qt runtime DLLs and plugin directories, excluding scripts, build artifacts, and output folders
Source: "{#AppSourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.pdb,*.ilk,*.exp,*.lib,*.iss,*.log,Output\*"

[Icons]
; Start Menu shortcut (automatically inherits icon embedded in VideoMerger.exe)
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
; Desktop shortcut (optional task)
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
; Uninstaller shortcut
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

[Run]
; Launch option after installation completes
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent