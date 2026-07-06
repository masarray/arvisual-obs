[CmdletBinding()]
param(
    [switch] $Quiet,
    [ValidateSet('Auto', '2026', '2022')]
    [string] $VisualStudio = 'Auto'
)

$ErrorActionPreference = 'Stop'

function Write-Ok {
    param([string]$Message)
    if (-not $Quiet) { Write-Host "[OK] $Message" -ForegroundColor Green }
}

function Write-InfoLine {
    param([string]$Message)
    if (-not $Quiet) { Write-Host "[INFO] $Message" -ForegroundColor Cyan }
}

function Fail-WithHelp {
    param([string]$Message)
    Write-Host ''
    Write-Host '[ArVisual build environment check failed]' -ForegroundColor Red
    Write-Host $Message -ForegroundColor Yellow
    Write-Host ''
    Write-Host 'Required Windows build tools:'
    Write-Host '  1. Git for Windows'
    Write-Host '  2. CMake 3.28 or newer for VS 2022, or CMake with Visual Studio 18 2026 generator support for VS 2026'
    Write-Host '  3. Visual Studio Community/Professional/Enterprise 2026 or 2022 with C++ tools'
    Write-Host '  4. Workload: Desktop development with C++ / Microsoft.VisualStudio.Workload.VCTools'
    Write-Host ''
    Write-Host 'Check your CMake generators:'
    Write-Host '  cmake --help | findstr /C:"Visual Studio"'
    Write-Host ''
    Write-Host 'Manual install/repair option:'
    Write-Host '  Open Visual Studio Installer -> Modify -> Desktop development with C++ -> Install'
    Write-Host ''
    throw $Message
}

function Require-Command {
    param(
        [Parameter(Mandatory=$true)][string]$Name,
        [Parameter(Mandatory=$true)][string]$Help
    )
    $cmd = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $cmd) { Fail-WithHelp "$Name was not found. $Help" }
    Write-Ok "$Name found: $($cmd.Source)"
    return $cmd
}

function Get-VsWhere {
    # IMPORTANT:
    # A single PowerShell string can be indexed like an array, so `$path[0]`
    # returns only the first character (`C`) instead of the full `C:\...` path.
    # Force the pipeline result to remain an array before selecting the first item.
    $candidateList = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe",
        (Get-Command vswhere.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue)
    )

    $candidates = @(
        $candidateList | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -Unique
    )

    if (-not $candidates -or $candidates.Count -eq 0) {
        Fail-WithHelp 'Visual Studio Installer/vswhere was not found. CMake cannot locate Visual Studio.'
    }

    return [string]($candidates | Select-Object -First 1)
}

function Find-VsInstall {
    param(
        [Parameter(Mandatory=$true)][string]$VsWhere,
        [Parameter(Mandatory=$true)][string]$VersionRange
    )

    $install = (& $VsWhere -latest -version $VersionRange -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath) 2>$null
    return ($install | Select-Object -First 1)
}

Require-Command git 'Install Git for Windows: https://git-scm.com/download/win' | Out-Null
Require-Command cmake 'Install CMake from https://cmake.org/download/ or through Visual Studio Installer.' | Out-Null

$cmakeVersionText = (& cmake --version | Select-Object -First 1)
Write-InfoLine $cmakeVersionText
if ($cmakeVersionText -match 'version\s+([0-9]+)\.([0-9]+)\.([0-9]+)') {
    $maj = [int]$Matches[1]
    $min = [int]$Matches[2]
    if (($maj -lt 3) -or (($maj -eq 3) -and ($min -lt 28))) {
        Fail-WithHelp "CMake 3.28 or newer is required by this OBS plugin template preset. Current: $cmakeVersionText"
    }
}

$cmakeHelp = (& cmake --help) -join "`n"
$hasVs2026Generator = $cmakeHelp -match 'Visual Studio 18 2026'
$hasVs2022Generator = $cmakeHelp -match 'Visual Studio 17 2022'

