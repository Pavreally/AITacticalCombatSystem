// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AITCSReinforcement : ModuleRules
{
	public AITCSReinforcement(ReadOnlyTargetRules Target) : base(Target)
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
