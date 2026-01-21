// Copyright Epic Games, Inc. All Rights Reserved.

#include "VrmRetargetScaffoldGenerator.h"
#include "VrmToolchainEditor.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "IKRigDefinition.h"
#include "Retargeter/IKRetargeter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "PackageTools.h"
#include "Factories/Factory.h"

// Helper to convert array of names to lowercase strings
static TArray<FString> NamesToLower(const TArray<FName>& Names)
{
	TArray<FString> Result;
	Result.Reserve(Names.Num());
	for (const FName& Name : Names)
	{
		Result.Add(Name.ToString().ToLower());
	}
	return Result;
}

// Case-insensitive string comparison for bone names
static bool MatchesBoneName(const FString& BoneName, const FString& SearchName)
{
	return BoneName.ToLower().Contains(SearchName.ToLower());
}

FName FVrmRetargetScaffoldGenerator::FindBoneByName(const TArray<FName>& BoneNames, const TArray<FString>& SearchNames)
{
	TArray<FString> BoneNamesLower = NamesToLower(BoneNames);
	
	for (const FString& SearchName : SearchNames)
	{
		for (int32 i = 0; i < BoneNamesLower.Num(); ++i)
		{
			if (MatchesBoneName(BoneNamesLower[i], SearchName))
			{
				return BoneNames[i];
			}
		}
	}
	
	return NAME_None;
}

FName FVrmRetargetScaffoldGenerator::FindRootBone(const TArray<FName>& BoneNames)
{
	TArray<FString> RootNames = { TEXT("pelvis"), TEXT("hips"), TEXT("root") };
	return FindBoneByName(BoneNames, RootNames);
}

bool FVrmRetargetScaffoldGenerator::FindSpineChain(const TArray<FName>& BoneNames, FName& OutStart, FName& OutEnd)
{
	// Find root/pelvis as start
	OutStart = FindRootBone(BoneNames);
	if (OutStart == NAME_None)
	{
		return false;
	}

	// Find neck or head as end
	TArray<FString> EndNames = { TEXT("neck"), TEXT("head"), TEXT("chest"), TEXT("upperchest") };
	OutEnd = FindBoneByName(BoneNames, EndNames);
	
	return OutEnd != NAME_None;
}

bool FVrmRetargetScaffoldGenerator::FindArmChain(const TArray<FName>& BoneNames, bool bLeftSide, FName& OutStart, FName& OutEnd)
{
	const FString Prefix = bLeftSide ? TEXT("left") : TEXT("right");
	const FString PrefixAlt = bLeftSide ? TEXT("l_") : TEXT("r_");
	const FString PrefixShort = bLeftSide ? TEXT("l") : TEXT("r");

	// Find shoulder or clavicle as start
	TArray<FString> StartNames = {
		Prefix + TEXT("shoulder"),
		Prefix + TEXT("clavicle"),
		PrefixAlt + TEXT("shoulder"),
		PrefixAlt + TEXT("clavicle"),
		TEXT("shoulder") + TEXT(".") + PrefixShort,
		TEXT("clavicle") + TEXT(".") + PrefixShort
	};
	
	OutStart = FindBoneByName(BoneNames, StartNames);
	
	// If no shoulder, try upperarm
	if (OutStart == NAME_None)
	{
		TArray<FString> ArmNames = {
			Prefix + TEXT("upperarm"),
			Prefix + TEXT("arm"),
			PrefixAlt + TEXT("upperarm"),
			PrefixAlt + TEXT("arm")
		};
		OutStart = FindBoneByName(BoneNames, ArmNames);
	}

	if (OutStart == NAME_None)
	{
		return false;
	}

	// Find hand as end
	TArray<FString> EndNames = {
		Prefix + TEXT("hand"),
		PrefixAlt + TEXT("hand"),
		TEXT("hand") + TEXT(".") + PrefixShort
	};
	OutEnd = FindBoneByName(BoneNames, EndNames);

	return OutEnd != NAME_None;
}

bool FVrmRetargetScaffoldGenerator::FindLegChain(const TArray<FName>& BoneNames, bool bLeftSide, FName& OutStart, FName& OutEnd)
{
	const FString Prefix = bLeftSide ? TEXT("left") : TEXT("right");
	const FString PrefixAlt = bLeftSide ? TEXT("l_") : TEXT("r_");
	const FString PrefixShort = bLeftSide ? TEXT("l") : TEXT("r");

	// Find thigh or upleg as start
	TArray<FString> StartNames = {
		Prefix + TEXT("thigh"),
		Prefix + TEXT("upleg"),
		Prefix + TEXT("upperleg"),
		PrefixAlt + TEXT("thigh"),
		PrefixAlt + TEXT("upleg"),
		TEXT("thigh") + TEXT(".") + PrefixShort
	};
	OutStart = FindBoneByName(BoneNames, StartNames);

	if (OutStart == NAME_None)
	{
		return false;
	}

	// Find foot as end
	TArray<FString> EndNames = {
		Prefix + TEXT("foot"),
		PrefixAlt + TEXT("foot"),
		TEXT("foot") + TEXT(".") + PrefixShort
	};
	OutEnd = FindBoneByName(BoneNames, EndNames);

	return OutEnd != NAME_None;
}

