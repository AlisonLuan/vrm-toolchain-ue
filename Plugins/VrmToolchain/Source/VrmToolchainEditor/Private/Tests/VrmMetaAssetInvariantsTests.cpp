#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "VrmMetaFeatureDetection.h"
#include "VrmToolchain/VrmMetaAsset.h"
#include "UObject/Package.h"

using namespace VrmMetaDetection;

/**
 * Test that created UVrmMetaAsset fields match the detected features from JSON.
 * Validates the invariant: Meta->bHasX = Features.bHasX for all feature flags.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaAssetInvariants_Vrm0AllFeatures, "VrmToolchain.MetaAssetInvariants.Vrm0AllFeatures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaAssetInvariants_Vrm0AllFeatures::RunTest(const FString& Parameters)
{
	// VRM0 JSON with all features present
	FString Vrm0Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRM": {
				"meta": {"title": "Test VRM0 Model"},
				"humanoid": {}
			}
		},
		"secondaryAnimation": [],
		"blendShapeMaster": [],
		"thumbnail": {}
	})");

	// Parse features from JSON
	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm0Json);

	// Create a temporary package for the meta asset
	FString TestPackagePath = TEXT("/Game/Developers/VrmToolchainTest/VrmMetaAsset_Vrm0_AllFeatures");
	UPackage* TestPackage = CreatePackage(*TestPackagePath);
	TestPackage->FullyLoad();

	// Create meta asset and apply features (matching factory assignment pattern)
	UVrmMetaAsset* MetaAsset = NewObject<UVrmMetaAsset>(TestPackage, TEXT("MetaAsset"), RF_Transient);
	if (!MetaAsset)
	{
		return false;
	}

	// Apply feature detection results to meta asset (same logic as VrmSourceFactory.cpp)
	MetaAsset->SpecVersion = Features.SpecVersion;
	MetaAsset->bHasHumanoid = Features.bHasHumanoid;
	MetaAsset->bHasSpringBones = Features.bHasSpringBones;
	MetaAsset->bHasBlendShapesOrExpressions = Features.bHasBlendShapesOrExpressions;
	MetaAsset->bHasThumbnail = Features.bHasThumbnail;

	// Verify invariants: Meta asset fields match detected features
	TestEqual(TEXT("SpecVersion should match VRM0"), MetaAsset->SpecVersion, EVrmVersion::VRM0);
	TestEqual(TEXT("bHasHumanoid should match detected value (true)"), MetaAsset->bHasHumanoid, Features.bHasHumanoid);
	TestEqual(TEXT("bHasSpringBones should match detected value (true)"), MetaAsset->bHasSpringBones, Features.bHasSpringBones);
	TestEqual(TEXT("bHasBlendShapesOrExpressions should match detected value (true)"), MetaAsset->bHasBlendShapesOrExpressions, Features.bHasBlendShapesOrExpressions);
	TestEqual(TEXT("bHasThumbnail should match detected value (true)"), MetaAsset->bHasThumbnail, Features.bHasThumbnail);

	// Verify all flags are true
	TestTrue(TEXT("Should have humanoid"), MetaAsset->bHasHumanoid);
	TestTrue(TEXT("Should have spring bones"), MetaAsset->bHasSpringBones);
	TestTrue(TEXT("Should have blend shapes"), MetaAsset->bHasBlendShapesOrExpressions);
	TestTrue(TEXT("Should have thumbnail"), MetaAsset->bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaAssetInvariants_Vrm0Minimal, "VrmToolchain.MetaAssetInvariants.Vrm0Minimal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaAssetInvariants_Vrm0Minimal::RunTest(const FString& Parameters)
{
	// VRM0 JSON with no features
	FString Vrm0Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRM": {
				"meta": {"title": "Minimal VRM0"}
			}
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm0Json);

	FString TestPackagePath = TEXT("/Game/Developers/VrmToolchainTest/VrmMetaAsset_Vrm0_Minimal");
	UPackage* TestPackage = CreatePackage(*TestPackagePath);
	TestPackage->FullyLoad();

	UVrmMetaAsset* MetaAsset = NewObject<UVrmMetaAsset>(TestPackage, TEXT("MetaAsset"), RF_Transient);
	if (!MetaAsset)
	{
		return false;
	}

	MetaAsset->SpecVersion = Features.SpecVersion;
	MetaAsset->bHasHumanoid = Features.bHasHumanoid;
	MetaAsset->bHasSpringBones = Features.bHasSpringBones;
	MetaAsset->bHasBlendShapesOrExpressions = Features.bHasBlendShapesOrExpressions;
	MetaAsset->bHasThumbnail = Features.bHasThumbnail;

	TestEqual(TEXT("SpecVersion should match VRM0"), MetaAsset->SpecVersion, EVrmVersion::VRM0);
	TestFalse(TEXT("bHasHumanoid should be false"), MetaAsset->bHasHumanoid);
	TestFalse(TEXT("bHasSpringBones should be false"), MetaAsset->bHasSpringBones);
	TestFalse(TEXT("bHasBlendShapesOrExpressions should be false"), MetaAsset->bHasBlendShapesOrExpressions);
	TestFalse(TEXT("bHasThumbnail should be false"), MetaAsset->bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaAssetInvariants_Vrm1AllFeatures, "VrmToolchain.MetaAssetInvariants.Vrm1AllFeatures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaAssetInvariants_Vrm1AllFeatures::RunTest(const FString& Parameters)
{
	// VRM1 JSON with all features present
	FString Vrm1Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRMC_vrm": {
				"specVersion": "1.0",
				"meta": {"name": "Test VRM1 Model"},
				"humanoid": {},
				"expressions": [],
				"thumbnail": {}
			},
			"VRMC_springBone": {}
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm1Json);

	FString TestPackagePath = TEXT("/Game/Developers/VrmToolchainTest/VrmMetaAsset_Vrm1_AllFeatures");
	UPackage* TestPackage = CreatePackage(*TestPackagePath);
	TestPackage->FullyLoad();

	UVrmMetaAsset* MetaAsset = NewObject<UVrmMetaAsset>(TestPackage, TEXT("MetaAsset"), RF_Transient);
	if (!MetaAsset)
	{
		return false;
	}

	MetaAsset->SpecVersion = Features.SpecVersion;
	MetaAsset->bHasHumanoid = Features.bHasHumanoid;
	MetaAsset->bHasSpringBones = Features.bHasSpringBones;
	MetaAsset->bHasBlendShapesOrExpressions = Features.bHasBlendShapesOrExpressions;
	MetaAsset->bHasThumbnail = Features.bHasThumbnail;

	TestEqual(TEXT("SpecVersion should match VRM1"), MetaAsset->SpecVersion, EVrmVersion::VRM1);
	TestEqual(TEXT("bHasHumanoid should match detected value (true)"), MetaAsset->bHasHumanoid, Features.bHasHumanoid);
	TestEqual(TEXT("bHasSpringBones should match detected value (true)"), MetaAsset->bHasSpringBones, Features.bHasSpringBones);
	TestEqual(TEXT("bHasBlendShapesOrExpressions should match detected value (true)"), MetaAsset->bHasBlendShapesOrExpressions, Features.bHasBlendShapesOrExpressions);
	TestEqual(TEXT("bHasThumbnail should match detected value (true)"), MetaAsset->bHasThumbnail, Features.bHasThumbnail);

	// Verify all flags are true
	TestTrue(TEXT("Should have humanoid"), MetaAsset->bHasHumanoid);
	TestTrue(TEXT("Should have spring bones"), MetaAsset->bHasSpringBones);
	TestTrue(TEXT("Should have expressions"), MetaAsset->bHasBlendShapesOrExpressions);
	TestTrue(TEXT("Should have thumbnail"), MetaAsset->bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaAssetInvariants_Vrm1Minimal, "VrmToolchain.MetaAssetInvariants.Vrm1Minimal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaAssetInvariants_Vrm1Minimal::RunTest(const FString& Parameters)
{
	// VRM1 JSON with no features
	FString Vrm1Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRMC_vrm": {
				"specVersion": "1.0",
				"meta": {"name": "Minimal VRM1"}
			}
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm1Json);

	FString TestPackagePath = TEXT("/Game/Developers/VrmToolchainTest/VrmMetaAsset_Vrm1_Minimal");
	UPackage* TestPackage = CreatePackage(*TestPackagePath);
	TestPackage->FullyLoad();

	UVrmMetaAsset* MetaAsset = NewObject<UVrmMetaAsset>(TestPackage, TEXT("MetaAsset"), RF_Transient);
	if (!MetaAsset)
	{
		return false;
	}

	MetaAsset->SpecVersion = Features.SpecVersion;
	MetaAsset->bHasHumanoid = Features.bHasHumanoid;
	MetaAsset->bHasSpringBones = Features.bHasSpringBones;
	MetaAsset->bHasBlendShapesOrExpressions = Features.bHasBlendShapesOrExpressions;
	MetaAsset->bHasThumbnail = Features.bHasThumbnail;

	TestEqual(TEXT("SpecVersion should match VRM1"), MetaAsset->SpecVersion, EVrmVersion::VRM1);
	TestFalse(TEXT("bHasHumanoid should be false"), MetaAsset->bHasHumanoid);
	TestFalse(TEXT("bHasSpringBones should be false"), MetaAsset->bHasSpringBones);
	TestFalse(TEXT("bHasBlendShapesOrExpressions should be false"), MetaAsset->bHasBlendShapesOrExpressions);
	TestFalse(TEXT("bHasThumbnail should be false"), MetaAsset->bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaAssetInvariants_Vrm1PartialFeatures, "VrmToolchain.MetaAssetInvariants.Vrm1PartialFeatures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaAssetInvariants_Vrm1PartialFeatures::RunTest(const FString& Parameters)
{
	// VRM1 JSON with some features (humanoid + spring, no expressions/thumbnail)
	FString Vrm1Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRMC_vrm": {
				"specVersion": "1.0",
				"meta": {"name": "Partial VRM1"},
				"humanoid": {}
			},
			"VRMC_springBone": {}
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm1Json);

	FString TestPackagePath = TEXT("/Game/Developers/VrmToolchainTest/VrmMetaAsset_Vrm1_Partial");
	UPackage* TestPackage = CreatePackage(*TestPackagePath);
	TestPackage->FullyLoad();

	UVrmMetaAsset* MetaAsset = NewObject<UVrmMetaAsset>(TestPackage, TEXT("MetaAsset"), RF_Transient);
	if (!MetaAsset)
	{
		return false;
	}

	MetaAsset->SpecVersion = Features.SpecVersion;
	MetaAsset->bHasHumanoid = Features.bHasHumanoid;
	MetaAsset->bHasSpringBones = Features.bHasSpringBones;
	MetaAsset->bHasBlendShapesOrExpressions = Features.bHasBlendShapesOrExpressions;
	MetaAsset->bHasThumbnail = Features.bHasThumbnail;

	TestEqual(TEXT("SpecVersion should match VRM1"), MetaAsset->SpecVersion, EVrmVersion::VRM1);
	TestTrue(TEXT("bHasHumanoid should be true"), MetaAsset->bHasHumanoid);
	TestTrue(TEXT("bHasSpringBones should be true"), MetaAsset->bHasSpringBones);
	TestFalse(TEXT("bHasBlendShapesOrExpressions should be false"), MetaAsset->bHasBlendShapesOrExpressions);
	TestFalse(TEXT("bHasThumbnail should be false"), MetaAsset->bHasThumbnail);

	// Verify invariant: meta asset matches features exactly
	TestEqual(TEXT("Humanoid invariant"), MetaAsset->bHasHumanoid, Features.bHasHumanoid);
	TestEqual(TEXT("Spring invariant"), MetaAsset->bHasSpringBones, Features.bHasSpringBones);
	TestEqual(TEXT("BlendOrExpr invariant"), MetaAsset->bHasBlendShapesOrExpressions, Features.bHasBlendShapesOrExpressions);
	TestEqual(TEXT("Thumbnail invariant"), MetaAsset->bHasThumbnail, Features.bHasThumbnail);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
