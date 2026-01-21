#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/SkeletalMesh.h"
#include "VrmToolchainWrapper.h"
#include "VrmMetadataAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetadataUpsertTest, "VrmToolchain.Metadata.UpsertIdempotency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetadataUpsertTest::RunTest(const FString& Parameters)
{
    // 1. Setup Transient Mesh
    USkeletalMesh* TestMesh = NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, RF_Transient);
    TestNotNull(TEXT("Transient mesh created"), TestMesh);

    // 2. First Upsert (simulate import behavior)
    FVrmMetadata InitialData;
    InitialData.Title = TEXT("Test Model");
    InitialData.Author = TEXT("Author A");
    InitialData.Version = TEXT("1.0");

    UVrmMetadataAsset* MetadataAsset = FVrmSdkFacade::UpsertVrmMetadata(TestMesh, InitialData);
    TestNotNull(TEXT("Metadata should be created"), MetadataAsset);

    const UVrmMetadataAsset* ReadBack = TestMesh->GetAssetUserData<UVrmMetadataAsset>();
    TestNotNull(TEXT("Metadata should be present after first upsert"), ReadBack);
    TestEqual(TEXT("Title should match after first upsert"), ReadBack->Metadata.Title, InitialData.Title);

    // 3. Second Upsert (update existing)
    FVrmMetadata UpdatedData;
    UpdatedData.Title = TEXT("Test Model Updated");
    UpdatedData.Author = TEXT("Author B");

    UVrmMetadataAsset* MetadataAsset2 = FVrmSdkFacade::UpsertVrmMetadata(TestMesh, UpdatedData);
    TestNotNull(TEXT("Metadata still present on second upsert"), MetadataAsset2);

    // Upsert performed, check in-place update (the returned object is the same user data)
    TestEqual(TEXT("Returned object should contain updated Title"), MetadataAsset2->Metadata.Title, UpdatedData.Title);

    // 4. Assertions for Idempotency
    int32 FoundCount = 0;
    for (UAssetUserData* Entry : *TestMesh->GetAssetUserDataArray())
    {
        if (Cast<UVrmMetadataAsset>(Entry))
        {
            FoundCount++;
        }
    }

    TestEqual(TEXT("Should NOT have duplicate metadata entries"), FoundCount, 1);

    const UVrmMetadataAsset* FinalMetadata = TestMesh->GetAssetUserData<UVrmMetadataAsset>();
    TestNotNull(TEXT("Final metadata should be present"), FinalMetadata);
    TestEqual(TEXT("Data should be updated in-place (Author)"), FinalMetadata->Metadata.Author, UpdatedData.Author);
    TestEqual(TEXT("Title should be updated in-place"), FinalMetadata->Metadata.Title, UpdatedData.Title);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
