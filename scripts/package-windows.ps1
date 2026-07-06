[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $StageRoot,

    [string] $Version = "0.1.0",
    [string] $Configuration = "RelWithDebInfo"
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$ReleaseDir = Join-Path $RepoRoot 'release'
$PackageName = "arvisual-obs-v$Version-windows-x64"
$PackageRoot = Join-Path $ReleaseDir $PackageName
$ZipPath = Join-Path $ReleaseDir "$PackageName.zip"

if (Test-Path $PackageRoot) { Remove-Item $PackageRoot -Recurse -Force }
New-Item -ItemType Directory -Force -Path $PackageRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $PackageRoot 'obs-plugins/64bit') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $PackageRoot 'data/obs-plugins/arvisual') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $PackageRoot 'tools') | Out-Null

$DllCandidates = @(
    (Join-Path $StageRoot 'arvisual/bin/64bit/arvisual.dll'),
    (Join-Path $StageRoot 'obs-plugins/64bit/arvisual.dll')
)

$Dll = $DllCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Dll) {
    $Dll = Get-ChildItem -Path $StageRoot -Filter 'arvisual.dll' -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
}
if (-not $Dll) { throw "arvisual.dll was not found under stage root: $StageRoot" }
$DllPath = if ($Dll -is [System.IO.FileInfo]) { $Dll.FullName } else { [string] $Dll }

Copy-Item $DllPath -Destination (Join-Path $PackageRoot 'obs-plugins/64bit/arvisual.dll') -Force

$DataCandidates = @(
    (Join-Path $StageRoot 'arvisual/data'),
    (Join-Path $StageRoot 'data/obs-plugins/arvisual')
)
$DataRoot = $DataCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $DataRoot) { throw "ArVisual data folder was not found under stage root: $StageRoot" }
Copy-Item (Join-Path $DataRoot '*') -Destination (Join-Path $PackageRoot 'data/obs-plugins/arvisual') -Recurse -Force

@"
ArVisual Smart Color Enhancer for OBS
=====================================

Install for normal OBS Studio on Windows:
1. Close OBS Studio.
2. Right-click install-to-obs-windows.bat -> Run as administrator.
3. Open OBS Studio.
4. Right-click a video source -> Filters -> + -> ArVisual - Smart Color Enhancer.
5. Start with preset: Kids Toy Pop or MasAri Dopamine.

Portable OBS / non-default OBS folder:
1. Copy this package's 'obs-plugins' and 'data' folders into the root folder that contains OBS's 'bin' folder.
2. The expected final files are:
   - <OBS ROOT>\obs-plugins\64bit\arvisual.dll
   - <OBS ROOT>\data\obs-plugins\arvisual\effects\arvisual.effect
   - <OBS ROOT>\data\obs-plugins\arvisual\locale\en-US.ini

If the filter does not appear:
1. Run tools\diagnose-arvisual-obs.bat
2. Open OBS -> Help -> Log Files -> View Current Log
3. Search for: arvisual

Uninstall:
Delete:
- obs-plugins\64bit\arvisual.dll
- data\obs-plugins\arvisual
"@ | Set-Content -Path (Join-Path $PackageRoot 'README-INSTALL-WINDOWS.txt') -Encoding UTF8

@"
@echo off
setlocal EnableExtensions

net session >nul 2>&1
if not "%errorlevel%"=="0" (
  echo Requesting administrator permission...
  powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\install-to-obs-windows.ps1" -PackageRoot "%~dp0"
set ERR=%errorlevel%
echo.
if "%ERR%"=="0" (
  echo Done. Restart OBS Studio and add filter: ArVisual - Smart Color Enhancer.
) else (
  echo Install failed. Run tools\diagnose-arvisual-obs.bat for details.
)
pause
exit /b %ERR%
"@ | Set-Content -Path (Join-Path $PackageRoot 'install-to-obs-windows.bat') -Encoding ASCII

@"
[CmdletBinding()]
param(
    [Parameter(Mandatory = `$true)]
    [string] `$PackageRoot,

    [string] `$ObsRoot = ''
)

`$ErrorActionPreference = 'Stop'

function Get-ObsRootCandidate {
    `$runningObs = Get-Process -Name obs64 -ErrorAction SilentlyContinue | Select-Object -First 1
    if (`$runningObs -and `$runningObs.Path) {
        try {
            `$bin64 = Split-Path -Parent `$runningObs.Path
            `$bin = Split-Path -Parent `$bin64
            `$root = Split-Path -Parent `$bin
            if (Test-Path -LiteralPath (Join-Path `$root 'bin\64bit\obs64.exe')) {
                Write-Host "[INFO] Detected running OBS root: `$root"
                return `$root
            }
        } catch {}
    }

    `$default = Join-Path `$env:ProgramFiles 'obs-studio'
    if (Test-Path -LiteralPath (Join-Path `$default 'bin\64bit\obs64.exe')) {
        return `$default
    }

    `$defaultX86 = Join-Path `${env:ProgramFiles(x86)} 'obs-studio'
    if (Test-Path -LiteralPath (Join-Path `$defaultX86 'bin\64bit\obs64.exe')) {
        return `$defaultX86
    }

    return `$default
}

if (-not `$ObsRoot) { `$ObsRoot = Get-ObsRootCandidate }

if (-not (Test-Path -LiteralPath (Join-Path `$ObsRoot 'bin\64bit\obs64.exe'))) {
    throw "OBS Studio was not found at '`$ObsRoot'. For portable OBS, run this PowerShell manually with -ObsRoot '<OBS root folder>'."
}

`$obsProcess = Get-Process -Name obs64 -ErrorAction SilentlyContinue
if (`$obsProcess) {
    Write-Warning 'OBS is currently running. Close OBS now, then press Enter to continue.'
    Read-Host | Out-Null
}

`$sourceObsPlugins = Join-Path `$PackageRoot 'obs-plugins'
`$sourceData = Join-Path `$PackageRoot 'data'
`$targetObsPlugins = Join-Path `$ObsRoot 'obs-plugins'
`$targetData = Join-Path `$ObsRoot 'data'

Write-Host "[INFO] Installing ArVisual to: `$ObsRoot"
Copy-Item -Path `$sourceObsPlugins -Destination `$ObsRoot -Recurse -Force
Copy-Item -Path `$sourceData -Destination `$ObsRoot -Recurse -Force

`$checks = @(
    (Join-Path `$ObsRoot 'obs-plugins\64bit\arvisual.dll'),
    (Join-Path `$ObsRoot 'data\obs-plugins\arvisual\effects\arvisual.effect'),
    (Join-Path `$ObsRoot 'data\obs-plugins\arvisual\locale\en-US.ini')
)

