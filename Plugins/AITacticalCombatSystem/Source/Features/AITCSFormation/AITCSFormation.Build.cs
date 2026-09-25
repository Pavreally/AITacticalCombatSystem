// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AITCSFormation : ModuleRules
{
	public AITCSFormation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"AITCSCore",
				"StateTreeModule",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AITCSRuntime",
				"NavigationSystem",
			}
			);
	}
}
