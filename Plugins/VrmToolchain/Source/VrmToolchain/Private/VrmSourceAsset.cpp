#include "VrmToolchain/VrmSourceAsset.h"

#include "VrmToolchain/VrmMetadataAsset.h"

#if WITH_EDITOR
#include "EditorFramework/AssetImportData.h"
#include "HAL/PlatformProcess.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#if __has_include("AssetRegistry/AssetRegistryTagsContext.h")
  #include "AssetRegistry/AssetRegistryTagsContext.h"
#endif

// Try to include the concrete tag header if available
#if __has_include("AssetRegistry/AssetRegistryTags.h")
  #include "AssetRegistry/AssetRegistryTags.h"
  #define VRM_HAS_ASSET_REGISTRY_TAG 1
#elif __has_include("AssetRegistry/AssetRegistryTag.h")
  #include "AssetRegistry/AssetRegistryTag.h"
  #define VRM_HAS_ASSET_REGISTRY_TAG 1
#else
  #define VRM_HAS_ASSET_REGISTRY_TAG 0
#endif
#endif

DEFINE_LOG_CATEGORY_STATIC(LogVrmSourceAsset, Log, All);

UVrmSourceAsset::UVrmSourceAsset()
{
    // AssetImportData is declared with UPROPERTY(Instanced) which handles automatic instancing.
    // Do not manually create it here to avoid CDO reference issues that prevent SavePackage.
    // The factory will create and initialize it when needed.
}

#if WITH_EDITOR

#if VRM_HAS_ASSET_REGISTRY_TAG
static void AddVrmTags_Array(
    TArray<FAssetRegistryTag>& OutTags,
    const FString& SourceFilename,
    int32 VrmSpecVersionMajor,
    const FString& DetectedVrmExtension)
{
    if (!SourceFilename.IsEmpty())
    {
        OutTags.Add(FAssetRegistryTag(TEXT("VrmSourceFile"), SourceFilename, FAssetRegistryTag::TT_Alphabetical));
    }
    if (VrmSpecVersionMajor >= 0)
    {
        OutTags.Add(FAssetRegistryTag(TEXT("VrmSpecVersion"), FString::FromInt(VrmSpecVersionMajor), FAssetRegistryTag::TT_Numerical));
    }
    if (!DetectedVrmExtension.IsEmpty())
    {
        OutTags.Add(FAssetRegistryTag(TEXT("VrmDetectedExtension"), DetectedVrmExtension, FAssetRegistryTag::TT_Alphabetical));
    }
}
#endif

#if VRM_HAS_TAGS_CONTEXT
void UVrmSourceAsset::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
    Super::GetAssetRegistryTags(Context);

#if VRM_HAS_ASSET_REGISTRY_TAG
    TArray<FAssetRegistryTag> Tmp;
    AddVrmTags_Array(Tmp, SourceFilename, VrmSpecVersionMajor, DetectedVrmExtension);

    for (const FAssetRegistryTag& Tag : Tmp)
    {
        Context.AddTag(Tag);
    }
#endif
}
#endif

void UVrmSourceAsset::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
#if VRM_HAS_TAGS_CONTEXT
    // When TagsContext is available (UE5.7+), the modern override handles calling Super.
    // For backward compatibility, just add our custom tags without calling deprecated Super.
#if VRM_HAS_ASSET_REGISTRY_TAG
    TArray<FAssetRegistryTag> Tmp;
    AddVrmTags_Array(Tmp, SourceFilename, VrmSpecVersionMajor, DetectedVrmExtension);
    OutTags.Append(Tmp);
#endif
#else
    // When TagsContext is not available (pre-UE5.7), call Super with deprecation warning suppressed.
#pragma warning(push)
#pragma warning(disable: 4996)
    Super::GetAssetRegistryTags(OutTags);
#pragma warning(pop)
#if VRM_HAS_ASSET_REGISTRY_TAG
    AddVrmTags_Array(OutTags, SourceFilename, VrmSpecVersionMajor, DetectedVrmExtension);
