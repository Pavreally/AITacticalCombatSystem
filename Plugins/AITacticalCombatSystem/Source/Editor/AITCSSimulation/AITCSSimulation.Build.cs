// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AITCSSimulation : ModuleRules
{
	public AITCSSimulation(ReadOnlyTargetRules Target) : base(Target)
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
				"ToolMenus",
			}
			);
	}
}
