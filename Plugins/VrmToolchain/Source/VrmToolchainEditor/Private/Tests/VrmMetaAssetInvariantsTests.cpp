#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "VrmMetaFeatureDetection.h"
#include "VrmToolchain/VrmMetaAsset.h"
#include "UObject/Package.h"

using namespace VrmMetaDetection;

/**
 * Helper: Create a unique transient package to avoid collisions across test runs
 */
static UPackage* MakeUniqueTestPackage()
{
	const FString UniqueSuffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString PkgPath = FString::Printf(TEXT("/Engine/Transient/VrmToolchainTest_%s"), *UniqueSuffix);
	return CreatePackage(*PkgPath);
}

/**
 * Helper: Create a meta asset in the given package (or transient if null)
 */
static UVrmMetaAsset* MakeMetaAsset(UPackage* Outer)
{
	return NewObject<UVrmMetaAsset>(Outer ? Outer : GetTransientPackage(), TEXT("MetaAsset"), RF_Transient);
}

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
	TestEqual(TEXT("Detected spec version"), Features.SpecVersion, EVrmVersion::VRM0);

	// Create meta asset with unique package
	UPackage* TestPackage = MakeUniqueTestPackage();
	UVrmMetaAsset* MetaAsset = MakeMetaAsset(TestPackage);
	TestNotNull(TEXT("Meta asset created"), MetaAsset);

	// Apply feature detection results using shared helper
	ApplyFeaturesToMetaAsset(MetaAsset, Features);

	// Verify invariants: Meta asset fields match detected features
	TestEqual(TEXT("SpecVersion invariant"), MetaAsset->SpecVersion, Features.SpecVersion);
	TestEqual(TEXT("Humanoid invariant"), MetaAsset->bHasHumanoid, Features.bHasHumanoid);
	TestEqual(TEXT("Spring bones invariant"), MetaAsset->bHasSpringBones, Features.bHasSpringBones);
	TestEqual(TEXT("Blend shapes invariant"), MetaAsset->bHasBlendShapesOrExpressions, Features.bHasBlendShapesOrExpressions);
	TestEqual(TEXT("Thumbnail invariant"), MetaAsset->bHasThumbnail, Features.bHasThumbnail);

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
	TestEqual(TEXT("Detected spec version"), Features.SpecVersion, EVrmVersion::VRM0);

	UPackage* TestPackage = MakeUniqueTestPackage();
	UVrmMetaAsset* MetaAsset = MakeMetaAsset(TestPackage);
	TestNotNull(TEXT("Meta asset created"), MetaAsset);

	ApplyFeaturesToMetaAsset(MetaAsset, Features);

	TestEqual(TEXT("SpecVersion invariant"), MetaAsset->SpecVersion, Features.SpecVersion);
	TestFalse(TEXT("Humanoid invariant"), MetaAsset->bHasHumanoid);
	TestFalse(TEXT("Spring bones invariant"), MetaAsset->bHasSpringBones);
	TestFalse(TEXT("Blend shapes invariant"), MetaAsset->bHasBlendShapesOrExpressions);
	TestFalse(TEXT("Thumbnail invariant"), MetaAsset->bHasThumbnail);

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
	TestEqual(TEXT("Detected spec version"), Features.SpecVersion, EVrmVersion::VRM1);

	UPackage* TestPackage = MakeUniqueTestPackage();
	UVrmMetaAsset* MetaAsset = MakeMetaAsset(TestPackage);
	TestNotNull(TEXT("Meta asset created"), MetaAsset);

	ApplyFeaturesToMetaAsset(MetaAsset, Features);

	TestEqual(TEXT("SpecVersion invariant"), MetaAsset->SpecVersion, Features.SpecVersion);
	TestEqual(TEXT("Humanoid invariant"), MetaAsset->bHasHumanoid, Features.bHasHumanoid);
	TestEqual(TEXT("Spring bones invariant"), MetaAsset->bHasSpringBones, Features.bHasSpringBones);
	TestEqual(TEXT("Expressions invariant"), MetaAsset->bHasBlendShapesOrExpressions, Features.bHasBlendShapesOrExpressions);
	TestEqual(TEXT("Thumbnail invariant"), MetaAsset->bHasThumbnail, Features.bHasThumbnail);

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
	TestEqual(TEXT("Detected spec version"), Features.SpecVersion, EVrmVersion::VRM1);

	UPackage* TestPackage = MakeUniqueTestPackage();
	UVrmMetaAsset* MetaAsset = MakeMetaAsset(TestPackage);
	TestNotNull(TEXT("Meta asset created"), MetaAsset);

	ApplyFeaturesToMetaAsset(MetaAsset, Features);

	TestEqual(TEXT("SpecVersion invariant"), MetaAsset->SpecVersion, Features.SpecVersion);
	TestFalse(TEXT("Humanoid invariant"), MetaAsset->bHasHumanoid);
	TestFalse(TEXT("Spring bones invariant"), MetaAsset->bHasSpringBones);
	TestFalse(TEXT("Expressions invariant"), MetaAsset->bHasBlendShapesOrExpressions);
	TestFalse(TEXT("Thumbnail invariant"), MetaAsset->bHasThumbnail);

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
	TestEqual(TEXT("Detected spec version"), Features.SpecVersion, EVrmVersion::VRM1);

	UPackage* TestPackage = MakeUniqueTestPackage();
	UVrmMetaAsset* MetaAsset = MakeMetaAsset(TestPackage);
	TestNotNull(TEXT("Meta asset created"), MetaAsset);

	ApplyFeaturesToMetaAsset(MetaAsset, Features);

	TestEqual(TEXT("SpecVersion invariant"), MetaAsset->SpecVersion, Features.SpecVersion);
	TestEqual(TEXT("Humanoid invariant"), MetaAsset->bHasHumanoid, Features.bHasHumanoid);
	TestEqual(TEXT("Spring bones invariant"), MetaAsset->bHasSpringBones, Features.bHasSpringBones);
	TestEqual(TEXT("Expressions invariant"), MetaAsset->bHasBlendShapesOrExpressions, Features.bHasBlendShapesOrExpressions);
	TestEqual(TEXT("Thumbnail invariant"), MetaAsset->bHasThumbnail, Features.bHasThumbnail);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
