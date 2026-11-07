# Changelog

## 0.2.4
Thanks to the Reddit testers u/LeFlambeurHimself and u/LostInTheRapGame for their feedback.
- New look **Compact** (Window > Easy Theme > Look): Unreal's own colours with the compact layout, square corners, Tahoma and all fixes. Switching to or from it applies at the next start. Asked by u/LeFlambeurHimself.
- Home tab hidden by default (Unreal's HomeScreen.EnableHomeScreen, written to your user UserEngine.ini, applies to every project from the next start); Window > Easy Theme > Layout > Show Home Tab brings it back. Thanks u/LeFlambeurHimself.
- Look menu: only the active look shows a dot (both Dark and Light showed one). Thanks u/LeFlambeurHimself.
- Square corners everywhere: every rounded brush in every style set, plugins included, standalone or inside widget styles; radio dots, colour wheels and toggles stay round. Thanks u/LostInTheRapGame.
- Light: menu section headings (LOOK, LAYOUT...) were white on light grey; now dark grey. Menus are now part of the automatic checks.
- Light: no pure white backgrounds any more (text fields, lists, drop-downs: #F2F0EB, alternate rows #EAE8E2).
- Checked: Animation Blueprint node titles keep white text on a title darkened to at most #808080 (3.9:1 or better; reported unreadable on 0.2.1, fixed since 0.2.2). Plugins window tooltips use the light tooltip (since 0.2.1).

## 0.2.3
- Floating panels: their tabs were hidden under the window's title bar (Unreal draws the title bar over the tab row there; it now stays clear in those windows). Affected most panels from the Window and Tools menus once undocked: World Bookmarks, Levels, Data Layers, Message Log, Class Viewer...
- Toolbar in Tab Bar: closing some asset editors (Skeletal Mesh) could crash the editor (a closed editor's toolbar was kept alive).
- Texture editor: R/G/B/A buttons no longer cut in the tab row.
- Disabled text readable: Dark #C0C0C0 and Light #303030 before Unreal's own dimming (was about 1.6:1 contrast, now about 2.4:1).
- Light: graph node titles in white on their darkened colour (black on dark red before); plugin style sets get the light pass too (PCG, StateTree, Interchange import dialog, Take Recorder, Niagara, Plugins window), including plugins that load later.

## 0.2.2
- Toolbar in Tab Bar (on by default): tabs and the editor toolbars share one row (level editor and docked asset editors), at a fixed height so switching tabs keeps everything in place; tabs get an adaptive width; a + button opens recent levels and recent assets. Frees a row for the viewport. Window > Easy Theme > Layout > Toolbar in Tab Bar, or console `EasyTheme.TabToolbar 0|1`.
- Graph node titles readable: bright title colours are darkened under the white title text.
- Toolbars line up with the tabs: same 27 px height, no vertical padding around blocks.
- Full tab row: tabs shrink (down to 72 px, at most half the row together) and a toolbar that still does not fit shows Unreal's ">>" overflow button instead of running off the window. Tab widths never depend on the editor in front.
- Window > Easy Theme > Help > Report a Bug...: saves a screenshot, a cleaned-up log (user name, computer name and folders removed) and system info, then opens that folder and a pre-filled GitHub issue. Nothing is uploaded unless you attach it.
- Updater: files another open editor still holds are moved aside (Windows allows it) instead of retrying for minutes, so an update never stops half-way; that editor keeps the old version until it restarts.

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
