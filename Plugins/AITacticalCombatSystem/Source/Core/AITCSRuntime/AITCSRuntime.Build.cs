// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AITCSRuntime : ModuleRules
{
	public AITCSRuntime(ReadOnlyTargetRules Target) : base(Target)
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
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AIModule",
			}
			);
	}
}