FName FVrmRetargetScaffoldGenerator::FindHeadBone(const TArray<FName>& BoneNames)
{
	TArray<FString> HeadNames = { TEXT("head"), TEXT("neck") };
	return FindBoneByName(BoneNames, HeadNames);
}

void FVrmRetargetScaffoldGenerator::InferBoneChains(const USkeleton* Skeleton, TArray<FChainInfo>& OutChains)
{
	OutChains.Reset();
	
	if (!Skeleton)
	{
		return;
	}

	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	TArray<FName> BoneNames;
	for (int32 i = 0; i < RefSkeleton.GetNum(); ++i)
	{
		BoneNames.Add(RefSkeleton.GetBoneName(i));
	}

	// Spine chain
	{
		FName Start, End;
		if (FindSpineChain(BoneNames, Start, End))
		{
			OutChains.Add(FChainInfo(TEXT("Spine"), Start, End));
		}
	}

	// Head
	{
		FName Head = FindHeadBone(BoneNames);
		if (Head != NAME_None)
		{
			OutChains.Add(FChainInfo(TEXT("Head"), Head, Head));
		}
	}

	// Left arm
	{
		FName Start, End;
		if (FindArmChain(BoneNames, true, Start, End))
		{
			OutChains.Add(FChainInfo(TEXT("LeftArm"), Start, End));
		}
	}

	// Right arm
	{
		FName Start, End;
		if (FindArmChain(BoneNames, false, Start, End))
		{
			OutChains.Add(FChainInfo(TEXT("RightArm"), Start, End));
		}
	}

	// Left leg
	{
		FName Start, End;
		if (FindLegChain(BoneNames, true, Start, End))
		{
			OutChains.Add(FChainInfo(TEXT("LeftLeg"), Start, End));
		}
	}

	// Right leg
	{
		FName Start, End;
		if (FindLegChain(BoneNames, false, Start, End))
		{
			OutChains.Add(FChainInfo(TEXT("RightLeg"), Start, End));
		}
	}
}

FString FVrmRetargetScaffoldGenerator::GetRetargetOutputPath(USkeletalMesh* SourceMesh)
{
	if (!SourceMesh)
	{
		return FString();
	}

	// Get the package path of the source mesh
	FString MeshPath = SourceMesh->GetPathName();
	FString PackagePath = FPackageName::GetLongPackagePath(MeshPath);
	
	// Create a Retarget subfolder
	FString OutputPath = PackagePath / TEXT("Retarget");
	
	return OutputPath;
}

