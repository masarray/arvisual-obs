@echo off
setlocal
cd /d "%~dp0"

where pwsh >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\create-public-repo-gh.ps1"
) else (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\create-public-repo-gh.ps1"
)

if %ERRORLEVEL% NEQ 0 (
  echo.
  echo Repo creation failed. Install GitHub CLI, then run: gh auth login
  pause
  exit /b %ERRORLEVEL%
)

echo.
echo Done.
pause
