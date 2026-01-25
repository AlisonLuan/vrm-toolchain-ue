#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "VrmConversionService.h"
#include "VrmGltfParser.h"
#include "VrmSourceAsset.h"
#include "Engine/SkeletalMesh.h"
class USkeleton;
#include "VrmToolchain/VrmMetadataAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmConversionPathDerivationTest, "VrmToolchain.Conversion.PathDerivation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmConversionPathDerivationTest::RunTest(const FString& Parameters)
{
	// Create a transient source asset in a /Game/ path by creating a package
	UPackage* Pkg = CreatePackage(TEXT("/Game/TestAssets/Test_VrmSource"));
	UVrmSourceAsset* Source = NewObject<UVrmSourceAsset>(Pkg, TEXT("Test_VrmSource"), RF_Public | RF_Standalone);
	TestNotNull(TEXT("Source created"), Source);

	// Create and attach a descriptor metadata asset to the source so conversion attaches it
	UVrmMetadataAsset* Desc = NewObject<UVrmMetadataAsset>(Pkg, TEXT("Test_Metadata"), RF_Public | RF_Standalone);
	TestNotNull(TEXT("Source descriptor created"), Desc);
	Desc->Metadata.Title = TEXT("Test Model");
	Desc->Metadata.Version = TEXT("1.0");
	Desc->Metadata.Author = TEXT("Author A");
	Desc->Metadata.LicenseName = TEXT("MIT");
	Desc->SpecVersion = EVrmVersion::VRM1;

	Source->Descriptor = Desc;

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

	// Metadata was attached
	UVrmMetadataAsset* Attached = Mesh->GetAssetUserData<UVrmMetadataAsset>();
	TestNotNull(TEXT("Metadata user data attached"), Attached);

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

	// Test applying a simple GLTF skeleton via JSON string
	{
		// Minimal GLTF JSON with two nodes: root (0) and child (1)
		FString Json = R"({
		  "nodes": [
		    { "name": "Hips", "translation": [0,0,0], "children": [1] },
		    { "name": "Spine", "translation": [0,10,0] }
		  ]
		})";

		FVrmGltfSkeleton Skel;
		FString ParseErr;
		bool bParsed = FVrmGltfParser::ExtractSkeletonFromGltfJsonString(Json, Skel, ParseErr);
		TestTrue(TEXT("Parse GLTF JSON string succeeds"), bParsed);
		TestEqual(TEXT("Parsed bone count"), Skel.Bones.Num(), 2);

		// Create fresh generated assets to apply to
		USkeletalMesh* MeshA = nullptr;
		USkeleton* SkeletonA = nullptr;
		FVrmConvertOptions Opts;
		Opts.bOverwriteExisting = true;
		FVrmConvertOptions TmpOptions = Opts;
		// Use the existing source to create assets
		bool bConv = FVrmConversionService::ConvertSourceToPlaceholderSkeletalMesh(Source, TmpOptions, MeshA, SkeletonA, Error);
		TestTrue(TEXT("Created placeholder assets for apply test"), bConv);
		TestNotNull(TEXT("MeshA not null"), MeshA);
		TestNotNull(TEXT("SkeletonA not null"), SkeletonA);

		FString ApplyErr;
		bool bApplied = FVrmConversionService::ApplyGltfSkeletonToAssets(Skel, SkeletonA, MeshA, ApplyErr);
		TestTrue(TEXT("Apply GLTF skeleton succeeds"), bApplied);
		TestEqual(TEXT("Ref skeleton bone count matches"), MeshA->GetRefSkeleton().GetNum(), Skel.Bones.Num());
		TestEqual(TEXT("First bone name"), MeshA->GetRefSkeleton().GetBoneName(0).ToString(), FString(TEXT("Hips")));
		TestEqual(TEXT("Second bone name"), MeshA->GetRefSkeleton().GetBoneName(1).ToString(), FString(TEXT("Spine")));
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
