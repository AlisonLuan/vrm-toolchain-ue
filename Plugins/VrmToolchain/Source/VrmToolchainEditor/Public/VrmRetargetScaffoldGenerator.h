// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

class USkeletalMesh;
class USkeleton;
class UIKRigDefinition;
class UIKRetargeter;

/**
 * Helper class to generate IKRig and IK Retargeter scaffolding for VRM assets.
 * Creates deterministic, idempotent retargeting assets based on skeleton structure.
 */
class VRMTOOLCHAINEDITOR_API FVrmRetargetScaffoldGenerator
{
public:
	/**
	 * Describes which bones were found during chain inference
	 */
	struct FChainInfo
	{
		FName ChainName;
		FName StartBone;
		FName EndBone;
		bool bIsValid;

		FChainInfo() : bIsValid(false) {}
		FChainInfo(FName InChainName, FName InStartBone, FName InEndBone)
			: ChainName(InChainName), StartBone(InStartBone), EndBone(InEndBone), bIsValid(true)
		{}
	};

	/**
	 * Result of scaffold generation
	 */
	struct FScaffoldResult
	{
		bool bSuccess;
		FString ErrorMessage;
		TArray<FString> Warnings;
		UIKRigDefinition* SourceIKRig;
		UIKRigDefinition* TargetIKRig;
		UIKRetargeter* Retargeter;

		FScaffoldResult() : bSuccess(false), SourceIKRig(nullptr), TargetIKRig(nullptr), Retargeter(nullptr) {}
	};

	/**
	 * Generate IKRig and IK Retargeter scaffolding for a VRM skeletal mesh
	 * @param SourceMesh The VRM skeletal mesh to create retargeting for
	 * @param TargetSkeleton The target skeleton to retarget to (can be null to skip target IKRig creation)
	 * @param bCreateTargetIKRig Whether to create a target IKRig if it doesn't exist
	 * @return Result structure with created assets and any warnings/errors
	 */
	static FScaffoldResult GenerateRetargetScaffolding(
		USkeletalMesh* SourceMesh,
		USkeleton* TargetSkeleton,
		bool bCreateTargetIKRig = false);

	/**
	 * Infer bone chains from a skeleton (deterministic, case-insensitive matching)
	 * @param Skeleton The skeleton to analyze
	 * @param OutChains Array of inferred chains
	 */
	static void InferBoneChains(const USkeleton* Skeleton, TArray<FChainInfo>& OutChains);

private:
	/**
	 * Find a bone by name (case-insensitive, handles common naming variants)
	 */
	static FName FindBoneByName(const TArray<FName>& BoneNames, const TArray<FString>& SearchNames);

	/**
	 * Find the root bone (pelvis or hips)
	 */
	static FName FindRootBone(const TArray<FName>& BoneNames);

	/**
	 * Find spine chain bones
	 */
	static bool FindSpineChain(const TArray<FName>& BoneNames, FName& OutStart, FName& OutEnd);

	/**
	 * Find arm chain bones (left or right)
	 */
	static bool FindArmChain(const TArray<FName>& BoneNames, bool bLeftSide, FName& OutStart, FName& OutEnd);

	/**
	 * Find leg chain bones (left or right)
	 */
	static bool FindLegChain(const TArray<FName>& BoneNames, bool bLeftSide, FName& OutStart, FName& OutEnd);

	/**
	 * Find head/neck bone
	 */
	static FName FindHeadBone(const TArray<FName>& BoneNames);

	/**
	 * Create or update an IKRig asset
	 */
	static UIKRigDefinition* CreateOrUpdateIKRig(
		const FString& AssetName,
		const FString& PackagePath,
		USkeleton* Skeleton,
		const TArray<FChainInfo>& Chains,
		TArray<FString>& OutWarnings);

	/**
	 * Create or update an IK Retargeter asset
	 */
	static UIKRetargeter* CreateOrUpdateRetargeter(
		const FString& AssetName,
		const FString& PackagePath,
		UIKRigDefinition* SourceIKRig,
		UIKRigDefinition* TargetIKRig,
		TArray<FString>& OutWarnings);

	/**
	 * Get a deterministic output path for retarget assets
	 */
	static FString GetRetargetOutputPath(USkeletalMesh* SourceMesh);
};
