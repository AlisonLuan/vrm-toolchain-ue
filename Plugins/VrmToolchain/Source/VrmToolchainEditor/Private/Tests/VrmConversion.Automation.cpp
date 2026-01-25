#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "VrmConversionService.h"
#include "VrmGltfParser.h"
#include "VrmSourceAsset.h"
#include "Engine/SkeletalMesh.h"
#if WITH_EDITOR
#include "ReferenceSkeleton.h"
#endif
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

// Editor-only test: parse and apply skeleton to generated placeholder assets
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

#if WITH_EDITOR
		// Build an explicit skeleton structure programmatically (same as JSON above)
		FVrmGltfSkeleton ApplySkel;
		{
			FVrmGltfBone Root;
			Root.Name = FName(TEXT("Hips"));
			Root.ParentIndex = INDEX_NONE;
			Root.LocalTransform = FTransform::Identity;
			ApplySkel.Bones.Add(Root);

			FVrmGltfBone Child;
			Child.Name = FName(TEXT("Spine"));
			Child.ParentIndex = 0;
			Child.LocalTransform = FTransform(FRotator::ZeroRotator, FVector(0,10,0));
			ApplySkel.Bones.Add(Child);
		}

		// Create transient source & generated placeholders
		UPackage* Pkg2 = CreatePackage(TEXT("/Game/TestAssets/Test_VrmSource_Apply"));
		UVrmSourceAsset* Source2 = NewObject<UVrmSourceAsset>(Pkg2, TEXT("Test_VrmSource_Apply"), RF_Public | RF_Standalone);
		UVrmMetadataAsset* Desc2 = NewObject<UVrmMetadataAsset>(Pkg2, TEXT("Test_Metadata_Apply"), RF_Public | RF_Standalone);
		Source2->Descriptor = Desc2;

		USkeletalMesh* MeshOut = nullptr;
		USkeleton* SkeletonOut = nullptr;
		FVrmConvertOptions Opts;
		Opts.bOverwriteExisting = true;
		FString ConvErr;
		bool bConv = FVrmConversionService::ConvertSourceToPlaceholderSkeletalMesh(Source2, Opts, MeshOut, SkeletonOut, ConvErr);
		TestTrue(TEXT("Placeholder creation succeeded"), bConv);
		TestNotNull(TEXT("MeshOut created"), MeshOut);
		TestNotNull(TEXT("SkeletonOut created"), SkeletonOut);

		// Apply GLTF skeleton to generated assets
		FString ApplyErr;
		bool bApplied = FVrmConversionService::ApplyGltfSkeletonToAssets(ApplySkel, SkeletonOut, MeshOut, ApplyErr);
		TestTrue(TEXT("ApplyGltfSkeletonToAssets should succeed in editor"), bApplied);

		const FReferenceSkeleton& Ref = MeshOut->GetRefSkeleton();
		TestEqual(TEXT("Ref bone count matches"), Ref.GetNum(), ApplySkel.Bones.Num());
		for (int32 I = 0; I < ApplySkel.Bones.Num(); ++I)
		{
			TestEqual(FString::Printf(TEXT("Bone name %d"), I), Ref.GetBoneName(I).ToString(), ApplySkel.Bones[I].Name.ToString());
		}
#endif
	}

	return true;
}

#endif // WITH_AUTOMATION_TESTS
