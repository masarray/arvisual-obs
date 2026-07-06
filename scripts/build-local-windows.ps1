[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo',

    [ValidateSet('Auto', '2026', '2022')]
    [string] $VisualStudio = 'Auto',

    [string] $TemplateRef = 'master',

    [switch] $NoPackage,
    [switch] $Ci
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
$BuildRoot = Join-Path $RepoRoot '.build'
$TemplateDir = Join-Path $BuildRoot 'obs-plugintemplate'
$StageRoot = Join-Path $RepoRoot "release/stage/$Configuration"
$Version = (Get-Content (Join-Path $RepoRoot 'buildspec.json') -Raw | ConvertFrom-Json).version

function Invoke-Logged {
    param([Parameter(Mandatory=$true)][string]$FilePath, [string[]]$Arguments = @(), [string]$WorkingDirectory = $RepoRoot)
    Write-Host "> $FilePath $($Arguments -join ' ')"
    $p = Start-Process -FilePath $FilePath -ArgumentList $Arguments -WorkingDirectory $WorkingDirectory -Wait -NoNewWindow -PassThru
    if ($p.ExitCode -ne 0) { throw "$FilePath failed with exit code $($p.ExitCode)" }
}

function Require-Command {
    param([string]$Name, [string]$Help)
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "$Name was not found. $Help"
    }
}

# Fail early with a friendly message and auto-select VS 2026 or VS 2022.
$envInfo = & (Join-Path $PSScriptRoot 'check-windows-build-env.ps1') -VisualStudio $VisualStudio
$selected = $envInfo | Where-Object { $_.CMakePreset } | Select-Object -Last 1
$Preset = $selected.CMakePreset
$BuildPreset = $selected.BuildPreset
$BuildDir = $selected.BuildDir

if (-not $Preset) {
    throw 'Unable to select a CMake preset. Run scripts\check-windows-build-env.ps1 for details.'
}

Require-Command git 'Install Git for Windows: https://git-scm.com/download/win'
Require-Command cmake 'Install CMake or Visual Studio with CMake tools.'

New-Item -ItemType Directory -Force -Path $BuildRoot | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $RepoRoot 'release') | Out-Null

if (-not (Test-Path $TemplateDir)) {
    Write-Host 'Cloning official OBS plugin template...'
    Invoke-Logged git @('clone', '--depth', '1', '--branch', $TemplateRef, 'https://github.com/obsproject/obs-plugintemplate.git', $TemplateDir) $BuildRoot
} elseif (-not $Ci) {
    Write-Host 'Updating official OBS plugin template...'
    Invoke-Logged git @('fetch', '--depth', '1', 'origin', $TemplateRef) $TemplateDir
    Invoke-Logged git @('checkout', 'FETCH_HEAD') $TemplateDir
}

Write-Host 'Overlaying ArVisual source into template workspace...'
$OverlayItems = @('src', 'data', 'CMakeLists.txt', 'CMakePresets.json', 'buildspec.json')
foreach ($item in $OverlayItems) {
    $src = Join-Path $RepoRoot $item
    $dst = Join-Path $TemplateDir $item
    if (Test-Path $dst) { Remove-Item $dst -Recurse -Force }
    Copy-Item $src $dst -Recurse -Force
}

Write-Host "Configuring ArVisual with CMake preset $Preset..."
Invoke-Logged cmake @('--preset', $Preset) $TemplateDir

Write-Host "Building ArVisual with preset $BuildPreset..."
Invoke-Logged cmake @('--build', '--preset', $BuildPreset, '--config', $Configuration, '--parallel') $TemplateDir

if (Test-Path $StageRoot) { Remove-Item $StageRoot -Recurse -Force }
New-Item -ItemType Directory -Force -Path $StageRoot | Out-Null

Write-Host 'Installing to release staging folder...'
Invoke-Logged cmake @('--install', $BuildDir, '--prefix', $StageRoot, '--config', $Configuration) $TemplateDir

if (-not $NoPackage) {
    & (Join-Path $PSScriptRoot 'package-windows.ps1') -StageRoot $StageRoot -Version $Version -Configuration $Configuration
}

Write-Host ''
Write-Host 'Build completed.'
Write-Host "Visual Studio: $($selected.VisualStudioYear)"
Write-Host "Preset: $Preset"
Write-Host "Stage: $StageRoot"
Write-Host "Release: $(Join-Path $RepoRoot "release/arvisual-obs-v$Version-windows-x64.zip")"
