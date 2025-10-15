// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TopDownProtoServerTarget : TargetRules
{
	public TopDownProtoServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("TopDownProto");
		
		// Server optimizations
		bUseLoggingInShipping = true;
		bCompileWithAccessibilitySupport = false;
	}
}
