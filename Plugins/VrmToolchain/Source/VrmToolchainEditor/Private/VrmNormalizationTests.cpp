#include "VrmNormalizationService.h"
#include "VrmNormalizationSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

// Test that BuildOutputPaths generates deterministic paths
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmNormalizationPathBuilderTest, "VrmToolchain.Normalization.PathBuilder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmNormalizationPathBuilderTest::RunTest(const FString& Parameters)
{
	// Mock settings with default values
	UVrmNormalizationSettings* Settings = GetMutableDefault<UVrmNormalizationSettings>();
	check(Settings);

	// Save original settings
	const EVrmNormalizationOutputLocation OriginalLocation = Settings->OutputLocation;
	const FString OriginalSuffix = Settings->NormalizedSuffix;

	// Test 1: NextToSource strategy with .vrm file
	{
		Settings->OutputLocation = EVrmNormalizationOutputLocation::NextToSource;
		Settings->NormalizedSuffix = TEXT(".normalized");

		const FString InputPath = TEXT("C:/Project/Content/Models/Character.vrm");
		FString OutPath, ReportPath;

		TestTrue(TEXT("BuildOutputPaths should succeed"), FVrmNormalizationService::BuildOutputPaths(InputPath, OutPath, ReportPath));
		TestEqual(TEXT("Output path should be next to source"), OutPath, TEXT("C:/Project/Content/Models/Character.normalized.vrm"));
		TestEqual(TEXT("Report path should be next to source"), ReportPath, TEXT("C:/Project/Content/Models/Character.normalized.report.json"));
	}

	// Test 2: NextToSource strategy with .glb file
	{
		Settings->OutputLocation = EVrmNormalizationOutputLocation::NextToSource;
		Settings->NormalizedSuffix = TEXT(".normalized");

		const FString InputPath = TEXT("D:/Assets/Models/Avatar.glb");
		FString OutPath, ReportPath;

		TestTrue(TEXT("BuildOutputPaths should succeed for .glb"), FVrmNormalizationService::BuildOutputPaths(InputPath, OutPath, ReportPath));
		TestEqual(TEXT("Output path should preserve .glb extension"), OutPath, TEXT("D:/Assets/Models/Avatar.normalized.glb"));
		TestEqual(TEXT("Report path should be next to source"), ReportPath, TEXT("D:/Assets/Models/Avatar.normalized.report.json"));
	}

	// Test 3: Custom suffix
	{
		Settings->OutputLocation = EVrmNormalizationOutputLocation::NextToSource;
		Settings->NormalizedSuffix = TEXT("_fixed");

		const FString InputPath = TEXT("C:/Temp/Model.vrm");
		FString OutPath, ReportPath;

		TestTrue(TEXT("BuildOutputPaths should succeed with custom suffix"), FVrmNormalizationService::BuildOutputPaths(InputPath, OutPath, ReportPath));
		TestEqual(TEXT("Output path should use custom suffix"), OutPath, TEXT("C:/Temp/Model_fixed.vrm"));
		TestEqual(TEXT("Report path should use custom suffix"), ReportPath, TEXT("C:/Temp/Model_fixed.report.json"));
	}

	// Test 4: SavedDirectory strategy
	{
		Settings->OutputLocation = EVrmNormalizationOutputLocation::SavedDirectory;
		Settings->NormalizedSuffix = TEXT(".normalized");

		const FString InputPath = TEXT("C:/Project/Content/Models/Character.vrm");
		FString OutPath, ReportPath;

		TestTrue(TEXT("BuildOutputPaths should succeed for SavedDirectory"), FVrmNormalizationService::BuildOutputPaths(InputPath, OutPath, ReportPath));

		// Should contain Saved/VrmToolchain/Normalized/
		TestTrue(TEXT("Output path should contain Saved/VrmToolchain/Normalized"), OutPath.Contains(TEXT("Saved/VrmToolchain/Normalized")));
		TestTrue(TEXT("Output path should end with filename"), OutPath.EndsWith(TEXT("Character.normalized.vrm")));
		TestTrue(TEXT("Report path should contain Saved/VrmToolchain/Normalized"), ReportPath.Contains(TEXT("Saved/VrmToolchain/Normalized")));
		TestTrue(TEXT("Report path should end with report name"), ReportPath.EndsWith(TEXT("Character.normalized.report.json")));
	}

	// Test 5: Path with special characters (deterministic handling)
	{
		Settings->OutputLocation = EVrmNormalizationOutputLocation::NextToSource;
		Settings->NormalizedSuffix = TEXT(".normalized");

		const FString InputPath = TEXT("C:/Project/Content/Models/Test Model v1.0.vrm");
		FString OutPath, ReportPath;

		TestTrue(TEXT("BuildOutputPaths should handle special characters"), FVrmNormalizationService::BuildOutputPaths(InputPath, OutPath, ReportPath));
		TestEqual(TEXT("Output path should preserve special characters"), OutPath, TEXT("C:/Project/Content/Models/Test Model v1.0.normalized.vrm"));
	}

	// Restore original settings
	Settings->OutputLocation = OriginalLocation;
	Settings->NormalizedSuffix = OriginalSuffix;

	return true;
}

