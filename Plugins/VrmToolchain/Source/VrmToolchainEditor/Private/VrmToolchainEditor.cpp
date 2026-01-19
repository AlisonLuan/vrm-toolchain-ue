#include "VrmToolchainEditor.h"
#include "VrmContentBrowserActions.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformMisc.h"

#ifndef VRMTOOLCHAIN_ENABLE_PATH_PROBE
  #if UE_BUILD_SHIPPING
    #define VRMTOOLCHAIN_ENABLE_PATH_PROBE 0
  #else
    #define VRMTOOLCHAIN_ENABLE_PATH_PROBE 1
  #endif
#endif

DEFINE_LOG_CATEGORY(LogVrmToolchainEditor);

static FString GetEnvVar(const TCHAR* Name)
{
    // Use the new API that returns an FString (fixes the deprecation warning)
    return FPlatformMisc::GetEnvironmentVariable(Name).TrimStartAndEnd();
}

static bool TryFindValidateExe(FString& OutExePath, TArray<FString>& OutTriedPaths, const FString& PluginDir, FString& OutFoundVia, FString& OutOverriddenPath)
{
    OutExePath.Reset();
    OutTriedPaths.Reset();
    OutFoundVia.Reset();
    OutOverriddenPath.Reset();

    const FString SdkRoot = GetEnvVar(TEXT("VRM_SDK_ROOT"));

    auto Try = [&](const FString& Candidate, const TCHAR* Source) -> bool
    {
        FString Full = FPaths::ConvertRelativePathToFull(Candidate);
        FPaths::NormalizeFilename(Full);
        OutTriedPaths.Add(Full);
        if (FPaths::FileExists(Full))
        {
            OutExePath = Full;
            OutFoundVia = Source;
            return true;
        }
        return false;
    };

    // 1) Preferred: external SDK install
    if (!SdkRoot.IsEmpty())
    {
        if (Try(FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("Win64"), TEXT("vrm_validate.exe")), TEXT("SDK"))) return true;
        if (Try(FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("Win64"), TEXT("Release"), TEXT("vrm_validate.exe")), TEXT("SDK"))) return true;
        if (Try(FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("Win64"), TEXT("Debug"), TEXT("vrm_validate.exe")), TEXT("SDK"))) return true;
        // also check bin/vrm_validate.exe
        if (Try(FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("vrm_validate.exe")), TEXT("SDK"))) return true;
    }

    // 2) Dev fallback: tool dropped inside plugin tree (developer only)
    const FString Local = FPaths::Combine(PluginDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("VrmSdk"),
                                         TEXT("bin"), TEXT("Win64"), TEXT("vrm_validate.exe"));
    if (Try(Local, TEXT("LOCAL"))) return true;

    // 3) Optional: PATH search (developer convenience) - last-resort
#if VRMTOOLCHAIN_ENABLE_PATH_PROBE
    {
#if PLATFORM_WINDOWS
        const FString ExeName = TEXT("vrm_validate.exe");
        const FString PathDelim = TEXT(";");
#else
        const FString ExeName = TEXT("vrm_validate");
        const FString PathDelim = TEXT(":");
#endif
        const FString PathEnv = GetEnvVar(TEXT("PATH"));
        if (!PathEnv.IsEmpty())
        {
            TArray<FString> PathEntries;
            PathEnv.ParseIntoArray(PathEntries, *PathDelim, true);
            for (const FString& Dir : PathEntries)
            {
                if (Dir.IsEmpty()) continue;
                FString Candidate = FPaths::Combine(Dir, ExeName);
                // If PATH provides a candidate but a plugin-local tool exists, prefer LOCAL and record the PATH candidate
                FString CandidateFull = FPaths::ConvertRelativePathToFull(Candidate);
                FPaths::NormalizeFilename(CandidateFull);
                OutTriedPaths.Add(CandidateFull);

                FString LocalFull = FPaths::ConvertRelativePathToFull(Local);
                FPaths::NormalizeFilename(LocalFull);
                if (FPaths::FileExists(CandidateFull))
                {
                    if (FPaths::FileExists(LocalFull))
                    {
                        // Prefer local; record overridden PATH for diagnostics
                        OutExePath = LocalFull;
                        OutFoundVia = TEXT("LOCAL");
                        OutOverriddenPath = CandidateFull;
                        return true;
                    }
                    OutExePath = CandidateFull;
                    OutFoundVia = TEXT("PATH");
                    return true;
                }
            }
        }
        else
        {
#if PLATFORM_WINDOWS
            OutTriedPaths.Add(TEXT("<PATH>: vrm_validate.exe (not found)"));
#else
            OutTriedPaths.Add(TEXT("<PATH>: vrm_validate (not found)"));
#endif
        }
    }
