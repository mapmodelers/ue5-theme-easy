Clean, compact, high-clarity editor theme for Unreal Engine 5. Dark by default, Light included.

**Install:** unzip `EasyTheme-0.2.3-UE5.8-Win64.zip` into `YourProject/Plugins/` (or `Engine/Plugins/Marketplace/` for every project), then start the editor. Prebuilt for UE 5.8, Win64. Options: **Window > Easy Theme**.

**In 0.2.3**
- Floating panels (World Bookmarks, Levels, Message Log, most Window and Tools panels): their tabs were hidden under the title bar.
- Fixed a crash when closing some asset editors (Skeletal Mesh) with Toolbar in Tab Bar on.
- Light: plugin editors (PCG, StateTree, Niagara, import dialog, Plugins window) follow the light look; node titles readable; disabled text readable in both modes.

**In 0.2.2**
- Toolbar in Tab Bar (on by default): tabs and main toolbar share one row, + button for recent levels and assets. Switch: Window > Easy Theme > Layout.
- Full tab row: tabs shrink, extra toolbar buttons go behind Unreal's overflow button.
- Graph node titles readable; toolbars aligned with the tabs.
- Report a Bug: pre-filled GitHub issue, screenshot and a cleaned-up log (no user name or folders) to attach.
- Updater: finishes even when another open editor still uses the plugin.

**In 0.2.1**
- Dark: tooltips readable (light grey tooltip, black text).
- Light: Details name field readable.
- Engine-wide install (Engine/Plugins/Marketplace) active in every project.
- Window > Easy Theme > Check for Updates: download, SHA-256 check and install on close (GitHub installs).

**In 0.2.0**
- Dark (default) and Light, switched live.
- Compact density, pixel-exact bevels and frames, Tahoma UI font.
- Readability fixes across the editor: text fields, search highlight, Content Browser, Place Actors.
- Light Graphs: light canvas for Blueprint and Material graphs in Light mode; your grid colours are always restored.
- Slim title bar, optional 3-pane layout, Report an Issue, Remove Saved Settings.

Editor-only, no telemetry. Network access only when you click Check for Updates. MIT.
