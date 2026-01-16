using System;
using System.IO;
using UnrealBuildTool;

public class VrmToolchain : ModuleRules
{
    public VrmToolchain(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Engine"
        });

        string sdkRoot = LocateVrmSdkRoot();
        string includePath = Path.Combine(sdkRoot, "include");
        if (!Directory.Exists(includePath))
        {
            throw new BuildException($"VRM SDK include directory '{includePath}' does not exist.");
        }

        PublicSystemIncludePaths.Add(includePath);

        string libsRoot = Path.Combine(sdkRoot, "lib", "Win64");
        string configurationDir = Target.Configuration == UnrealTargetConfiguration.Debug ||
                                  Target.Configuration == UnrealTargetConfiguration.DebugGame
            ? "Debug"
            : "Release";

        string configurationLibPath = Path.Combine(libsRoot, configurationDir);
        if (!Directory.Exists(configurationLibPath))
        {
            throw new BuildException($"Could not find VRM SDK libraries for {configurationDir} at '{configurationLibPath}'.");
        }

        // Only link the expected VRM SDK libraries; do not reference vrm_avatar_model.lib.
        string[] requiredLibs = new[] { "vrm_glb_parser.lib", "vrm_normalizers.lib", "vrm_validate.lib" };
        var missing = new System.Collections.Generic.List<string>();
        foreach (var lib in requiredLibs)
        {
            var libPath = Path.Combine(configurationLibPath, lib);
            if (!File.Exists(libPath))
            {
                missing.Add(lib);
            }
            else
            {
                PublicAdditionalLibraries.Add(libPath);
            }
        }

        if (missing.Count > 0)
        {
            throw new BuildException($"Missing required libraries in VRM SDK {configurationDir} folder: {string.Join(", ", missing)}");
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
