#include "VrmSdkFacadeEditor.h"
#include "VrmToolchain/VrmMetadataAsset.h"
#include "VrmMetadata.h"
#include "Engine/AssetUserData.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"

UVrmMetadataAsset* FVrmSdkFacadeEditor::UpsertVrmMetadata(UObject* Object, const FVrmMetadata& Metadata)
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
            // Convert FVrmMetadata to FVrmMetadataRecord
            Existing->Metadata.Title = Metadata.Name;
            Existing->Metadata.Version = Metadata.ModelVersion;
            Existing->Metadata.Author = Metadata.Authors.Num() > 0 ? Metadata.Authors[0] : FString();
            Existing->Metadata.LicenseName = Metadata.License;
            Existing->MarkPackageDirty();
            Mesh->MarkPackageDirty();
            return Existing;
        }

        UVrmMetadataAsset* NewMetadata = NewObject<UVrmMetadataAsset>(Mesh, NAME_None, RF_Public | RF_Transactional);
        if (NewMetadata)
        {
            // Convert FVrmMetadata to FVrmMetadataRecord
            NewMetadata->Metadata.Title = Metadata.Name;
            NewMetadata->Metadata.Version = Metadata.ModelVersion;
            NewMetadata->Metadata.Author = Metadata.Authors.Num() > 0 ? Metadata.Authors[0] : FString();
            NewMetadata->Metadata.LicenseName = Metadata.License;
            Mesh->AddAssetUserData(NewMetadata);
            NewMetadata->MarkPackageDirty();
            Mesh->MarkPackageDirty();
        }
        return NewMetadata;
    }

    // Common case: static mesh
    if (UStaticMesh* SM = Cast<UStaticMesh>(Object))
    {
        UVrmMetadataAsset* Existing = SM->GetAssetUserData<UVrmMetadataAsset>();
        if (Existing)
        {
            // Convert FVrmMetadata to FVrmMetadataRecord
            Existing->Metadata.Title = Metadata.Name;
            Existing->Metadata.Version = Metadata.ModelVersion;
            Existing->Metadata.Author = Metadata.Authors.Num() > 0 ? Metadata.Authors[0] : FString();
            Existing->Metadata.LicenseName = Metadata.License;
            Existing->MarkPackageDirty();
            SM->MarkPackageDirty();
            return Existing;
        }

        UVrmMetadataAsset* NewMetadata = NewObject<UVrmMetadataAsset>(SM, NAME_None, RF_Public | RF_Transactional);
        if (NewMetadata)
        {
            // Convert FVrmMetadata to FVrmMetadataRecord
            NewMetadata->Metadata.Title = Metadata.Name;
            NewMetadata->Metadata.Version = Metadata.ModelVersion;
            NewMetadata->Metadata.Author = Metadata.Authors.Num() > 0 ? Metadata.Authors[0] : FString();
            NewMetadata->Metadata.LicenseName = Metadata.License;
            SM->AddAssetUserData(NewMetadata);
            NewMetadata->MarkPackageDirty();
            SM->MarkPackageDirty();
        }
        return NewMetadata;
    }

    // Unsupported object type
    return nullptr;
}

FVrmSkeletonCoverage FVrmSdkFacadeEditor::ComputeSkeletonCoverage(const TArray<FName>& BoneNames)
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
