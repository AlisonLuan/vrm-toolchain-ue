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

// ==============================================================================
// Test: Import summary formatting for VRM0 with all features
// ==============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmImportSummary_Vrm0AllFeatures, "VrmToolchain.ImportSummary.Vrm0AllFeatures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmImportSummary_Vrm0AllFeatures::RunTest(const FString& Parameters)
{
	FString Vrm0Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRM": {
				"meta": {"title": "Test VRM0"},
				"humanoid": {}
			}
		},
		"secondaryAnimation": [],
		"blendShapeMaster": [],
		"thumbnail": {}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm0Json);
	FString Summary = FormatImportSummary(Features);

	// Verify deterministic output format
	TestEqual(TEXT("Summary contains 'Imported VRM'"), Summary.Contains(TEXT("Imported VRM")), true);
	TestEqual(TEXT("Summary contains 'spec=vrm0'"), Summary.Contains(TEXT("spec=vrm0")), true);
	TestEqual(TEXT("Summary contains 'humanoid=1'"), Summary.Contains(TEXT("humanoid=1")), true);
	TestEqual(TEXT("Summary contains 'spring=1'"), Summary.Contains(TEXT("spring=1")), true);
	TestEqual(TEXT("Summary contains 'blendOrExpr=1'"), Summary.Contains(TEXT("blendOrExpr=1")), true);
	TestEqual(TEXT("Summary contains 'thumb=1'"), Summary.Contains(TEXT("thumb=1")), true);

	return true;
}

// ==============================================================================
// Test: Import summary formatting for VRM0 with minimal features
// ==============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmImportSummary_Vrm0Minimal, "VrmToolchain.ImportSummary.Vrm0Minimal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmImportSummary_Vrm0Minimal::RunTest(const FString& Parameters)
{
	FString Vrm0Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRM": {
				"meta": {"title": "Minimal VRM0"}
			}
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm0Json);
	FString Summary = FormatImportSummary(Features);

	// Verify minimal feature flags are all zero
	TestEqual(TEXT("Summary contains 'spec=vrm0'"), Summary.Contains(TEXT("spec=vrm0")), true);
	TestEqual(TEXT("Summary contains 'humanoid=0'"), Summary.Contains(TEXT("humanoid=0")), true);
	TestEqual(TEXT("Summary contains 'spring=0'"), Summary.Contains(TEXT("spring=0")), true);
	TestEqual(TEXT("Summary contains 'blendOrExpr=0'"), Summary.Contains(TEXT("blendOrExpr=0")), true);
	TestEqual(TEXT("Summary contains 'thumb=0'"), Summary.Contains(TEXT("thumb=0")), true);

	return true;
}

