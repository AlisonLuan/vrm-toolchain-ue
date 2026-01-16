// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class VrmToolchainRuntime : ModuleRules
{
	public VrmToolchainRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		// Add ThirdParty VRM SDK integration
		string ThirdPartyPath = Path.Combine(ModuleDirectory, "../../ThirdParty");
		string VrmSdkPath = Path.Combine(ThirdPartyPath, "VrmSdk");
		
		// Check for VRM_SDK_ROOT environment variable
		string VrmSdkRoot = System.Environment.GetEnvironmentVariable("VRM_SDK_ROOT");
		if (!string.IsNullOrEmpty(VrmSdkRoot))
		{
			VrmSdkPath = VrmSdkRoot;
		}

		// Add include paths
		string IncludePath = Path.Combine(VrmSdkPath, "include");
		if (Directory.Exists(IncludePath))
		{
			PublicIncludePaths.Add(IncludePath);
		}

		// Add library paths
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string LibPath = Path.Combine(VrmSdkPath, "lib", "Win64");
			if (Directory.Exists(LibPath))
			{
				PublicAdditionalLibraries.Add(Path.Combine(LibPath, "vrm-sdk.lib"));
				
				string DllPath = Path.Combine(VrmSdkPath, "bin", "Win64", "vrm-sdk.dll");
				if (File.Exists(DllPath))
				{
					RuntimeDependencies.Add(DllPath);
				}
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string LibPath = Path.Combine(VrmSdkPath, "lib", "Linux");
			if (Directory.Exists(LibPath))
			{
				PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libvrm-sdk.so"));
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string LibPath = Path.Combine(VrmSdkPath, "lib", "Mac");
			if (Directory.Exists(LibPath))
			{
				PublicAdditionalLibraries.Add(Path.Combine(LibPath, "libvrm-sdk.dylib"));
			}
		}
	}
}
