#define AppName "ArVisual Smart Color Enhancer for OBS"
#ifndef AppVersion
  #define AppVersion "0.5.8"
#endif
#ifndef SourceDir
  #define SourceDir "..\\..\\dist\\windows-manual"
#endif
#ifndef OutputDir
  #define OutputDir "..\\..\\release"
#endif

#define DllSource SourceDir + "\\obs-plugins\\64bit\\arvisual.dll"
#define EffectSource SourceDir + "\\data\\obs-plugins\\arvisual\\effects\\arvisual.effect"
#define EnLocaleSource SourceDir + "\\data\\obs-plugins\\arvisual\\locale\\en-US.ini"
#define IdLocaleSource SourceDir + "\\data\\obs-plugins\\arvisual\\locale\\id-ID.ini"
#define DllHash GetSHA256OfFile(DllSource)
#define EffectHash GetSHA256OfFile(EffectSource)
#define EnLocaleHash GetSHA256OfFile(EnLocaleSource)
#define IdLocaleHash GetSHA256OfFile(IdLocaleSource)

[Setup]
AppId={{5B3E3D32-314A-4D1F-BBB7-9A9E8F0A0011}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Mas Ari
AppPublisherURL=https://github.com/masarray/arvisual-obs
AppSupportURL=https://github.com/masarray/arvisual-obs/issues
AppUpdatesURL=https://github.com/masarray/arvisual-obs/releases
DefaultDirName={code:GetDefaultObsDir}
UsePreviousAppDir=yes
DirExistsWarning=no
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
Source: "{#DllSource}"; DestDir: "{app}\obs-plugins\64bit"; Flags: ignoreversion
Source: "{#SourceDir}\data\obs-plugins\arvisual\*"; DestDir: "{app}\data\obs-plugins\arvisual"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\README-INSTALL-WINDOWS.txt"; DestDir: "{app}\data\obs-plugins\arvisual"; Flags: ignoreversion skipifsourcedoesntexist

[InstallDelete]
Type: files; Name: "{app}\obs-plugins\64bit\arvisual.pdb"

[UninstallDelete]
Type: files; Name: "{app}\obs-plugins\64bit\arvisual.dll"
Type: filesandordirs; Name: "{app}\data\obs-plugins\arvisual"

[Code]
var
  ObsAutoDetected: Boolean;
  ObsDetectionWarningShown: Boolean;

function IsObsRoot(const Candidate: String): Boolean;
var
  Root: String;
begin
  Root := AddBackslash(Candidate);
  Result :=
    FileExists(Root + 'bin\64bit\obs64.exe') and
    DirExists(Root + 'obs-plugins\64bit') and
    DirExists(Root + 'data\obs-plugins');
end;

function StripOuterQuotes(Value: String): String;
begin
  Value := Trim(Value);

  if (Length(Value) > 0) and (Value[1] = '"') then
    Delete(Value, 1, 1);

  if (Length(Value) > 0) and (Value[Length(Value)] = '"') then
    Delete(Value, Length(Value), 1);

  Result := Trim(Value);
end;

function ExtractExecutablePath(Value: String): String;
var
  ExeEnd: Integer;
begin
  Value := Trim(Value);
  ExeEnd := Pos('.exe', Lowercase(Value));

  if ExeEnd > 0 then
    Value := Copy(Value, 1, ExeEnd + 3);

  Result := StripOuterQuotes(Value);
end;

function TryObsExecutable(const ExecutablePath: String; var ObsDir: String): Boolean;
var
  Candidate: String;
begin
  Result := False;

  if not FileExists(ExecutablePath) then
    exit;

  Candidate := ExtractFileDir(ExtractFileDir(ExtractFileDir(ExecutablePath)));
  if IsObsRoot(Candidate) then
  begin
    ObsDir := Candidate;
    Result := True;
  end;
end;

function TryObsDirectory(const Candidate: String; var ObsDir: String): Boolean;
begin
  Result := IsObsRoot(Candidate);
  if Result then
    ObsDir := Candidate;
end;

function FindObsFromAppPaths(RootKey: Integer; var ObsDir: String): Boolean;
var
  Value: String;
begin
  Result := False;

  if RegQueryStringValue(
       RootKey,
       'SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\obs64.exe',
       '',
       Value) then
  begin
    Value := ExtractExecutablePath(Value);
    Result := TryObsExecutable(Value, ObsDir);
  end;
end;

function FindObsFromUninstall(RootKey: Integer; var ObsDir: String): Boolean;
var
  UninstallRoot: String;
  SubKeys: TArrayOfString;
  I: Integer;
  KeyPath: String;
  DisplayName: String;
  InstallLocation: String;
  DisplayIcon: String;
begin
  Result := False;
  UninstallRoot := 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall';

  if not RegGetSubkeyNames(RootKey, UninstallRoot, SubKeys) then
    exit;

  if GetArrayLength(SubKeys) = 0 then
    exit;

  for I := 0 to GetArrayLength(SubKeys) - 1 do
  begin
    KeyPath := UninstallRoot + '\' + SubKeys[I];
    DisplayName := '';

    if RegQueryStringValue(RootKey, KeyPath, 'DisplayName', DisplayName) and
       (Pos('obs studio', Lowercase(Trim(DisplayName))) = 1) then
    begin
      InstallLocation := '';
      if RegQueryStringValue(RootKey, KeyPath, 'InstallLocation', InstallLocation) then
      begin
        InstallLocation := StripOuterQuotes(InstallLocation);
        if TryObsDirectory(InstallLocation, ObsDir) then
        begin
          Result := True;
          exit;
        end;
      end;

      DisplayIcon := '';
      if RegQueryStringValue(RootKey, KeyPath, 'DisplayIcon', DisplayIcon) then
      begin
        DisplayIcon := ExtractExecutablePath(DisplayIcon);
        if TryObsExecutable(DisplayIcon, ObsDir) then
        begin
          Result := True;
          exit;
        end;
      end;
    end;
  end;
end;

function FindObsInRegistry(var ObsDir: String): Boolean;
begin
  Result :=
    FindObsFromAppPaths(HKLM64, ObsDir) or
    FindObsFromAppPaths(HKCU64, ObsDir) or
    FindObsFromUninstall(HKLM64, ObsDir) or
    FindObsFromUninstall(HKCU64, ObsDir) or
    FindObsFromAppPaths(HKLM32, ObsDir) or
    FindObsFromAppPaths(HKCU32, ObsDir) or
    FindObsFromUninstall(HKLM32, ObsDir) or
    FindObsFromUninstall(HKCU32, ObsDir);
end;

function FindObsInstallDir(var ObsDir: String): Boolean;
var
  Candidate: String;
begin
  Result := False;
  ObsDir := '';

  if FindObsInRegistry(ObsDir) then
  begin
    Result := True;
    exit;
  end;

  Candidate := ExpandConstant('{autopf}\obs-studio');
  if TryObsDirectory(Candidate, ObsDir) then
  begin
    Result := True;
    exit;
  end;

  Candidate := ExpandConstant('{localappdata}\Programs\obs-studio');
  if TryObsDirectory(Candidate, ObsDir) then
  begin
    Result := True;
    exit;
  end;

  Candidate := ExpandConstant('{pf32}\obs-studio');
  Result := TryObsDirectory(Candidate, ObsDir);
end;

function GetDefaultObsDir(Param: String): String;
var
  DetectedDir: String;
begin
  ObsAutoDetected := FindObsInstallDir(DetectedDir);

  if ObsAutoDetected then
    Result := DetectedDir
  else
    Result := ExpandConstant('{autopf}\obs-studio');
end;

function VerifyInstalledFile(const RelativePath, ExpectedHash: String): Boolean;
var
  InstalledPath: String;
  ActualHash: String;
begin
  InstalledPath := AddBackslash(ExpandConstant('{app}')) + RelativePath;

  if not FileExists(InstalledPath) then
  begin
    Log('ArVisual verification failed: missing ' + InstalledPath);
    Result := False;
    exit;
  end;

  ActualHash := GetSHA256OfFile(InstalledPath);
  Result := CompareText(ActualHash, ExpectedHash) = 0;

  if Result then
    Log('ArVisual verified: ' + InstalledPath)
  else
    Log('ArVisual verification failed: hash mismatch for ' + InstalledPath);
end;

procedure VerifyInstalledPayload();
begin
  if not VerifyInstalledFile('obs-plugins\64bit\arvisual.dll', '{#DllHash}') then
    RaiseException('The installed ArVisual DLL could not be verified.');

  if not VerifyInstalledFile('data\obs-plugins\arvisual\effects\arvisual.effect', '{#EffectHash}') then
    RaiseException('The installed ArVisual effect file could not be verified.');

  if not VerifyInstalledFile('data\obs-plugins\arvisual\locale\en-US.ini', '{#EnLocaleHash}') then
    RaiseException('The installed ArVisual English locale could not be verified.');

  if not VerifyInstalledFile('data\obs-plugins\arvisual\locale\id-ID.ini', '{#IdLocaleHash}') then
    RaiseException('The installed ArVisual Indonesian locale could not be verified.');
end;

function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
begin
  if Exec(
       ExpandConstant('{cmd}'),
       '/C tasklist /FI "IMAGENAME eq obs64.exe" | find /I "obs64.exe"',
       '',
       SW_HIDE,
       ewWaitUntilTerminated,
       ResultCode) then
  begin
    if ResultCode = 0 then
    begin
      MsgBox(
        'OBS Studio is currently running. Close OBS Studio before installing ArVisual, then run this installer again.',
        mbError,
        MB_OK);
      Result := False;
      exit;
    end;
  end;

  Result := True;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if (CurPageID = wpSelectDir) and
     (not ObsAutoDetected) and
     (not ObsDetectionWarningShown) then
  begin
    ObsDetectionWarningShown := True;
    MsgBox(
      'OBS Studio was not detected automatically.' + #13#10 + #13#10 +
      'Install OBS Studio first, or select the root folder of an existing or portable OBS installation. The selected folder must contain:' + #13#10 +
      'bin\64bit\obs64.exe',
      mbInformation,
      MB_OK);
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;

  if (CurPageID = wpSelectDir) and (not IsObsRoot(WizardDirValue)) then
  begin
    MsgBox(
      'The selected folder is not a valid 64-bit OBS Studio installation.' + #13#10 + #13#10 +
      'Select the OBS Studio root folder that contains:' + #13#10 +
      'bin\64bit\obs64.exe',
      mbError,
      MB_OK);
    Result := False;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  if not IsObsRoot(ExpandConstant('{app}')) then
    Result := 'ArVisual cannot be installed because the selected folder is not a valid 64-bit OBS Studio installation.'
  else
    Result := '';
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    VerifyInstalledPayload();
    MsgBox(
      'ArVisual is installed and verified in OBS Studio.' + #13#10 + #13#10 +
      'Open OBS Studio, then add the filter from:' + #13#10 +
      'Source > Filters > + > ArVisual - Smart Color Enhancer',
      mbInformation,
      MB_OK);
  end;
end;
