@echo off
setlocal
echo.
echo ArVisual - Visual Studio 2022 Build Tools installer helper
echo ---------------------------------------------------------
echo This will install Microsoft Visual Studio 2022 Build Tools with the C++ workload.
echo You may need to run this file as Administrator.
echo.
choice /M "Continue"
if errorlevel 2 exit /b 1

where winget >nul 2>nul
if %ERRORLEVEL% EQU 0 (
  echo.
  echo Installing via winget...
  winget install --id Microsoft.VisualStudio.2022.BuildTools -e --source winget --override "--wait --quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
  if %ERRORLEVEL% NEQ 0 (
    echo.
    echo winget install failed. Opening the official download page instead.
    start https://visualstudio.microsoft.com/visual-cpp-build-tools/
    pause
    exit /b %ERRORLEVEL%
  )
  echo.
  echo Install command completed. Restart Command Prompt/PowerShell, then run build-local-windows.bat again.
  pause
  exit /b 0
)

echo.
echo winget was not found. Opening the official Microsoft Build Tools download page.
start https://visualstudio.microsoft.com/visual-cpp-build-tools/
echo.
echo In Visual Studio Installer, choose: Desktop development with C++.
pause
