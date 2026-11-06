// Copyright MapModelers. All Rights Reserved.

using UnrealBuildTool;

public class EasyTheme : ModuleRules
{
	public EasyTheme(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"Projects",
			"LevelEditor",
			"ToolMenus",
			"UnrealEd",
			"HTTP",
			"Json"
		});
	}
}
