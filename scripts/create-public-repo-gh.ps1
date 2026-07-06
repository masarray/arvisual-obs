[CmdletBinding()]
param(
    [string] $Repo = 'masarray/arvisual-obs',
    [string] $Description = 'ArVisual Smart Color Enhancer - one-click OBS color pop filter for clean, vivid, dopamine visuals.'
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot '..')
Set-Location $RepoRoot

if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    throw 'GitHub CLI (gh) was not found. Install from https://cli.github.com/ and run gh auth login.'
}

if (-not (Test-Path (Join-Path $RepoRoot '.git'))) {
    git init
    git branch -M main
}

git add .
git commit -m "Initial public release scaffold for ArVisual OBS plugin" 2>$null

$repoExists = $false
try {
    gh repo view $Repo *> $null
    $repoExists = $true
} catch {
    $repoExists = $false
}

if (-not $repoExists) {
    gh repo create $Repo --public --description $Description --source . --remote origin --push
} else {
    git remote remove origin 2>$null
    git remote add origin "https://github.com/$Repo.git"
    git push -u origin main
}

$version = (Get-Content (Join-Path $RepoRoot 'buildspec.json') -Raw | ConvertFrom-Json).version
$tag = "v$version"
if (-not (git tag --list $tag)) {
    git tag $tag
}
git push origin $tag
Write-Host "Public repo ready: https://github.com/$Repo"
Write-Host "Release workflow will run from tag: $tag"
