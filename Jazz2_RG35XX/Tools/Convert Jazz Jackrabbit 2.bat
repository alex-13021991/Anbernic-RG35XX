@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Convert.ps1" %*
if errorlevel 1 (
    echo Conversion failed.
    pause
    exit /b 1
)
pause
