[Setup]
AppName=Tonic
AppVersion=2.0.0
AppVerName=Tonic v2.0.0
AppPublisher=Carbonated Audio
AppPublisherURL=https://carbonatedaudio.com
AppSupportURL=https://carbonatedaudio.com/faq
DefaultDirName={commonpf64}\Carbonated Audio\Tonic
DefaultGroupName=Carbonated Audio
OutputBaseFilename=Tonic-v2.0.0-Windows-Installer
OutputDir=..\..\dist\Windows\Installer
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
UninstallDisplayName=Tonic
WizardStyle=modern
DisableProgramGroupPage=yes
PrivilegesRequired=admin

[Types]
Name: "full";       Description: "Full installation (VST3 + Standalone)"
Name: "vst3only";   Description: "VST3 plugin only"
Name: "custom";     Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3";       Description: "Tonic VST3 Plugin";    Types: full vst3only custom; Flags: fixed
Name: "standalone"; Description: "Tonic Standalone App"; Types: full custom

[Files]
Source: "..\..\dist\Windows\VST3\Tonic.vst3\*"; DestDir: "{commonpf64}\Common Files\VST3\Tonic.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\dist\Windows\Standalone\Tonic.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\Tonic"; Filename: "{app}\Tonic.exe"; Components: standalone
Name: "{group}\Uninstall Tonic"; Filename: "{uninstallexe}"
Name: "{commondesktop}\Tonic"; Filename: "{app}\Tonic.exe"; Components: standalone; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Components: standalone; GroupDescription: "Additional options:"

[Run]
Filename: "{app}\Tonic.exe"; Description: "Launch Tonic"; Flags: nowait postinstall skipifsilent; Components: standalone

[Messages]
WelcomeLabel2=This will install Tonic v2.0.0 by Carbonated Audio on your computer.%n%nThe VST3 plugin will be installed to the standard VST3 folder so FL Studio, Cubase, Reaper and other DAWs can find it automatically.
