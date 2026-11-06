@echo off
rem Creates the public GitHub repo, pushes, sets topics and turns on GitHub Pages (/docs). Run once, from anywhere.
setlocal
cd /d "%~dp0.."
set "REPO=mapmodelers/ue5-theme-easy"
set "DESC=Clean, compact, high-clarity editor theme for Unreal Engine 5. Square controls, crisp 1-pixel frames, readable text in every panel. Dark by default, Light included."
if not exist ".git" git init -b main
rem Privacy: commit with your GitHub no-reply address, never your personal email (this repo only).
for /f "delims=" %%I in ('gh api user --jq ".id"') do set "GHID=%%I"
for /f "delims=" %%L in ('gh api user --jq ".login"') do set "GHLOGIN=%%L"
if not defined GHID (echo GitHub CLI not signed in: run "gh auth login" first & exit /b 1)
git config user.name "%GHLOGIN%"
git config user.email "%GHID%+%GHLOGIN%@users.noreply.github.com"
git add -A
rem Safety: show exactly what will be published.
git status --short
echo.
echo Review the list above. Ctrl+C to abort, any key to publish.
pause >nul
git commit -m "Easy Theme 0.2.0"
gh repo create %REPO% --public --source . --push --homepage "https://mapmodelers.github.io/ue5-theme-easy/" --description "%DESC%"
gh repo edit %REPO% --add-topic unreal-engine,ue5,unreal-editor,editor-theme,dark-theme,light-theme,compact-ui,slate,editor-plugin,productivity
gh api repos/%REPO%/pages -X POST -f "source[branch]=main" -f "source[path]=/docs"
echo.
echo Done. Social preview image: Settings ^> Social preview ^> upload docs\media\social.png