// ==============================================================================
// Test: Import summary formatting for VRM1 with all features
// ==============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmImportSummary_Vrm1AllFeatures, "VrmToolchain.ImportSummary.Vrm1AllFeatures", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmImportSummary_Vrm1AllFeatures::RunTest(const FString& Parameters)
{
	FString Vrm1Json = TEXT(R"({
		"asset": {"version": "3.0"},
		"extensions": {
			"VRMC_vrm": {
				"humanoid": {},
				"expressions": {},
				"thumbnail": {}
			},
			"VRMC_springBone": {}
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm1Json);
	FString Summary = FormatImportSummary(Features);

	// Verify deterministic output format for VRM1
	TestEqual(TEXT("Summary contains 'Imported VRM'"), Summary.Contains(TEXT("Imported VRM")), true);
	TestEqual(TEXT("Summary contains 'spec=vrm1'"), Summary.Contains(TEXT("spec=vrm1")), true);
	TestEqual(TEXT("Summary contains 'humanoid=1'"), Summary.Contains(TEXT("humanoid=1")), true);
	TestEqual(TEXT("Summary contains 'spring=1'"), Summary.Contains(TEXT("spring=1")), true);
	TestEqual(TEXT("Summary contains 'blendOrExpr=1'"), Summary.Contains(TEXT("blendOrExpr=1")), true);
	TestEqual(TEXT("Summary contains 'thumb=1'"), Summary.Contains(TEXT("thumb=1")), true);

	return true;
}

// ==============================================================================
// Test: Import summary formatting for VRM1 with minimal features
// ==============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmImportSummary_Vrm1Minimal, "VrmToolchain.ImportSummary.Vrm1Minimal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmImportSummary_Vrm1Minimal::RunTest(const FString& Parameters)
{
	FString Vrm1Json = TEXT(R"({
		"asset": {"version": "3.0"},
		"extensions": {
			"VRMC_vrm": {}
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm1Json);
	FString Summary = FormatImportSummary(Features);

	// Verify minimal feature flags are all zero
	TestEqual(TEXT("Summary contains 'spec=vrm1'"), Summary.Contains(TEXT("spec=vrm1")), true);
	TestEqual(TEXT("Summary contains 'humanoid=0'"), Summary.Contains(TEXT("humanoid=0")), true);
	TestEqual(TEXT("Summary contains 'spring=0'"), Summary.Contains(TEXT("spring=0")), true);
	TestEqual(TEXT("Summary contains 'blendOrExpr=0'"), Summary.Contains(TEXT("blendOrExpr=0")), true);
	TestEqual(TEXT("Summary contains 'thumb=0'"), Summary.Contains(TEXT("thumb=0")), true);

	return true;
}

// ==============================================================================
// Test: Import summary formatting for unknown version
// ==============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmImportSummary_UnknownVersion, "VrmToolchain.ImportSummary.UnknownVersion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmImportSummary_UnknownVersion::RunTest(const FString& Parameters)
{
	FString UnknownJson = TEXT(R"({
		"asset": {"version": "1.0"}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(UnknownJson);
	FString Summary = FormatImportSummary(Features);

	// Verify unknown version handling
	TestEqual(TEXT("Summary contains 'spec=unknown'"), Summary.Contains(TEXT("spec=unknown")), true);
	TestEqual(TEXT("Summary contains 'humanoid=0'"), Summary.Contains(TEXT("humanoid=0")), true);
	TestEqual(TEXT("Summary contains 'spring=0'"), Summary.Contains(TEXT("spring=0")), true);
	TestEqual(TEXT("Summary contains 'blendOrExpr=0'"), Summary.Contains(TEXT("blendOrExpr=0")), true);
	TestEqual(TEXT("Summary contains 'thumb=0'"), Summary.Contains(TEXT("thumb=0")), true);

	return true;
}

/**
 * Test: Warnings are deterministic and in stable order
 * 
 * This is a contract test — if you change warning strings or order,
 * you must update this test. It ensures CI catches accidental regressions.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmImportReport_WarningsStableOrder, "VrmToolchain.ImportReport.WarningsStableOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmImportReport_WarningsStableOrder::RunTest(const FString& Parameters)
{
	// VRM0 with NO features (all warnings should appear)
	FVrmMetaFeatures NoFeatures;
	NoFeatures.SpecVersion = EVrmVersion::VRM0;
	NoFeatures.bHasHumanoid = false;
	NoFeatures.bHasSpringBones = false;
	NoFeatures.bHasBlendShapesOrExpressions = false;
	NoFeatures.bHasThumbnail = false;

	FVrmImportReport Report = BuildImportReport(NoFeatures);

	// Expected warnings in deterministic order (contract)
	TArray<FString> ExpectedWarnings = {
		TEXT("Missing humanoid definition"),
		TEXT("No spring bones found"),
		TEXT("No blendshapes/expressions found"),
		TEXT("No thumbnail found")
	};

	// Verify warning count
	TestEqual(TEXT("All features missing should produce 4 warnings"), Report.Warnings.Num(), ExpectedWarnings.Num());

	// Verify exact array equality (order + content)
	if (Report.Warnings.Num() == ExpectedWarnings.Num())
	{
		for (int32 i = 0; i < Report.Warnings.Num(); ++i)
		{
			TestEqual(
				*FString::Printf(TEXT("Warning[%d] exact"), i),
				Report.Warnings[i],
				ExpectedWarnings[i]
			);

			// Strict: no embedded newlines (safe for GitHub annotations / display)
			TestFalse(
				*FString::Printf(TEXT("Warning[%d] no newlines"), i),
				Report.Warnings[i].Contains(TEXT("\n")) || Report.Warnings[i].Contains(TEXT("\r"))
			);
		}
	}

	// Verify summary still generated
	TestTrue(TEXT("Summary not empty"), !Report.Summary.IsEmpty());
	TestTrue(TEXT("Summary contains spec"), Report.Summary.Contains(TEXT("spec=")));

	return true;
}

/**
 * Test: All features present = no warnings
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmImportReport_AllFeaturesNoWarnings, "VrmToolchain.ImportReport.AllFeaturesNoWarnings", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmImportReport_AllFeaturesNoWarnings::RunTest(const FString& Parameters)
{
	FVrmMetaFeatures AllFeatures;
	AllFeatures.SpecVersion = EVrmVersion::VRM1;
	AllFeatures.bHasHumanoid = true;
	AllFeatures.bHasSpringBones = true;
	AllFeatures.bHasBlendShapesOrExpressions = true;
	AllFeatures.bHasThumbnail = true;

	FVrmImportReport Report = BuildImportReport(AllFeatures);

	// All features present → no warnings
	TestEqual(TEXT("All features present produces 0 warnings"), Report.Warnings.Num(), 0);

	// Summary should still reflect all features
	TestTrue(TEXT("Summary contains humanoid=1"), Report.Summary.Contains(TEXT("humanoid=1")));
	TestTrue(TEXT("Summary contains spring=1"), Report.Summary.Contains(TEXT("spring=1")));
	TestTrue(TEXT("Summary contains blendOrExpr=1"), Report.Summary.Contains(TEXT("blendOrExpr=1")));
	TestTrue(TEXT("Summary contains thumb=1"), Report.Summary.Contains(TEXT("thumb=1")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
