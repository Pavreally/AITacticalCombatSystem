// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AITCSEditor : ModuleRules
{
	public AITCSEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"AssetTools",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"Slate",
				"SlateCore",
				"AITCSCore",
				"AITCSRuntime",
				"UnrealEd",
				"GraphEditor",
				"PropertyEditor",
				"EditorFramework",
				"EditorStyle",
				"Projects",
				"InputCore",
				"ApplicationCore",
				"ToolMenus",
			}
			);
	}
}
