using System;
using System.IO;
using UnrealBuildTool;

public class VrmToolchain : ModuleRules
{
    public VrmToolchain(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Some third-party code uses C++ exceptions; enable exceptions for this module so try/catch compiles (/EHsc).
        bEnableExceptions = true;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
        });

        // Kismet provides BlueprintFunctionLibrary support; only link it for editor builds to avoid requiring editor-only modules
        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.Add("Kismet");
        }

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Engine",
            "Json",
            "JsonUtilities"
        });

string sdkRoot = LocateVrmSdkRoot();
        
        // 1. Fixed Include Path (Matches your C:\VRM_SDK\include)
        string includePath = Path.Combine(sdkRoot, "include");
        if (!Directory.Exists(includePath))
        {
            throw new BuildException($"VRM SDK include directory '{includePath}' does not exist.");
        }
        PublicSystemIncludePaths.Add(includePath);

        // 2. Updated Library Path (Removed "Win64" and handled the flat structure)
        string libsRoot = Path.Combine(sdkRoot, "lib");

        // Determine if we use the root lib folder (for Release) or the Debug subfolder
        string configurationLibPath;
        if (Target.Configuration == UnrealTargetConfiguration.Debug || 
            Target.Configuration == UnrealTargetConfiguration.DebugGame)
        {
            configurationLibPath = Path.Combine(libsRoot, "Debug");
        }
        else
        {
            // Your 'dir' showed release libs are directly in \lib\
            configurationLibPath = libsRoot; 
        }

        if (!Directory.Exists(configurationLibPath))
        {
            throw new BuildException($"Could not find VRM SDK libraries at '{configurationLibPath}'.");
        }

        // Only link the expected VRM SDK libraries.
        string[] requiredLibs = new[] { "vrm_glb_parser.lib", "vrm_normalizers.lib", "vrm_validate.lib" };
        var missing = new System.Collections.Generic.List<string>();

        foreach (var lib in requiredLibs)
        {
            var libPath = Path.Combine(configurationLibPath, lib);
            if (!File.Exists(libPath))
            {
                missing.Add(libPath); // Changed to show the full path if missing
            }
            else
            {
                PublicAdditionalLibraries.Add(libPath);
            }
        }

        if (missing.Count > 0)
        {
            // Changed "configurationDir" to "configurationLibPath" to fix the CS0103 error
            throw new BuildException($"Missing required libraries in VRM SDK folder '{configurationLibPath}': {string.Join(", ", missing)}");
        }
    }

    private string LocateVrmSdkRoot()
    {
        string envRoot = Environment.GetEnvironmentVariable("VRM_SDK_ROOT");
        if (!string.IsNullOrEmpty(envRoot) && Directory.Exists(envRoot))
        {
            return Path.GetFullPath(envRoot);
        }

        string candidate = Path.Combine(ModuleDirectory, "..", "ThirdParty", "VrmSdk");
        candidate = Path.GetFullPath(candidate);
        string includeCandidate = Path.Combine(candidate, "include");
        if (Directory.Exists(includeCandidate))
        {
            return candidate;
        }

        throw new BuildException(
            "Unable to locate the VRM SDK. Set VRM_SDK_ROOT or stage the SDK inside Plugins/VrmToolchain/Source/ThirdParty/VrmSdk.");
    }
}
