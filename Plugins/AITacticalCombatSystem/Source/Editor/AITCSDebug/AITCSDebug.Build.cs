// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AITCSDebug : ModuleRules
{
	public AITCSDebug(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"AITCSCore",
				"AITCSRuntime",
				"UnrealEd",
				"InputCore",
				"ToolMenus",
			}
			);
	}
}