#endif

    return false;
}

// Helper: build the list of candidate paths (deterministic, no FS checks)
static void BuildValidateCandidates(TArray<FString>& OutCandidates, const FString& SdkRoot, const FString& PluginDir)
{
    OutCandidates.Reset();

    if (!SdkRoot.IsEmpty())
    {
        const TArray<FString> SdkCandidates = {
            FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("Win64"), TEXT("vrm_validate.exe")),
            FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("Win64"), TEXT("Release"), TEXT("vrm_validate.exe")),
            FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("Win64"), TEXT("Debug"), TEXT("vrm_validate.exe")),
            FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("vrm_validate.exe"))
        };

        for (const FString& C : SdkCandidates)
        {
            FString Full = FPaths::ConvertRelativePathToFull(C);
            FPaths::NormalizeFilename(Full);
            OutCandidates.Add(Full);
        }
    }

    const FString Local = FPaths::Combine(PluginDir, TEXT("Source"), TEXT("ThirdParty"), TEXT("VrmSdk"),
                                         TEXT("bin"), TEXT("Win64"), TEXT("vrm_validate.exe"));
    {
        FString Full = FPaths::ConvertRelativePathToFull(Local);
        FPaths::NormalizeFilename(Full);
        OutCandidates.Add(Full);
    }

#if VRMTOOLCHAIN_ENABLE_PATH_PROBE
#if PLATFORM_WINDOWS
    OutCandidates.Add(TEXT("<PATH>: vrm_validate.exe (not found)"));
#else
    OutCandidates.Add(TEXT("<PATH>: vrm_validate (not found)"));
#endif
#endif
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTryFindValidateExeCandidateOrderTest, "VrmToolchain.TryFindValidateExe.CandidateOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTryFindValidateExeCandidateOrderTest::RunTest(const FString& Parameters)
{
    TArray<FString> Candidates;
    const FString SdkRoot = TEXT("C:/VRM_SDK_ROOT_TEST");
    const FString PluginDir = TEXT("C:/Project/Plugins/VrmToolchain");
    BuildValidateCandidates(Candidates, SdkRoot, PluginDir);

    int32 idx = 0;
    // SDK candidates
    TestTrue(TEXT("SDK candidate 1 present"), Candidates.IsValidIndex(idx) && Candidates[idx].Contains(TEXT("VRM_SDK_ROOT_TEST"))); ++idx;
    TestTrue(TEXT("SDK candidate 2 present"), Candidates.IsValidIndex(idx) && Candidates[idx].Contains(TEXT("VRM_SDK_ROOT_TEST"))); ++idx;
    TestTrue(TEXT("SDK candidate 3 present"), Candidates.IsValidIndex(idx) && Candidates[idx].Contains(TEXT("VRM_SDK_ROOT_TEST"))); ++idx;
    TestTrue(TEXT("SDK candidate 4 present"), Candidates.IsValidIndex(idx) && Candidates[idx].Contains(TEXT("VRM_SDK_ROOT_TEST"))); ++idx;

    // Local should come after SDK candidates
    TestTrue(TEXT("Local candidate present at expected position"), Candidates.IsValidIndex(idx) && Candidates[idx].Contains(TEXT("Plugins"))); ++idx;

#if VRMTOOLCHAIN_ENABLE_PATH_PROBE
#if PLATFORM_WINDOWS
    TestTrue(TEXT("PATH marker present after local"), Candidates.IsValidIndex(idx) && Candidates[idx].StartsWith(TEXT("<PATH>: vrm_validate.exe"))); ++idx;
#else
    TestTrue(TEXT("PATH marker present after local"), Candidates.IsValidIndex(idx) && Candidates[idx].StartsWith(TEXT("<PATH>: vrm_validate"))); ++idx;
#endif
#endif
    return true;
}
#endif

