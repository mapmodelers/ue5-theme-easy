# Changelog

## 0.2.2
- Toolbar in Tab Bar (on by default): tabs and the editor toolbars share one row (level editor and docked asset editors), at a fixed height so switching tabs keeps everything in place; tabs get an adaptive width; a + button opens recent levels and recent assets. Frees a row for the viewport. Window > Easy Theme > Layout > Toolbar in Tab Bar, or console `EasyTheme.TabToolbar 0|1`.
- Graph node titles readable: bright title colours are darkened under the white title text.
- Toolbars line up with the tabs: same 27 px height, no vertical padding around blocks.
- Full tab row: tabs shrink (down to 72 px, at most half the row together) and a toolbar that still does not fit shows Unreal's ">>" overflow button instead of running off the window. Tab widths never depend on the editor in front.
- Window > Easy Theme > Help > Report a Bug...: saves a screenshot, a cleaned-up log (user name, computer name and folders removed) and system info, then opens that folder and a pre-filled GitHub issue. Nothing is uploaded unless you attach it.

## 0.2.1
- Dark: tooltips readable (Unreal draws tooltip text in black, so tooltips are now light grey in Dark).
- Light: Details name field readable.
- Engine-wide install now active in every project (packaged descriptor kept "EnabledByDefault").
- Window > Easy Theme > Check for Updates: on click, asks GitHub for the latest release; GitHub installs can download it (SHA-256 checked) and install it when Unreal closes, with a backup. Other installs open the releases page.

## 0.2.0
- Renamed to Easy Theme (from the internal "Theme 2001" prototype).
- Dark is the default; Light included; live switch with UI refresh and optional restart.
- Pixel-exact frames and bevels; compact density; Tahoma UI font.
- Fixes hard-coded engine colours; text-entry and light-text readability sweeps.
- Graphs follow the mode: light canvas in Light (Light Graphs option, user grid colours always restored); slim title bar; optional 3-pane layout.
- Classic yellow search highlight everywhere; Content Browser hover and selection readable in Light.
- Diagnostics run only in the repo's test project.
