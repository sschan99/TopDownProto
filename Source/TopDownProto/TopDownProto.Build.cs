// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TopDownProto : ModuleRules
{
	public TopDownProto(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// Core modules for basic functionality
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"HeadMountedDisplay"  // For VR support if needed
		});

		// Networking and multiplayer modules
		PrivateDependencyModuleNames.AddRange(new string[] { 
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"Sockets",
			"Networking"
		});

		// UI modules
		PrivateDependencyModuleNames.AddRange(new string[] { 
			"Slate", 
			"SlateCore",
			"UMG"
		});

		// Additional gameplay modules
		PrivateDependencyModuleNames.AddRange(new string[] {
			"AIModule",
			"GameplayTasks"
		});
	}
}