void FVrmToolchainEditorModule::StartupModule()
{
    UE_LOG(LogVrmToolchainEditor, Log, TEXT("VrmToolchainEditor startup"));

#if WITH_EDITOR
    static bool bStartupNoticeLogged = false;

    FString PluginDir;
    if (TSharedPtr<IPlugin> P = IPluginManager::Get().FindPlugin(TEXT("VrmToolchain")))
    {
        UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmToolchain plugin resolved: BaseDir='%s' Descriptor='%s'"), *P->GetBaseDir(), *P->GetDescriptorFileName());
        PluginDir = P->GetBaseDir();
    }
    else
    {
        PluginDir = FPaths::ConvertRelativePathToFull(
            FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins"), TEXT("VrmToolchain"))
        );
    }

    FString FoundExe;
    TArray<FString> Tried;
    FString FoundVia;
    FString OverriddenPath;
    const bool bFound = TryFindValidateExe(FoundExe, Tried, PluginDir, FoundVia, OverriddenPath);
    bValidationCliAvailable = bFound;
    ValidationCliExePath = FoundExe; // optional member, useful for menu actions

    if (!bStartupNoticeLogged)
    {
        const FString SdkRoot = GetEnvVar(TEXT("VRM_SDK_ROOT"));

        if (bFound)
        {
            UE_LOG(LogVrmToolchainEditor, Display, TEXT("Validation CLI found via %s at '%s'"), *FoundVia, *FoundExe);

            if (!OverriddenPath.IsEmpty())
            {
                UE_LOG(LogVrmToolchainEditor, Warning,
                    TEXT("Validation CLI found on PATH at '%s' but plugin-local tool exists at '%s'; preferring plugin-local (do not redistribute)."),
                    *OverriddenPath,
                    *FoundExe);
            }
        }
        else if (!SdkRoot.IsEmpty())
        {
            UE_LOG(LogVrmToolchainEditor, Warning,
                TEXT("VRM_SDK_ROOT is set to '%s' but vrm_validate.exe was not found. Tried:\n  - %s"),
                *SdkRoot,
                *FString::Join(Tried, TEXT("\n  - ")));
        }
        else
        {
            UE_LOG(LogVrmToolchainEditor, Display,
                TEXT("vrm_validate.exe not found. CLI-based editor validation is disabled. To enable, install the VRM SDK developer tools and set VRM_SDK_ROOT (preferred). For local development you may place vrm_validate.exe under Source/ThirdParty/VrmSdk/bin/Win64 (do not redistribute)."));
        }

        bStartupNoticeLogged = true;
    }

    // Register Content Browser actions for VRM normalization
    FVrmContentBrowserActions::RegisterMenuExtensions();
#endif
}

void FVrmToolchainEditorModule::ShutdownModule()
{
#if WITH_EDITOR
    // Unregister Content Browser actions
    FVrmContentBrowserActions::UnregisterMenuExtensions();
#endif

    // Editor-only cleanup can be performed here when/if needed.
    // Currently we have no dynamic registrations that require explicit teardown.
}

// Ensure the editor module is registered with the module manager
IMPLEMENT_MODULE(FVrmToolchainEditorModule, VrmToolchainEditor)
