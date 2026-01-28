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
            "IKRig",
        });

        // Editor-only functionality is provided by the Editor module; keep runtime module editor-free.

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Engine",
            "Json",
            "JsonUtilities"
        });

        // Editor-only dependencies for UVrmSourceAsset editor methods (AssetRegistry tags, etc.)
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "AssetRegistry",
                "EditorFramework"
            });
        }

string sdkRoot = LocateVrmSdkRoot();
        
        // 1. Fixed Include Path (Matches your C:\VRM_SDK\include)
        string includePath = Path.Combine(sdkRoot, "include");
        if (!Directory.Exists(includePath))
        {
            throw new BuildException($"VRM SDK include directory '{includePath}' does not exist.");
        }
        PublicSystemIncludePaths.Add(includePath);

        // 1. Define configuration string (Debug vs Release)
        string configurationDir = (Target.Configuration == UnrealTargetConfiguration.Debug ||
                                  Target.Configuration == UnrealTargetConfiguration.DebugGame)
            ? "Debug"
            : "Release";

        // 2. Point to the base lib folder
        string libsRoot = Path.Combine(sdkRoot, "lib");

        // 3. Match folder layout: Debug libs in \lib\Debug, Release libs directly in \lib\
        string configurationLibPath = (configurationDir == "Debug")
            ? Path.Combine(libsRoot, "Debug")
            : libsRoot;

        // Fallback to Debug if the chosen path doesn't exist
        if (!Directory.Exists(configurationLibPath))
        {
            configurationLibPath = Path.Combine(libsRoot, "Debug");
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
                missing.Add(libPath);
            }
            else
            {
                PublicAdditionalLibraries.Add(libPath);
            }
        }

        if (missing.Count > 0)
        {
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

