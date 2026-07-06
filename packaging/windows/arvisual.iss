#define AppName "ArVisual Smart Color Enhancer for OBS"
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\\..\\dist\\windows-manual"
#endif
#ifndef OutputDir
  #define OutputDir "..\\..\\release"
#endif

[Setup]
AppId={{5B3E3D32-314A-4D1F-BBB7-9A9E8F0A0011}}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Mas Ari
AppPublisherURL=https://github.com/masarray/arvisual-obs
AppSupportURL=https://github.com/masarray/arvisual-obs/issues
AppUpdatesURL=https://github.com/masarray/arvisual-obs/releases
DefaultDirName={autopf}\obs-studio
DisableProgramGroupPage=yes
DisableReadyPage=no
DisableFinishedPage=no
PrivilegesRequired=admin
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
OutputDir={#OutputDir}
OutputBaseFilename=ArVisual-OBS-Setup-v{#AppVersion}-windows-x64
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=ArVisual Smart Color Enhancer for OBS

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#SourceDir}\obs-plugins\64bit\arvisual.dll"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion
Source: "{#SourceDir}\data\obs-plugins\arvisual\*"; DestDir: "{app}\data\obs-plugins\arvisual"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\README-INSTALL-WINDOWS.txt"; DestDir: "{app}\data\obs-plugins\arvisual"; Flags: ignoreversion skipifsourcedoesntexist

[InstallDelete]
Type: files; Name: "{app}\obs-plugins\64bit\arvisual.pdb"

[UninstallDelete]
Type: files; Name: "{app}\obs-plugins\64bit\arvisual.dll"
Type: filesandordirs; Name: "{app}\data\obs-plugins\arvisual"

[Code]
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  if Exec(ExpandConstant('{cmd}'), '/C tasklist /FI "IMAGENAME eq obs64.exe" | find /I "obs64.exe"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode) then
  begin
    if ResultCode = 0 then
    begin
      MsgBox('OBS Studio is currently running. Please close OBS Studio before installing ArVisual, then run this installer again.', mbError, MB_OK);
      Result := False;
      exit;
    end;
  end;

  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    MsgBox('ArVisual files are installed. Restart OBS Studio, then add the filter from Source > Filters > + > ArVisual - Smart Color Enhancer.', mbInformation, MB_OK);
  end;
end;
