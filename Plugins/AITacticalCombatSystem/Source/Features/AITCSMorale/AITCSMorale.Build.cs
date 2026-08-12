// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AITCSMorale : ModuleRules
{
	public AITCSMorale(ReadOnlyTargetRules Target) : base(Target)
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
				"AITCSCore",
				"AITCSRuntime",
			}
			);
	}
}
