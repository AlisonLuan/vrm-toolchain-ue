// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class HostProjectEditorTarget : TargetRules
{
	public HostProjectEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		CppStandard = CppStandardVersion.Cpp20;
		ExtraModuleNames.AddRange(new string[] { "HostProject" });
	}
}
