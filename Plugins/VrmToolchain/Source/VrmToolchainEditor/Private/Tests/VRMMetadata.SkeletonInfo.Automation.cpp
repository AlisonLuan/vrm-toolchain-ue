#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/SkeletalMesh.h"
#include "VrmSdkFacadeEditor.h"
#include "VrmMetadataAsset.h"
#include "VrmMetadata.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmSkeletonInfoTest, "VrmToolchain.Metadata.SkeletonInfo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmSkeletonInfoTest::RunTest(const FString& Parameters)
{
    // Prepare a shuffled bone list
    TArray<FName> Bones = {
        FName("spine"), FName("leftHand"), FName("rightFoot"), FName("hip"), FName("head"), FName("leftFoot"), FName("rightHand")
    };

    // Shuffle deterministicly by reversing (keeps test deterministic without randomness)
    Algo::Reverse(Bones);

    // Compute coverage using facade helper
    FVrmSkeletonCoverage Coverage = FVrmSdkFacadeEditor::ComputeSkeletonCoverage(Bones);

    TestEqual(TEXT("Total bone count should match input"), Coverage.TotalBoneCount, Bones.Num());

    // Verify sorting A-Z: first < last
    TestTrue(TEXT("Skeleton data should be alphabetically sorted"), Coverage.SortedBoneNames.Num() > 1 && Coverage.SortedBoneNames[0].LexicalLess(Coverage.SortedBoneNames.Last()));

    // Verify deterministic order equals manually sorted list
    TArray<FName> ManuallySorted = Bones;
    ManuallySorted.Sort([](const FName& A, const FName& B){ return A.LexicalLess(B); });
    TestEqual(TEXT("Sorted names should match manual sort"), Coverage.SortedBoneNames, ManuallySorted);

    // Idempotency: Upsert metadata twice should result in single user data entry
    USkeletalMesh* TestMesh = NewObject<USkeletalMesh>(GetTransientPackage(), NAME_None, RF_Transient);
    TestNotNull(TEXT("Transient mesh created"), TestMesh);

    FVrmMetadata InitialMetadata;
    InitialMetadata.Name = TEXT("SkeletonTest");

    UVrmMetadataAsset* Asset1 = FVrmSdkFacadeEditor::UpsertVrmMetadata(TestMesh, InitialMetadata);
    TestNotNull(TEXT("Metadata created on first upsert"), Asset1);

    // Assign coverage and mark package as dirty for editor-friendly behavior
    Asset1->SkeletonCoverage = Coverage;

    // Second upsert
    FVrmMetadata Updated;
    Updated.Name = TEXT("SkeletonTestUpdated");
    UVrmMetadataAsset* Asset2 = FVrmSdkFacadeEditor::UpsertVrmMetadata(TestMesh, Updated);
    TestNotNull(TEXT("Metadata returned on second upsert"), Asset2);

    // Ensure same object pointer (in-place update)
    TestTrue(TEXT("Upsert should return same userdata object"), Asset1 == Asset2);

    // Ensure only one UVrmMetadataAsset attached
    int32 FoundCount = 0;
    for (UAssetUserData* Entry : *TestMesh->GetAssetUserDataArray())
    {
        if (Cast<UVrmMetadataAsset>(Entry)) { FoundCount++; }
    }
    TestEqual(TEXT("Should NOT have duplicate metadata entries after repeated upsert"), FoundCount, 1);

    // Validate the skeleton coverage persisted
    const UVrmMetadataAsset* Final = TestMesh->GetAssetUserData<UVrmMetadataAsset>();
    TestNotNull(TEXT("Final metadata present"), Final);
    TestEqual(TEXT("Final skeleton coverage sorted names"), Final->SkeletonCoverage.SortedBoneNames, Coverage.SortedBoneNames);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
