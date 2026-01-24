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
class USkeleton;
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"

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

	OutSkeletalMesh = NewMesh;
	OutSkeleton = NewSkeleton;

	return true;
} 
