#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "VrmMetaFeatureDetection.h"

using namespace VrmMetaDetection;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_Vrm0Features, "VrmToolchain.MetaDetection.Vrm0Features", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_Vrm0Features::RunTest(const FString& Parameters)
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

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm0Json);

	TestEqual(TEXT("Should detect VRM0 version"), Features.SpecVersion, EVrmVersion::VRM0);
	TestTrue(TEXT("Should detect humanoid"), Features.bHasHumanoid);
	TestTrue(TEXT("Should detect spring bones"), Features.bHasSpringBones);
	TestTrue(TEXT("Should detect blend shapes"), Features.bHasBlendShapesOrExpressions);
	TestTrue(TEXT("Should detect thumbnail"), Features.bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_Vrm0Minimal, "VrmToolchain.MetaDetection.Vrm0Minimal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_Vrm0Minimal::RunTest(const FString& Parameters)
{
	// VRM0 JSON with minimal features
	FString Vrm0Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRM": {
				"meta": {"title": "Minimal VRM0"}
			}
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm0Json);

	TestEqual(TEXT("Should detect VRM0 version"), Features.SpecVersion, EVrmVersion::VRM0);
	TestFalse(TEXT("Should not detect humanoid"), Features.bHasHumanoid);
	TestFalse(TEXT("Should not detect spring bones"), Features.bHasSpringBones);
	TestFalse(TEXT("Should not detect blend shapes"), Features.bHasBlendShapesOrExpressions);
	TestFalse(TEXT("Should not detect thumbnail"), Features.bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_Vrm1Features, "VrmToolchain.MetaDetection.Vrm1Features", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_Vrm1Features::RunTest(const FString& Parameters)
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

	TestEqual(TEXT("Should detect VRM1 version"), Features.SpecVersion, EVrmVersion::VRM1);
	TestTrue(TEXT("Should detect humanoid"), Features.bHasHumanoid);
	TestTrue(TEXT("Should detect spring bones (VRMC_springBone)"), Features.bHasSpringBones);
	TestTrue(TEXT("Should detect expressions"), Features.bHasBlendShapesOrExpressions);
	TestTrue(TEXT("Should detect thumbnail"), Features.bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_Vrm1WithSpringBoneInVrm, "VrmToolchain.MetaDetection.Vrm1WithSpringBoneInVrm", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_Vrm1WithSpringBoneInVrm::RunTest(const FString& Parameters)
{
	// VRM1 JSON where springBone is inside VRMC_vrm extension (alternate location)
	FString Vrm1Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRMC_vrm": {
				"specVersion": "1.0",
				"meta": {"name": "Test VRM1"},
				"springBone": []
			}
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(Vrm1Json);

	TestEqual(TEXT("Should detect VRM1 version"), Features.SpecVersion, EVrmVersion::VRM1);
	TestTrue(TEXT("Should detect spring bones (in VRMC_vrm)"), Features.bHasSpringBones);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_Vrm1Minimal, "VrmToolchain.MetaDetection.Vrm1Minimal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_Vrm1Minimal::RunTest(const FString& Parameters)
{
	// VRM1 JSON with minimal features
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

	TestEqual(TEXT("Should detect VRM1 version"), Features.SpecVersion, EVrmVersion::VRM1);
	TestFalse(TEXT("Should not detect humanoid"), Features.bHasHumanoid);
	TestFalse(TEXT("Should not detect spring bones"), Features.bHasSpringBones);
	TestFalse(TEXT("Should not detect expressions"), Features.bHasBlendShapesOrExpressions);
	TestFalse(TEXT("Should not detect thumbnail"), Features.bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_NoVrmExtensions, "VrmToolchain.MetaDetection.NoVrmExtensions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_NoVrmExtensions::RunTest(const FString& Parameters)
{
	// Plain glTF JSON without VRM extensions
	FString PlainJson = TEXT(R"({
		"asset": {"version": "2.0"},
		"scenes": [{"nodes": [0]}]
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(PlainJson);

	TestEqual(TEXT("Should detect Unknown version"), Features.SpecVersion, EVrmVersion::Unknown);
	TestFalse(TEXT("Should not detect humanoid"), Features.bHasHumanoid);
	TestFalse(TEXT("Should not detect spring bones"), Features.bHasSpringBones);
	TestFalse(TEXT("Should not detect blend shapes"), Features.bHasBlendShapesOrExpressions);
	TestFalse(TEXT("Should not detect thumbnail"), Features.bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_InvalidJson, "VrmToolchain.MetaDetection.InvalidJson", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_InvalidJson::RunTest(const FString& Parameters)
{
	// Malformed JSON
	FString BadJson = TEXT(R"({invalid json})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(BadJson);

	TestEqual(TEXT("Should return Unknown on parse failure"), Features.SpecVersion, EVrmVersion::Unknown);
	TestFalse(TEXT("Should default to no humanoid on parse failure"), Features.bHasHumanoid);
	TestFalse(TEXT("Should default to no spring bones on parse failure"), Features.bHasSpringBones);
	TestFalse(TEXT("Should default to no blend shapes on parse failure"), Features.bHasBlendShapesOrExpressions);
	TestFalse(TEXT("Should default to no thumbnail on parse failure"), Features.bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_EmptyJson, "VrmToolchain.MetaDetection.EmptyJson", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_EmptyJson::RunTest(const FString& Parameters)
{
	// Empty JSON object (valid but no VRM data)
	FString EmptyJson = TEXT("{}");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(EmptyJson);

	TestEqual(TEXT("Should detect Unknown for empty JSON"), Features.SpecVersion, EVrmVersion::Unknown);
	TestFalse(TEXT("Should not detect features in empty JSON"), Features.bHasHumanoid);
	TestFalse(TEXT("Should not detect features in empty JSON"), Features.bHasSpringBones);
	TestFalse(TEXT("Should not detect features in empty JSON"), Features.bHasBlendShapesOrExpressions);
	TestFalse(TEXT("Should not detect features in empty JSON"), Features.bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_ExtensionsNotObject, "VrmToolchain.MetaDetection.ExtensionsNotObject", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_ExtensionsNotObject::RunTest(const FString& Parameters)
{
	// Malformed: extensions exists but is an array, not an object
	FString MalformedJson = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": []
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(MalformedJson);

	TestEqual(TEXT("Should return Unknown when extensions is not an object"), Features.SpecVersion, EVrmVersion::Unknown);
	TestFalse(TEXT("Should not detect any flags when extensions is malformed"), Features.bHasHumanoid);
	TestFalse(TEXT("Should not detect any flags when extensions is malformed"), Features.bHasSpringBones);
	TestFalse(TEXT("Should not detect any flags when extensions is malformed"), Features.bHasBlendShapesOrExpressions);
	TestFalse(TEXT("Should not detect any flags when extensions is malformed"), Features.bHasThumbnail);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaDetection_VrmcVrmNotObject, "VrmToolchain.MetaDetection.VrmcVrmNotObject", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaDetection_VrmcVrmNotObject::RunTest(const FString& Parameters)
{
	// Malformed: VRMC_vrm field exists (so version is detected as VRM1) but is not an object
	FString MalformedJson = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRMC_vrm": true
		}
	})");

	FVrmMetaFeatures Features = ParseMetaFeaturesFromJson(MalformedJson);

	TestEqual(TEXT("Should detect VRM1 by presence of VRMC_vrm field"), Features.SpecVersion, EVrmVersion::VRM1);
	TestFalse(TEXT("Should not detect humanoid when VRMC_vrm is not an object"), Features.bHasHumanoid);
	TestFalse(TEXT("Should not detect blend/expr when VRMC_vrm is not an object"), Features.bHasBlendShapesOrExpressions);
	TestFalse(TEXT("Should not detect thumbnail when VRMC_vrm is not an object"), Features.bHasThumbnail);
	TestFalse(TEXT("Should not detect spring bones when VRMC_vrm is not an object"), Features.bHasSpringBones);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
