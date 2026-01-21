#include "CoreMinimal.h"
#include "VrmToolchainWrapper.h"
#include "VrmMetadataAsset.h"
#include "Engine/AssetUserData.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"

#include "Containers/Array.h"
#include "Misc/Char.h"


UVrmMetadataAsset* FVrmSdkFacade::UpsertVrmMetadata(UObject* Object, const FVrmMetadata& Metadata)
{
    if (!Object)
    {
        return nullptr;
    }

    // Common case: skeletal mesh
    if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Object))
    {
        UVrmMetadataAsset* Existing = Mesh->GetAssetUserData<UVrmMetadataAsset>();
        if (Existing)
        {
            Existing->Metadata = Metadata;
#if WITH_EDITOR
            Existing->MarkPackageDirty();
            Mesh->MarkPackageDirty();
#endif
            return Existing;
        }

        UVrmMetadataAsset* NewMetadata = NewObject<UVrmMetadataAsset>(Mesh, NAME_None, RF_Public | RF_Transactional);
        if (NewMetadata)
        {
            NewMetadata->Metadata = Metadata;
            Mesh->AddAssetUserData(NewMetadata);
#if WITH_EDITOR
            NewMetadata->MarkPackageDirty();
            Mesh->MarkPackageDirty();
#endif
        }
        return NewMetadata;
    }

    // Common case: static mesh
    if (UStaticMesh* SM = Cast<UStaticMesh>(Object))
    {
        UVrmMetadataAsset* Existing = SM->GetAssetUserData<UVrmMetadataAsset>();
        if (Existing)
        {
            Existing->Metadata = Metadata;
#if WITH_EDITOR
            Existing->MarkPackageDirty();
            SM->MarkPackageDirty();
#endif
            return Existing;
        }

        UVrmMetadataAsset* NewMetadata = NewObject<UVrmMetadataAsset>(SM, NAME_None, RF_Public | RF_Transactional);
        if (NewMetadata)
        {
            NewMetadata->Metadata = Metadata;
            SM->AddAssetUserData(NewMetadata);
#if WITH_EDITOR
            NewMetadata->MarkPackageDirty();
            SM->MarkPackageDirty();
#endif
        }
        return NewMetadata;
    }

    // Unsupported object type
    return nullptr;
}


// Deterministic skeleton coverage computation helper
FVrmSkeletonCoverage FVrmSdkFacade::ComputeSkeletonCoverage(const TArray<FName>& BoneNames)
{
    FVrmSkeletonCoverage OutCoverage;
    OutCoverage.TotalBoneCount = BoneNames.Num();

    OutCoverage.SortedBoneNames = BoneNames;
    OutCoverage.SortedBoneNames.Sort([](const FName& A, const FName& B){ return A.LexicalLess(B); });

    auto HasBone = [&](const TCHAR* SearchTerm) -> bool {
        FString SearchStr(SearchTerm);
        for (const FName& BoneName : OutCoverage.SortedBoneNames)
        {
            if (BoneName.ToString().Contains(SearchStr, ESearchCase::IgnoreCase))
            {
                return true;
            }
        }
        return false;
    };

    OutCoverage.bHasHips = HasBone(TEXT("hip"));
    OutCoverage.bHasSpine = HasBone(TEXT("spine"));
    OutCoverage.bHasHead = HasBone(TEXT("head"));
    OutCoverage.bHasLeftHand = HasBone(TEXT("hand")) && HasBone(TEXT("left"));
    OutCoverage.bHasRightHand = HasBone(TEXT("hand")) && HasBone(TEXT("right"));
    OutCoverage.bHasLeftFoot = HasBone(TEXT("foot")) && HasBone(TEXT("left"));
    OutCoverage.bHasRightFoot = HasBone(TEXT("foot")) && HasBone(TEXT("right"));

    return OutCoverage;
}