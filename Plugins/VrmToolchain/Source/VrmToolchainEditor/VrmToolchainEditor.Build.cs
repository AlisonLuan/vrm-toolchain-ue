using System.IO;
using UnrealBuildTool;

public class VrmToolchainEditor : ModuleRules
{
    public VrmToolchainEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateDefinitions.Add("VRM_TOOLCHAIN_WIN64=1");
            
            // Developer tool staging removed: do NOT ship developer-only executables like vrm_validate.exe via build rules.
            // Editor code should locate optional developer tooling at runtime via VRM_SDK_ROOT, PATH, or local dev-only locations.
        }

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "VrmToolchain",
            "UnrealEd",
            "Kismet",
            "Slate",
            "SlateCore",
            // Needed for IPluginManager & plugin discovery at editor startup
            "Projects",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "LevelEditor",
            "ToolMenus",
            "ContentBrowser",
            "ContentBrowserData",
            "AssetTools",
            "AssetRegistry",
            "AssetDefinition",
            "EditorSubsystem",
            "EditorFramework",
            "UnrealEd",
            "IKRig",
            "IKRigEditor",
            "Json",
            "JsonUtilities",
            "DeveloperSettings",
            // Ensure animation/reference skeleton headers are resolvable in editor builds
            "AnimationCore",
            "Engine",
            // B2: Mesh utilities for skeletal mesh building
            "MeshUtilities",
            "SkeletalMeshUtilitiesCommon",
            "MeshDescription",
            "StaticMeshDescription",
            "RenderCore"
        });
    }
}
