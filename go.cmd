@echo off
rem Easy Theme: build the plugin, install it into the test project, launch the editor.
rem Results: _run\status.txt, _run\build.log, TestProject\Saved\EasyTheme\*
setlocal
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "ENGINE=C:\Program Files\Epic Games\UE_5.8"
rem Machine-specific overrides (not committed): _tools\local.cmd, e.g.  set "ENGINE=D:\Epic\UE_5.8"
if exist "%~dp0_tools\local.cmd" call "%~dp0_tools\local.cmd"
set "RUN=%ROOT%\_run"
if not exist "%RUN%" mkdir "%RUN%"
> "%RUN%\status.txt" echo START %date% %time%

rem 1. engine reference files (read-only, local only, never committed)
call "%ROOT%\_tools\copy_eng.cmd" "%ENGINE%" "%ROOT%\_eng"

rem 2. close only this repo's test editor
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\_tools\close_editor.ps1" >> "%RUN%\status.txt" 2>&1

rem 3. build
if exist "%ROOT%\_build" rmdir /s /q "%ROOT%\_build"
echo Building Easy Theme (2-5 min)...
call "%ENGINE%\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin="%ROOT%\Plugin\EasyTheme\EasyTheme.uplugin" -Package="%ROOT%\_build\EasyTheme" -TargetPlatforms=Win64 > "%RUN%\build.log" 2>&1
set "BE=%errorlevel%"
if not "%BE%"=="0" goto :buildfail
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\_tools\fix_uplugin.ps1" "%ROOT%\_build\EasyTheme\EasyTheme.uplugin"

rem 4. install into the test project
robocopy "%ROOT%\_build\EasyTheme" "%ROOT%\TestProject\Plugins\EasyTheme" /MIR /NFL /NDL /NJH /NJS /NP > "%RUN%\install.log"
if %errorlevel% GEQ 8 goto :installfail
mkdir "%ROOT%\TestProject\Plugins\EasyTheme\Updater" 2>nul
copy /y "%ROOT%\_tools\updater\EasyThemeUpdate.ps1" "%ROOT%\TestProject\Plugins\EasyTheme\Updater\" >nul

rem 5. launch
if exist "%ROOT%\TestProject\Saved\EasyTheme" rmdir /s /q "%ROOT%\TestProject\Saved\EasyTheme"
mkdir "%ROOT%\TestProject\Saved\EasyTheme" 2>nul
type nul > "%ROOT%\TestProject\Saved\EasyTheme\dev.flag"
rem _run\light.flag: test editor starts in Light (dev only, the shared setting is not touched)
if exist "%RUN%\light.flag" type nul > "%ROOT%\TestProject\Saved\EasyTheme\light.flag"
if exist "%RUN%\compact.flag" type nul > "%ROOT%\TestProject\Saved\EasyTheme\compact.flag"
rem Start-Process: the editor must not inherit this window's output (it would hold the watcher).
powershell -NoProfile -Command "Start-Process -FilePath '%ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList '\"%ROOT%\TestProject\EasyThemeTest.uproject\"'"
>> "%RUN%\status.txt" echo BUILD_OK INSTALL_OK LAUNCHED %date% %time%
echo.
echo OK - editor is starting. Wait until it is fully open (+20 s), then check the report in TestProject\Saved\EasyTheme.
exit /b 0

:buildfail
>> "%RUN%\status.txt" echo BUILD_FAIL %BE% %date% %time%
echo.
echo BUILD FAILED - see _run\build.log.
exit /b 1

:installfail
>> "%RUN%\status.txt" echo INSTALL_FAIL %date% %time%
echo.
echo INSTALL FAILED - see _run\install.log.
exit /b 1
