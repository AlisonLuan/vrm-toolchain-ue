#include "VrmConversionService.h"
#include "VrmSourceAsset.h"
#include "VrmToolchain/VrmMetadataAsset.h"
#include "VrmToolchain/VrmMetadata.h"
#include "VrmToolchainEditor.h"
#include "VrmSdkFacadeEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "ObjectTools.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "Engine/SkeletalMesh.h"
#include "VrmGltfParser.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
// Try both include styles; some build environments resolve one or the other
#include "ReferenceSkeleton.h"
#include "Animation/ReferenceSkeleton.h"
#include "ReferenceSkeletonModifier.h"
#include "Animation/ReferenceSkeletonModifier.h"
#include "Engine/Skeleton.h"
#endif


bool FVrmConversionService::DeriveGeneratedPaths(UVrmSourceAsset* Source, FString& OutFolderPath, FString& OutBaseName, FString& OutError)
{
	OutFolderPath.Reset();
	OutBaseName.Reset();

	if (!Source)
	{
		OutError = TEXT("Source is null");
		return false;
	}

	FString PackagePath = Source->GetOutermost()->GetName();
	// Normalize package path like /Game/Folder/Name
	FString FolderPath;
	FString ObjectName = Source->GetName();

	// If the source package is under /Game, strip the object name
	if (PackagePath.StartsWith(TEXT("/Game/")))
	{
		int32 LastSlash = INDEX_NONE;
		PackagePath.FindLastChar('/', LastSlash);
		if (LastSlash != INDEX_NONE)
		{
			FolderPath = PackagePath.Left(LastSlash);
		}
		else
		{
			FolderPath = PackagePath;
		}
	}
	else
	{
		// Fallback to /Game/Generated
		FolderPath = TEXT("/Game/Generated");
	}

	// Derive base name from object name, strip trailing _VrmSource if present
	OutBaseName = ObjectName;
	const FString Suffix = TEXT("_VrmSource");
	if (OutBaseName.EndsWith(Suffix))
	{
		OutBaseName = OutBaseName.LeftChop(Suffix.Len());
	}

	OutFolderPath = FolderPath / (OutBaseName + TEXT("_Generated"));
	return true;
}

// Editor-only helper: apply parsed GLTF skeleton to generated assets (mutates mesh ref-skeleton in-place)
static bool ApplyGltfBonesToGeneratedAssets(
	const FVrmGltfSkeleton& GltfSkel,
	USkeleton* TargetSkeleton,
	USkeletalMesh* TargetMesh,
	FString& OutError)
{
	OutError.Reset();

	if (!TargetSkeleton || !TargetMesh)
	{
		OutError = TEXT("TargetSkeleton/TargetMesh is null.");
		return false;
	}

	if (GltfSkel.Bones.Num() == 0)
	{
		OutError = TEXT("GLTF skeleton had zero bones.");
		return false;
	}

	// Mutate mesh's ref skeleton in-place (editor-only).
	FReferenceSkeleton& RefSkel = const_cast<FReferenceSkeleton&>(TargetMesh->GetRefSkeleton());

	// Overwrite semantics: clear then add
	RefSkel = FReferenceSkeleton();

	FReferenceSkeletonModifier Modifier(RefSkel, *TargetSkeleton);

	for (int32 i = 0; i < GltfSkel.Bones.Num(); ++i)
	{
		const FVrmGltfBone& B = GltfSkel.Bones[i];

		const FName BoneName = B.Name.IsNone() ? FName(*FString::Printf(TEXT("node_%d"), i)) : B.Name;

		int32 ParentIndex = B.ParentIndex;
		if (ParentIndex < 0 || ParentIndex >= RefSkel.GetNum())
		{
			ParentIndex = INDEX_NONE;
		}

		FMeshBoneInfo BoneInfo(BoneName, BoneName.ToString(), ParentIndex);
		Modifier.Add(BoneInfo, B.LocalTransform);
	}

	// Sync the USkeleton from the mesh
	TargetSkeleton->MergeAllBonesToBoneTree(TargetMesh);

	// Ensure the mesh points at the skeleton
	TargetMesh->SetSkeleton(TargetSkeleton);

	TargetSkeleton->MarkPackageDirty();
	TargetMesh->MarkPackageDirty();

#if WITH_EDITOR
	TargetSkeleton->PostEditChange();
	TargetMesh->PostEditChange();
#endif

	return true;
}

