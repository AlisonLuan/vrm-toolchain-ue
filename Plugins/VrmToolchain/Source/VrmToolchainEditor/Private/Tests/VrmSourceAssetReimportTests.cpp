#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UObject/Package.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"

#include "VrmToolchain/VrmSourceAsset.h"
#include "VrmSourceAssetReimportHandler.h"
#include "VrmToolchain/VrmMetadataAsset.h"
#include "EditorFramework/AssetImportData.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmSourceAssetReimport_RefreshesBytes,
    "VrmToolchain.Editor.Reimport.VrmSourceAsset.RefreshesBytes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmSourceAssetReimport_RefreshesBytes::RunTest(const FString& Parameters)
{
    const FString TempDir = FPaths::ProjectIntermediateDir() / TEXT("VrmToolchainTests");
    IFileManager::Get().MakeDirectory(*TempDir, true);

    const FString TempFile = TempDir / TEXT("ReimportTemp.vrm");

    // Initial bytes
    TArray<uint8> BytesA;
    BytesA.AddUninitialized(16);
    for (int32 i = 0; i < BytesA.Num(); ++i) { BytesA[i] = uint8(0xA0 + i); }
    TestTrue(TEXT("Write initial file"), FFileHelper::SaveArrayToFile(BytesA, *TempFile));

    // Create assets
    const FString PackageName = FString::Printf(TEXT("/Game/VrmToolchainTests/Reimport_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    UPackage* Pkg = CreatePackage(*PackageName);
    TestNotNull(TEXT("Package created"), Pkg);

    UVrmSourceAsset* Source = NewObject<UVrmSourceAsset>(Pkg, UVrmSourceAsset::StaticClass(), TEXT("ReimportSource"),
        RF_Public | RF_Standalone);
    TestNotNull(TEXT("Source created"), Source);

    UVrmMetadataAsset* Meta = NewObject<UVrmMetadataAsset>(Pkg, UVrmMetadataAsset::StaticClass(), TEXT("ReimportMeta"),
        RF_Public | RF_Standalone);
    Source->Descriptor = Meta;

    Source->SourceFilename = TempFile;
    if (Source->AssetImportData)
    {
        Source->AssetImportData->Update(TempFile);
    }

    // Modify file bytes
    TArray<uint8> BytesB;
    BytesB.AddUninitialized(32);
    for (int32 i = 0; i < BytesB.Num(); ++i) { BytesB[i] = uint8(0xB0 + i); }
    TestTrue(TEXT("Write updated file"), FFileHelper::SaveArrayToFile(BytesB, *TempFile));

    // Reimport
    FVrmSourceAssetReimportHandler Handler;
    const EReimportResult::Type Result = Handler.Reimport(Source);
    TestTrue(TEXT("Reimport succeeded"), Result == EReimportResult::Succeeded);

#if WITH_EDITORONLY_DATA
    TestEqual(TEXT("Bytes updated"), Source->GetSourceBytes().Num(), BytesB.Num());
#endif

    return true;
}

#endif
