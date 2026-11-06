@echo off
rem Pushes pending repo changes, then publishes GitHub release v%VER% with a prebuilt plugin zip.
rem Needs a fresh build in _build (go.cmd). Run from anywhere.
setlocal
cd /d "%~dp0.."
set "REPO=mapmodelers/ue5-theme-easy"
for /f "usebackq delims=" %%v in (`powershell -NoProfile -Command "(Get-Content -Raw Plugin\EasyTheme\EasyTheme.uplugin | ConvertFrom-Json).VersionName"`) do set "VER=%%v"
set "ZIP=_run\EasyTheme-%VER%-UE5.8-Win64.zip"
if not exist "_build\EasyTheme\Binaries\Win64" (echo No build found: run go.cmd first. & exit /b 1)

rem 1. repo changes: review before anything is published
git add -A
git status --short
echo.
echo Review the list above. Ctrl+C to abort, any key to commit, push and release.
pause >nul
git diff --cached --quiet || git commit -m "Public page, release notes, download instructions"
git push || exit /b 1

rem 2. plugin zip: no Intermediate folder, no debug symbols
if exist "_run\pkg" rmdir /s /q "_run\pkg"
robocopy "_build\EasyTheme" "_run\pkg\EasyTheme" /E /XD Intermediate /XF *.pdb /NFL /NDL /NJH /NJS /NP >nul
copy /y "Plugin\EasyTheme\README.md" "_run\pkg\EasyTheme\README.md" >nul
mkdir "_run\pkg\EasyTheme\Updater" 2>nul
copy /y "_tools\updater\EasyThemeUpdate.ps1" "_run\pkg\EasyTheme\Updater\" >nul
if exist "%ZIP%" del "%ZIP%"
powershell -NoProfile -Command "Compress-Archive -Path '_run\pkg\EasyTheme' -DestinationPath '%ZIP%'"
if not exist "%ZIP%" (echo Zip failed. & exit /b 1)
powershell -NoProfile -Command "(Get-FileHash -Algorithm SHA256 '%ZIP%').Hash.ToLower() + '  EasyTheme-%VER%-UE5.8-Win64.zip' | Set-Content -NoNewline -Encoding ascii '%ZIP%.sha256'"

rem 3. release (replaces an existing one with the same tag)
gh release view v%VER% --repo %REPO% >nul 2>&1 && gh release delete v%VER% --repo %REPO% --yes --cleanup-tag
gh release create v%VER% "%ZIP%" "%ZIP%.sha256" --repo %REPO% --title "Easy Theme %VER%" --notes-file _tools\release-notes.md --latest
gh repo edit %REPO% --add-topic unreal-engine-plugin,ue5-plugin,editor-ui,free
echo.
echo Released %VER%: https://github.com/%REPO%/releases/latest
