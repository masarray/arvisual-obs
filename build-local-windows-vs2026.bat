@echo off
setlocal
cd /d "%~dp0"

where pwsh >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  pwsh -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-local-windows.ps1" -VisualStudio 2026
) else (
  powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\build-local-windows.ps1" -VisualStudio 2026
)

if %ERRORLEVEL% NEQ 0 (
  echo.
  echo Build failed. Check the message above.
  echo VS 2026 path selected. Make sure your CMake supports generator Visual Studio 18 2026.
  echo Check with:
  echo   cmake --help ^| findstr /C:"Visual Studio"
  pause
  exit /b %ERRORLEVEL%
)

echo.
echo Done. ZIP file is in the release folder.
pause
