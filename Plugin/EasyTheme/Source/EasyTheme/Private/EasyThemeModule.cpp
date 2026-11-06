// Copyright MapModelers. All Rights Reserved.
//
// Easy Theme: clean, compact, high-clarity editor look. Dark by default, Light included.
// Works by (1) writing the active Slate theme colours, (2) mutating the editor's
// widget styles and brushes IN PLACE (live widgets keep their pointers, so they
// update without a restart), (3) swapping the default UI font to Tahoma.
// Look patches are absolute and re-run on a Light/Dark switch; density patches
// are relative and run once per style. Every step logs OK/MISS to
// Saved/EasyTheme/report.txt; a screenshot of the main window is saved next to it.

#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/ISlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateTypes.h"
#include "Styling/ToolBarStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/SlateApplication.h"
#include "IDocumentation.h"
#include "Widgets/SToolTip.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "ILevelEditor.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Images/SImage.h"
#include "FileHelpers.h"
#include "Interfaces/IMainFrameModule.h"
#include "MRUFavoritesList.h"
#include "Misc/PackageName.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Docking/LayoutService.h"
#include "Widgets/SWindow.h"
#include "Widgets/Docking/SDockTab.h"
#include "Rendering/SlateRenderer.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ImageUtils.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Engine/Engine.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "UnrealEdGlobals.h"
#include "UnrealEdMisc.h"
#include "Misc/MessageDialog.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/EngineVersion.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Styling/StyleColors.h"
#include "Styling/WidgetStyle.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "Misc/CoreDelegates.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformOutputDevices.h"
#include "Internationalization/Regex.h"

#include "GenericPlatform/ICursor.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogEasyTheme, Log, All);

namespace EasyTheme
{
	static TArray<FString> GReport;
	static FString GResDir;
	static TSet<const void*> GDense;
	static int32 GMiss = 0;
	static bool GFontDone = false;
	// Diagnostics (startup screenshot, report files, command file) only run when
	// Saved/EasyTheme/dev.flag exists: the repo's test project creates it, buyers never do.
	static bool GDev = false;
	static bool GDark = true;   // Dark is the default; Light is chosen from Window > Easy Theme.
	static bool GLightGraphs = true;
	static FString GVersion = TEXT("?");
	static bool GToolbarInTitle = true;   // level toolbar moved into the tab bar (option "Toolbar in Tab Bar")   // from EasyTheme.uplugin
	static const TCHAR* GReleasesUrl = TEXT("https://github.com/mapmodelers/ue5-theme-easy/releases/latest");   // Light mode also lightens graph canvases (option in the menu).
	static const TCHAR* GConfigSection = TEXT("EasyTheme");
	static TSharedPtr<FSlateStyleSet> GResStyle;

	// Compact density: horizontal / vertical padding factors and text scale.
	static constexpr float GPadX = 0.6f;
	static constexpr float GPadY = 0.5f;
	static constexpr float GFontScale = 0.9f;

	static void Note(const FString& S)
	{
		UE_LOG(LogEasyTheme, Log, TEXT("%s"), *S);
		GReport.Add(S);
	}

	static FLinearColor Hex(const TCHAR* H)
	{
		return FLinearColor(FColor::FromHex(FString(H)));
	}

	// ---------------------------------------------------------------- palettes
	struct FNamedColor { const TCHAR* Name; const TCHAR* HexValue; };

	static const FNamedColor GLightColors[] =
	{
		{ TEXT("Black"),              TEXT("808080") },
		{ TEXT("Title"),              TEXT("D4D0C8") },
		{ TEXT("WindowBorder"),       TEXT("808080") },
		{ TEXT("Foldout"),            TEXT("ECE9E2") },
		{ TEXT("Input"),              TEXT("FFFFFF") },
		{ TEXT("InputOutline"),       TEXT("7F7F7F") },
		{ TEXT("Recessed"),           TEXT("FFFFFF") },
		{ TEXT("Background"),         TEXT("BDB9B0") },
		{ TEXT("Panel"),              TEXT("D4D0C8") },
		{ TEXT("Header"),             TEXT("DEDBD3") },
		{ TEXT("Dropdown"),           TEXT("FFFFFF") },
		{ TEXT("DropdownOutline"),    TEXT("7F7F7F") },
		{ TEXT("Hover"),              TEXT("E6EEF9") },
		{ TEXT("Hover2"),             TEXT("A0A0A0") },
		{ TEXT("White"),              TEXT("FFFFFF") },
		{ TEXT("Highlight"),          TEXT("B9CCEA") },
		{ TEXT("Primary"),            TEXT("B9CCEA") },
		{ TEXT("PrimaryHover"),       TEXT("A9C1E8") },
		{ TEXT("PrimaryPress"),       TEXT("94B2DF") },
		{ TEXT("Secondary"),          TEXT("D4D0C8") },
		{ TEXT("Foreground"),         TEXT("000000") },
		{ TEXT("ForegroundHover"),    TEXT("000000") },
		{ TEXT("ForegroundInverted"), TEXT("FFFFFF") },
		{ TEXT("ForegroundHeader"),   TEXT("000000") },
		{ TEXT("Select"),             TEXT("C1D2EE") },
		{ TEXT("SelectHover"),        TEXT("B0C6EC") },
		{ TEXT("SelectParent"),       TEXT("E4ECF7") },
		{ TEXT("SelectInactive"),     TEXT("DCDCDC") },
		{ TEXT("Notifications"),      TEXT("FFFFE1") },
		{ TEXT("Warning"),            TEXT("C06000") },
		{ TEXT("Error"),              TEXT("C00000") },
		{ TEXT("Success"),            TEXT("008000") },
		{ TEXT("AccentBlue"),         TEXT("0A246A") },
		{ TEXT("AccentGreen"),        TEXT("008000") },
		{ TEXT("AccentRed"),          TEXT("C00000") },
		{ TEXT("AccentYellow"),       TEXT("B08000") },
		{ TEXT("AccentOrange"),       TEXT("C06000") },
	};

	static const FNamedColor GDarkColors[] =
	{
		{ TEXT("Black"),              TEXT("101010") },
		{ TEXT("Title"),              TEXT("333333") },
		{ TEXT("WindowBorder"),       TEXT("101010") },
		{ TEXT("Foldout"),            TEXT("333333") },
		{ TEXT("Input"),              TEXT("262626") },
		{ TEXT("InputOutline"),       TEXT("101010") },
		{ TEXT("Recessed"),           TEXT("262626") },
		{ TEXT("Background"),         TEXT("2A2A2A") },
		{ TEXT("Panel"),              TEXT("3A3A3A") },
		{ TEXT("Header"),             TEXT("444444") },
		{ TEXT("Dropdown"),           TEXT("2E2E2E") },
		{ TEXT("DropdownOutline"),    TEXT("101010") },
		{ TEXT("Hover"),              TEXT("4A4F58") },
		{ TEXT("Hover2"),             TEXT("6A6A6A") },
		{ TEXT("White"),              TEXT("FFFFFF") },
		{ TEXT("Highlight"),          TEXT("3D5A8C") },
		{ TEXT("Primary"),            TEXT("3D5A8C") },
		{ TEXT("PrimaryHover"),       TEXT("4A6AA0") },
		{ TEXT("PrimaryPress"),       TEXT("2E4A7A") },
		{ TEXT("Secondary"),          TEXT("4A4A4A") },
		{ TEXT("Foreground"),         TEXT("E6E6E6") },
		{ TEXT("ForegroundHover"),    TEXT("FFFFFF") },
		{ TEXT("ForegroundInverted"), TEXT("101010") },
		{ TEXT("ForegroundHeader"),   TEXT("F0F0F0") },
		{ TEXT("Select"),             TEXT("2E4A7A") },
		{ TEXT("SelectHover"),        TEXT("3D5A8C") },
		{ TEXT("SelectParent"),       TEXT("34404F") },
		{ TEXT("SelectInactive"),     TEXT("4A4A4A") },
		{ TEXT("Notifications"),      TEXT("4A4630") },
		{ TEXT("Warning"),            TEXT("FFB84D") },
		{ TEXT("Error"),              TEXT("FF6060") },
		{ TEXT("Success"),            TEXT("6BD16B") },
		{ TEXT("AccentBlue"),         TEXT("6F9FE8") },
		{ TEXT("AccentGreen"),        TEXT("6BD16B") },
		{ TEXT("AccentRed"),          TEXT("FF6060") },
		{ TEXT("AccentYellow"),       TEXT("FFD24D") },
		{ TEXT("AccentOrange"),       TEXT("FFA040") },
	};

	// Flat fills/lines and text colours that are not theme colours.
	struct FFlat
	{
		const TCHAR* TabIdle;  const TCHAR* TabLine;  const TCHAR* TabHover;
		const TCHAR* MenuBg;   const TCHAR* MenuLine; const TCHAR* MenuHot; const TCHAR* MenuHotLine;
		const TCHAR* Tip;      const TCHAR* TipLine;  const TCHAR* Track;
		const TCHAR* RowEven;  const TCHAR* RowOdd;   const TCHAR* RowHot;
		const TCHAR* Face;     const TCHAR* Ink;      const TCHAR* Grey;
		const TCHAR* Canvas;   const TCHAR* GridLine; const TCHAR* GridRule; const TCHAR* GridCenter;
	};

	static const FFlat GLightFlat = { TEXT("C8C4BC"), TEXT("A0A0A0"), TEXT("E6EEF9"),
		TEXT("D4D0C8"), TEXT("808080"), TEXT("C1D2EE"), TEXT("0A246A"),
		TEXT("FFFFE1"), TEXT("000000"), TEXT("ECE9E2"),
		TEXT("FFFFFF"), TEXT("F4F3EF"), TEXT("E6EEF9"),
		TEXT("D4D0C8"), TEXT("000000"), TEXT("808080"),
		TEXT("D8D5CE"), TEXT("C4C0B8"), TEXT("ADA9A1"), TEXT("8C8880") };

	static const FFlat GDarkFlat = { TEXT("2E2E2E"), TEXT("1A1A1A"), TEXT("4A4F58"),
		TEXT("3A3A3A"), TEXT("1A1A1A"), TEXT("3D5A8C"), TEXT("6F8FC8"),
		TEXT("F0F0F0"), TEXT("101010"), TEXT("2A2A2A"),
		TEXT("262626"), TEXT("2C2C2C"), TEXT("3A404A"),
		TEXT("3A3A3A"), TEXT("E6E6E6"), TEXT("808080"),
		TEXT("262626"), TEXT("303030"), TEXT("3A3A3A"), TEXT("4A4A4A") };

	// ---------------------------------------------------------------- colours
	// The theme manager keeps the live colours in a reflected array, found by
	// name so nothing here depends on its private layout.
	static void DumpProps(const UStruct* S, const TCHAR* Label)
	{
		for (TFieldIterator<FProperty> It(S); It; ++It)
		{
			Note(FString::Printf(TEXT("  prop %s.%s (%s, dim %d)"), Label, *It->GetName(), *It->GetClass()->GetName(), It->ArrayDim));
		}
	}

	// Live theme colour array (USlateThemeManager::ActiveColors.StyleColors), found by reflection.
	static FLinearColor* ColorArray(int32& OutDim, UEnum*& OutEnum)
	{
		OutDim = 0; OutEnum = nullptr;
		UClass* C = FindObject<UClass>(nullptr, TEXT("/Script/SlateCore.SlateThemeManager"));
		if (!C) { Note(TEXT("MISS colours: SlateThemeManager class")); ++GMiss; return nullptr; }
		UObject* TM = C->GetDefaultObject();
		FStructProperty* ListProp = FindFProperty<FStructProperty>(C, TEXT("ActiveColors"));
		if (!ListProp) { Note(TEXT("MISS colours: ActiveColors")); ++GMiss; DumpProps(C, TEXT("SlateThemeManager")); return nullptr; }
		void* List = ListProp->ContainerPtrToValuePtr<void>(TM);
		FStructProperty* ArrProp = FindFProperty<FStructProperty>(ListProp->Struct, TEXT("StyleColors"));
		if (!ArrProp || ArrProp->Struct != TBaseStructure<FLinearColor>::Get())
		{
			Note(TEXT("MISS colours: StyleColors array")); ++GMiss; DumpProps(ListProp->Struct, TEXT("ColorList")); return nullptr;
		}
		OutEnum = FindObject<UEnum>(nullptr, TEXT("/Script/SlateCore.EStyleColor"));
		if (!OutEnum) { OutEnum = FindFirstObject<UEnum>(TEXT("EStyleColor"), EFindFirstObjectOptions::None); }
		if (!OutEnum) { Note(TEXT("MISS colours: EStyleColor enum")); ++GMiss; return nullptr; }
		OutDim = ArrProp->ArrayDim;
		return ArrProp->ContainerPtrToValuePtr<FLinearColor>(List, 0);
	}

	static int32 ColorIndex(UEnum* E, const TCHAR* Name)
	{
		int64 V = E->GetValueByNameString(Name);
		if (V == INDEX_NONE) { V = E->GetValueByNameString(E->GenerateFullEnumName(Name)); }
		return (int32)V;
	}

	// Engine colours before any theme work (the user's own theme) and our palette per index.
	static TArray<FLinearColor> GDefaults;
	static TArray<FLinearColor> GPalette;
	static TArray<bool> GPaletteSet;
	static int32 GSentinel = INDEX_NONE;

	static void ApplyColors(bool bVerbose)
	{
		int32 Dim = 0; UEnum* E = nullptr;
		FLinearColor* Arr = ColorArray(Dim, E);
		if (!Arr) { return; }
		if (GDefaults.Num() == 0) { GDefaults = TArray<FLinearColor>(Arr, Dim); }

		const FNamedColor* Colors = GDark ? GDarkColors : GLightColors;
		const int32 Num = GDark ? (int32)UE_ARRAY_COUNT(GDarkColors) : (int32)UE_ARRAY_COUNT(GLightColors);
		GPalette = GDefaults;
		GPaletteSet.Init(false, Dim);

		int32 Set = 0;
		for (int32 i = 0; i < Num; ++i)
		{
			const FNamedColor& NC = Colors[i];
			const int32 V = ColorIndex(E, NC.Name);
			if (V == INDEX_NONE || V < 0 || V >= Dim)
			{
				if (bVerbose) { Note(FString::Printf(TEXT("MISS colour %s"), NC.Name)); ++GMiss; }
				continue;
			}
			Arr[V] = Hex(NC.HexValue);
			GPalette[V] = Arr[V];
			GPaletteSet[V] = true;
			++Set;
		}
		GSentinel = ColorIndex(E, TEXT("Panel"));
		if (bVerbose)
		{
			Note(FString::Printf(TEXT("colours (%s) set %d / %d (array dim %d)"), GDark ? TEXT("dark") : TEXT("light"), Set, Num, Dim));
		}
	}

	// Unreal re-applies its own theme colours at times (e.g. Load Layout). Cheap check, re-apply if so.
	static void WatchColors()
	{
		if (GSentinel == INDEX_NONE) { return; }
		int32 Dim = 0; UEnum* E = nullptr;
		FLinearColor* Arr = ColorArray(Dim, E);
		if (!Arr || GSentinel >= Dim || GSentinel >= GPalette.Num()) { return; }
		if (!Arr[GSentinel].Equals(GPalette[GSentinel], 0.001f))
		{
			ApplyColors(false);
			Note(TEXT("watchdog: engine reset the theme colours, re-applied"));
		}
	}

	// Many engine styles copy a theme colour at startup (GetSpecifiedColor) instead of linking it,
	// so they stay dark whatever the theme. Every such copy that equals an engine theme colour is
	// recorded once and re-pointed to our palette on each apply.
	struct FRemap { FSlateColor* Target; int32 Index; float Alpha; };
	static TArray<FRemap> GRemaps;
	static bool GRemapScanned = false;

	static TSet<int32> GSkip;      // never a remap target (white, black...)
	static TSet<int32> GTextOnly;  // text colours: never the target of a fill

	// IsColorSpecified() is also true for theme-linked colours; those already follow the theme.
	static bool IsLinked(const FSlateColor& C)
	{
		for (int32 i = 0; i < GDefaults.Num(); ++i)
		{
			if (C == FSlateColor((EStyleColor)i)) { return true; }
		}
		return false;
	}

	static void ScanColor(FSlateColor& C, bool bFill)
	{
		if (!C.IsColorSpecified() || IsLinked(C)) { return; }
		const FLinearColor V = C.GetSpecifiedColor();
		if (V.A <= 0.01f) { return; }
		const float Max = FMath::Max3(V.R, V.G, V.B), Min = FMath::Min3(V.R, V.G, V.B);
		if (Max < 0.003f || Min > 0.997f) { return; }
		for (int32 i = 0; i < GDefaults.Num(); ++i)
		{
			if (GSkip.Contains(i) || (bFill && GTextOnly.Contains(i)) || !GPaletteSet.IsValidIndex(i) || !GPaletteSet[i]) { continue; }
			const FLinearColor& D = GDefaults[i];
			if (FMath::Abs(V.R - D.R) < 0.002f && FMath::Abs(V.G - D.G) < 0.002f && FMath::Abs(V.B - D.B) < 0.002f)
			{
				GRemaps.Add({ &C, i, V.A });
				return;
			}
		}
	}