foreach (`$path in `$checks) {
    if (Test-Path -LiteralPath `$path) {
        Write-Host "[OK] `$path"
    } else {
        throw "Missing expected installed file: `$path"
    }
}

Write-Host ''
Write-Host '[OK] ArVisual files installed.'
Write-Host 'Restart OBS Studio. Then open Filters -> + -> ArVisual - Smart Color Enhancer.'
Write-Host 'If it still does not appear, run tools\diagnose-arvisual-obs.bat and check the OBS log for arvisual.'
"@ | Set-Content -Path (Join-Path $PackageRoot 'tools/install-to-obs-windows.ps1') -Encoding UTF8

@"
@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0diagnose-arvisual-obs.ps1"
echo.
pause
"@ | Set-Content -Path (Join-Path $PackageRoot 'tools/diagnose-arvisual-obs.bat') -Encoding ASCII

@"
[CmdletBinding()]
param([string] `$ObsRoot = '')

function Get-ObsRootCandidate {
    `$runningObs = Get-Process -Name obs64 -ErrorAction SilentlyContinue | Select-Object -First 1
    if (`$runningObs -and `$runningObs.Path) {
        try {
            `$bin64 = Split-Path -Parent `$runningObs.Path
            `$bin = Split-Path -Parent `$bin64
            `$root = Split-Path -Parent `$bin
            if (Test-Path -LiteralPath (Join-Path `$root 'bin\64bit\obs64.exe')) {
                return `$root
            }
        } catch {}
    }
    `$default = Join-Path `$env:ProgramFiles 'obs-studio'
    if (Test-Path -LiteralPath (Join-Path `$default 'bin\64bit\obs64.exe')) { return `$default }
    return `$default
}

if (-not `$ObsRoot) { `$ObsRoot = Get-ObsRootCandidate }
Write-Host "[INFO] OBS root checked: `$ObsRoot"

`$checks = @(
    (Join-Path `$ObsRoot 'bin\64bit\obs64.exe'),
    (Join-Path `$ObsRoot 'obs-plugins\64bit\arvisual.dll'),
    (Join-Path `$ObsRoot 'data\obs-plugins\arvisual\effects\arvisual.effect'),
    (Join-Path `$ObsRoot 'data\obs-plugins\arvisual\locale\en-US.ini')
)

foreach (`$path in `$checks) {
    if (Test-Path -LiteralPath `$path) {
        Write-Host "[OK] `$path"
    } else {
        Write-Host "[MISSING] `$path" -ForegroundColor Yellow
    }
}

`$logDir = Join-Path `$env:APPDATA 'obs-studio\logs'
Write-Host ''
Write-Host "[INFO] OBS log folder: `$logDir"
if (Test-Path -LiteralPath `$logDir) {
    `$latestLog = Get-ChildItem -LiteralPath `$logDir -Filter '*.txt' | Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (`$latestLog) {
        Write-Host "[INFO] Latest OBS log: `$(`$latestLog.FullName)"
        `$matches = Select-String -Path `$latestLog.FullName -Pattern 'arvisual|arvisual.dll|Failed to load|LoadLibrary|module' -SimpleMatch:$false -ErrorAction SilentlyContinue
        if (`$matches) {
            Write-Host ''
            Write-Host '[INFO] Relevant log lines:'
            `$matches | Select-Object -Last 30 | ForEach-Object { Write-Host `$_.Line }
        } else {
            Write-Host '[INFO] No arvisual-related line found in latest log. This usually means OBS is not looking at this install folder or OBS has not been restarted after installation.'
        }
    } else {
        Write-Host '[INFO] No OBS log file found yet.'
    }
} else {
    Write-Host '[INFO] OBS log folder does not exist yet.'
}

Write-Host ''
Write-Host 'Expected success line in OBS log:'
Write-Host '  [ArVisual] Smart Color Enhancer loaded'
Write-Host ''
Write-Host 'If files exist but the filter still does not appear, open OBS -> Help -> Log Files -> View Current Log and search for arvisual.dll or ArVisual.'
"@ | Set-Content -Path (Join-Path $PackageRoot 'tools/diagnose-arvisual-obs.ps1') -Encoding UTF8

if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
Compress-Archive -Path (Join-Path $PackageRoot '*') -DestinationPath $ZipPath -Force
Write-Host "Packaged: $ZipPath"