#endif
#endif
}

static FString NormalizeToAbs(const FString& In)
{
    if (In.IsEmpty()) return {};
    FString P = In;
    FPaths::NormalizeFilename(P);
    if (!FPaths::IsRelative(P)) return P;
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), P);
}

bool UVrmSourceAsset::EditorRecomputeSpecVersion()
{
    // No-op guard: don't rerun if already set
    if (VrmSpecVersionMajor >= 0)
    {
        return true;
    }
    return EditorRecomputeSpecVersionForce();
}

bool UVrmSourceAsset::EditorRecomputeSpecVersionForce()
{
    // 1) Resolve source file (prefer AssetImportData->GetFirstFilename())
    FString Src = SourceFilename;
#if WITH_EDITORONLY_DATA
    if (AssetImportData)
    {
        const FString A = AssetImportData->GetFirstFilename();
        if (!A.IsEmpty())
        {
            Src = A;
        }
    }
#endif

    const FString AbsSrc = NormalizeToAbs(Src);
    if (AbsSrc.IsEmpty() || !FPaths::FileExists(AbsSrc))
    {
        UE_LOG(LogVrmSourceAsset, Warning, TEXT("EditorRecomputeSpecVersionForce: source missing: '%s' (abs='%s')"), *Src, *AbsSrc);
        return false;
    }

    // 2) Get validator exe - try to load VrmToolchainEditor module if available
    FString Exe;
    if (FModuleManager::Get().IsModuleLoaded("VrmToolchainEditor"))
    {
        // Use reflection to get the validation tool path without hard-linking to editor module
        IModuleInterface* EditorMod = FModuleManager::Get().GetModule("VrmToolchainEditor");
        if (EditorMod)
        {
            // Try to call GetValidationToolPath via UFunction reflection or static accessor
            // For now, we'll use environment variable fallback
        }
    }

    // Fallback: check environment variable VRM_SDK_ROOT
    FString SdkRoot = FPlatformMisc::GetEnvironmentVariable(TEXT("VRM_SDK_ROOT")).TrimStartAndEnd();
    if (!SdkRoot.IsEmpty())
    {
        TArray<FString> Candidates = {
            FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("Win64"), TEXT("vrm_validate.exe")),
            FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("Win64"), TEXT("Release"), TEXT("vrm_validate.exe")),
            FPaths::Combine(SdkRoot, TEXT("bin"), TEXT("vrm_validate.exe"))
        };
        for (const FString& C : Candidates)
        {
            if (FPaths::FileExists(C))
            {
                Exe = C;
                break;
            }
        }
    }

    if (Exe.IsEmpty() || !FPaths::FileExists(Exe))
    {
        UE_LOG(LogVrmSourceAsset, Warning, TEXT("EditorRecomputeSpecVersionForce: vrm_validate.exe unavailable. Set VRM_SDK_ROOT environment variable."));
        return false;
    }

    const FString Args = FString::Printf(TEXT("\"%s\" --json"), *AbsSrc);

    // 3) Capture stdout
    void* ReadPipe = nullptr;
    void* WritePipe = nullptr;
    FPlatformProcess::CreatePipe(ReadPipe, WritePipe);

    FProcHandle Proc = FPlatformProcess::CreateProc(
        *Exe,
        *Args,
        true,   // detached
        false,  // hidden
        false,  // really hidden
        nullptr,
        0,
        nullptr,
        WritePipe,
        nullptr
    );

    if (!Proc.IsValid())
    {
        FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
        UE_LOG(LogVrmSourceAsset, Error, TEXT("EditorRecomputeSpecVersionForce: failed to launch: %s %s"), *Exe, *Args);
        return false;
    }

    FString StdOut;
    while (FPlatformProcess::IsProcRunning(Proc))
    {
        StdOut += FPlatformProcess::ReadPipe(ReadPipe);
        FPlatformProcess::Sleep(0.01f);
    }
    StdOut += FPlatformProcess::ReadPipe(ReadPipe);

    int32 ReturnCode = 0;
    FPlatformProcess::GetProcReturnCode(Proc, &ReturnCode);
    FPlatformProcess::ClosePipe(ReadPipe, WritePipe);

    if (StdOut.IsEmpty())
    {
        UE_LOG(LogVrmSourceAsset, Warning, TEXT("EditorRecomputeSpecVersionForce: empty stdout. rc=%d exe='%s' args='%s'"), ReturnCode, *Exe, *Args);
        return false;
    }

    // 4) Parse JSON schema: files[0].vrm.version
    TSharedPtr<FJsonObject> RootObj;
    {
        const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(StdOut);
        if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
        {
            UE_LOG(LogVrmSourceAsset, Warning, TEXT("EditorRecomputeSpecVersionForce: JSON parse failed. rc=%d head='%s'"), ReturnCode, *StdOut.Left(200));
            return false;
        }
    }

    const TArray<TSharedPtr<FJsonValue>>* FilesArr = nullptr;
    if (!RootObj->TryGetArrayField(TEXT("files"), FilesArr) || !FilesArr || FilesArr->Num() == 0)
    {
        UE_LOG(LogVrmSourceAsset, Warning, TEXT("EditorRecomputeSpecVersionForce: JSON missing 'files'. head='%s'"), *StdOut.Left(200));
        return false;
    }

    const TSharedPtr<FJsonObject> FileObj = (*FilesArr)[0]->AsObject();
    const TSharedPtr<FJsonObject>* VrmObjPtr = nullptr;
    if (!FileObj.IsValid() || !FileObj->TryGetObjectField(TEXT("vrm"), VrmObjPtr) || !VrmObjPtr || !VrmObjPtr->IsValid())
    {
        UE_LOG(LogVrmSourceAsset, Warning, TEXT("EditorRecomputeSpecVersionForce: JSON missing files[0].vrm"));
        return false;
    }

    FString VersionStr;
    if (!(*VrmObjPtr)->TryGetStringField(TEXT("version"), VersionStr) || VersionStr.IsEmpty())
    {
        UE_LOG(LogVrmSourceAsset, Warning, TEXT("EditorRecomputeSpecVersionForce: JSON missing files[0].vrm.version"));
        return false;
    }

    FString MajorStr;
    int32 Major = -1;
    if (VersionStr.Split(TEXT("."), &MajorStr, nullptr))
    {
        Major = FCString::Atoi(*MajorStr);
    }
    else
    {
        Major = FCString::Atoi(*VersionStr);
    }

    if (Major < 0)
    {
        UE_LOG(LogVrmSourceAsset, Warning, TEXT("EditorRecomputeSpecVersionForce: invalid vrm.version '%s'"), *VersionStr);
        return false;
    }

    if (VrmSpecVersionMajor != Major)
    {
        VrmSpecVersionMajor = Major;
        MarkPackageDirty();
    }

    return true;
}

void UVrmSourceAsset::EditorBackfillDerivedFields()
{
    bool bChanged = false;

    // Backfill SourceFilename from AssetImportData if needed (source of truth)
    if (AssetImportData && SourceFilename.IsEmpty())
    {
        FString ImportedFilename = AssetImportData->GetFirstFilename();
        if (!ImportedFilename.IsEmpty())
        {
            SourceFilename = ImportedFilename;
            bChanged = true;
        }
    }

    // Backfill DetectedVrmExtension from SourceFilename
    if (DetectedVrmExtension.IsEmpty() && !SourceFilename.IsEmpty())
    {
        DetectedVrmExtension = FPaths::GetExtension(SourceFilename).ToLower();
        bChanged = true;
    }
    // Deterministically recompute spec version from JSON
    if (EditorRecomputeSpecVersion())
    {
        bChanged = true;
    }

    if (bChanged)
    {
        MarkPackageDirty();
    }
}

#endif // WITH_EDITOR
