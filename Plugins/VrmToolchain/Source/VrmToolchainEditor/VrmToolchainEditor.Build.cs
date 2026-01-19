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
            
            // Stage vrm_validate.exe runtime dependency for Win64
            string pluginDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", ".."));
            string exePath = Path.Combine(pluginDir, "Source", "ThirdParty", "VrmSdk", "bin", "Win64", "vrm_validate.exe");
            
            if (File.Exists(exePath))
            {
                // Stage as NonUFS so it ships as a loose file
                RuntimeDependencies.Add(exePath, StagedFileType.NonUFS);
            }
        }

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "VrmToolchain",
            "UnrealEd",
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
            "Json",
            "JsonUtilities"
        });
    }
}