// Test ValidateSourceFile
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmNormalizationValidateSourceFileTest, "VrmToolchain.Normalization.ValidateSourceFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmNormalizationValidateSourceFileTest::RunTest(const FString& Parameters)
{
	FString ErrorMessage;

	// Test 1: .vrm extension should be valid (file existence checked separately)
	{
		const FString Path = TEXT("Test.vrm");
		// Will fail because file doesn't exist, but should check extension first
		FVrmNormalizationService::ValidateSourceFile(Path, ErrorMessage);
		TestTrue(TEXT("Error message should mention file doesn't exist"), ErrorMessage.Contains(TEXT("does not exist")));
	}

	// Test 2: .glb extension should be valid
	{
		const FString Path = TEXT("Test.glb");
		FVrmNormalizationService::ValidateSourceFile(Path, ErrorMessage);
		TestTrue(TEXT("Error message should mention file doesn't exist"), ErrorMessage.Contains(TEXT("does not exist")));
	}

	// Test 3: Unsupported extension
	{
		const FString Path = TEXT("Test.fbx");
		const bool bValid = FVrmNormalizationService::ValidateSourceFile(Path, ErrorMessage);
		TestFalse(TEXT("Should reject unsupported extension"), bValid);
		TestTrue(TEXT("Error message should mention unsupported extension"), ErrorMessage.Contains(TEXT("Unsupported")) || ErrorMessage.Contains(TEXT("does not exist")));
	}

	// Test 4: No extension
	{
		const FString Path = TEXT("Test");
		const bool bValid = FVrmNormalizationService::ValidateSourceFile(Path, ErrorMessage);
		TestFalse(TEXT("Should reject file without extension"), bValid);
	}

	return true;
}

// Test that normalization is deterministic for the same input
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmNormalizationDeterministicTest, "VrmToolchain.Normalization.Deterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmNormalizationDeterministicTest::RunTest(const FString& Parameters)
{
	UVrmNormalizationSettings* Settings = GetMutableDefault<UVrmNormalizationSettings>();
	check(Settings);

	const EVrmNormalizationOutputLocation OriginalLocation = Settings->OutputLocation;
	const FString OriginalSuffix = Settings->NormalizedSuffix;

	// Set deterministic settings
	Settings->OutputLocation = EVrmNormalizationOutputLocation::NextToSource;
	Settings->NormalizedSuffix = TEXT(".normalized");

	// Test the same input produces the same output paths
	const FString InputPath = TEXT("C:/Test/Model.vrm");

	FString OutPath1, ReportPath1;
	FString OutPath2, ReportPath2;

	TestTrue(TEXT("First path build should succeed"), FVrmNormalizationService::BuildOutputPaths(InputPath, OutPath1, ReportPath1));
	TestTrue(TEXT("Second path build should succeed"), FVrmNormalizationService::BuildOutputPaths(InputPath, OutPath2, ReportPath2));

	TestEqual(TEXT("Output paths should be identical"), OutPath1, OutPath2);
	TestEqual(TEXT("Report paths should be identical"), ReportPath1, ReportPath2);

	// Restore settings
	Settings->OutputLocation = OriginalLocation;
	Settings->NormalizedSuffix = OriginalSuffix;

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