bool FVrmConversionService::ConvertSourceToPlaceholderSkeletalMesh(UVrmSourceAsset* Source, const FVrmConvertOptions& Options, USkeletalMesh*& OutSkeletalMesh, USkeleton*& OutSkeleton, FString& OutError)
{
	OutSkeletalMesh = nullptr;
	OutSkeleton = nullptr;
	OutError.Reset();

	if (!Source)
	{
		OutError = TEXT("Source is null");
		return false;
	}

	FString FolderPath;
	FString BaseName;
	if (!DeriveGeneratedPaths(Source, FolderPath, BaseName, OutError))
	{
		return false;
	}

	// Asset names
	FString SkeletonName = BaseName + TEXT("_Skeleton");
	FString MeshName = BaseName + TEXT("_SK");

	// Full package names
	FString SkeletonPackageName = FolderPath + TEXT("/") + SkeletonName;
	FString MeshPackageName = FolderPath + TEXT("/") + MeshName;

	// Check existing
	{
		FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		if (!Options.bOverwriteExisting)
		{
			if (ARM.Get().GetAssetByObjectPath(*FString::Printf(TEXT("%s.%s"), *SkeletonPackageName, *SkeletonName)).IsValid() ||
				ARM.Get().GetAssetByObjectPath(*FString::Printf(TEXT("%s.%s"), *MeshPackageName, *MeshName)).IsValid())
			{
				OutError = TEXT("Target assets already exist");
				return false;
			}
		}
	}

	// Ensure folder exists via AssetTools
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	AssetToolsModule.Get().CreateUniqueAssetName(SkeletonPackageName, TEXT(""), SkeletonPackageName, SkeletonName);
	AssetToolsModule.Get().CreateUniqueAssetName(MeshPackageName, TEXT(""), MeshPackageName, MeshName);

	// Convert package names to package objects
	UPackage* SkeletonPackage = CreatePackage(*SkeletonPackageName);
	UPackage* MeshPackage = CreatePackage(*MeshPackageName);

	if (!SkeletonPackage || !MeshPackage)
	{
		OutError = TEXT("Failed to create packages");
		return false;
	}

	// Create skeleton and skeletal mesh
	USkeleton* NewSkeleton = NewObject<USkeleton>(SkeletonPackage, *SkeletonName, RF_Public | RF_Standalone);
	USkeletalMesh* NewMesh = NewObject<USkeletalMesh>(MeshPackage, *MeshName, RF_Public | RF_Standalone);

	if (!NewSkeleton || !NewMesh)
	{
		OutError = TEXT("Failed to create skeleton or mesh objects");
		return false;
	}

	// Register assets with AssetRegistry
	FAssetRegistryModule::AssetCreated(NewSkeleton);
	FAssetRegistryModule::AssetCreated(NewMesh);

	// Mark packages dirty
	NewSkeleton->MarkPackageDirty();
	NewMesh->MarkPackageDirty();

	// Attach metadata (canonical path: mesh-owned UAssetUserData via facade)
	{
		FVrmMetadata Parsed;

		if (UVrmMetadataAsset* Desc = Source->Descriptor.Get())
		{
			Parsed.Version = Desc->SpecVersion;
			Parsed.Name = Desc->Metadata.Title;
			Parsed.ModelVersion = Desc->Metadata.Version;

			if (!Desc->Metadata.Author.IsEmpty())
			{
				Parsed.Authors = { Desc->Metadata.Author };
			}

			Parsed.License = Desc->Metadata.LicenseName;
		}

		FVrmSdkFacadeEditor::UpsertVrmMetadata(NewMesh, Parsed);
	}

	// Add provenance note: (UPackage does not expose SetMetaData; skip explicit package metadata write)
	// Consider attaching a dedicated UAssetUserData if persistent provenance is required later.

	// Optionally parse GLB/Vrm source and apply bone hierarchy to generated assets
	if (Options.bApplyGltfSkeleton)
	{
		FString SourceFilePath = Source->SourceFilename;
		if (SourceFilePath.IsEmpty() && Source->AssetImportData && Source->AssetImportData->GetSourceData().SourceFiles.Num() > 0)
		{
			SourceFilePath = Source->AssetImportData->GetSourceData().SourceFiles[0].AbsoluteFilename;
		}

		if (SourceFilePath.IsEmpty())
		{
			OutError = TEXT("No source file path available to extract skeleton from.");
			return false;
		}

		FString Ext = FPaths::GetExtension(SourceFilePath).ToLower();
		if (Ext != TEXT("glb") && Ext != TEXT("vrm"))
		{
			OutError = TEXT("Source file is not a GLB/VRM file.");
			return false;
		}

		FVrmGltfSkeleton GltfSkel;
		FString ParseError;
		if (!FVrmGltfParser::ExtractSkeletonFromGlbFile(SourceFilePath, GltfSkel, ParseError))
		{
			OutError = FString::Printf(TEXT("Failed to parse GLB skeleton: %s"), *ParseError);
			return false;
		}

		FString ApplyError;
		if (!ApplyGltfBonesToGeneratedAssets(GltfSkel, NewSkeleton, NewMesh, ApplyError))
		{
			OutError = FString::Printf(TEXT("Failed to apply GLTF skeleton: %s"), *ApplyError);
			return false;
		}
	}

	OutSkeletalMesh = NewMesh;
	OutSkeleton = NewSkeleton;

	return true;
}

bool FVrmConversionService::ApplyGltfSkeletonToAssets(const FVrmGltfSkeleton& GltfSkel, USkeleton* TargetSkeleton, USkeletalMesh* TargetMesh, FString& OutError)
{
	return ApplyGltfBonesToGeneratedAssets(GltfSkel, TargetSkeleton, TargetMesh, OutError);
}
 
