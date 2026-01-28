#include "VrmConversionService.h"
#include "VrmToolchain/VrmSourceAsset.h"
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
#include "VrmGlbAccessorReader.h"
#include "VrmSkeletalMeshBuilder.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
// Editor-only APIs are needed for applying skeletons in a follow-up PR; keep includes minimal here
#include "ReferenceSkeleton.h"
#include "Animation/Skeleton.h"
#include "EditorFramework/AssetImportData.h"
#endif

#include "VrmGltfParser.h"


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

	// Derive base name from object name
	// For backward compatibility, strip trailing _VrmSource if present (legacy naming)
	// New assets use package name directly (no suffix)
	OutBaseName = ObjectName;
	const FString LegacySuffix = TEXT("_VrmSource");
	if (OutBaseName.EndsWith(LegacySuffix))
	{
		OutBaseName = OutBaseName.LeftChop(LegacySuffix.Len());
	}

	OutFolderPath = FolderPath / (OutBaseName + TEXT("_Generated"));
	return true;
}

// NOTE: ApplyGltfBonesToGeneratedAssets was removed from this PR to avoid editor-only
// include resolution issues during packaging CI. The parser/types remain and are still
// useful for B2 implementation (application of bones will be added in a follow-up PR).

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
			if (ARM.Get().GetAssetByObjectPath(FSoftObjectPath(SkeletonPackageName + TEXT(".") + SkeletonName)).IsValid() ||
				ARM.Get().GetAssetByObjectPath(FSoftObjectPath(MeshPackageName + TEXT(".") + MeshName)).IsValid())
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

		if (UVrmMetadataAsset* Desc = Source->Descriptor)
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

	// B1.1: Apply glTF skeleton by default when possible (fail-soft with warnings)
	if (Options.bApplyGltfSkeleton)
	{
		auto ResolveSourcePath = [](UVrmSourceAsset* InSource, FString& OutPath) -> bool
		{
			OutPath.Reset();
			if (!InSource)
			{
				return false;
			}

			if (!InSource->SourceFilename.IsEmpty())
			{
				OutPath = InSource->SourceFilename;
				return true;
			}

			if (InSource->AssetImportData)
			{
				const FString First = InSource->AssetImportData->GetFirstFilename();
				if (!First.IsEmpty())
				{
					OutPath = First;
					return true;
				}
			}

			return false;
		};

		FString SourcePath;
		if (!ResolveSourcePath(Source, SourcePath))
		{
			Source->ImportWarnings.Add(TEXT("B1.1: Skeleton not applied (no source path resolved)."));
		}
		else
		{
			FVrmGltfSkeleton GltfSkel;
			FString ParseError;

			if (!FVrmGltfParser::ExtractSkeletonFromGlbFile(SourcePath, GltfSkel, ParseError))
			{
				Source->ImportWarnings.Add(FString::Printf(TEXT("B1.1: Skeleton not applied (parse failed): %s"), *ParseError));
			}
			else if (GltfSkel.Bones.Num() == 0)
			{
				Source->ImportWarnings.Add(TEXT("B1.1: Skeleton not applied (zero bones)."));
			}
			else
			{
				FString ApplyError;
				if (!FVrmConversionService::ApplyGltfSkeletonToAssets(GltfSkel, NewSkeleton, NewMesh, ApplyError))
				{
					Source->ImportWarnings.Add(FString::Printf(TEXT("B1.1: Skeleton not applied (apply failed): %s"), *ApplyError));
				}
			}
		}

		Source->MarkPackageDirty();
	}

	// B2: Build actual skinned mesh geometry using MeshUtilities
	if (Options.bApplyGltfSkeleton)
	{
		auto ResolveSourcePath = [](UVrmSourceAsset* InSource, FString& OutPath) -> bool
		{
			OutPath.Reset();
			if (!InSource)
			{
				return false;
			}

			if (!InSource->SourceFilename.IsEmpty())
			{
				OutPath = InSource->SourceFilename;
				return true;
			}

			if (InSource->AssetImportData)
			{
				const FString First = InSource->AssetImportData->GetFirstFilename();
				if (!First.IsEmpty())
				{
					OutPath = First;
					return true;
				}
			}

			return false;
		};

		FString SourcePath;
		if (!ResolveSourcePath(Source, SourcePath))
		{
			Source->ImportWarnings.Add(TEXT("B2: Mesh not built (no source path resolved)."));
		}
		else
		{
			// Load and decode GLB data
			FVrmGlbAccessorReader AccessorReader;
			FString JsonString;
			
			FVrmGlbAccessorReader::FDecodeResult LoadResult = AccessorReader.LoadGlbFile(SourcePath, JsonString);
			if (!LoadResult.bSuccess)
			{
				Source->ImportWarnings.Add(FString::Printf(TEXT("B2: Mesh not built (GLB load failed): %s"), *LoadResult.ErrorMessage));
			}
			else
			{
				FVrmGlbAccessorReader::FDecodeResult DecodeResult = AccessorReader.DecodeAccessors(JsonString);
				if (!DecodeResult.bSuccess)
				{
					Source->ImportWarnings.Add(FString::Printf(TEXT("B2: Mesh not built (accessor decode failed): %s"), *DecodeResult.ErrorMessage));
				}
				else
				{
					// Compute joint ordinal to bone index mapping
					TMap<int32, int32> JointOrdinalToBoneIndex;
					
					// Parse JSON to extract skin joints
					TSharedPtr<FJsonObject> RootObject;
					TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
					
					if (FJsonSerializer::Deserialize(Reader, RootObject) && RootObject.IsValid())
					{
						TArray<int32> SkinJoints;
						if (FVrmGltfParser::TryExtractSkin0Joints(RootObject, SkinJoints))
						{
							// Get the skeleton we applied to build node index to bone index mapping
							FVrmGltfSkeleton GltfSkel;
							FString ParseError;
							if (FVrmGltfParser::ExtractSkeletonFromGlbFile(SourcePath, GltfSkel, ParseError))
							{
								// Build node index to bone index mapping
								TMap<int32, int32> NodeToBoneIndex;
								for (int32 BoneIndex = 0; BoneIndex < GltfSkel.Bones.Num(); ++BoneIndex)
								{
									NodeToBoneIndex.Add(GltfSkel.Bones[BoneIndex].GltfNodeIndex, BoneIndex);
								}
								
								// Map skin joint node indices to bone indices
								for (int32 JointOrdinal = 0; JointOrdinal < SkinJoints.Num(); ++JointOrdinal)
								{
									int32 NodeIndex = SkinJoints[JointOrdinal];
									const int32* BoneIndexPtr = NodeToBoneIndex.Find(NodeIndex);
									if (BoneIndexPtr)
									{
										JointOrdinalToBoneIndex.Add(JointOrdinal, *BoneIndexPtr);
									}
								}
							}
						}
					}
					
					if (JointOrdinalToBoneIndex.Num() == 0)
					{
						Source->ImportWarnings.Add(TEXT("B2: Mesh not built (no joint mapping available)."));
					}
					else
					{
						// Build the mesh
						FVrmSkeletalMeshBuilder::FBuildResult BuildResult = FVrmSkeletalMeshBuilder::BuildLod0SkinnedPrimitive(
							AccessorReader, NewSkeleton, JointOrdinalToBoneIndex, MeshPackageName, MeshName);
							
						if (!BuildResult.bSuccess)
						{
							Source->ImportWarnings.Add(FString::Printf(TEXT("B2: Mesh build failed: %s"), *BuildResult.ErrorMessage));
						}
						else
						{
							// Replace the placeholder mesh with the built mesh
							NewMesh = BuildResult.BuiltMesh;
							OutSkeletalMesh = NewMesh;
						}
					}
				}
			}
		}
	}

	OutSkeletalMesh = NewMesh;
	OutSkeleton = NewSkeleton;

	return true;
}

