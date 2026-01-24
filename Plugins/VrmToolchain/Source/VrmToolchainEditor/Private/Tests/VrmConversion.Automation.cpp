#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "VrmConversionService.h"
#include "VrmSourceAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Skeleton.h"
#include "VrmToolchain/VrmMetadataAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmConversionPathDerivationTest, "VrmToolchain.Conversion.PathDerivation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmConversionPathDerivationTest::RunTest(const FString& Parameters)
{
	// Create a transient source asset in a /Game/ path by creating a package
	UPackage* Pkg = CreatePackage(TEXT("/Game/TestAssets/Test_VrmSource"));
	UVrmSourceAsset* Source = NewObject<UVrmSourceAsset>(Pkg, TEXT("Test_VrmSource"), RF_Public | RF_Standalone);
	TestNotNull(TEXT("Source created"), Source);

	FString FolderPath, BaseName, Error;
	bool bOk = false;
	// Derive using the public Convert function indirectly
	USkeletalMesh* Mesh = nullptr;
	USkeleton* Skeleton = nullptr;
	FVrmConvertOptions Options;
	Options.bOverwriteExisting = true;

	bOk = FVrmConversionService::ConvertSourceToPlaceholderSkeletalMesh(Source, Options, Mesh, Skeleton, Error);

	TestTrue(TEXT("Conversion should succeed for transient source"), bOk);
	TestNotNull(TEXT("Mesh created"), Mesh);
	TestNotNull(TEXT("Skeleton created"), Skeleton);

	// Names and packages
	TestEqual(TEXT("Mesh name should match pattern"), Mesh->GetName(), FString(TEXT("Test_SK")));
	TestEqual(TEXT("Skeleton name should match pattern"), Skeleton->GetName(), FString(TEXT("Test_Skeleton")));

	FString MeshPkg = Mesh->GetOutermost()->GetName();
	TestTrue(TEXT("Mesh package should be in Generated folder"), MeshPkg.Contains(TEXT("/Game/TestAssets/Test_Generated")));

	// Re-run without overwrite should return error
	USkeletalMesh* Mesh2 = nullptr;
	USkeleton* Skeleton2 = nullptr;
	Options.bOverwriteExisting = false;
	bool bOk2 = FVrmConversionService::ConvertSourceToPlaceholderSkeletalMesh(Source, Options, Mesh2, Skeleton2, Error);
	TestFalse(TEXT("Conversion without overwrite should fail when assets exist"), bOk2);

	return true;
}

#endif // WITH_AUTOMATION_TESTS