	static TArray<const ISlateStyle*> StyleSets()
	{
		TArray<const ISlateStyle*> Sets;
		auto AddSet = [&Sets](const ISlateStyle* S) { if (S && !Sets.Contains(S)) { Sets.Add(S); } };
		AddSet(&FAppStyle::Get());
		AddSet(FSlateStyleRegistry::FindSlateStyle(FName(TEXT("CoreStyle"))));
		AddSet(FSlateStyleRegistry::FindSlateStyle(FName(TEXT("EditorStyle"))));
		AddSet(&FCoreStyle::Get());
		return Sets;
	}

	// Every registered style set (plugins included). Only used for sweeps that are safe anywhere.
	static TArray<const ISlateStyle*> AllStyleSets()
	{
		TArray<const ISlateStyle*> Sets = StyleSets();
		FSlateStyleRegistry::IterateAllStyles([&Sets](const ISlateStyle& S)
		{
			if (!Sets.Contains(&S)) { Sets.Add(&S); }
			return true;
		});
		return Sets;
	}

	static void ScanRemaps()
	{
		if (GRemapScanned || GDefaults.Num() == 0) { return; }
		GRemapScanned = true;
		int32 Dim = 0; UEnum* E = nullptr;
		ColorArray(Dim, E);
		if (E)
		{
			GSkip.Add(ColorIndex(E, TEXT("White")));
			GSkip.Add(ColorIndex(E, TEXT("Black")));
			for (const TCHAR* N : { TEXT("Foreground"), TEXT("ForegroundHover"), TEXT("ForegroundHeader"), TEXT("ForegroundInverted") })
			{
				GTextOnly.Add(ColorIndex(E, N));
			}
		}

		TSet<const FSlateBrush*> Brushes;
		for (const ISlateStyle* S : StyleSets())
		{
			TArray<const FSlateBrush*> Res;
			S->GetResources(Res);
			for (const FSlateBrush* B : Res) { if (B) { Brushes.Add(B); } }
			for (const FName& Key : S->GetStyleKeys())
			{
				if (const FSlateBrush* B = S->GetOptionalBrush(Key, nullptr, nullptr)) { Brushes.Add(B); }
				if (S->HasWidgetStyle<FTextBlockStyle>(Key))
				{
					FTextBlockStyle& T = const_cast<FTextBlockStyle&>(S->GetWidgetStyle<FTextBlockStyle>(Key));
					ScanColor(T.ColorAndOpacity, false);
				}
				if (S->HasWidgetStyle<FButtonStyle>(Key))
				{
					FButtonStyle& B = const_cast<FButtonStyle&>(S->GetWidgetStyle<FButtonStyle>(Key));
					ScanColor(B.NormalForeground, false); ScanColor(B.HoveredForeground, false);
					ScanColor(B.PressedForeground, false); ScanColor(B.DisabledForeground, false);
				}
				if (S->HasWidgetStyle<FCheckBoxStyle>(Key))
				{
					FCheckBoxStyle& C = const_cast<FCheckBoxStyle&>(S->GetWidgetStyle<FCheckBoxStyle>(Key));
					ScanColor(C.ForegroundColor, false); ScanColor(C.HoveredForeground, false);
					ScanColor(C.PressedForeground, false); ScanColor(C.CheckedForeground, false);
					ScanColor(C.CheckedHoveredForeground, false); ScanColor(C.CheckedPressedForeground, false);
				}
				if (S->HasWidgetStyle<FEditableTextBoxStyle>(Key))
				{
					FEditableTextBoxStyle& T = const_cast<FEditableTextBoxStyle&>(S->GetWidgetStyle<FEditableTextBoxStyle>(Key));
					ScanColor(T.ForegroundColor, false);
				}
			}
		}
		for (const FSlateBrush* B : Brushes)
		{
			FSlateBrush* M = const_cast<FSlateBrush*>(B);
			// Icons (images) may follow the text colour; fills and outlines may not.
			const bool bIcon = M->DrawAs == ESlateBrushDrawType::Image && !M->GetResourceName().IsNone();
			ScanColor(M->TintColor, !bIcon);
			ScanColor(M->OutlineSettings.Color, true);
		}
		Note(FString::Printf(TEXT("remap: %d hard-coded colour copies found in %d brushes"), GRemaps.Num(), Brushes.Num()));
	}

	static bool IsInherited(const FSlateColor& C)
	{
		return C == FSlateColor::UseForeground() || C == FSlateColor::UseSubduedForeground();
	}

	// Text styles with a fixed light colour (written for dark panels) become unreadable in Light.
	// Graph, viewport and code text sit on dark backgrounds in both modes and are left alone.
	static TMap<FTextBlockStyle*, FSlateColor> GTextOriginal;

	static void FixLightText(const TArray<const ISlateStyle*>& Sets, const FSlateColor& Ink, const FSlateColor& Dim)
	{
		static const TCHAR* Keep[] = { TEXT("Graph"), TEXT("Kismet"), TEXT("Viewport"), TEXT("Node"), TEXT("SyntaxHighlight"), TEXT("Credits"), TEXT("Sequencer") };
		const FWidgetStyle WS;
		int32 Fixed = 0;
		for (const ISlateStyle* S : Sets)
		{
			for (const FName& Key : S->GetStyleKeys())
			{
				if (!S->HasWidgetStyle<FTextBlockStyle>(Key)) { continue; }
				FTextBlockStyle* T = const_cast<FTextBlockStyle*>(&S->GetWidgetStyle<FTextBlockStyle>(Key));
				if (!GTextOriginal.Contains(T))
				{
					const FSlateColor& C = T->ColorAndOpacity;
					if (IsInherited(C) || !C.IsColorSpecified() || IsLinked(C)) { continue; }
					const FString K = Key.ToString();
					bool bKeep = false;
					for (const TCHAR* W : Keep) { if (K.Contains(W)) { bKeep = true; break; } }
					const FLinearColor V = C.GetSpecifiedColor();
					const float L = 0.2126f * V.R + 0.7152f * V.G + 0.0722f * V.B;
					if (bKeep || V.A < 0.5f || L < 0.45f) { continue; }
					GTextOriginal.Add(T, C);
				}
				const FSlateColor& Orig = GTextOriginal[T];
				const FLinearColor V = Orig.GetSpecifiedColor();
				const float L = 0.2126f * V.R + 0.7152f * V.G + 0.0722f * V.B;
				T->ColorAndOpacity = GDark ? Orig : (L > 0.6f ? Ink : Dim);
				++Fixed;
			}
		}
		Note(FString::Printf(TEXT("OK   light-text styles %s: %d"), GDark ? TEXT("restored") : TEXT("darkened"), Fixed));
	}

	// Name fields (Details header, inline rename) draw their resting text in the theme's White,
	// which stays white in Light. Those go to the text colour in Light and back in Dark.
	static TMap<FTextBlockStyle*, FSlateColor> GWhiteText;
	static void InkWhiteText(FTextBlockStyle& T, const FSlateColor& Ink)
	{
		if (!GWhiteText.Contains(&T))
		{
			if (!(T.ColorAndOpacity == FSlateColor(EStyleColor::White))) { return; }
			GWhiteText.Add(&T, T.ColorAndOpacity);
		}
		T.ColorAndOpacity = GDark ? GWhiteText[&T] : Ink;
	}

	static void ApplyRemaps()
	{
		for (const FRemap& R : GRemaps)
		{
			if (!GPalette.IsValidIndex(R.Index)) { continue; }
			FLinearColor V = GPalette[R.Index];
			V.A = R.Alpha;
			*R.Target = FSlateColor(V);
		}
	}

	// ---------------------------------------------------------------- brushes
	static FString Res(const TCHAR* Base)
	{
		return GResDir / FString::Printf(TEXT("%s%s.png"), Base, GDark ? TEXT("_d") : TEXT(""));
	}

	static FSlateBrush Box(const TCHAR* Base)
	{
		// 64 px images with 1 px bands: Slate sizes box edges from the real texture (2/64 of 64 = 2 px)
		// and the smoothing at the centre stays under a pixel. The declared 8 px only drives layout.
		return FSlateBoxBrush(Res(Base), FVector2D(8.0, 8.0), FMargin(2.f / 64.f));
	}

	static FSlateBrush Img(const TCHAR* Base, double W, double H)
	{
		return FSlateImageBrush(Res(Base), FVector2D(W, H));
	}

	// Flat fill + 1 px border as a 4x4 pixel image: crisp edges, unlike the anti-aliased box shader.
	static FSlateBrush Pixel(const TCHAR* Base)
	{
		return FSlateBoxBrush(Res(Base), FVector2D(4.0, 4.0), FMargin(1.f / 64.f));
	}

	static FMargin Shrink(const FMargin& M, float SX, float SY)
	{
		return FMargin(FMath::RoundToFloat(M.Left * SX), FMath::RoundToFloat(M.Top * SY),
			FMath::RoundToFloat(M.Right * SX), FMath::RoundToFloat(M.Bottom * SY));
	}

	static FString M2S(const FMargin& M)
	{
		return FString::Printf(TEXT("%.0f,%.0f,%.0f,%.0f"), M.Left, M.Top, M.Right, M.Bottom);
	}

	// Density changes are relative: once per value, never again on a mode switch.
	static bool OnceD(const void* P)
	{
		bool bAlready = false;
		GDense.Add(P, &bAlready);
		return !bAlready;
	}

	template<typename T, typename FFn>
	static void PatchStyle(const TArray<const ISlateStyle*>& Sets, const TCHAR* Key, FFn&& Fn)
	{
		TSet<const void*> Seen;
		for (const ISlateStyle* S : Sets)
		{
			if (!S->HasWidgetStyle<T>(FName(Key))) { continue; }
			T* M = const_cast<T*>(&S->GetWidgetStyle<T>(FName(Key)));
			bool bAlready = false;
			Seen.Add(M, &bAlready);
			if (!bAlready) { Fn(*M); }
		}
		Note(FString::Printf(TEXT("%s style %s"), Seen.Num() ? TEXT("OK  ") : TEXT("MISS"), Key));
		if (!Seen.Num()) { ++GMiss; }
	}

	static void PatchBrush(const TArray<const ISlateStyle*>& Sets, const TCHAR* Key, const FSlateBrush& New)
	{
		int32 Found = 0;
		for (const ISlateStyle* S : Sets)
		{
			const FSlateBrush* B = S->GetOptionalBrush(FName(Key), nullptr, nullptr);
			if (!B) { continue; }
			++Found;
			*const_cast<FSlateBrush*>(B) = New;
		}
		Note(FString::Printf(TEXT("%s brush %s"), Found ? TEXT("OK  ") : TEXT("MISS"), Key));
		if (!Found) { ++GMiss; }
	}

	static void SetMarginAbs(const TArray<const ISlateStyle*>& Sets, const TCHAR* Key, const FMargin& New)
	{
		static const FMargin Sentinel(-9999.0f);
		int32 Found = 0;
		for (const ISlateStyle* S : Sets)
		{
			const FMargin& M = S->GetMargin(FName(Key), nullptr, Sentinel);
			if (&M == &Sentinel) { continue; }
			++Found;
			*const_cast<FMargin*>(&M) = New;
		}
		Note(FString::Printf(TEXT("%s margin %s = %s"), Found ? TEXT("OK  ") : TEXT("MISS"), Key, *M2S(New)));
		if (!Found) { ++GMiss; }
	}

	static void SetColorAbs(const TArray<const ISlateStyle*>& Sets, const TCHAR* Key, const FLinearColor& New)
	{
		static const FLinearColor Sentinel(-9.f, -9.f, -9.f, -9.f);
		int32 Found = 0;
		for (const ISlateStyle* S : Sets)
		{
			const FLinearColor& C = S->GetColor(FName(Key), nullptr, Sentinel);
			if (&C == &Sentinel) { continue; }
			++Found;
			*const_cast<FLinearColor*>(&C) = New;
		}
		Note(FString::Printf(TEXT("%s colour %s"), Found ? TEXT("OK  ") : TEXT("MISS"), Key));
		if (!Found) { ++GMiss; }
	}