bool FVrmConversionService::ApplyGltfSkeletonToAssets(const FVrmGltfSkeleton& GltfSkel, USkeleton* TargetSkeleton, USkeletalMesh* TargetMesh, FString& OutError)
{
#if WITH_EDITOR
	OutError.Reset();
	if (!TargetSkeleton || !TargetMesh)
	{
		OutError = TEXT("TargetSkeleton or TargetMesh is null");
		return false;
	}

	if (GltfSkel.Bones.Num() == 0)
	{
		OutError = TEXT("GLTF skeleton contains no bones");
		return false;
	}

	// Modify the mesh's reference skeleton in-place using the modifier, then sync to the USkeleton
	FReferenceSkeleton& MeshRefSkel = TargetMesh->GetRefSkeleton();
	FReferenceSkeletonModifier RefSkelModifier(MeshRefSkel, TargetSkeleton);

	// Add bones in order from the parsed GLTF skeleton
	for (int32 Index = 0; Index < GltfSkel.Bones.Num(); ++Index)
	{
		const FVrmGltfBone& Bone = GltfSkel.Bones[Index];
		FName BoneName = Bone.Name;
		FName ParentName = NAME_None;
		if (Bone.ParentIndex != INDEX_NONE && GltfSkel.Bones.IsValidIndex(Bone.ParentIndex))
		{
			ParentName = GltfSkel.Bones[Bone.ParentIndex].Name;
		}

		// FMeshBoneInfo: (Name, ExportName, ParentIndex)
		FMeshBoneInfo BoneInfo(BoneName, BoneName.ToString(), Bone.ParentIndex);
		RefSkelModifier.Add(BoneInfo, Bone.LocalTransform);
	}

	// Ensure the skeleton asset is aware of bones added to the mesh
	TargetSkeleton->MergeAllBonesToBoneTree(TargetMesh);

	return true;
#else
	OutError = TEXT("ApplyGltfSkeletonToAssets is editor-only");
	return false;
#endif
}
 
