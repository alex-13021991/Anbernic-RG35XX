@echo off
setlocal enabledelayedexpansion
title Jazz Jackrabbit 2 - Miyoo Mini content converter

rem Turns an installed copy of Jazz Jackrabbit 2 into the content tree the Miyoo Mini port loads.
rem Drag the game's installation folder onto this file, or run it and type the path when asked.
rem Nothing is written to the game folder; everything lands in Content\ next to this script.

set "HERE=%~dp0"
set "PACKER=%HERE%AssetPacker.exe"
set "ENGINE=%HERE%EngineContent"
set "OUT=%HERE%Content"
set "STAGE=%TEMP%\jj2-miyoo-convert"

if not exist "%PACKER%" (
    echo AssetPacker.exe is missing. It must sit in the same folder as this script.
    goto :fail
)
if not exist "%ENGINE%\Metadata" (
    echo EngineContent\ is missing. It must sit in the same folder as this script.
    goto :fail
)

set "SRC=%~1"
if "%SRC%"=="" (
    echo Drag your Jazz Jackrabbit 2 folder onto this file, or type its path below.
    echo Example: C:\GOG Games\Jazz Jackrabbit 2 Collection
    echo.
    set /p "SRC=Game folder: "
)
rem Strip the quotes a pasted path may carry; every use below quotes the variable itself
set "SRC=%SRC:"=%"
if "%SRC%"=="" goto :nosource
if not exist "%SRC%\Anims.j2a" goto :nosource

echo.
echo Source: %SRC%
echo Target: %OUT%
echo.

rem AssetPacker takes the engine's own assets from a "Content" directory next to a "Source" one, and
rem without them the converted data is missing every engine-side asset. Build that layout in TEMP:
rem Source is a junction, so the game files are not copied anywhere.
if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%" 2>nul
mklink /J "%STAGE%\Source" "%SRC%" >nul
if errorlevel 1 (
    echo Could not link the game folder into %STAGE%.
    echo If the game sits on a network drive, copy it to a local disk first.
    goto :fail
)
xcopy "%ENGINE%" "%STAGE%\Content" /e /i /q /y >nul
if errorlevel 1 goto :fail

rem --originals-only + --skip-non-episode-levels drop the multiplayer and test levels the port has no
rem way to reach; --video-downscale=2 halves the cinematics, which the 640x480 panel does not miss and
rem the SD card and the decoder both do.
"%PACKER%" "%STAGE%" "%OUT%" --target=console --originals-only --skip-non-episode-levels --video-downscale=2
set "RC=%errorlevel%"
rmdir "%STAGE%\Source" 2>nul
rmdir /s /q "%STAGE%" 2>nul
if not "%RC%"=="0" goto :fail

rem The level converter only copies music a surviving level references, so the tracks the engine asks
rem for by name - the main menu's, and one per cinematic - have to be copied by hand or they are silent.
echo.
echo Copying menu and cinematic music...
if not exist "%OUT%\Music" mkdir "%OUT%\Music"
for %%T in (menu bonus2 bonus3 intro ending) do (
    if exist "%SRC%\%%T.j2b" (
        copy /y "%SRC%\%%T.j2b" "%OUT%\Music\%%T.j2b" >nul
        echo   %%T.j2b
    ) else (
        echo   WARNING: %%T.j2b not found - some music will be missing
    )
)

echo.
echo Done. Copy this folder's "Content" into
echo   Roms\PORTS\Games\Jazz Jackrabbit 2\
echo on your Miyoo Mini SD card.
goto :end

:nosource
echo.
echo That does not look like a Jazz Jackrabbit 2 installation - Anims.j2a is not in it.
echo Point this at the folder holding Jazz2.exe and Anims.j2a.
goto :fail

:fail
echo.
echo Conversion failed.
pause
exit /b 1

:end
echo.
pause
