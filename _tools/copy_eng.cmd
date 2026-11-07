@echo off
rem Copies engine style sources for local reference (exact UE version). Never committed.
rem %1 = engine root, %2 = destination
set "E=%~1\Engine"
set "D=%~2"
if not exist "%D%" mkdir "%D%"
for /f "delims=" %%P in ('where /r "%E%\Source\Runtime\SlateCore" StyleColors.h StyleColors.cpp SlateTypes.h SlateBrush.h ISlateStyle.h CompositeFont.h StarshipCoreStyle.cpp StarshipCoreStyle.h SlateStyle.h 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Runtime\Slate" StarshipCoreStyle.cpp StarshipCoreStyle.h 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Editor\EditorStyle" StarshipStyle.cpp StarshipStyle.h 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Editor" LayoutsMenu.cpp LayoutsMenu.h SLevelEditor.cpp 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Runtime\Slate" ToolBarStyle.h TabManager.h LayoutService.h 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Runtime\SlateCore" ToolBarStyle.h SlateColor.h WidgetStyle.h 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Editor\UnrealEd" UnrealEdGlobals.h AssetEditorSubsystem.h 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Editor\GraphEditor" SNodePanel.cpp SGraphPin.cpp SGraphNode.cpp GraphEditorSettings.h 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Editor" AssetViewWidgets.cpp AssetViewWidgets.h EditorStyleSettings.h SGraphPanel.cpp SGraphEditorImpl.cpp ConnectionDrawingPolicy.cpp SGraphNodeComment.cpp SGraphNodeK2Base.cpp SMaterialEditorSubstrateWidget.cpp SMaterialEditorSubstrateWidget.h SDocumentationToolTip.cpp MainMRUFavoritesList.h MainMRUFavoritesList.cpp MRUFavoritesList.h IMainFrameModule.h AssetEditorSubsystem.cpp MaterialGraphSchema.h MaterialGraphConnectionDrawingPolicy.cpp SStandaloneAssetEditorToolkitHost.cpp SStandaloneAssetEditorToolkitHost.h AssetEditorToolkit.cpp 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Runtime\Slate" SMenuEntryBlock.cpp SDockTab.h SDockTab.cpp SDockingTabStack.cpp SDockingTabStack.h SDockingTabWell.cpp SWindowTitleBar.h TabManager.cpp SToolTip.cpp SToolTip.h SlateTextHighlightRunRenderer.cpp MultiBox.cpp SMultiBoxWidget.cpp SWindowTitleBar.cpp SDockingArea.cpp SDockingArea.h SDockingNode.cpp SDockingNode.h SDockingSplitter.cpp SClippingHorizontalBox.cpp 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Runtime\SlateCore" SWindow.h SWindow.cpp 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Developer\ToolMenus" ToolMenus.cpp 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Editor\WorldBrowser" *.cpp 2^>nul') do copy /y "%%P" "%D%\WB_%%~nxP" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Editor\LevelEditor\Private" LevelEditor.cpp 2^>nul') do copy /y "%%P" "%D%\" >nul
for /f "delims=" %%P in ('where /r "%E%\Source\Editor\WorkspaceMenuStructure" *.cpp *.h 2^>nul') do copy /y "%%P" "%D%\" >nul
if not exist "%D%\Themes" mkdir "%D%\Themes"
copy /y "%E%\Content\Slate\Themes\*.json" "%D%\Themes\" >nul 2>&1
if not exist "%D%\Splash" mkdir "%D%\Splash"
copy /y "%E%\Content\Splash\*.*" "%D%\Splash\" >nul 2>&1
dir /b "%D%" > "%D%\done.txt"