$vswhere = Get-VsWhere
Write-Ok "vswhere found: $vswhere"

$vs2026 = Find-VsInstall -VsWhere $vswhere -VersionRange '[18.0,19.0)'
$vs2022 = Find-VsInstall -VsWhere $vswhere -VersionRange '[17.0,18.0)'

$selectedYear = $null
$selectedInstall = $null
$selectedPreset = $null
$selectedBuildDir = $null

if (($VisualStudio -eq '2026' -or $VisualStudio -eq 'Auto') -and $vs2026) {
    if (-not $hasVs2026Generator) {
        if ($VisualStudio -eq '2026') {
            Fail-WithHelp 'Visual Studio 2026 was found, but your CMake does not list the "Visual Studio 18 2026" generator. Install/update CMake that supports VS 2026, or use -VisualStudio 2022 if VS 2022 is installed.'
        }
    } else {
        $selectedYear = '2026'
        $selectedInstall = $vs2026
        $selectedPreset = 'windows-vs2026-x64'
        $selectedBuildDir = 'build_vs2026_x64'
    }
}

if (-not $selectedInstall -and ($VisualStudio -eq '2022' -or $VisualStudio -eq 'Auto') -and $vs2022) {
    if (-not $hasVs2022Generator) {
        if ($VisualStudio -eq '2022') {
            Fail-WithHelp 'Visual Studio 2022 was found, but your CMake does not list the "Visual Studio 17 2022" generator.'
        }
    } else {
        $selectedYear = '2022'
        $selectedInstall = $vs2022
        $selectedPreset = 'windows-vs2022-x64'
        $selectedBuildDir = 'build_vs2022_x64'
    }
}

if (-not $selectedInstall) {
    $found = @()
    if ($vs2026) { $found += "VS2026 at $vs2026" }
    if ($vs2022) { $found += "VS2022 at $vs2022" }
    if ($found.Count -eq 0) { $foundText = 'No VS 2026/2022 C++ instance found by vswhere.' } else { $foundText = $found -join '; ' }
    Fail-WithHelp "No compatible Visual Studio generator+installation pair was found. $foundText"
}

Write-Ok "Visual Studio $selectedYear C++ instance found: $selectedInstall"
Write-Ok "Selected CMake preset: $selectedPreset"

$msbuild = Join-Path $selectedInstall 'MSBuild\Current\Bin\MSBuild.exe'
if (-not (Test-Path $msbuild)) {
    Fail-WithHelp "MSBuild.exe was not found at expected path: $msbuild"
}
Write-Ok "MSBuild found: $msbuild"

$vcTools = Join-Path $selectedInstall 'VC\Tools\MSVC'
if (-not (Test-Path $vcTools)) {
    Fail-WithHelp "MSVC toolset folder was not found: $vcTools"
}
Write-Ok "MSVC toolset folder found: $vcTools"

$sdkLibRoot = 'C:\Program Files (x86)\Windows Kits\10\Lib'
$sdkOk = $false
if (Test-Path $sdkLibRoot) {
    $sdkOk = [bool](Get-ChildItem $sdkLibRoot -Directory -ErrorAction SilentlyContinue | Where-Object {
        Test-Path (Join-Path $_.FullName 'um\x64')
    } | Select-Object -First 1)
}
if (-not $sdkOk) {
    Write-Host '[WARN] Windows 10/11 SDK x64 library was not detected. If CMake fails next, modify Visual Studio and install a Windows SDK component.' -ForegroundColor Yellow
} else {
    Write-Ok 'Windows SDK x64 library detected.'
}

Write-Host ''
Write-Ok 'Windows build environment looks ready for ArVisual.'

[pscustomobject]@{
    VisualStudioYear = $selectedYear
    InstallationPath = $selectedInstall
    CMakePreset      = $selectedPreset
    BuildPreset      = $selectedPreset
    BuildDir         = $selectedBuildDir
}
