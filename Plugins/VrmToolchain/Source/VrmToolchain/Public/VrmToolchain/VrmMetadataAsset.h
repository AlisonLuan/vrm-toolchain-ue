#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"
#include "VrmToolchain/VrmMetadata.h"
#include "VrmMetadataAsset.generated.h"

/**
 * Struct holding VRM metadata extracted from the imported VRM file.
 */
USTRUCT(BlueprintType)
struct VRMTOOLCHAIN_API FVrmMetadataRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Metadata")
	FString Title;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Metadata")
	FString Version;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Metadata")
	FString Author;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Metadata")
	FString ContactInformation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Metadata")
	FString Reference;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Metadata")
	FString LicenseName;
};

/**
 * Struct holding minimal skeleton mapping/coverage information.
 */
USTRUCT(BlueprintType)
struct VRMTOOLCHAIN_API FVrmSkeletonCoverage
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Skeleton Coverage")
	int32 TotalBoneCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Skeleton Coverage")
	TArray<FName> SortedBoneNames;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Skeleton Coverage")
	bool bHasHips = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Skeleton Coverage")
	bool bHasSpine = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Skeleton Coverage")
	bool bHasHead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Skeleton Coverage")
	bool bHasLeftHand = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Skeleton Coverage")
	bool bHasRightHand = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Skeleton Coverage")
	bool bHasLeftFoot = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM Skeleton Coverage")
	bool bHasRightFoot = false;
};

/**
 * Asset user data that stores VRM metadata and skeleton coverage information.
 * Attached to SkeletalMesh assets imported via a VRM importer (e.g., VRM4U).
 * 
 * This class is runtime-safe and can be accessed from both runtime and editor code.
 */
UCLASS()
class VRMTOOLCHAIN_API UVrmMetadataAsset : public UAssetUserData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM")
	EVrmVersion SpecVersion = EVrmVersion::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM")
	FVrmMetadataRecord Metadata;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM")
	FVrmSkeletonCoverage SkeletonCoverage;
};