UIKRigDefinition* FVrmRetargetScaffoldGenerator::CreateOrUpdateIKRig(
	const FString& AssetName,
	const FString& PackagePath,
	USkeleton* Skeleton,
	const TArray<FChainInfo>& Chains,
	TArray<FString>& OutWarnings)
{
	if (!Skeleton)
	{
		OutWarnings.Add(TEXT("Cannot create IKRig: Invalid skeleton"));
		return nullptr;
	}

	// Check if asset already exists
	FString FullPackagePath = PackagePath / AssetName;
	UIKRigDefinition* IKRig = LoadObject<UIKRigDefinition>(nullptr, *FullPackagePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	
	if (!IKRig)
	{
		// Create new asset
		UPackage* Package = CreatePackage(*FullPackagePath);
		if (!Package)
		{
			OutWarnings.Add(FString::Printf(TEXT("Failed to create package: %s"), *FullPackagePath));
			return nullptr;
		}
		
		IKRig = NewObject<UIKRigDefinition>(Package, *AssetName, RF_Public | RF_Standalone);
	}

	if (!IKRig)
	{
		OutWarnings.Add(FString::Printf(TEXT("Failed to create IKRig asset: %s"), *AssetName));
		return nullptr;
	}

	// Basic setup - set preview skeletal mesh if available
	USkeletalMesh* PreviewMesh = Skeleton->GetPreviewMesh();
	if (PreviewMesh)
	{
		IKRig->PreviewSkeletalMesh = PreviewMesh;
	}

	// Mark package as dirty
	IKRig->MarkPackageDirty();
	
	UE_LOG(LogVrmToolchainEditor, Display, TEXT("Created/Updated IKRig: %s"), *AssetName);
	
	return IKRig;
}

UIKRetargeter* FVrmRetargetScaffoldGenerator::CreateOrUpdateRetargeter(
	const FString& AssetName,
	const FString& PackagePath,
	UIKRigDefinition* SourceIKRig,
	UIKRigDefinition* TargetIKRig,
	TArray<FString>& OutWarnings)
{
	if (!SourceIKRig || !TargetIKRig)
	{
		OutWarnings.Add(TEXT("Cannot create retargeter: Invalid source or target IKRig"));
		return nullptr;
	}

	// Check if asset already exists
	FString FullPackagePath = PackagePath / AssetName;
	UIKRetargeter* Retargeter = LoadObject<UIKRetargeter>(nullptr, *FullPackagePath, nullptr, LOAD_NoWarn | LOAD_Quiet);
	
	if (!Retargeter)
	{
		// Create new asset
		UPackage* Package = CreatePackage(*FullPackagePath);
		if (!Package)
		{
			OutWarnings.Add(FString::Printf(TEXT("Failed to create package: %s"), *FullPackagePath));
			return nullptr;
		}
		
		Retargeter = NewObject<UIKRetargeter>(Package, *AssetName, RF_Public | RF_Standalone);
	}

	if (!Retargeter)
	{
		OutWarnings.Add(FString::Printf(TEXT("Failed to create retargeter asset: %s"), *AssetName));
		return nullptr;
	}

	// Set source and target IKRigs
	Retargeter->SetSourceIKRig(SourceIKRig);
	Retargeter->SetTargetIKRig(TargetIKRig);

	// Mark package as dirty
	Retargeter->MarkPackageDirty();
	
	UE_LOG(LogVrmToolchainEditor, Display, TEXT("Created/Updated Retargeter: %s"), *AssetName);
	
	return Retargeter;
}

FVrmRetargetScaffoldGenerator::FScaffoldResult FVrmRetargetScaffoldGenerator::GenerateRetargetScaffolding(
	USkeletalMesh* SourceMesh,
	USkeleton* TargetSkeleton,
	bool bCreateTargetIKRig)
{
	FScaffoldResult Result;

	if (!SourceMesh)
	{
		Result.ErrorMessage = TEXT("Invalid source mesh");
		return Result;
	}

	USkeleton* SourceSkeleton = SourceMesh->GetSkeleton();
	if (!SourceSkeleton)
	{
		Result.ErrorMessage = TEXT("Source mesh has no skeleton");
		return Result;
	}

	// Get output path
	FString OutputPath = GetRetargetOutputPath(SourceMesh);
	FString SourceMeshName = SourceMesh->GetName();

	// Infer bone chains for source
	TArray<FChainInfo> SourceChains;
	InferBoneChains(SourceSkeleton, SourceChains);

	if (SourceChains.Num() == 0)
	{
		Result.Warnings.Add(TEXT("No bone chains could be inferred from source skeleton"));
	}
	else
	{
		UE_LOG(LogVrmToolchainEditor, Display, TEXT("Inferred %d bone chains from source skeleton"), SourceChains.Num());
	}

	// Create source IKRig
	FString SourceIKRigName = FString::Printf(TEXT("IKRig_%s"), *SourceMeshName);
	Result.SourceIKRig = CreateOrUpdateIKRig(SourceIKRigName, OutputPath, SourceSkeleton, SourceChains, Result.Warnings);

	if (!Result.SourceIKRig)
	{
		Result.ErrorMessage = TEXT("Failed to create source IKRig");
		return Result;
	}

	// Create or find target IKRig
	if (TargetSkeleton)
	{
		FString TargetSkeletonName = TargetSkeleton->GetName();
		FString TargetIKRigName = FString::Printf(TEXT("IKRig_%s"), *TargetSkeletonName);

		if (bCreateTargetIKRig)
		{
			TArray<FChainInfo> TargetChains;
			InferBoneChains(TargetSkeleton, TargetChains);

			if (TargetChains.Num() == 0)
			{
				Result.Warnings.Add(TEXT("No bone chains could be inferred from target skeleton"));
			}

			Result.TargetIKRig = CreateOrUpdateIKRig(TargetIKRigName, OutputPath, TargetSkeleton, TargetChains, Result.Warnings);
		}
		else
		{
			// Try to find existing target IKRig
			FString TargetIKRigPath = OutputPath / TargetIKRigName;
			Result.TargetIKRig = LoadObject<UIKRigDefinition>(nullptr, *TargetIKRigPath, nullptr, LOAD_NoWarn | LOAD_Quiet);

			if (!Result.TargetIKRig)
			{
				Result.Warnings.Add(FString::Printf(TEXT("Target IKRig not found at %s. Create it manually or enable bCreateTargetIKRig."), *TargetIKRigPath));
			}
		}

		// Create retargeter if both IKRigs exist
		if (Result.TargetIKRig)
		{
			FString RetargeterName = FString::Printf(TEXT("RTG_%s_To_%s"), *SourceMeshName, *TargetSkeletonName);
			Result.Retargeter = CreateOrUpdateRetargeter(RetargeterName, OutputPath, Result.SourceIKRig, Result.TargetIKRig, Result.Warnings);

			if (!Result.Retargeter)
			{
				Result.ErrorMessage = TEXT("Failed to create retargeter");
				return Result;
			}
		}
	}

	Result.bSuccess = true;
	
	UE_LOG(LogVrmToolchainEditor, Display, TEXT("Retarget scaffolding created successfully for %s"), *SourceMeshName);

	return Result;
}
