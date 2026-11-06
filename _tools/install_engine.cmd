@echo off
rem Installs the built plugin (_build, from go.cmd) for every project of this engine:
rem   <Engine>\Engine\Plugins\Marketplace\EasyTheme
rem Asks for admin rights (Program Files). Uninstall: delete that folder.
setlocal
cd /d "%~dp0.."
set "ENGINE=C:\Program Files\Epic Games\UE_5.8"
if exist "_tools\local.cmd" call "_tools\local.cmd"
net session >nul 2>&1 || (powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs" & exit /b)
if not exist "_build\EasyTheme\Binaries\Win64" (echo No build found: run go.cmd first. & pause & exit /b 1)
set "DST=%ENGINE%\Engine\Plugins\Marketplace\EasyTheme"
robocopy "_build\EasyTheme" "%DST%" /MIR /XD Intermediate /XF *.pdb /NFL /NDL /NJH /NJS /NP
if %errorlevel% GEQ 8 (echo Install failed. & pause & exit /b 1)
mkdir "%DST%\Updater" 2>nul
copy /y "_tools\updater\EasyThemeUpdate.ps1" "%DST%\Updater\" >nul
echo.
echo Installed: %DST%
echo Restart any open Unreal editor to load it.
pause
