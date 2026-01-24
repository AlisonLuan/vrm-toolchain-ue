#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UObject/Package.h"

#include "VrmSourceFactory.h"
#include "VrmSourceAsset.h"
#include "VrmToolchain/VrmMetadataAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmSourceFactory_CreatesSourceAndMetadata,
    "VrmToolchain.Editor.Import.VrmSourceFactory.CreatesSourceAndMetadata",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmSourceFactory_CreatesSourceAndMetadata::RunTest(const FString& Parameters)
{
    const FString TempDir = FPaths::ProjectIntermediateDir() / TEXT("VrmToolchainTests");
    IFileManager::Get().MakeDirectory(*TempDir, true);

    const FString TempFile = TempDir / TEXT("TempAvatar.vrm");

    TArray<uint8> Bytes;
    Bytes.AddUninitialized(128);
    for (int32 i = 0; i < Bytes.Num(); ++i)
    {
        Bytes[i] = uint8(i & 0xFF);
    }

    TestTrue(TEXT("Write temp file"), FFileHelper::SaveArrayToFile(Bytes, *TempFile));

    UPackage* Pkg = CreatePackage(TEXT("/Game/VrmToolchainTests/TempAvatar"));
    TestNotNull(TEXT("Package created"), Pkg);

    UVrmSourceFactory* Factory = NewObject<UVrmSourceFactory>();
    TestNotNull(TEXT("Factory created"), Factory);

    bool bCanceled = false;
    UObject* Created = Factory->FactoryCreateFile(
        UVrmSourceAsset::StaticClass(),
        Pkg,
        FName(TEXT("TempAvatar")),
        RF_Public | RF_Standalone,
        TempFile,
        nullptr,
        GWarn,
        bCanceled);

    TestFalse(TEXT("Not canceled"), bCanceled);

    UVrmSourceAsset* Source = Cast<UVrmSourceAsset>(Created);
    TestNotNull(TEXT("Created UVrmSourceAsset"), Source);

#if WITH_EDITORONLY_DATA
    if (Source)
    {
        TestTrue(TEXT("Captured SourceBytes"), Source->SourceBytes.Num() > 0);
    }
#endif

    UVrmMetadataAsset* Meta = Source ? Source->Descriptor.Get() : nullptr;
    TestNotNull(TEXT("Created metadata descriptor"), Meta);

    if (Source && Source->AssetImportData)
    {
        TestTrue(TEXT("ImportData has filename"), !Source->AssetImportData->GetFirstFilename().IsEmpty());
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