	// Static brushes only draw if the renderer loaded their texture; brushes created after
	// startup (e.g. the Dark set) are not. A style set holding every theme image makes the
	// renderer load them all once, up front.
	static void RegisterResources()
	{
		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(GResDir / TEXT("*.png")), true, false);
		GResStyle = MakeShared<FSlateStyleSet>(TEXT("EasyThemeResources"));
		for (const FString& F : Files)
		{
			GResStyle->Set(FName(*F), new FSlateImageBrush(GResDir / F, FVector2D(8.0, 8.0)));
		}
		FSlateStyleRegistry::RegisterSlateStyle(*GResStyle);
		Note(FString::Printf(TEXT("OK   resources registered: %d images"), Files.Num()));
	}

	static void PatchMargin(const TArray<const ISlateStyle*>& Sets, const TCHAR* Key, float SX, float SY)
	{
		static const FMargin Sentinel(-9999.0f);
		int32 Found = 0;
		for (const ISlateStyle* S : Sets)
		{
			const FMargin& M = S->GetMargin(FName(Key), nullptr, Sentinel);
			if (&M == &Sentinel) { continue; }
			++Found;
			if (OnceD(&M))
			{
				const FString Before = M2S(M);
				*const_cast<FMargin*>(&M) = Shrink(M, SX, SY);
				Note(FString::Printf(TEXT("  margin %s %s -> %s"), Key, *Before, *M2S(M)));
			}
		}
		Note(FString::Printf(TEXT("%s margin %s"), Found ? TEXT("OK  ") : TEXT("MISS"), Key));
		if (!Found) { ++GMiss; }
	}

	// ---------------------------------------------------------------- graph preferences
	// Grid and exec-wire colours are Editor Preferences values. Light graphs change them in memory
	// only; the user's values are backed up in [EasyTheme] and put back on Dark, on exit, and on the
	// next start after a crash. If Unreal ever saved the light values to disk, they are rewritten.
	struct FGraphPref { const TCHAR* Class; const TCHAR* Prop; const TCHAR* Light; };
	static const FGraphPref GGraphPrefs[] =
	{
		{ TEXT("EditorStyleSettings"), TEXT("RegularColor"),          TEXT("C4C0B8") },
		{ TEXT("EditorStyleSettings"), TEXT("RuleColor"),             TEXT("ADA9A1") },
		{ TEXT("EditorStyleSettings"), TEXT("CenterColor"),           TEXT("8C8880") },
		{ TEXT("GraphEditorSettings"), TEXT("ExecutionPinTypeColor"), TEXT("6E6E6E") },
	};

	static FLinearColor* GraphPrefPtr(const FGraphPref& R, UObject*& OutCDO)
	{
		UClass* C = FindFirstObject<UClass>(R.Class, EFindFirstObjectOptions::NativeFirst);
		if (!C) { return nullptr; }
		FStructProperty* P = CastField<FStructProperty>(C->FindPropertyByName(FName(R.Prop)));
		if (!P || P->Struct != TBaseStructure<FLinearColor>::Get()) { return nullptr; }
		OutCDO = C->GetDefaultObject();
		return OutCDO ? P->ContainerPtrToValuePtr<FLinearColor>(OutCDO) : nullptr;
	}

	static bool GraphPrefOnDiskIsLight(UObject* CDO, const FGraphPref& R)
	{
		FString Str;
		if (!GConfig->GetString(*CDO->GetClass()->GetPathName(), R.Prop, Str, CDO->GetClass()->GetConfigName())) { return false; }
		FLinearColor C;
		return C.InitFromString(Str) && C.Equals(Hex(R.Light), 0.002f);
	}

	static void ApplyGraphColors(bool bLight)
	{
		TSet<UObject*> Rewrite;
		int32 Done = 0;
		for (const FGraphPref& R : GGraphPrefs)
		{
			UObject* CDO = nullptr;
			FLinearColor* V = GraphPrefPtr(R, CDO);
			if (!V) { Note(FString::Printf(TEXT("MISS graph pref %s.%s"), R.Class, R.Prop)); continue; }
			const FLinearColor Light = Hex(R.Light);
			const FString Key = FString(TEXT("Orig")) + R.Prop;
			const bool bIsLight = V->Equals(Light, 0.002f);
			if (bLight)
			{
				if (!bIsLight) { GConfig->SetString(GConfigSection, *Key, *V->ToString(), GEditorSettingsIni); *V = Light; }
			}
			else
			{
				FString Saved; FLinearColor Orig;
				if (bIsLight && GConfig->GetString(GConfigSection, *Key, Saved, GEditorSettingsIni) && Orig.InitFromString(Saved)) { *V = Orig; }
				if (GraphPrefOnDiskIsLight(CDO, R)) { Rewrite.Add(CDO); }
			}
			++Done;
		}
		GConfig->Flush(false, GEditorSettingsIni);
		for (UObject* O : Rewrite) { O->SaveConfig(); Note(FString::Printf(TEXT("fixed saved graph colours in %s"), *O->GetClass()->GetName())); }
		Note(FString::Printf(TEXT("%s graph preferences %s: %d"), Done ? TEXT("OK  ") : TEXT("MISS"), bLight ? TEXT("light") : TEXT("user"), Done));
	}

	static void ForgetGraphColors()
	{
		ApplyGraphColors(false);
		for (const FGraphPref& R : GGraphPrefs) { GConfig->RemoveKey(GConfigSection, *(FString(TEXT("Orig")) + R.Prop), GEditorSettingsIni); }
		GConfig->RemoveKey(GConfigSection, TEXT("LightGraphs"), GEditorSettingsIni);
		GConfig->Flush(false, GEditorSettingsIni);
	}

	static void ApplyStyles()
	{
		const TArray<const ISlateStyle*> Sets = StyleSets();
		Note(FString::Printf(TEXT("styles (%s): %d style sets"), GDark ? TEXT("dark") : TEXT("light"), Sets.Num()));

		const FFlat& F = GDark ? GDarkFlat : GLightFlat;

		const FSlateBrush Raised      = Box(TEXT("raised"));
		const FSlateBrush RaisedHover = Box(TEXT("raised_hover"));
		const FSlateBrush Sunken      = Box(TEXT("sunken"));
		const FSlateBrush Field       = Box(TEXT("field"));
		const FSlateBrush FieldHover  = Box(TEXT("field_hover"));
		const FSlateBrush FieldFocus  = Box(TEXT("field_focus"));
		const FSlateBrush FieldRO     = Box(TEXT("field_ro"));
		const FSlateBrush TabFore     = Box(TEXT("tab_fore"));
		const FSlateBrush Tick        = Img(TEXT("check_on"), 10, 10);
		const FSlateBrush Mixed       = Img(TEXT("check_mixed"), 10, 10);
		const FSlateBrush Blank       = Img(TEXT("blank10"), 10, 10);
		// Light: panel grey with an etched line under the menus. Dark: navy gradient.
		const FSlateBrush TitleOn     = GDark ? Img(TEXT("title_active"), 256, 2)
			: FSlateBrush(FSlateBoxBrush(Res(TEXT("title_active")), FVector2D(8.0, 8.0), FMargin(0.f, 0.f, 0.f, 2.f / 64.f)));
		const FSlateBrush TitleOff    = GDark ? Img(TEXT("title_inactive"), 256, 2)
			: FSlateBrush(FSlateBoxBrush(Res(TEXT("title_inactive")), FVector2D(8.0, 8.0), FMargin(0.f, 0.f, 0.f, 2.f / 64.f)));
		const FSlateBrush TabIdle     = Pixel(TEXT("flat_tabidle"));
		const FSlateBrush TabHover    = Pixel(TEXT("flat_tabhover"));
		const FSlateBrush MenuBg      = Pixel(TEXT("flat_menubg"));
		const FSlateBrush MenuHot     = Pixel(TEXT("flat_menuhot"));
		const FSlateBrush Tip         = Pixel(TEXT("flat_tip"));
		const FSlateBrush ToolIdle    = Pixel(TEXT("flat_toolidle"));
		const FSlateBrush Track       = FSlateColorBrush(Hex(F.Track));
		const FSlateBrush RowEven     = FSlateColorBrush(Hex(F.RowEven));
		const FSlateBrush RowOdd      = FSlateColorBrush(Hex(F.RowOdd));
		const FSlateBrush RowHot      = FSlateColorBrush(Hex(F.RowHot));
		const FSlateBrush FaceFill    = FSlateColorBrush(Hex(F.Face));
		const FSlateBrush None        = FSlateNoResource();
		const FSlateBrush Sep         = Img(TEXT("sep"), 2, 8);
		const FSlateColor Ink(Hex(F.Ink));
		const FSlateColor Grey(Hex(F.Grey));
		const FSlateColor NoTint(FLinearColor::White);

		auto Compact = [](FButtonStyle& B)
		{
			if (!OnceD(&B.NormalPadding)) { return; }
			const FString Before = M2S(B.NormalPadding);
			B.NormalPadding = Shrink(B.NormalPadding, GPadX, GPadY);
			const FMargin& N = B.NormalPadding;
			B.PressedPadding = FMargin(N.Left + 1.f, N.Top + 1.f, FMath::Max(N.Right - 1.f, 0.f), FMath::Max(N.Bottom - 1.f, 0.f));
			Note(FString::Printf(TEXT("  button pad %s -> %s"), *Before, *M2S(N)));
		};
		auto RaisedButton = [&](FButtonStyle& B)
		{
			B.Normal = Raised; B.Hovered = RaisedHover; B.Pressed = Sunken; B.Disabled = Raised;
			B.NormalForeground = Ink; B.HoveredForeground = Ink; B.PressedForeground = Ink; B.DisabledForeground = Grey;
			Compact(B);
		};
		auto ToolButton = [&](FButtonStyle& B)
		{
			B.Normal = None; B.Hovered = Raised; B.Pressed = Sunken; B.Disabled = None;
			B.NormalForeground = Ink; B.HoveredForeground = Ink; B.PressedForeground = Ink; B.DisabledForeground = Grey;
			Compact(B);
		};
		auto TextBox = [&](FEditableTextBoxStyle& T)
		{
			T.BackgroundImageNormal = Field; T.BackgroundImageHovered = FieldHover;
			T.BackgroundImageFocused = FieldFocus; T.BackgroundImageReadOnly = FieldRO;
			T.BackgroundColor = NoTint; T.ForegroundColor = Ink;
			if (OnceD(&T.Padding)) { T.Padding = Shrink(T.Padding, 0.8f, GPadY); }
		};
		auto Spin = [&](FSpinBoxStyle& S)
		{
			S.BackgroundBrush = Field; S.HoveredBackgroundBrush = FieldHover; S.ForegroundColor = Ink;
			if (OnceD(&S.TextPadding)) { S.TextPadding = Shrink(S.TextPadding, 0.8f, GPadY); }
		};
		auto Tab = [&](FDockTabStyle& D)
		{
			D.NormalBrush = TabIdle; D.HoveredBrush = TabHover; D.ForegroundBrush = TabFore;
			D.NormalForegroundColor = Ink; D.HoveredForegroundColor = Ink; D.ActiveForegroundColor = Ink; D.ForegroundForegroundColor = Ink;
			if (OnceD(&D.TabPadding))
			{
				const FString Before = M2S(D.TabPadding);
				D.TabPadding = Shrink(D.TabPadding, 0.8f, 0.4f);
				Note(FString::Printf(TEXT("  tab pad %s -> %s"), *Before, *M2S(D.TabPadding)));
			}
		};
		auto Bar = [&](FToolBarStyle& T)
		{
			if (OnceD(&T.ButtonPadding))
			{
				T.ButtonPadding = Shrink(T.ButtonPadding, GPadX, GPadY);
				T.CheckBoxPadding = Shrink(T.CheckBoxPadding, GPadX, GPadY);
				T.ComboButtonPadding = Shrink(T.ComboButtonPadding, GPadX, GPadY);
				T.BlockPadding = Shrink(T.BlockPadding, GPadX, GPadY);
				T.BackgroundPadding = Shrink(T.BackgroundPadding, GPadX, GPadY);
				T.SetIconSize(FVector2D(16.0, 16.0));
			}
			// Same height as a tab (27 px) so toolbars line up with the tabs in the tab row:
			// no vertical padding around blocks, 2 px inside buttons.
			auto Vert = [](FMargin M, float V) { M.Top = V; M.Bottom = V; return M; };
			T.BackgroundPadding = Vert(T.BackgroundPadding, 0.f);
			T.BlockPadding = Vert(T.BlockPadding, 0.f);
			T.ButtonPadding = Vert(T.ButtonPadding, 2.f);
			T.ComboButtonPadding = Vert(T.ComboButtonPadding, 2.f);
			T.CheckBoxPadding = Vert(T.CheckBoxPadding, 2.f);
			ToolButton(T.ButtonStyle);
			// Toolbar buttons: faint outline at rest (also when disabled), full bevel on hover.
			T.ButtonStyle.Normal = ToolIdle; T.ButtonStyle.Disabled = ToolIdle;
			T.SetSeparatorBrush(Sep);
		};
		auto Row = [&](FTableRowStyle& R)
		{
			R.TextColor = Ink; R.SelectedTextColor = Ink;
			R.EvenRowBackgroundBrush = RowEven; R.OddRowBackgroundBrush = RowOdd;
			R.EvenRowBackgroundHoveredBrush = RowHot; R.OddRowBackgroundHoveredBrush = RowHot;
		};
		auto MenuButton = [&](FButtonStyle& B)
		{
			B.Hovered = MenuHot; B.Pressed = MenuHot; B.HoveredForeground = Ink; B.PressedForeground = Ink;
		};

		PatchStyle<FButtonStyle>(Sets, TEXT("Button"), RaisedButton);
		PatchStyle<FButtonStyle>(Sets, TEXT("PrimaryButton"), RaisedButton);
		PatchStyle<FButtonStyle>(Sets, TEXT("SimpleButton"), ToolButton);
		PatchStyle<FComboButtonStyle>(Sets, TEXT("ComboButton"), [&](FComboButtonStyle& C) { RaisedButton(C.ButtonStyle); });
		PatchStyle<FComboBoxStyle>(Sets, TEXT("ComboBox"), [&](FComboBoxStyle& C) { RaisedButton(C.ComboButtonStyle.ButtonStyle); });

		PatchStyle<FCheckBoxStyle>(Sets, TEXT("Checkbox"), [&](FCheckBoxStyle& C)
		{
			if (OnceD(&C.Padding)) { C.Padding = Shrink(C.Padding, 0.8f, GPadY); }
			C.BackgroundImage = Field; C.BackgroundHoveredImage = FieldHover; C.BackgroundPressedImage = FieldHover;
			C.UncheckedImage = Blank; C.UncheckedHoveredImage = Blank; C.UncheckedPressedImage = Blank;
			C.CheckedImage = Tick; C.CheckedHoveredImage = Tick; C.CheckedPressedImage = Tick;
			C.UndeterminedImage = Mixed; C.UndeterminedHoveredImage = Mixed; C.UndeterminedPressedImage = Mixed;
		});

		PatchStyle<FScrollBarStyle>(Sets, TEXT("Scrollbar"), [&](FScrollBarStyle& S)
		{
			S.NormalThumbImage = Raised; S.HoveredThumbImage = RaisedHover; S.DraggedThumbImage = Sunken;
			S.VerticalBackgroundImage = Track; S.HorizontalBackgroundImage = Track;
		});

		PatchStyle<FEditableTextBoxStyle>(Sets, TEXT("NormalEditableTextBox"), TextBox);
		PatchStyle<FSearchBoxStyle>(Sets, TEXT("SearchBox"), [&](FSearchBoxStyle& S) { TextBox(S.TextBoxStyle); });
		PatchStyle<FSpinBoxStyle>(Sets, TEXT("SpinBox"), Spin);
		PatchStyle<FSpinBoxStyle>(Sets, TEXT("NumericEntrySpinBox"), Spin);

		PatchStyle<FDockTabStyle>(Sets, TEXT("Docking.Tab"), Tab);
		PatchStyle<FDockTabStyle>(Sets, TEXT("Docking.MajorTab"), Tab);

		PatchStyle<FWindowStyle>(Sets, TEXT("Window"), [&](FWindowStyle& W)
		{
			W.ActiveTitleBrush = TitleOn; W.InactiveTitleBrush = TitleOff; W.FlashTitleBrush = TitleOn;
			W.BackgroundBrush = FaceFill;
		});

		PatchStyle<FTableRowStyle>(Sets, TEXT("TableView.Row"), Row);
		PatchStyle<FTableRowStyle>(Sets, TEXT("SceneOutliner.TableViewRow"), Row);
		PatchStyle<FTableRowStyle>(Sets, TEXT("ContentBrowser.AssetListView.ColumnListTableRow"), Row);
		PatchStyle<FTableRowStyle>(Sets, TEXT("ContentBrowser.AssetListView.TileTableRow"), Row);
		PatchStyle<FTableRowStyle>(Sets, TEXT("PropertyTable.TableRow"), Row);
		PatchStyle<FTableRowStyle>(Sets, TEXT("SimpleTableView.Row"), Row);

		// Content Browser tiles and list rows draw their text in hard-coded white when hovered or
		// selected. Light mode gives those states a classic selection blue so white stays readable.
		{
			struct FTileTint { const TCHAR* Key; EStyleColor Link; int32 Level; bool bOutline; };
			static const FTileTint Tiles[] =
			{
				{ TEXT("ContentBrowser.AssetTileItem.NameAreaHoverBackground"),          EStyleColor::Hover,        0, false },
				{ TEXT("ContentBrowser.AssetTileItem.AssetContentHoverBackground"),      EStyleColor::Hover,        0, false },
				{ TEXT("ContentBrowser.AssetTileItem.FolderAreaHoveredBackground"),      EStyleColor::Hover,        0, false },
				{ TEXT("ContentBrowser.AssetTileItem.NameAreaSelectedBackground"),       EStyleColor::Primary,      1, false },
				{ TEXT("ContentBrowser.AssetTileItem.AssetContentSelectedBackground"),   EStyleColor::Primary,      1, false },
				{ TEXT("ContentBrowser.AssetTileItem.FolderAreaSelectedBackground"),     EStyleColor::Primary,      1, false },
				{ TEXT("ContentBrowser.AssetTileItem.NameAreaSelectedHoverBackground"),  EStyleColor::PrimaryHover, 2, false },
				{ TEXT("ContentBrowser.AssetTileItem.AssetContentSelectedHoverBackground"), EStyleColor::PrimaryHover, 2, false },
				{ TEXT("ContentBrowser.AssetTileItem.FolderAreaSelectedHoverBackground"), EStyleColor::PrimaryHover, 2, false },
				{ TEXT("ContentBrowser.AssetTileItem.AssetBorderHoverBackground"),       EStyleColor::Hover,        0, true },
				{ TEXT("ContentBrowser.AssetTileItem.HoverBorder"),                      EStyleColor::Hover,        0, true },
				{ TEXT("ContentBrowser.AssetTileItem.AssetBorderSelectedBackground"),    EStyleColor::Primary,      1, true },
				{ TEXT("ContentBrowser.AssetTileItem.SelectedBorder"),                   EStyleColor::Primary,      1, true },
				{ TEXT("ContentBrowser.AssetTileItem.AssetBorderSelectedHoverBackground"), EStyleColor::PrimaryHover, 2, true },
				{ TEXT("ContentBrowser.AssetTileItem.SelectedHoverBorder"),              EStyleColor::PrimaryHover, 2, true },
			};
			const FLinearColor Blues[3] = { Hex(TEXT("3D6FC0")), Hex(TEXT("0A246A")), Hex(TEXT("1C3F94")) };
			int32 Tinted = 0;
			for (const FTileTint& T : Tiles)
			{
				for (const ISlateStyle* S : Sets)
				{
					FSlateBrush* B = const_cast<FSlateBrush*>(S->GetOptionalBrush(FName(T.Key), nullptr, nullptr));
					if (!B) { continue; }
					const FSlateColor C = GDark ? FSlateColor(T.Link) : FSlateColor(Blues[T.Level]);
					if (T.bOutline) { B->OutlineSettings.Color = C; } else { B->TintColor = C; }
					++Tinted;
				}
			}
			Note(FString::Printf(TEXT("OK   content browser tile states: %d brushes"), Tinted));

			PatchStyle<FTableRowStyle>(Sets, TEXT("ContentBrowser.AssetListView.ColumnListTableRow"), [&](FTableRowStyle& R)
			{
				if (GDark)
				{
					R.ActiveBrush.TintColor = FSlateColor(EStyleColor::Select);
					R.ActiveHoveredBrush.TintColor = FSlateColor(EStyleColor::Select);
					R.InactiveBrush.TintColor = FSlateColor(EStyleColor::SelectInactive);
					R.InactiveHoveredBrush.TintColor = FSlateColor(EStyleColor::SelectInactive);
					return;
				}
				const FSlateBrush Hot = FSlateColorBrush(Blues[0]);
				R.EvenRowBackgroundHoveredBrush = Hot; R.OddRowBackgroundHoveredBrush = Hot;
				R.ActiveBrush.TintColor = FSlateColor(Blues[1]); R.ActiveHoveredBrush.TintColor = FSlateColor(Blues[2]);
				R.InactiveBrush.TintColor = FSlateColor(Hex(TEXT("6A6A6A"))); R.InactiveHoveredBrush.TintColor = FSlateColor(Hex(TEXT("6A6A6A")));
				R.SelectedTextColor = FSlateColor(FLinearColor::White);
			});
		}

		PatchStyle<FToolBarStyle>(Sets, TEXT("ToolBar"), Bar);
		PatchStyle<FToolBarStyle>(Sets, TEXT("SlimToolBar"), Bar);
		PatchStyle<FToolBarStyle>(Sets, TEXT("AssetEditorToolbar"), Bar);
		PatchStyle<FToolBarStyle>(Sets, TEXT("Toolbar.BackplateLeft"), Bar);
		PatchStyle<FToolBarStyle>(Sets, TEXT("Toolbar.BackplateLeftPlay"), [&](FToolBarStyle& T)
		{
			// Keep the Play button's own (green) icon colour.
			const FSlateColor N = T.ButtonStyle.NormalForeground, H = T.ButtonStyle.HoveredForeground, P = T.ButtonStyle.PressedForeground;
			Bar(T);
			T.ButtonStyle.NormalForeground = N; T.ButtonStyle.HoveredForeground = H; T.ButtonStyle.PressedForeground = P;
		});
		PatchStyle<FToolBarStyle>(Sets, TEXT("Toolbar.BackplateCenter"), Bar);
		PatchStyle<FToolBarStyle>(Sets, TEXT("Toolbar.BackplateCenterStop"), Bar);
		PatchStyle<FToolBarStyle>(Sets, TEXT("Toolbar.BackplateRight"), Bar);
		PatchStyle<FToolBarStyle>(Sets, TEXT("Toolbar.BackplateRightCombo"), Bar);
		PatchBrush(Sets, TEXT("EditorViewportToolBar.Group"), Raised);
		PatchBrush(Sets, TEXT("EditorViewportToolBar.Group.Pressed"), Sunken);
		PatchMargin(Sets, TEXT("Menu.Block.Padding"), 0.8f, GPadY);
		PatchMargin(Sets, TEXT("Menu.Block.IndentedPadding"), 0.8f, GPadY);

		// Title / menu-bar row: the editor re-tints this brush from its window-colour setting, so it
		// is an image (tint multiplies it) instead of a colour (tint replaces it).
		PatchBrush(Sets, TEXT("Brushes.Title"), Img(TEXT("face"), 4, 4));
		// No big app logo; slimmer project badge. Together they set the title row height.
		PatchBrush(Sets, TEXT("AppIcon"), FSlateNoResource());
		SetMarginAbs(Sets, TEXT("AppIconPadding"), FMargin(4.f, 0.f, 0.f, 0.f));
		PatchBrush(Sets, TEXT("AppIcon.Small"), FSlateNoResource());
		SetMarginAbs(Sets, TEXT("AppIconPadding.Small"), FMargin(2.f, 0.f, 0.f, 0.f));
		SetMarginAbs(Sets, TEXT("SProjectBadge.BadgePadding"), FMargin(12.f, 2.f, 12.f, 3.f));

		PatchBrush(Sets, TEXT("Menu.Background"), MenuBg);
		PatchStyle<FButtonStyle>(Sets, TEXT("Menu.Button"), MenuButton);
		PatchStyle<FButtonStyle>(Sets, TEXT("WindowMenuBar.Button"), MenuButton);
		// Menu-bar entries must fit inside the slim title row (the hover frame overflowed below it).
		SetMarginAbs(Sets, TEXT("WindowMenuBar.MenuBar.Padding"), FMargin(8.f, 2.f));
		PatchBrush(Sets, TEXT("WindowMenuBar.Button.SubMenuOpen"), MenuHot);
		PatchBrush(Sets, TEXT("ToolTip.Background"), Tip);
		PatchBrush(Sets, TEXT("ToolTip.BrightBackground"), Tip);
		// Unreal draws tooltip text in hard-coded black (SToolTip, SDocumentationToolTip), so the
		// tooltip stays light in both modes: classic yellow in Light, light grey in Dark.

		FixLightText(Sets, Ink, FSlateColor(Hex(TEXT("505050"))));

		// ---- every text-entry style: typed, focused and read-only text all in the text colour
		// (a search box showed white typed text on its white field).
		{
			auto InkBox = [&](FEditableTextBoxStyle& T)
			{
				T.ForegroundColor = Ink; T.FocusedForegroundColor = Ink; T.ReadOnlyForegroundColor = Ink;
				T.TextStyle.ColorAndOpacity = Ink;
			};
			int32 Swept = 0;
			TSet<const void*> Done;
			for (const ISlateStyle* S : Sets)
			{
				for (const FName& Key : S->GetStyleKeys())
				{
					bool bNew = false;
					if (S->HasWidgetStyle<FEditableTextBoxStyle>(Key))
					{
						FEditableTextBoxStyle* T = const_cast<FEditableTextBoxStyle*>(&S->GetWidgetStyle<FEditableTextBoxStyle>(Key));
						Done.Add(T, &bNew); if (!bNew) { InkBox(*T); ++Swept; }
					}
					if (S->HasWidgetStyle<FSearchBoxStyle>(Key))
					{
						FSearchBoxStyle* T = const_cast<FSearchBoxStyle*>(&S->GetWidgetStyle<FSearchBoxStyle>(Key));
						Done.Add(T, &bNew); if (!bNew) { InkBox(T->TextBoxStyle); ++Swept; }
					}
					if (S->HasWidgetStyle<FInlineEditableTextBlockStyle>(Key))
					{
						FInlineEditableTextBlockStyle* T = const_cast<FInlineEditableTextBlockStyle*>(&S->GetWidgetStyle<FInlineEditableTextBlockStyle>(Key));
						Done.Add(T, &bNew); if (!bNew) { InkBox(T->EditableTextBoxStyle); InkWhiteText(T->TextStyle, Ink); ++Swept; }
					}
					if (S->HasWidgetStyle<FEditableTextStyle>(Key))
					{
						FEditableTextStyle* T = const_cast<FEditableTextStyle*>(&S->GetWidgetStyle<FEditableTextStyle>(Key));
						Done.Add(T, &bNew); if (!bNew) { T->ColorAndOpacity = Ink; ++Swept; }
					}
					if (S->HasWidgetStyle<FSpinBoxStyle>(Key))
					{
						FSpinBoxStyle* T = const_cast<FSpinBoxStyle*>(&S->GetWidgetStyle<FSpinBoxStyle>(Key));
						Done.Add(T, &bNew); if (!bNew) { T->ForegroundColor = Ink; ++Swept; }
					}
				}
			}
			PatchStyle<FTextBlockStyle>(Sets, TEXT("DetailsView.ConstantTextBlockStyle"), [&](FTextBlockStyle& T) { InkWhiteText(T, Ink); });
			Note(FString::Printf(TEXT("OK   text-entry styles swept: %d (white name text: %d)"), Swept, GWhiteText.Num()));

			// Search matches: classic find highlight (pale yellow, black text; amber, white text in Dark).
			const FSlateBrush Hilite = FSlateColorBrush(Hex(GDark ? TEXT("7A6418") : TEXT("FFE680")));
			const FSlateColor HiliteText(Hex(GDark ? TEXT("FFFFFF") : TEXT("000000")));
			int32 Lit = 0;
			const TArray<const ISlateStyle*> AllSets = AllStyleSets();
			for (const ISlateStyle* S : AllSets)
			{
				for (const FName& Key : S->GetStyleKeys())
				{
					auto Lite = [&](FTextBlockStyle& T) { T.HighlightShape = Hilite; T.HighlightColor = HiliteText; ++Lit; };
					// Text styles are also copied inside other styles (inline rename, fields, search, links).
					if (S->HasWidgetStyle<FTextBlockStyle>(Key)) { Lite(const_cast<FTextBlockStyle&>(S->GetWidgetStyle<FTextBlockStyle>(Key))); }
					if (S->HasWidgetStyle<FEditableTextBoxStyle>(Key)) { Lite(const_cast<FEditableTextBoxStyle&>(S->GetWidgetStyle<FEditableTextBoxStyle>(Key)).TextStyle); }
					if (S->HasWidgetStyle<FSearchBoxStyle>(Key)) { Lite(const_cast<FSearchBoxStyle&>(S->GetWidgetStyle<FSearchBoxStyle>(Key)).TextBoxStyle.TextStyle); }
					if (S->HasWidgetStyle<FHyperlinkStyle>(Key)) { Lite(const_cast<FHyperlinkStyle&>(S->GetWidgetStyle<FHyperlinkStyle>(Key)).TextStyle); }
					if (S->HasWidgetStyle<FInlineEditableTextBlockStyle>(Key))
					{
						FInlineEditableTextBlockStyle& I = const_cast<FInlineEditableTextBlockStyle&>(S->GetWidgetStyle<FInlineEditableTextBlockStyle>(Key));
						Lite(I.TextStyle); Lite(I.EditableTextBoxStyle.TextStyle);
					}
				}
			}
			// Stand-alone copies of the stock green highlight shape (any style set).
			int32 Shapes = 0;
			for (const ISlateStyle* S : AllSets)
			{
				for (const FName& Key : S->GetStyleKeys())
				{
					if (Key.ToString().Contains(TEXT("ErrorReporting"))) { continue; }
					const FSlateBrush* B = S->GetOptionalBrush(Key, nullptr, nullptr);
					if (B && B->GetResourceName().ToString().Contains(TEXT("TextBlockHighlightShape")))
					{
						*const_cast<FSlateBrush*>(B) = Hilite; ++Shapes;
					}
				}
			}
			Note(FString::Printf(TEXT("OK   search highlight set on %d text styles in %d style sets, %d shape brushes"), Lit, AllSets.Num(), Shapes));
			PatchBrush(Sets, TEXT("TextBlock.HighlightShape"), Hilite);
		}

		// ---- graphs (Blueprint, Material...): square nodes, solid title bars.
		// Unreal hard-codes pin labels in white, so nodes keep a dark body in both modes.
		// Light mode (option "Light Graphs") puts them on a light canvas; the grid and exec-wire
		// colours live in Editor Preferences and are swapped in memory only (see ApplyGraphColors).
		const bool bLightGraph = !GDark && GLightGraphs;
		const FFlat& G = bLightGraph ? GLightFlat : GDarkFlat;
		auto DarkRes = [](const TCHAR* Base) { return GResDir / FString::Printf(TEXT("%s_d.png"), Base); };
		PatchBrush(Sets, TEXT("Graph.Panel.SolidBackground"), FSlateColorBrush(Hex(G.Canvas)));
		SetColorAbs(Sets, TEXT("Graph.Panel.GridLineColor"), Hex(G.GridLine));
		SetColorAbs(Sets, TEXT("Graph.Panel.GridRuleColor"), Hex(G.GridRule));
		SetColorAbs(Sets, TEXT("Graph.Panel.GridCenterColor"), Hex(G.GridCenter));
		// The zoom read-out is drawn in hard-coded white: a dark drop shadow keeps it readable on light.
		PatchStyle<FTextBlockStyle>(Sets, TEXT("Graph.ZoomText"), [&](FTextBlockStyle& T)
		{
			T.SetShadowOffset(bLightGraph ? FVector2f(1.f, 1.f) : FVector2f::ZeroVector);
			T.SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
		});
		ApplyGraphColors(bLightGraph);
		const FSlateBrush NodeBody = FSlateBoxBrush(DarkRes(TEXT("node")), FVector2D(8.0, 8.0), FMargin(2.f / 64.f));
		const FSlateBrush Spill    = FSlateBoxBrush(DarkRes(TEXT("spill")), FVector2D(8.0, 8.0), FMargin(0.25f));
		const FSlateBrush Selected = FSlateBoxBrush(DarkRes(TEXT("sel")), FVector2D(8.0, 8.0), FMargin(2.f / 64.f));
		for (const TCHAR* K : { TEXT("Graph.Node.Body"), TEXT("Graph.VarNode.Body"), TEXT("Graph.CollapsedNode.Body"), TEXT("Graph.StateNode.Body"),
			TEXT("Graph.AnimStateNode.Body"), TEXT("Graph.AnimConduitNode.Body"), TEXT("Graph.AnimAliasNode.Body"), TEXT("Graph.AnimationResultNode.Body"),
			TEXT("Graph.SoundResultNode.Body") }) { PatchBrush(Sets, K, NodeBody); }
		for (const TCHAR* K : { TEXT("Graph.Node.ColorSpill"), TEXT("Graph.VarNode.ColorSpill"), TEXT("Graph.StateNode.ColorSpill"), TEXT("Graph.TransitionNode.ColorSpill"),
			TEXT("Graph.AnimStateNode.ColorSpill"), TEXT("Graph.AnimConduitNode.ColorSpill"), TEXT("Graph.AnimAliasNode.ColorSpill"), TEXT("Graph.AnimTransitionNode.ColorSpill") }) { PatchBrush(Sets, K, Spill); }
		for (const TCHAR* K : { TEXT("Graph.Node.TitleGloss"), TEXT("Graph.Node.TitleHighlight"), TEXT("Graph.VarNode.Gloss"),
			TEXT("Graph.Node.Shadow"), TEXT("Graph.VarNode.Shadow"), TEXT("Graph.AnimAliasNode.Shadow"), TEXT("Graph.AnimTransitionNode.Shadow") }) { PatchBrush(Sets, K, None); }
		for (const TCHAR* K : { TEXT("Graph.Node.ShadowSelected"), TEXT("Graph.VarNode.ShadowSelected"), TEXT("Graph.CompactNode.ShadowSelected") }) { PatchBrush(Sets, K, Selected); }
	}

	// ---------------------------------------------------------------- font
	// Tahoma replaces the default faces, and a Latin-range sub-font carrying the
	// same faces at GFontScale shrinks UI text without touching any style size.
	static void ApplyFont()
	{
		if (GFontDone) { return; }
		const FString Win = FPlatformMisc::GetEnvironmentVariable(TEXT("WINDIR"));
		const FString Regular = Win / TEXT("Fonts/tahoma.ttf");
		const FString Bold = Win / TEXT("Fonts/tahomabd.ttf");
		if (!IFileManager::Get().FileExists(*Regular)) { Note(FString::Printf(TEXT("MISS font file %s"), *Regular)); ++GMiss; return; }
		const FString BoldFile = IFileManager::Get().FileExists(*Bold) ? Bold : Regular;

		FSlateFontInfo Info = FCoreStyle::GetDefaultFontStyle("Regular", 10);
		if (!Info.CompositeFont.IsValid()) { Note(TEXT("MISS font: default composite")); ++GMiss; return; }
		FCompositeFont* CF = const_cast<FCompositeFont*>(Info.CompositeFont.Get());

		FCompositeSubFont Sub;
		Sub.ScalingFactor = GFontScale;
		Sub.CharacterRanges.Add(FInt32Range::Inclusive(0x0000, 0x024F));
		Sub.CharacterRanges.Add(FInt32Range::Inclusive(0x2000, 0x206F));
		Sub.CharacterRanges.Add(FInt32Range::Inclusive(0x20AC, 0x20AC));

		TArray<FString> Faces;
		for (FTypefaceEntry& Entry : CF->DefaultTypeface.Fonts)
		{
			const FString N = Entry.Name.ToString();
			Faces.Add(N);
			if (N == TEXT("Regular") || N == TEXT("Light"))
			{
				Entry.Font = FFontData(Regular, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			}
			else if (N == TEXT("Bold") || N == TEXT("Black"))
			{
				Entry.Font = FFontData(BoldFile, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			}
			Sub.Typeface.Fonts.Add(Entry);
		}
		CF->SubTypefaces.Add(Sub);
		CF->MakeDirty();
		GFontDone = true;

		if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer())
		{
			FSlateApplication::Get().GetRenderer()->FlushFontCache(TEXT("EasyTheme"));
		}
		Note(FString::Printf(TEXT("OK   font Tahoma x%.2f; faces: %s; sub-fonts now %d"), GFontScale, *FString::Join(Faces, TEXT(", ")), CF->SubTypefaces.Num()));
	}

	// ---------------------------------------------------------------- report
	static FString ReportDir()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("EasyTheme"));
	}

	static void WriteReport()
	{
		if (!GDev) { return; }
		const FString Dir = ReportDir();
		IFileManager::Get().MakeDirectory(*Dir, true);
		TArray<FString> Out;
		Out.Add(FString::Printf(TEXT("EasyTheme report - mode %s - misses: %d"), GDark ? TEXT("dark") : TEXT("light"), GMiss));
		Out.Append(GReport);
		FFileHelper::SaveStringToFile(FString::Join(Out, TEXT("\n")), *(Dir / TEXT("report.txt")));
	}

	static void Shot(const TCHAR* Name, const FString& InDir = FString())
	{
		if (!FSlateApplication::IsInitialized()) { return; }
		TSharedPtr<SWindow> W = FGlobalTabmanager::Get()->GetRootWindow();
		if (!W.IsValid()) { W = FSlateApplication::Get().GetActiveTopLevelWindow(); }
		if (!W.IsValid()) { Note(TEXT("MISS shot: no window")); return; }

		TArray<FColor> Px;
		FIntVector Size;
		if (!FSlateApplication::Get().TakeScreenshot(W.ToSharedRef(), Px, Size) || Px.Num() == 0)
		{
			Note(TEXT("MISS shot: capture failed"));
			return;
		}
		for (FColor& C : Px) { C.A = 255; }
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Px.GetData(), Px.Num()), Png);
		const FString Dir = InDir.IsEmpty() ? ReportDir() : InDir;
		IFileManager::Get().MakeDirectory(*Dir, true);
		FFileHelper::SaveArrayToFile(Png, *(Dir / Name));
		Note(FString::Printf(TEXT("OK   shot %s %dx%d"), Name, Size.X, Size.Y));
	}

	// Live layouts (exact tab ids for this engine version).
	static void DumpLayouts()
	{
		const FString Dir = ReportDir();
		IFileManager::Get().MakeDirectory(*Dir, true);
		FFileHelper::SaveStringToFile(FGlobalTabmanager::Get()->PersistLayout()->ToString(), *(Dir / TEXT("layout_global.json")));
		if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
		{
			FLevelEditorModule& LE = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
			TSharedPtr<FTabManager> TM = LE.GetLevelEditorTabManager();
			if (TM.IsValid())
			{
				FFileHelper::SaveStringToFile(TM->PersistLayout()->ToString(), *(Dir / TEXT("layout_level.json")));
				Note(TEXT("OK   layouts dumped"));
				return;
			}
		}
		Note(TEXT("MISS layout: level editor tab manager"));
	}

	// Writes the "Easy Theme - 3 Panes" entry of Window > Load Layout (user layouts, all projects),
	// by the same route as the editor's Save Layout As. The editor layout file on disk is shared by
	// every project, so its exact bytes are restored right after the copy.
	static void WriteUserLayout(bool bForce)
	{
		const FString UserDir = FPaths::EngineUserLayoutDir();
		const FString Target = UserDir / TEXT("EasyTheme_3Panes.ini");
		if (!bForce && IFileManager::Get().FileExists(*Target))
		{
			Note(FString::Printf(TEXT("OK   user layout present %s"), *FPaths::ConvertRelativePathToFull(Target)));
			return;
		}
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor"))) { Note(TEXT("MISS user layout: no level editor")); ++GMiss; return; }
		FLevelEditorModule& LE = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
		TSharedPtr<FTabManager> TM = LE.GetLevelEditorTabManager();
		if (!TM.IsValid()) { Note(TEXT("MISS user layout: no tab manager")); ++GMiss; return; }

		const TSharedRef<FTabManager::FLayout> Live = TM->PersistLayout();
		const TSharedRef<FTabManager::FLayout> Panes = FTabManager::NewLayout(Live->GetLayoutName())
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->SetExtensionId("TopLevelArea")
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.78f)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Horizontal)
					->SetSizeCoefficient(0.72f)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.15f)
						->SetHideTabWell(true)
						->SetExtensionId("VerticalToolbar")
					)
					->Split
					(
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Vertical)
						->SetSizeCoefficient(0.3f)
						->Split(FTabManager::NewStack()->SetSizeCoefficient(0.5f)->AddTab("PlacementBrowser", ETabState::ClosedTab))
						->Split(FTabManager::NewStack()->SetSizeCoefficient(0.5f)->SetExtensionId("BottomLeftPanel"))
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(1.0f)
						->SetHideTabWell(true)
						->AddTab("LevelEditorViewport", ETabState::OpenedTab)
					)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.28f)
					->AddTab("ContentBrowserTab1", ETabState::OpenedTab)
					->AddTab("OutputLog", ETabState::ClosedTab)
					->AddTab("Sequencer", ETabState::ClosedTab)
					->SetForegroundTab(FName("ContentBrowserTab1"))
				)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.22f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->AddTab("LevelEditorSceneOutliner", ETabState::OpenedTab)
					->AddTab("LevelEditorLayerBrowser", ETabState::ClosedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab("LevelEditorSelectionDetails", ETabState::OpenedTab)
					->AddTab("WorldSettingsTab", ETabState::ClosedTab)
					->SetForegroundTab(FName("LevelEditorSelectionDetails"))
				)
			)
		);

		FString OriginalBytes;
		const bool bHadOriginal = FFileHelper::LoadFileToString(OriginalBytes, *GEditorLayoutIni);
		IFileManager::Get().MakeDirectory(*UserDir, true);

		FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, Panes);
		FLayoutSaveRestore::SaveSectionToConfig(GEditorLayoutIni, TEXT("LayoutName"), FString(TEXT("Easy Theme - 3 Panes")));
		FLayoutSaveRestore::SaveSectionToConfig(GEditorLayoutIni, TEXT("LayoutDescription"), FString(TEXT("Viewport, Content Browser docked below, Outliner over Details on the right.")));
		GConfig->Flush(false, GEditorLayoutIni);
		const bool bCopied = FLayoutSaveRestore::DuplicateConfig(GEditorLayoutIni, Target);

		// Put the live layout back in memory (what the editor writes on exit) and the exact bytes on disk.
		FLayoutSaveRestore::SaveToConfig(GEditorLayoutIni, Live);
		FLayoutSaveRestore::SaveSectionToConfig(GEditorLayoutIni, TEXT("LayoutName"), FString());
		FLayoutSaveRestore::SaveSectionToConfig(GEditorLayoutIni, TEXT("LayoutDescription"), FString());
		if (bHadOriginal) { FFileHelper::SaveStringToFile(OriginalBytes, *GEditorLayoutIni); }

		const bool bValid = bCopied && FLayoutSaveRestore::IsValidConfig(Target);
		if (bCopied) { IFileManager::Get().Copy(*(ReportDir() / TEXT("user_layout.ini")), *Target); }
		Note(FString::Printf(TEXT("%s user layout %s (copied %d, valid %d, name %s, editor ini %s, restored %d)"), bValid ? TEXT("OK  ") : TEXT("MISS"),
			*FPaths::ConvertRelativePathToFull(Target), bCopied ? 1 : 0, bValid ? 1 : 0, *Live->GetLayoutName().ToString(),
			*FPaths::ConvertRelativePathToFull(GEditorLayoutIni), bHadOriginal ? 1 : 0));
		if (!bValid) { ++GMiss; }
	}

	static void ShotWindows(const FString& Prefix)
	{
		if (!FSlateApplication::IsInitialized()) { return; }
		int32 N = 0;
		for (const TSharedRef<SWindow>& W : FSlateApplication::Get().GetTopLevelWindows())
		{
			if (!W->IsVisible() || W->GetSizeInScreen().X < 100.f) { continue; }
			TArray<FColor> Px; FIntVector Size;
			if (!FSlateApplication::Get().TakeScreenshot(W, Px, Size) || Px.Num() == 0) { continue; }
			for (FColor& C : Px) { C.A = 255; }
			TArray64<uint8> Png;
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Px.GetData(), Px.Num()), Png);
			const FString Name = FString::Printf(TEXT("%s_%d.png"), *Prefix, N++);
			FFileHelper::SaveArrayToFile(Png, *(ReportDir() / Name));
			Note(FString::Printf(TEXT("OK   shot %s %dx%d '%s'"), *Name, Size.X, Size.Y, *W->GetTitle().ToString()));
		}
	}

	static float Lum(const FLinearColor& C) { return 0.2126f * C.R + 0.7152f * C.G + 0.0722f * C.B; }

	// Lists what is still unreadable: dark colour fills and light text / foregrounds (Light mode).
	static void AuditScan()
	{
		TArray<FString> Out;
		const FWidgetStyle WS;
		int32 Keys = 0;
		for (const ISlateStyle* S : StyleSets())
		{
			const FString SetName = S->GetStyleSetName().ToString();
			for (const FName& Key : S->GetStyleKeys())
			{
				++Keys;
				if (const FSlateBrush* B = S->GetOptionalBrush(Key, nullptr, nullptr))
				{
					const bool bFill = B->DrawAs == ESlateBrushDrawType::Box || B->DrawAs == ESlateBrushDrawType::Border
						|| B->DrawAs == ESlateBrushDrawType::RoundedBox || B->GetResourceName().IsNone();
					const FLinearColor C = B->TintColor.GetColor(WS);
					if (bFill && C.A > 0.5f && Lum(C) < 0.05f && B->DrawAs != ESlateBrushDrawType::NoDrawType)
					{
						Out.Add(FString::Printf(TEXT("DARK FILL  %s.%s #%s %s"), *SetName, *Key.ToString(), *C.ToFColor(true).ToHex().Left(6),
							B->TintColor.IsColorSpecified() ? TEXT("fixed") : TEXT("theme")));
					}
				}
				if (S->HasWidgetStyle<FTextBlockStyle>(Key))
				{
					const FSlateColor& TC = S->GetWidgetStyle<FTextBlockStyle>(Key).ColorAndOpacity;
					const FLinearColor C = TC.GetColor(WS);
					if (!IsInherited(TC) && C.A > 0.5f && Lum(C) > 0.45f)
					{
						Out.Add(FString::Printf(TEXT("LIGHT TEXT %s.%s #%s"), *SetName, *Key.ToString(), *C.ToFColor(true).ToHex().Left(6)));
					}
				}
				if (S->HasWidgetStyle<FButtonStyle>(Key))
				{
					const FButtonStyle& Bs = S->GetWidgetStyle<FButtonStyle>(Key);
					const FLinearColor C = Bs.NormalForeground.GetColor(WS);
					if (C.A > 0.5f && Lum(C) > 0.45f && Bs.NormalForeground.IsColorSpecified())
					{
						Out.Add(FString::Printf(TEXT("LIGHT FG   %s.%s #%s"), *SetName, *Key.ToString(), *C.ToFColor(true).ToHex().Left(6)));
					}
				}
			}
		}
		Out.Sort();
		Out.Insert(FString::Printf(TEXT("audit (%s): %d keys, %d flags"), GDark ? TEXT("dark") : TEXT("light"), Keys, Out.Num()), 0);
		FFileHelper::SaveStringToFile(FString::Join(Out, TEXT("\n")), *(ReportDir() / TEXT("audit.txt")));
		Note(Out[0]);

		// Slate console variables, for the responsiveness pass.
		TArray<FString> Vars;
		IConsoleManager::Get().ForEachConsoleObjectThatStartsWith(FConsoleObjectVisitor::CreateLambda([&Vars](const TCHAR* Name, IConsoleObject* Obj)
		{
			if (IConsoleVariable* V = Obj ? Obj->AsVariable() : nullptr) { Vars.Add(FString::Printf(TEXT("%s = %s"), Name, *V->GetString())); }
		}), TEXT("Slate."));
		Vars.Sort();
		FFileHelper::SaveStringToFile(FString::Join(Vars, TEXT("\n")), *(ReportDir() / TEXT("cvars.txt")));
	}

	static void AuditStep(int32 Step);
	static void AuditNext(int32 Step, float Delay)
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Step](float) { AuditStep(Step); return false; }), Delay);
	}

	static void AuditStep(int32 Step)
	{
		if (Step == 0)
		{
			if (GEditor)
			{
				UWorld* World = GEditor->GetEditorWorldContext().World();
				for (TActorIterator<AActor> It(World); It && World; ++It)
				{
					if (It->GetActorLabel().Contains(TEXT("DirectionalLight")))
					{
						GEditor->SelectNone(false, true);
						GEditor->SelectActor(*It, true, true);
						break;
					}
				}
			}
			if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
			{
				TSharedPtr<FTabManager> TM = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor")).GetLevelEditorTabManager();
				if (TM.IsValid())
				{
					TM->TryInvokeTab(FTabId(FName(TEXT("PlacementBrowser"))));
					TM->TryInvokeTab(FTabId(FName(TEXT("OutputLog"))));
					TM->TryInvokeTab(FTabId(FName(TEXT("LevelEditorSelectionDetails"))));
				}
			}
			AuditNext(1, 4.0f);
		}
		else if (Step == 1)
		{
			ShotWindows(TEXT("audit_a"));
			FGlobalTabmanager::Get()->TryInvokeTab(FTabId(FName(TEXT("ProjectSettings"))));
			AuditNext(2, 4.0f);
		}
		else
		{
			ShotWindows(TEXT("audit_b"));
			AuditScan();
			WriteReport();
		}
	}

	// The viewport asks for a crosshair cursor; Windows draws the regular arrow instead.
	static void ArrowCursor()
	{
#if PLATFORM_WINDOWS
		TSharedPtr<ICursor> C = FSlateApplication::Get().GetPlatformCursor();
		if (C.IsValid())
		{
			C->SetTypeShape(EMouseCursor::Crosshairs, (void*)::LoadCursor(nullptr, IDC_ARROW));
			Note(TEXT("OK   crosshair cursor -> arrow"));
		}
#endif
	}

	static void ApplyAll()
	{
		ApplyColors(true);
		ScanRemaps();
		ApplyRemaps();
		ApplyStyles();
		ApplyFont();
	}

	// Rebuilds every editor panel in place (what Window > Load Layout does), so widgets that copied
	// a colour when they were built pick up the current mode. Open asset editors are reopened.
	static void RefreshUI(bool bSaveCurrentLayout = true)
	{
		if (!GEditor) { return; }
		if (GEditor->PlayWorld) { Note(TEXT("refresh skipped: Play In Editor is running")); return; }
		UAssetEditorSubsystem* AES = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		const bool bAssets = AES && AES->GetAllEditedAssets().Num() > 0;
		TOptional<bool> Override;
		if (bAssets)
		{
			AES->SaveOpenAssetEditors(true);
			Override = AES->GetAutoRestoreAndDisableSavingOverride();
			AES->SetAutoRestoreAndDisableSavingOverride(true);
		}
		if (bSaveCurrentLayout) { FGlobalTabmanager::Get()->SaveAllVisualState(); }
		FGlobalTabmanager::Get()->SetCanSavePersistentLayouts(false);
		EditorReinit();
		FGlobalTabmanager::Get()->SetCanSavePersistentLayouts(true);
		if (bAssets)
		{
			AES->RestorePreviouslyOpenAssets();
			AES->SetAutoRestoreAndDisableSavingOverride(Override);
		}
		Note(TEXT("OK   UI refreshed"));
	}

	static void RefreshUISoon()
	{
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) { RefreshUI(); WriteReport(); return false; }), 0.2f);
	}

	static void SetMode(bool bDark, bool bAskRestart = false)
	{
		GDark = bDark;
		// The choice is shared by every project; test-project switches (dev) are not saved.
		if (!GDev)
		{
			GConfig->SetBool(GConfigSection, TEXT("Dark"), GDark, GEditorSettingsIni);
			GConfig->Flush(false, GEditorSettingsIni);
		}
		ApplyColors(true);
		ApplyRemaps();
		ApplyStyles();
		Note(FString::Printf(TEXT("mode switched to %s"), GDark ? TEXT("dark") : TEXT("light")));
		WriteReport();
		RefreshUISoon();
		if (bAskRestart)
		{
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
			{
				const EAppReturnType::Type R = FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(
					TEXT("Easy Theme: a few panels keep a copy of the old look until Unreal restarts.\n\nRestart Unreal now?")));
				// RestartEditor(true): the editor asks to save unsaved work before restarting.
				if (R == EAppReturnType::Yes) { FUnrealEdMisc::Get().RestartEditor(true); }
				return false;
			}), 1.0f);
		}
	}

	// Dev hook: console commands dropped one per line in Saved/EasyTheme/cmd.txt run on the next
	// poll (lets an unattended test drive the editor). The file only exists if someone writes it.
	static void PollCommands()
	{
		if (!GDev) { return; }
		const FString File = ReportDir() / TEXT("cmd.txt");
		if (!IFileManager::Get().FileExists(*File) || !GEngine) { return; }
		TArray<FString> Lines;
		FFileHelper::LoadFileToStringArray(Lines, *File);
		IFileManager::Get().Delete(*File, false, true, true);
		for (const FString& L : Lines)
		{
			const FString C = L.TrimStartAndEnd();
			if (C.IsEmpty()) { continue; }
			Note(FString::Printf(TEXT("cmd: %s"), *C));
			GEngine->Exec(nullptr, *C);
		}
		WriteReport();
	}

	static FString PanesLayoutFile()
	{
		return FPaths::EngineUserLayoutDir() / TEXT("EasyTheme_3Panes.ini");
	}

	// Same route as Window > Load Layout: copy the layout over the editor layout file, then rebuild.
	static void UsePanesLayout()
	{
		WriteUserLayout(true);   // always the current preset, never a file left by an older version
		const FString Target = PanesLayoutFile();
		if (!IFileManager::Get().FileExists(*Target)) { Note(TEXT("MISS 3-pane layout file")); return; }
		if (FLayoutSaveRestore::DuplicateConfig(Target, GEditorLayoutIni))
		{
			GConfig->Flush(true, GEditorLayoutIni);
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float) { RefreshUI(false); WriteReport(); return false; }), 0.2f);
		}
	}

	// Leaves nothing behind: the saved Dark/Light choice and the 3-pane layout file.
	static void RemoveSavedSettings()
	{
		ForgetGraphColors();
		GConfig->RemoveKey(GConfigSection, TEXT("Dark"), GEditorSettingsIni);
		GConfig->RemoveKey(GConfigSection, TEXT("ToolbarInTabBar"), GEditorSettingsIni);
		GConfig->Flush(false, GEditorSettingsIni);
		IFileManager::Get().Delete(*PanesLayoutFile(), false, true, true);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(
			TEXT("Easy Theme: saved settings removed.\n\nTo uninstall, disable Easy Theme in Edit > Plugins (or remove it from the Epic Games Launcher), then restart.")));
	}

	// Minimal URL encoding (UTF-8, RFC 3986 unreserved characters kept), no network module needed.
	static FString UrlEncode(const FString& In)
	{
		FString Out;
		const FTCHARToUTF8 Utf8(*In);
		for (int32 i = 0; i < Utf8.Length(); ++i)
		{
			const uint8 C = (uint8)Utf8.Get()[i];
			if ((C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z') || (C >= '0' && C <= '9') || C == '-' || C == '_' || C == '.' || C == '~')
			{
				Out.AppendChar((TCHAR)C);
			}
			else
			{
				Out += FString::Printf(TEXT("%%%02X"), C);
			}
		}
		return Out;
	}

	// ---------------------------------------------------------------- updates
	// Only on request (menu). GitHub installs carry Updater/EasyThemeUpdate.ps1: they can download
	// the new release, check its SHA-256 and install it when Unreal closes. Other installs (Fab,
	// source) just open the releases page: Fab updates go through the Epic launcher.
	static const TCHAR* GApiLatest = TEXT("https://api.github.com/repos/mapmodelers/ue5-theme-easy/releases/latest");
	struct FPendingUpdate { FString Version, Zip, Sha, Script, PluginDir, UpdateDir; bool bQuiet = false; };
	static TOptional<FPendingUpdate> GPending;
	static FString GFakeVersion;   // dev: pretend to run this version
	static FString GPluginDir;

	static FString UpdaterScript() { return GPluginDir / TEXT("Updater") / TEXT("EasyThemeUpdate.ps1"); }
	static FString UpdateDir() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("EasyTheme") / TEXT("Update")); }

	static bool IsNewer(const FString& A, const FString& B)   // A > B, dotted numbers
	{
		TArray<FString> PA, PB;
		A.ParseIntoArray(PA, TEXT(".")); B.ParseIntoArray(PB, TEXT("."));
		for (int32 i = 0; i < FMath::Max(PA.Num(), PB.Num()); ++i)
		{
			const int32 X = PA.IsValidIndex(i) ? FCString::Atoi(*PA[i]) : 0;
			const int32 Y = PB.IsValidIndex(i) ? FCString::Atoi(*PB[i]) : 0;
			if (X != Y) { return X > Y; }
		}
		return false;
	}

	static void Toast(const FString& Msg, bool bOk = true)
	{
		FNotificationInfo Info(FText::FromString(Msg));
		Info.ExpireDuration = 4.f;
		TSharedPtr<SNotificationItem> N = FSlateNotificationManager::Get().AddNotification(Info);
		if (N.IsValid()) { N->SetCompletionState(bOk ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail); }
	}

	static TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Get(const FString& Url)
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> R = FHttpModule::Get().CreateRequest();
		R->SetURL(Url);
		R->SetVerb(TEXT("GET"));
		R->SetHeader(TEXT("User-Agent"), TEXT("EasyTheme-Updater"));
		return R;
	}

	static void LaunchUpdater(const FPendingUpdate& P, int32 Pid)
	{
		// Run a copy from the update folder: the original is replaced during the update.
		const FString Script = P.UpdateDir / TEXT("EasyThemeUpdate.ps1");
		IFileManager::Get().Copy(*Script, *P.Script, true, true);
		const FString Args = FString::Printf(
			TEXT("-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"%s\" -Zip \"%s\" -ShaFile \"%s\" -PluginDir \"%s\" -EditorPid %d -Version \"%s\"%s"),
			*Script, *P.Zip, *P.Sha, *P.PluginDir, Pid, *P.Version, P.bQuiet ? TEXT(" -Quiet") : TEXT(""));
		FProcHandle H = FPlatformProcess::CreateProc(TEXT("powershell.exe"), *Args, true, true, true, nullptr, 0, nullptr, nullptr);
		Note(FString::Printf(TEXT("%s updater launched for %s -> %s"), H.IsValid() ? TEXT("OK  ") : TEXT("MISS"), *P.Version, *P.PluginDir));
	}

	static void Download(const FString& Url, const FString& File, TFunction<void(bool)> Done)
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> R = Get(Url);
		R->OnProcessRequestComplete().BindLambda([File, Done](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
		{
			const bool bGood = bOk && Resp.IsValid() && Resp->GetResponseCode() == 200 && Resp->GetContent().Num() > 0
				&& FFileHelper::SaveArrayToFile(Resp->GetContent(), *File);
			Done(bGood);
		});
		R->ProcessRequest();
	}

	// bTestDir (dev): install into a scratch folder right away instead of the plugin on exit.
	static void CheckForUpdates(bool bTestDir = false)
	{
		const FString Current = GFakeVersion.IsEmpty() ? GVersion : GFakeVersion;
		if (!IFileManager::Get().FileExists(*UpdaterScript()))
		{
			FPlatformProcess::LaunchURL(GReleasesUrl, nullptr, nullptr);
			return;
		}
		if (GPending.IsSet() && !bTestDir)
		{
			const FText Q = FText::FromString(FString::Printf(TEXT("Easy Theme %s will be installed when Unreal closes.\n\nCancel this update?"), *GPending->Version));
			if (FMessageDialog::Open(EAppMsgType::YesNo, Q, FText::FromString(TEXT("Easy Theme"))) == EAppReturnType::Yes) { GPending.Reset(); Toast(TEXT("Easy Theme update cancelled.")); }
			return;
		}
		Toast(TEXT("Easy Theme: checking for updates..."));
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> R = Get(GApiLatest);
		R->SetHeader(TEXT("Accept"), TEXT("application/vnd.github+json"));
		R->OnProcessRequestComplete().BindLambda([Current, bTestDir](FHttpRequestPtr, FHttpResponsePtr Resp, bool bOk)
		{
			TSharedPtr<FJsonObject> J;
			if (!bOk || !Resp.IsValid() || Resp->GetResponseCode() != 200
				|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Resp->GetContentAsString()), J) || !J.IsValid())
			{
				Note(FString::Printf(TEXT("MISS update check: http %d"), Resp.IsValid() ? Resp->GetResponseCode() : 0));
				if (bTestDir) { return; }
				if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(TEXT("Could not reach GitHub to check for updates.\n\nOpen the releases page in your browser?")), FText::FromString(TEXT("Easy Theme"))) == EAppReturnType::Yes)
				{
					FPlatformProcess::LaunchURL(GReleasesUrl, nullptr, nullptr);
				}
				return;
			}
			FString Tag = J->GetStringField(TEXT("tag_name"));
			Tag.RemoveFromStart(TEXT("v"));
			const FString Want = FString::Printf(TEXT("EasyTheme-%s-UE%d.%d-Win64.zip"), *Tag, FEngineVersion::Current().GetMajor(), FEngineVersion::Current().GetMinor());
			FString ZipUrl, ShaUrl;
			const TArray<TSharedPtr<FJsonValue>>* Assets = nullptr;
			if (J->TryGetArrayField(TEXT("assets"), Assets))
			{
				for (const TSharedPtr<FJsonValue>& V : *Assets)
				{
					const TSharedPtr<FJsonObject> A = V->AsObject();
					if (!A.IsValid()) { continue; }
					const FString Name = A->GetStringField(TEXT("name"));
					if (Name == Want) { ZipUrl = A->GetStringField(TEXT("browser_download_url")); }
					if (Name == Want + TEXT(".sha256")) { ShaUrl = A->GetStringField(TEXT("browser_download_url")); }
				}
			}
			Note(FString::Printf(TEXT("OK   update check: latest %s, current %s, zip %s, sha %s"), *Tag, *Current, ZipUrl.IsEmpty() ? TEXT("no") : TEXT("yes"), ShaUrl.IsEmpty() ? TEXT("no") : TEXT("yes")));
			if (!IsNewer(Tag, Current))
			{
				if (bTestDir) { return; }
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Easy Theme %s is the latest version."), *Current)), FText::FromString(TEXT("Easy Theme")));
				return;
			}
			if (ZipUrl.IsEmpty() || ShaUrl.IsEmpty())
			{
				if (bTestDir) { return; }
				const FText Q = FText::FromString(FString::Printf(TEXT("Easy Theme %s is available (you have %s), but there is no automatic install for Unreal %d.%d.\n\nOpen the releases page?"),
					*Tag, *Current, FEngineVersion::Current().GetMajor(), FEngineVersion::Current().GetMinor()));
				if (FMessageDialog::Open(EAppMsgType::YesNo, Q, FText::FromString(TEXT("Easy Theme"))) == EAppReturnType::Yes) { FPlatformProcess::LaunchURL(GReleasesUrl, nullptr, nullptr); }
				return;
			}
			const FText Q = FText::FromString(FString::Printf(
				TEXT("Easy Theme %s is available (you have %s).\n\nYes: download it now and install it when Unreal closes.\nNo: open the release page instead."), *Tag, *Current));
			const EAppReturnType::Type Ans = bTestDir ? EAppReturnType::Yes : FMessageDialog::Open(EAppMsgType::YesNoCancel, Q, FText::FromString(TEXT("Easy Theme update")));
			if (Ans == EAppReturnType::No) { FPlatformProcess::LaunchURL(GReleasesUrl, nullptr, nullptr); return; }
			if (Ans != EAppReturnType::Yes) { return; }

			FPendingUpdate P;
			P.Version = Tag;
			P.UpdateDir = UpdateDir();
			IFileManager::Get().MakeDirectory(*P.UpdateDir, true);
			P.Zip = P.UpdateDir / Want;
			P.Sha = P.Zip + TEXT(".sha256");
			P.Script = UpdaterScript();
			P.PluginDir = bTestDir ? P.UpdateDir / TEXT("TestPlugin") : GPluginDir;
			P.bQuiet = bTestDir;
			if (bTestDir) { IFileManager::Get().MakeDirectory(*P.PluginDir, true); }
			Toast(FString::Printf(TEXT("Easy Theme: downloading %s..."), *Tag));
			Download(ZipUrl, P.Zip, [P, ShaUrl, bTestDir](bool bZip)
			{
				if (!bZip) { Toast(TEXT("Easy Theme: download failed."), false); return; }
				Download(ShaUrl, P.Sha, [P, bTestDir](bool bSha)
				{
					if (!bSha) { Toast(TEXT("Easy Theme: download failed."), false); return; }
					if (bTestDir) { LaunchUpdater(P, 0); return; }
					GPending = P;
					Toast(FString::Printf(TEXT("Easy Theme %s will be installed when Unreal closes."), *P.Version));
				});
			});
		});
		R->ProcessRequest();
	}

	// Called on exit: hand the pending update to the updater, which waits for this editor to close.
	static void RunPendingUpdate()
	{
		if (!GPending.IsSet()) { return; }
		LaunchUpdater(GPending.GetValue(), (int32)FPlatformProcess::GetCurrentProcessId());
		GPending.Reset();
	}

	// The editor log, cut down to what helps (Easy Theme lines, warnings, errors, the last 150 lines)
	// and scrubbed: user name, computer name, home, project and engine folders are replaced.
	static FString ScrubbedLog()
	{
		if (GLog) { GLog->Flush(); }
		TArray<uint8> Bytes;
		if (TUniquePtr<FArchive> Ar{ IFileManager::Get().CreateFileReader(*FString(FPlatformOutputDevices::GetAbsoluteLogFilename()), FILEREAD_AllowWrite) })
		{
			Bytes.SetNumUninitialized((int32)FMath::Min<int64>(Ar->TotalSize(), 64 * 1024 * 1024));
			Ar->Serialize(Bytes.GetData(), Bytes.Num());
		}
		FString All;
		if (Bytes.Num()) { FFileHelper::BufferToString(All, Bytes.GetData(), Bytes.Num()); }
		TArray<FString> Lines;
		All.ParseIntoArrayLines(Lines, false);

		TArray<TPair<FString, FString>> Subs;
		auto AddPath = [&Subs](FString Path, const TCHAR* Tag)
		{
			if (Path.Len() < 4) { return; }
			Path = FPaths::ConvertRelativePathToFull(Path);
			FPaths::NormalizeDirectoryName(Path);
			Subs.Add(TPair<FString, FString>(Path, Tag));
			Subs.Add(TPair<FString, FString>(Path.Replace(TEXT("/"), TEXT("\\")), Tag));
		};
		{
			FString Rel = FPaths::ProjectDir();   // as Unreal prints it, relative to the engine binaries
			FPaths::NormalizeDirectoryName(Rel);
			if (Rel.StartsWith(TEXT(".."))) { Subs.Add(TPair<FString, FString>(Rel, TEXT("<Project>"))); }
		}
		AddPath(FPaths::ProjectDir(), TEXT("<Project>"));
		AddPath(FPaths::EngineDir(), TEXT("<Engine>"));
		AddPath(FPlatformMisc::GetEnvironmentVariable(TEXT("USERPROFILE")), TEXT("<Home>"));
		const FString User = FPlatformProcess::UserName(false);
		const FString Pc = FPlatformProcess::ComputerName();
		if (User.Len() >= 3) { Subs.Add(TPair<FString, FString>(User, TEXT("<user>"))); }
		if (Pc.Len() >= 3) { Subs.Add(TPair<FString, FString>(Pc, TEXT("<pc>"))); }
		static const TCHAR* Drop[] = { TEXT("Login ID"), TEXT("Epic Account"), TEXT("LogInit: User:"), TEXT("LogInit: Computer:"), TEXT("Machine ID"), TEXT("SessionId"), TEXT("Session ID") };

		const FRegexPattern DrivePath(TEXT("(?<![A-Za-z0-9])[A-Za-z]:[\\\\/][^\\s\"'<>|*?]*[\\\\/]"));
		const FRegexPattern ClimbPath(TEXT("(?:\\.\\.[\\\\/]){4,}[^\\s\"'<>|*?]*[\\\\/]"));
		TArray<FString> Keep;
		const int32 TailFrom = FMath::Max(0, Lines.Num() - 150);
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			const FString& L = Lines[i];
			const bool bUseful = i >= TailFrom || i < 25 || L.Contains(TEXT("EasyTheme")) || L.Contains(TEXT("Warning:")) || L.Contains(TEXT("Error:")) || L.Contains(TEXT("Ensure"));
			if (!bUseful) { continue; }
			bool bDrop = false;
			for (const TCHAR* D : Drop) { if (L.Contains(D)) { bDrop = true; break; } }
			if (bDrop) { continue; }
			FString Clean = L;
			for (const TPair<FString, FString>& S : Subs) { Clean.ReplaceInline(*S.Key, *S.Value, ESearchCase::IgnoreCase); }
			// Any other folder (drive paths, relative paths climbing out of the engine): keep the file name only.
			for (const FRegexPattern* Pat : { &DrivePath, &ClimbPath })
			{
				FRegexMatcher M(*Pat, Clean);
				FString Out;
				int32 Last = 0;
				while (M.FindNext()) { Out += Clean.Mid(Last, M.GetMatchBeginning() - Last) + TEXT("<path>/"); Last = M.GetMatchEnding(); }
				if (Last > 0) { Clean = Out + Clean.Mid(Last); }
			}
			Keep.Add(MoveTemp(Clean));
		}
		if (Keep.Num() > 1500) { Keep.RemoveAt(25, Keep.Num() - 1500); }
		return FString::Join(Keep, TEXT("\n"));
	}

	// Private by default: files are written locally for the user to review and attach by hand;
	// nothing is uploaded. bOpen = false is for tests (no browser, no Explorer).
	static void ReportIssue(bool bOpen = true)
	{
		const FString Dir = ReportDir() / TEXT("Issue");
		IFileManager::Get().MakeDirectory(*Dir, true);
		Shot(TEXT("screenshot.png"), Dir);
		FFileHelper::SaveStringToFile(ScrubbedLog(), *(Dir / TEXT("log.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		TSharedPtr<SWindow> W = FGlobalTabmanager::Get()->GetRootWindow();
		const FVector2D Px = W.IsValid() ? W->GetSizeInScreen() : FVector2D::ZeroVector;
		const FString Info = FString::Printf(
			TEXT("Easy Theme %s\nUnreal Engine %s\nOS %s\nMode %s, Light Graphs %s, Toolbar in Tab Bar %s\nWindow %.0fx%.0f, DPI scale %.2f\n"),
			*GVersion, *FEngineVersion::Current().ToString(EVersionComponent::Patch), *FPlatformMisc::GetOSVersion(),
			GDark ? TEXT("Dark") : TEXT("Light"), GLightGraphs ? TEXT("on") : TEXT("off"), GToolbarInTitle ? TEXT("on") : TEXT("off"),
			Px.X, Px.Y, FSlateApplication::Get().GetApplicationScale());
		FFileHelper::SaveStringToFile(Info, *(Dir / TEXT("info.txt")));
		Note(FString::Printf(TEXT("OK   bug report in %s"), *Dir));
		if (!bOpen) { return; }
		const FString Body = Info + TEXT("\nWhat happened:\n\nWhere (panel / window):\n\n")
			+ TEXT("Attach: drag screenshot.png and log.txt from the folder Easy Theme just opened. ")
			+ TEXT("Your user name and folders are removed from the log; have a look before posting.\n");
		const FString Url = FString(TEXT("https://github.com/mapmodelers/ue5-theme-easy/issues/new?title="))
			+ UrlEncode(TEXT("[Bug] ")) + TEXT("&body=") + UrlEncode(Body);
		FPlatformProcess::ExploreFolder(*(Dir / TEXT("log.txt")));
		FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
	}

	static void SetToolbarInTitle(bool bOn);

	static void FillMenu(UToolMenu* Sub)
	{
		FToolMenuSection& Look = Sub->AddSection("EasyThemeLook", FText::FromString(TEXT("Look")));
		Look.AddMenuEntry("EasyThemeDark",
			FText::FromString(TEXT("Dark")),
			FText::FromString(TEXT("Dark look (default).")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { if (!GDark) { SetMode(true, true); } }), FCanExecuteAction(),
				FIsActionChecked::CreateLambda([]() { return GDark; })),
			EUserInterfaceActionType::RadioButton);
		Look.AddMenuEntry("EasyThemeLight",
			FText::FromString(TEXT("Light")),
			FText::FromString(TEXT("Light grey look.")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { if (GDark) { SetMode(false, true); } }), FCanExecuteAction(),
				FIsActionChecked::CreateLambda([]() { return !GDark; })),
			EUserInterfaceActionType::RadioButton);
		Look.AddMenuEntry("EasyThemeLightGraphs",
			FText::FromString(TEXT("Light Graphs")),
			FText::FromString(TEXT("Light mode: Blueprint and Material graphs on a light canvas. Your grid colours in Editor Preferences are kept and put back when this is off.")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
				{
					GLightGraphs = !GLightGraphs;
					GConfig->SetBool(GConfigSection, TEXT("LightGraphs"), GLightGraphs, GEditorSettingsIni);
					GConfig->Flush(false, GEditorSettingsIni);
					ApplyStyles();
					WriteReport();
				}),
				FCanExecuteAction::CreateLambda([]() { return !GDark; }),
				FIsActionChecked::CreateLambda([]() { return GLightGraphs; })),
			EUserInterfaceActionType::ToggleButton);
		Look.AddMenuEntry("EasyThemeRefresh",
			FText::FromString(TEXT("Refresh UI")),
			FText::FromString(TEXT("Rebuild every panel so all of it follows the current look.")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { RefreshUISoon(); })));

		FToolMenuSection& Layout = Sub->AddSection("EasyThemeLayout", FText::FromString(TEXT("Layout")));
		Layout.AddMenuEntry("EasyThemePanes",
			FText::FromString(TEXT("Use 3-Pane Layout")),
			FText::FromString(TEXT("Viewport, Content Browser docked below, Outliner over Details on the right.")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { UsePanesLayout(); })));
		Layout.AddMenuEntry("EasyThemeTitleToolbar",
			FText::FromString(TEXT("Toolbar in Tab Bar")),
			FText::FromString(TEXT("One row for tabs and the main toolbar (Save, Mode, Add, Play...), with a + button for recent levels and assets. Frees a row for the viewport.")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { SetToolbarInTitle(!GToolbarInTitle); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([]() { return GToolbarInTitle; })),
			EUserInterfaceActionType::ToggleButton);

		FToolMenuSection& Help = Sub->AddSection("EasyThemeHelp", FText::FromString(TEXT("Help")));
		Help.AddMenuEntry("EasyThemeReport",
			FText::FromString(TEXT("Report a Bug...")),
			FText::FromString(TEXT("Saves a screenshot, a cleaned-up log (no user name or folders) and system info in Saved/EasyTheme/Issue, then opens that folder and a pre-filled GitHub issue. Nothing is uploaded unless you attach it.")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { ReportIssue(); })));
		const bool bCanInstall = IFileManager::Get().FileExists(*UpdaterScript());
		Help.AddMenuEntry("EasyThemeUpdate",
			FText::FromString(FString::Printf(TEXT("Check for Updates (you have %s)..."), *GVersion)),
			FText::FromString(bCanInstall
				? TEXT("Asks GitHub for the latest release. If there is one, you can download it and have it installed when Unreal closes. This is the only time Easy Theme connects to the internet.")
				: TEXT("Opens the latest release on GitHub in your browser.")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { CheckForUpdates(); })));

		FToolMenuSection& Clean = Sub->AddSection("EasyThemeClean", FText::FromString(TEXT("Uninstall")));
		Clean.AddMenuEntry("EasyThemeRemove",
			FText::FromString(TEXT("Remove Saved Settings")),
			FText::FromString(TEXT("Delete what Easy Theme saved (look choice, 3-pane layout) and put your graph grid colours back. Then disable the plugin to uninstall.")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]() { RemoveSavedSettings(); })));
	}

	// ---------------------------------------------------------------- toolbar in the tab bar
	// One row for tabs and toolbars. For every editor tab in the main window's tab row (level
	// editor and docked asset editors), that editor's own toolbar widget is moved from its row
	// into the tab row, right after the tabs, with a "+" button for recent levels and assets.
	// Every tab gets the same layout and width, so switching tabs keeps everything at the same
	// pixels (only the toolbar buttons change). Tabs get an adaptive width. Off: back to stock.
	// Checked every half second: editors rebuild their toolbars and content.
	struct FTabBarState
	{
		TWeakPtr<SDockTab> Tab;
		TSharedPtr<SWidget> Toolbar;     // the moved toolbar (null if this editor has none)
		TWeakPtr<SVerticalBox> Box;      // its original parent
		TWeakPtr<SWidget> Content;       // our content right of the tabs
		TSharedPtr<SWidget> OrigRight;   // what the tab had there before
	};
	static TArray<FTabBarState> GTabBars;

	static TSharedPtr<SWidget> FindByType(const TSharedRef<SWidget>& Root, const FName Type, int32 MaxDepth,
		TFunctionRef<bool(const TSharedRef<SWidget>&)> Accept)
	{
		TArray<TPair<TSharedRef<SWidget>, int32>> Q;
		Q.Add(TPair<TSharedRef<SWidget>, int32>(Root, 0));
		for (int32 i = 0; i < Q.Num(); ++i)
		{
			const TSharedRef<SWidget> W = Q[i].Key;
			if (W->GetType() == Type && Accept(W)) { return W; }
			FChildren* C = W->GetChildren();
			if (!C || Q[i].Value >= MaxDepth) { continue; }
			for (int32 k = 0; k < C->Num(); ++k) { Q.Add(TPair<TSharedRef<SWidget>, int32>(C->GetChildAt(k), Q[i].Value + 1)); }
		}
		return nullptr;
	}

	// Editor content: SVerticalBox [ toolbar, (level editor: secondary mode toolbar), docking area ]
	static TSharedPtr<SVerticalBox> FindToolbarBox(const TSharedRef<SWidget>& TabContent)
	{
		TSharedPtr<SWidget> W = FindByType(TabContent, FName(TEXT("SVerticalBox")), 7,
			[](const TSharedRef<SWidget>& V)
			{
				FChildren* C = V->GetChildren();
				return C && (C->Num() == 2 || C->Num() == 3) && C->GetChildAt(C->Num() - 1)->GetType() == FName(TEXT("SDockingArea"));
			});
		return W.IsValid() ? StaticCastSharedPtr<SVerticalBox>(W) : nullptr;
	}

	static void SetBoxPadding(const TSharedPtr<SVerticalBox>& Box, float Bottom)
	{
		if (!Box.IsValid()) { return; }
		const int32 N = Box->GetChildren()->Num();
		for (int32 i = 0; i < N - 1; ++i) { Box->GetSlot(i).SetPadding(FMargin(0.f, 0.f, 0.f, Bottom)); }
	}

	// Tab width depends only on the row width and the number of tabs (never on the editor in
	// front, so switching tabs moves nothing): about 7.5% of the row, but all tabs together take
	// at most half of it so the toolbar keeps room; 72 to 180 px, labels may clip.
	// Unreal gives every tab the same slot, so icon-only tabs (Home) leave a gap: TabBarShift()
	// moves our content left by that much so "+" sits right after the last tab.
	static float TabW(float RowW, int32 N)
	{
		return FMath::Clamp(FMath::Min(RowW * 0.075f, RowW * 0.5f / FMath::Max(1, N)), 72.f, 180.f);
	}

	static FOptionalSize TabBarWidth(TWeakPtr<SWidget> Weak)
	{
		TSharedPtr<SWidget> W = Weak.Pin();
		TSharedPtr<SWidget> Row = W.IsValid() ? W->GetParentWidget() : nullptr;
		FChildren* C = Row.IsValid() ? Row->GetChildren() : nullptr;
		if (!C || C->Num() < 3) { return FOptionalSize(); }
		const float RowW = Row->GetTickSpaceGeometry().GetLocalSize().X;
		int32 N = 1;
		if (TSharedPtr<SWidget> Well = FindByType(C->GetChildAt(1), FName(TEXT("SDockingTabWell")), 4, [](const TSharedRef<SWidget>&) { return true; }))
		{
			N = FMath::Max(1, Well->GetChildren()->Num());
		}
		const float Tabs = N * TabW(RowW, N);
		// Correct from the last layout: the tab well should be exactly Tabs wide.
		const float WellW = C->GetChildAt(1)->GetTickSpaceGeometry().GetLocalSize().X;
		const float OursW = W->GetTickSpaceGeometry().GetLocalSize().X;
		if (OursW <= 0.f || WellW <= 0.f)
		{
			return FMath::Max(0.f, RowW - C->GetChildAt(0)->GetTickSpaceGeometry().GetLocalSize().X - Tabs - 12.f);
		}
		// Never squeeze "+" and the toolbar's overflow button out (many tabs on a small screen):
		// past that point the tabs shrink instead, Unreal's own way.
		return FMath::Clamp(OursW + (WellW - Tabs), FMath::Min(120.f, RowW * 0.5f), RowW);
	}

	static FMargin TabBarShift(TWeakPtr<SWidget> Weak)
	{
		TSharedPtr<SWidget> W = Weak.Pin();
		TSharedPtr<SWidget> Row = W.IsValid() ? W->GetParentWidget() : nullptr;
		FChildren* C = Row.IsValid() ? Row->GetChildren() : nullptr;
		if (!C || C->Num() < 3) { return FMargin(0.f); }
		TSharedPtr<SWidget> Well = FindByType(C->GetChildAt(1), FName(TEXT("SDockingTabWell")), 4, [](const TSharedRef<SWidget>&) { return true; });
		if (!Well.IsValid()) { return FMargin(0.f); }
		FChildren* T = Well->GetChildren();
		const int32 N = FMath::Max(1, T->Num());
		const float Slot = Well->GetTickSpaceGeometry().GetLocalSize().X / N;
		float Gap = 0.f;
		for (int32 i = 0; i < T->Num(); ++i)
		{
			TSharedRef<SDockTab> Tab = StaticCastSharedRef<SDockTab>(T->GetChildAt(i));
			if (Tab->IsTabNameHidden()) { Gap += FMath::Max(0.f, Slot - (Tab->CanCloseTab() ? 53.f : 32.f)); }
		}
		return FMargin(-Gap, 0.f, 0.f, 0.f);
	}

	static void OpenRecentLevel(const FString& Item)
	{
		FString File = Item;
		if (FPackageName::IsValidLongPackageName(Item)) { File = FPackageName::LongPackageNameToFilename(Item, FPackageName::GetMapPackageExtension()); }
		if (!FEditorFileUtils::SaveDirtyPackages(true, true, false)) { return; }   // user cancelled
		FEditorFileUtils::LoadMap(File, false, true);
	}

	// "+" menu: recent levels, then recent assets (Unreal's own lists).
	static void FillRecentMenu(UToolMenu* Menu)
	{
		FToolMenuSection& Levels = Menu->AddSection("EasyThemeRecentLevels", FText::FromString(TEXT("Recent Levels")));
		int32 Added = 0;
		if (FMainMRUFavoritesList* MRU = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame")).GetMRUFavoritesList())
		{
			for (int32 i = 0; i < MRU->GetNumItems() && Added < 12; ++i)
			{
				const FString Item = MRU->GetMRUItem(i);
				Levels.AddMenuEntry(NAME_None, FText::FromString(FPaths::GetBaseFilename(Item)),
					FText::FromString(Item + TEXT("\nOpens in place of the current level (Unreal edits one level at a time).")),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Level"),
					FUIAction(FExecuteAction::CreateLambda([Item]() { OpenRecentLevel(Item); })));
				++Added;
			}
		}
		if (Added == 0)
		{
			Levels.AddMenuEntry(NAME_None, FText::FromString(TEXT("No recent levels")), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
		}
		FToolMenuSection& Assets = Menu->AddSection("EasyThemeRecentAssets", FText::FromString(TEXT("Recent Assets")));
		UAssetEditorSubsystem* AES = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
		FMainMRUFavoritesList* Recent = AES ? AES->GetRecentlyOpenedAssets() : nullptr;
		int32 AssetsAdded = 0;
		for (int32 i = 0; Recent && i < Recent->GetNumItems() && AssetsAdded < 15; ++i)
		{
			FSoftObjectPath Path(Recent->GetMRUItem(i));
			if (Path.GetAssetFName().IsNone())
			{
				const FName Pkg = Path.GetLongPackageFName();
				Path = FSoftObjectPath::ConstructFromAssetPath(FTopLevelAssetPath(Pkg, FPackageName::GetShortFName(Pkg)));
			}
			Assets.AddMenuEntry(NAME_None, FText::FromString(Path.GetAssetName()), FText::FromString(Path.ToString() + TEXT("\nOpens in its own editor tab.")), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Path]()
				{
					UAssetEditorSubsystem* Sub = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
					if (UObject* Obj = Path.TryLoad()) { if (Sub) { Sub->OpenEditorForAsset(Obj); } }
				})));
			++AssetsAdded;
		}
		if (AssetsAdded == 0)
		{
			Assets.AddMenuEntry(NAME_None, FText::FromString(TEXT("No recent assets")), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction(), FCanExecuteAction::CreateLambda([]() { return false; })));
		}
	}

	static TSharedRef<SWidget> MakeTabBarContent(const FTabBarState& S)
	{
		TSharedRef<SWidget> Plus = SNew(SComboButton)
			.ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>("SimpleComboButton"))
			.HasDownArrow(false)
			.ContentPadding(FMargin(4.f, 2.f))
			.ToolTipText(FText::FromString(TEXT("Open a recent level or asset")))
			.OnGetMenuContent_Lambda([]() { return UToolMenus::Get()->GenerateWidget("EasyTheme.Recent", FToolMenuContext()); })
			.ButtonContent()
			[
				SNew(SImage).Image(FAppStyle::Get().GetBrush("Icons.Plus")).ColorAndOpacity(FSlateColor::UseForeground())
			];
		TSharedRef<SBox> Box = SNew(SBox);
		TWeakPtr<SWidget> Weak = Box;
		TWeakPtr<SWidget> WeakPlus = Plus;
		const bool bRight = S.OrigRight.IsValid() && S.OrigRight != SNullWidget::NullWidget;
		TWeakPtr<SWidget> WeakRight = bRight ? TWeakPtr<SWidget>(S.OrigRight) : TWeakPtr<SWidget>();
		// When the row is full the toolbar gets what is left and shows Unreal's own ">>" overflow
		// button for the rest, instead of running off the window.
		TSharedRef<SBox> Fit = SNew(SBox)
			.MaxDesiredWidth(TAttribute<FOptionalSize>::CreateLambda([Weak, WeakPlus, WeakRight]()
			{
				TSharedPtr<SWidget> B = Weak.Pin();
				TSharedPtr<SWidget> P = WeakPlus.Pin();
				if (!B.IsValid() || !P.IsValid()) { return FOptionalSize(); }
				const float BoxW = B->GetTickSpaceGeometry().GetLocalSize().X;
				if (BoxW <= 0.f) { return FOptionalSize(); }
				TSharedPtr<SWidget> R = WeakRight.Pin();
				const float Used = TabBarShift(Weak).Left + P->GetDesiredSize().X + 8.f + (R.IsValid() ? R->GetDesiredSize().X : 0.f) + 4.f;
				return FOptionalSize(FMath::Max(32.f, BoxW - Used));
			}))
			[ S.Toolbar.IsValid() ? S.Toolbar.ToSharedRef() : SNullWidget::NullWidget ];
		TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				.Padding(TAttribute<FMargin>::CreateLambda([Weak]() { return TabBarShift(Weak); }))[ Plus ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(FMargin(8.f, 0.f, 0.f, 0.f))[ Fit ]
			+ SHorizontalBox::Slot().FillWidth(1.f)[ SNullWidget::NullWidget ];
		if (bRight)
		{
			Row->AddSlot().AutoWidth().VAlign(VAlign_Center)[ S.OrigRight.ToSharedRef() ];
		}
		Box->SetContent(Row);
		const TAttribute<FOptionalSize> Width = TAttribute<FOptionalSize>::CreateLambda([Weak]() { return TabBarWidth(Weak); });
		Box->SetMinDesiredWidth(Width);
		Box->SetMaxDesiredWidth(Width);
		// One fixed row height for every editor (their toolbars differ by a few pixels), so the
		// content below starts at the same pixel whatever tab is in front.
		Box->SetHeightOverride(28.f);
		return Box;
	}

	static FTabBarState* FindState(const TSharedRef<SDockTab>& Tab)
	{
		for (FTabBarState& S : GTabBars) { if (S.Tab.Pin() == Tab) { return &S; } }
		return nullptr;
	}

	static void EnsureToolbarInTitle()
	{
		if (!GToolbarInTitle) { return; }
		GTabBars.RemoveAll([](const FTabBarState& S) { return !S.Tab.IsValid(); });
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor"))) { return; }
		TSharedPtr<ILevelEditor> LE = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor")).GetFirstLevelEditor();
		TSharedPtr<FTabManager> TM = LE.IsValid() ? LE->GetTabManager() : nullptr;
		TSharedPtr<SDockTab> LevelTab = TM.IsValid() ? TM->GetOwnerTab() : nullptr;
		// The main window's tab row: the tab well holding the level editor tab.
		TSharedPtr<SWidget> Well = LevelTab.IsValid() ? LevelTab->GetParentWidget() : nullptr;
		if (!Well.IsValid() || Well->GetType() != FName(TEXT("SDockingTabWell")))
		{
			static bool bWarned = false;
			if (GDev && !bWarned) { bWarned = true; Note(FString::Printf(TEXT("MISS tab row: level tab parent is %s"), Well.IsValid() ? *Well->GetTypeAsString() : TEXT("none"))); }
			return;
		}
		FChildren* Tabs = Well->GetChildren();
		for (int32 i = 0; i < Tabs->Num(); ++i)
		{
			TSharedRef<SDockTab> Tab = StaticCastSharedRef<SDockTab>(Tabs->GetChildAt(i));
			if (Tab->IsTabNameHidden()) { continue; }   // Home
			FTabBarState* S = FindState(Tab);
			if (!S) { FTabBarState New; New.Tab = Tab; S = &GTabBars.Add_GetRef(New); }

			// 1. toolbar out of its row (again, if the editor rebuilt it)
			bool bChanged = false;
			// Cached box while it lives (a rebuilt editor gets a new one); search only otherwise.
			TSharedPtr<SVerticalBox> Box = S->Box.Pin();
			if (!Box.IsValid()) { Box = FindToolbarBox(Tab->GetContent()); }
			if (Box.IsValid())
			{
				TSharedRef<SWidget> First = Box->GetChildren()->GetChildAt(0);
				if (First != SNullWidget::NullWidget || S->Box.Pin() != Box)
				{
					if (First != SNullWidget::NullWidget) { S->Toolbar = First; }
					S->Box = Box;
					Box->GetSlot(0).AttachWidget(SNullWidget::NullWidget);
					SetBoxPadding(Box, 0.f);
					bChanged = true;
					if (GDev) { Note(FString::Printf(TEXT("OK   toolbar to tab row: %s (%s)"), *Tab->GetTabLabel().ToString(), *First->GetTypeAsString())); }
				}
			}

			// 2. our content right of the tabs
			TSharedPtr<SWidget> Ours = S->Content.Pin();
			if (!bChanged && Ours.IsValid() && Tab->GetRightContent() == Ours)
			{
				static TSet<FString> Reported;
				const FString Key = Tab->GetTabLabel().ToString();
				if (GDev && !Reported.Contains(Key) && Ours->GetTickSpaceGeometry().GetLocalSize().Y > 0.f)
				{
					Reported.Add(Key);
					FString D = FString::Printf(TEXT("     tabrow %s: row %.0f tab %.0f toolbar %.0f |"), *Key,
						Ours->GetTickSpaceGeometry().GetLocalSize().Y, Tab->GetDesiredSize().Y, S->Toolbar.IsValid() ? S->Toolbar->GetDesiredSize().Y : 0.f);
					// toolbar children heights (blocks)
					if (S->Toolbar.IsValid())
					{
						TSharedPtr<SWidget> MB = FindByType(S->Toolbar.ToSharedRef(), FName(TEXT("SMultiBoxWidget")), 6, [](const TSharedRef<SWidget>&) { return true; });
						if (MB.IsValid()) { D += FString::Printf(TEXT(" multibox %.0f"), MB->GetDesiredSize().Y); }
					}
					Note(D);
				}
				continue;
			}
			if (!Ours.IsValid() || Tab->GetRightContent() != Ours) { S->OrigRight = Tab->GetRightContent(); }
			TSharedRef<SWidget> Content = MakeTabBarContent(*S);
			S->Content = Content;
			Tab->SetRightContent(Content);
		}
	}

	static void RestoreToolbarRow()
	{
		for (FTabBarState& S : GTabBars)
		{
			if (TSharedPtr<SDockTab> Tab = S.Tab.Pin())
			{
				Tab->SetRightContent(S.OrigRight.IsValid() ? S.OrigRight.ToSharedRef() : SNullWidget::NullWidget);
			}
			TSharedPtr<SVerticalBox> Box = S.Box.Pin();
			if (Box.IsValid())
			{
				if (S.Toolbar.IsValid() && Box->GetChildren()->GetChildAt(0) == SNullWidget::NullWidget)
				{
					Box->GetSlot(0).AttachWidget(S.Toolbar.ToSharedRef());
				}
				SetBoxPadding(Box, 2.f);
			}
		}
		GTabBars.Reset();
	}

	static void SetToolbarInTitle(bool bOn)
	{
		GToolbarInTitle = bOn;
		GConfig->SetBool(GConfigSection, TEXT("ToolbarInTabBar"), bOn, GEditorSettingsIni);
		GConfig->Flush(false, GEditorSettingsIni);
		if (bOn) { EnsureToolbarInTitle(); } else { RestoreToolbarRow(); }
	}

	static void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(FName(TEXT("EasyTheme")));
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		if (!Menu) { Note(TEXT("MISS menu: Window")); ++GMiss; return; }
		FToolMenuSection& Section = Menu->FindOrAddSection("EasyTheme", FText::FromString(TEXT("Easy Theme")));
		Section.AddSubMenu("EasyThemeMenu",
			FText::FromString(TEXT("Easy Theme")),
			FText::FromString(TEXT("Easy Theme options")),
			FNewToolMenuDelegate::CreateStatic(&FillMenu));
		Note(TEXT("OK   menu Window > Easy Theme"));
		if (!UToolMenus::Get()->IsMenuRegistered("EasyTheme.Recent"))
		{
			UToolMenu* Recent = UToolMenus::Get()->RegisterMenu("EasyTheme.Recent");
			Recent->AddDynamicSection("EasyThemeRecent", FNewToolMenuDelegate::CreateStatic(&FillRecentMenu));
		}

	}
}

class FEasyThemeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		using namespace EasyTheme;
		if (!FSlateApplication::IsInitialized()) { Note(TEXT("Slate not initialised - theme skipped")); return; }

		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("EasyTheme"));
		GResDir = Plugin.IsValid() ? FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir() / TEXT("Resources")) : FString();
		if (Plugin.IsValid())
		{
			GVersion = Plugin->GetDescriptor().VersionName;
			GPluginDir = FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir());
		}
		GConfig->GetBool(GConfigSection, TEXT("Dark"), GDark, GEditorSettingsIni);
		GConfig->GetBool(GConfigSection, TEXT("LightGraphs"), GLightGraphs, GEditorSettingsIni);
		GConfig->GetBool(GConfigSection, TEXT("ToolbarInTabBar"), GToolbarInTitle, GEditorSettingsIni);
		GDev = IFileManager::Get().FileExists(*(ReportDir() / TEXT("dev.flag")));
		Note(FString::Printf(TEXT("EasyTheme %s - mode %s - resources %s"), *GVersion, GDark ? TEXT("dark") : TEXT("light"), *GResDir));

		RegisterResources();
		ApplyAll();
		ArrowCursor();

		MenuHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateStatic(&EasyTheme::RegisterMenus));

		Tickers.Add(FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			EasyTheme::ApplyColors(false);
			EasyTheme::ApplyGraphColors(!EasyTheme::GDark && EasyTheme::GLightGraphs);   // graph modules may load late
			return false;
		}), 3.0f));

		// Toolbar in the title bar: retried every second (the level editor can rebuild its content).
		Tickers.Add(FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			EasyTheme::EnsureToolbarInTitle();
			return true;
		}), 0.5f));

		// Put the user's graph colours back before Unreal writes its settings on exit.
		PreExitHandle = FCoreDelegates::OnPreExit.AddLambda([]() { EasyTheme::ApplyGraphColors(false); EasyTheme::RunPendingUpdate(); });

		Tickers.Add(FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			EasyTheme::ApplyColors(false);
			EasyTheme::WriteUserLayout(false);
			if (EasyTheme::GDev)
			{
				EasyTheme::Shot(TEXT("main.png"));
				EasyTheme::DumpLayouts();
				EasyTheme::WriteReport();
			}
			return false;
		}), 20.0f));

		Tickers.Add(FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
		{
			EasyTheme::PollCommands();
			EasyTheme::WatchColors();
			return true;
		}), 2.0f));

		Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("EasyTheme.Shot"),
			TEXT("Easy Theme: save a screenshot of the main window to Saved/EasyTheme/<name>.png"),
			FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
			{
				const FString Name = (Args.Num() > 0 ? Args[0] : FString(TEXT("shot"))) + TEXT(".png");
				EasyTheme::Shot(*Name);
				EasyTheme::WriteReport();
			})));
		Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("EasyTheme.Audit"),
			TEXT("Easy Theme: open the main panels, screenshot every window, list unreadable styles to Saved/EasyTheme/audit.txt"),
			FConsoleCommandDelegate::CreateLambda([]() { EasyTheme::AuditStep(0); })));
		Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("EasyTheme.Apply"),
			TEXT("Re-apply Easy Theme and write Saved/EasyTheme/report.txt + shot.png"),
			FConsoleCommandDelegate::CreateLambda([]()
			{
				EasyTheme::ApplyAll();
				EasyTheme::Shot(TEXT("shot.png"));
				EasyTheme::WriteReport();
			})));
		Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("EasyTheme.Dark"),
			TEXT("Easy Theme: 1 = dark, 0 = light"),
			FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
			{
				EasyTheme::SetMode(Args.Num() > 0 ? Args[0] != TEXT("0") : !EasyTheme::GDark);
			})));
		// Dev-only helpers for staging screenshots (test project).
		if (EasyTheme::GDev)
		{
			Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
				TEXT("EasyTheme.Tab"),
				TEXT("Easy Theme (dev): EasyTheme.Tab <LevelEditorTabId> [close]"),
				FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
				{
					if (Args.Num() == 0 || !FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor"))) { return; }
					TSharedPtr<FTabManager> TM = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor")).GetLevelEditorTabManager();
					if (!TM.IsValid()) { return; }
					const FTabId Id = FTabId(FName(*Args[0]));
					if (Args.Num() > 1 && Args[1] == TEXT("close"))
					{
						if (TSharedPtr<SDockTab> T = TM->FindExistingLiveTab(Id)) { T->RequestCloseTab(); }
					}
					else
					{
						TM->TryInvokeTab(Id);
					}
				})));
			// Shows a plain and a documentation tooltip in a window and screenshots it (tips_N.png).
			Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
				TEXT("EasyTheme.Tips"),
				TEXT("Easy Theme (dev): screenshot sample tooltips"),
				FConsoleCommandDelegate::CreateLambda([]()
				{
					const FText Txt = FText::FromString(TEXT("List of world Blueprints available to the user for editing or creation."));
					TSharedRef<SWindow> W = SNew(SWindow).Title(FText::FromString(TEXT("EasyTheme Tips")))
						.ClientSize(FVector2D(520.0, 220.0)).SupportsMaximize(false).SupportsMinimize(false)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(10.f)[ SNew(SToolTip).Text(Txt) ]
							+ SVerticalBox::Slot().AutoHeight().Padding(10.f)[ IDocumentation::Get()->CreateToolTip(Txt, nullptr, FString(), FString()) ]
						];
					FSlateApplication::Get().AddWindow(W);
					FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([W](float)
					{
						TArray<FColor> Px; FIntVector Size;
						if (FSlateApplication::Get().TakeScreenshot(W, Px, Size) && Px.Num())
						{
							for (FColor& C : Px) { C.A = 255; }
							TArray64<uint8> Png;
							FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Px.GetData(), Px.Num()), Png);
							FFileHelper::SaveArrayToFile(Png, *(EasyTheme::ReportDir() / (EasyTheme::GDark ? TEXT("tips_dark.png") : TEXT("tips_light.png"))));
							EasyTheme::Note(TEXT("OK   shot tips"));
						}
						W->RequestDestroyWindow();
						return false;
					}), 1.0f);
				})));
			Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
				TEXT("EasyTheme.Bug"),
				TEXT("Easy Theme (dev): write the bug report files without opening the browser or Explorer"),
				FConsoleCommandDelegate::CreateLambda([]() { EasyTheme::ReportIssue(false); EasyTheme::WriteReport(); })));
			Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
				TEXT("EasyTheme.UpdateTest"),
				TEXT("Easy Theme (dev): EasyTheme.UpdateTest <fake version> - full update into Saved/EasyTheme/Update/TestPlugin"),
				FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
				{
					EasyTheme::GFakeVersion = Args.Num() ? Args[0] : TEXT("0.0.1");
					EasyTheme::CheckForUpdates(true);
				})));
			Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
				TEXT("EasyTheme.RecentShot"),
				TEXT("Easy Theme (dev): screenshot the + menu (recent_menu.png)"),
				FConsoleCommandDelegate::CreateLambda([]()
				{
					TSharedRef<SWindow> W = SNew(SWindow).Title(FText::FromString(TEXT("EasyTheme Recent"))).ClientSize(FVector2D(360.0, 420.0))
						[ UToolMenus::Get()->GenerateWidget("EasyTheme.Recent", FToolMenuContext()) ];
					FSlateApplication::Get().AddWindow(W);
					FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([W](float)
					{
						TArray<FColor> Px; FIntVector Size;
						if (FSlateApplication::Get().TakeScreenshot(W, Px, Size) && Px.Num())
						{
							for (FColor& C : Px) { C.A = 255; }
							TArray64<uint8> Png;
							FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Px.GetData(), Px.Num()), Png);
							FFileHelper::SaveArrayToFile(Png, *(EasyTheme::ReportDir() / TEXT("recent_menu.png")));
							EasyTheme::Note(TEXT("OK   shot recent menu"));
						}
						W->RequestDestroyWindow();
						return false;
					}), 1.0f);
				})));
			Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
				TEXT("EasyTheme.Panes"),
				TEXT("Easy Theme (dev): apply the 3-pane layout"),
				FConsoleCommandDelegate::CreateLambda([]() { EasyTheme::UsePanesLayout(); })));
		}
		Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("EasyTheme.TabToolbar"),
			TEXT("Easy Theme: EasyTheme.TabToolbar 1|0 - main toolbar in the tab bar, or back in its row"),
			FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
			{
				EasyTheme::SetToolbarInTitle(Args.Num() ? Args[0] != TEXT("0") : !EasyTheme::GToolbarInTitle);
			})));
		Cmds.Add(IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("EasyTheme.Layout"),
			TEXT("Easy Theme: rewrite the 3-pane user layout"),
			FConsoleCommandDelegate::CreateLambda([]()
			{
				EasyTheme::WriteUserLayout(true);
				EasyTheme::WriteReport();
			})));
	}

	virtual void ShutdownModule() override
	{
		FCoreDelegates::OnPreExit.Remove(PreExitHandle);
		EasyTheme::GTabBars.Reset();
		for (const FTSTicker::FDelegateHandle& H : Tickers) { FTSTicker::GetCoreTicker().RemoveTicker(H); }
		if (EasyTheme::GResStyle.IsValid()) { FSlateStyleRegistry::UnRegisterSlateStyle(*EasyTheme::GResStyle); EasyTheme::GResStyle.Reset(); }
		Tickers.Reset();
		for (IConsoleObject* C : Cmds) { IConsoleManager::Get().UnregisterConsoleObject(C); }
		Cmds.Reset();
		if (UObjectInitialized())
		{
			UToolMenus::UnRegisterStartupCallback(MenuHandle);
			UToolMenus::UnregisterOwner(FName(TEXT("EasyTheme")));
		}
	}

private:
	TArray<FTSTicker::FDelegateHandle> Tickers;
	TArray<IConsoleObject*> Cmds;
	FDelegateHandle MenuHandle;
	FDelegateHandle PreExitHandle;
};

IMPLEMENT_MODULE(FEasyThemeModule, EasyTheme)
