#include "VrmSkeletalMeshBuilder.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "MeshUtilities.h"
#include "Rendering/SkeletalMeshLODImporterData.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "ImportUtils/SkeletalMeshImportUtils.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"

FVrmSkeletalMeshBuilder::FBuildResult FVrmSkeletalMeshBuilder::BuildLod0SkinnedPrimitive(
    const FVrmGlbAccessorReader& AccessorReader,
    USkeleton* TargetSkeleton,
    const TMap<int32, int32>& JointOrdinalToBoneIndex,
    const FString& PackageName,
    const FString& AssetName)
{
    FBuildResult Result;

    // Validate inputs
    if (!TargetSkeleton)
    {
        Result.ErrorMessage = TEXT("TargetSkeleton is null");
        return Result;
    }

    if (AccessorReader.Positions.Num() == 0)
    {
        Result.ErrorMessage = TEXT("No vertex positions in accessor data");
        return Result;
    }

    if (AccessorReader.Indices.Num() == 0)
    {
        Result.ErrorMessage = TEXT("No triangle indices in accessor data");
        return Result;
    }

    if (AccessorReader.Weights.Num() == 0 || AccessorReader.Joints.Num() == 0)
    {
        Result.ErrorMessage = TEXT("Missing skinning data (weights/joints)");
        return Result;
    }

    // Create package if it doesn't exist
    UPackage* Package = CreatePackage(*PackageName);
    if (!Package)
    {
        Result.ErrorMessage = FString::Printf(TEXT("Failed to create package: %s"), *PackageName);
        return Result;
    }

    // Create skeletal mesh asset
    USkeletalMesh* SkeletalMesh = NewObject<USkeletalMesh>(Package, *AssetName, RF_Public | RF_Standalone);
    if (!SkeletalMesh)
    {
        Result.ErrorMessage = FString::Printf(TEXT("Failed to create skeletal mesh asset: %s"), *AssetName);
        return Result;
    }

    // Set skeleton reference
    SkeletalMesh->SetSkeleton(TargetSkeleton);

    // Get mesh utilities
    IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");

    // Create import data structure
    FSkeletalMeshImportData ImportData;

    // Populate vertex positions
    ImportData.Points.Reserve(AccessorReader.Positions.Num());
    for (const FVector3f& Position : AccessorReader.Positions)
    {
        ImportData.Points.Add(Position);
    }

    // Populate wedges (vertex data with UVs)
    ImportData.Wedges.Reserve(AccessorReader.Positions.Num());
    for (int32 i = 0; i < AccessorReader.Positions.Num(); ++i)
    {
        SkeletalMeshImportData::FVertex Wedge;
        Wedge.VertexIndex = i;
        
        // Use texcoords if available, otherwise default to zero
        if (AccessorReader.TexCoords.Num() > i)
        {
            Wedge.UVs[0] = FVector2f(AccessorReader.TexCoords[i].X, AccessorReader.TexCoords[i].Y);
        }
        else
        {
            Wedge.UVs[0] = FVector2f::ZeroVector;
        }

        Wedge.Color = FColor::White;

        ImportData.Wedges.Add(Wedge);
    }

    // Populate faces (triangles)
    ImportData.Faces.Reserve(AccessorReader.Indices.Num() / 3);
    for (int32 i = 0; i < AccessorReader.Indices.Num(); i += 3)
    {
        SkeletalMeshImportData::FTriangle Triangle;
        Triangle.WedgeIndex[0] = AccessorReader.Indices[i];
        Triangle.WedgeIndex[1] = AccessorReader.Indices[i + 1];
        Triangle.WedgeIndex[2] = AccessorReader.Indices[i + 2];
        Triangle.MatIndex = 0;
        
        // Set up tangent space (basic calculation)
        Triangle.TangentX[0] = FVector3f(1, 0, 0);
        Triangle.TangentY[0] = FVector3f(0, 1, 0);
        Triangle.TangentZ[0] = FVector3f(0, 0, 1);
        
        Triangle.TangentX[1] = FVector3f(1, 0, 0);
        Triangle.TangentY[1] = FVector3f(0, 1, 0);
        Triangle.TangentZ[1] = FVector3f(0, 0, 1);
        
        Triangle.TangentX[2] = FVector3f(1, 0, 0);
        Triangle.TangentY[2] = FVector3f(0, 1, 0);
        Triangle.TangentZ[2] = FVector3f(0, 0, 1);

        ImportData.Faces.Add(Triangle);
    }

    // Populate influences (skinning data)
    ImportData.Influences.Reserve(AccessorReader.Weights.Num() * 4); // Up to 4 influences per vertex

    for (int32 VertexIndex = 0; VertexIndex < AccessorReader.Weights.Num(); ++VertexIndex)
    {
        const FVector4f& Weight = AccessorReader.Weights[VertexIndex];
        const FIntVector4& Joint = AccessorReader.Joints[VertexIndex];

        // Process up to 4 influences per vertex
        for (int32 InfluenceIndex = 0; InfluenceIndex < 4; ++InfluenceIndex)
        {
            float InfluenceWeight = 0.0f;
            int32 JointOrdinal = 0;

            switch (InfluenceIndex)
            {
            case 0:
                InfluenceWeight = Weight.X;
                JointOrdinal = Joint.X;
                break;
            case 1:
                InfluenceWeight = Weight.Y;
                JointOrdinal = Joint.Y;
                break;
            case 2:
                InfluenceWeight = Weight.Z;
                JointOrdinal = Joint.Z;
                break;
            case 3:
                InfluenceWeight = Weight.W;
                JointOrdinal = Joint.W;
                break;
            }

            // Skip zero weights
            if (FMath::IsNearlyZero(InfluenceWeight))
            {
                continue;
            }

            // Map joint ordinal to bone index
            const int32* BoneIndexPtr = JointOrdinalToBoneIndex.Find(JointOrdinal);
            if (!BoneIndexPtr)
            {
                Result.ErrorMessage = FString::Printf(TEXT("Joint ordinal %d not found in bone mapping"), JointOrdinal);
                return Result;
            }

            SkeletalMeshImportData::FRawBoneInfluence Influence;
            Influence.VertexIndex = VertexIndex;
            Influence.BoneIndex = *BoneIndexPtr;
            Influence.Weight = InfluenceWeight;

            ImportData.Influences.Add(Influence);
        }
    }

    // Process influences
    SkeletalMeshImportUtils::ProcessImportMeshInfluences(ImportData, SkeletalMesh->GetPathName());

    // Convert import data to LOD format
    TArray<FVector3f> LODPoints;
    TArray<SkeletalMeshImportData::FMeshWedge> LODWedges;
    TArray<SkeletalMeshImportData::FMeshFace> LODFaces;
    TArray<SkeletalMeshImportData::FVertInfluence> LODInfluences;
    ImportData.CopyLODImportData(LODPoints, LODWedges, LODFaces, LODInfluences, ImportData.PointToRawMap);

    // Create LOD model
    FSkeletalMeshLODModel* LODModel = new FSkeletalMeshLODModel();
    SkeletalMesh->GetImportedModel()->LODModels.Add(LODModel);

    // Build the skeletal mesh
    IMeshUtilities::MeshBuildOptions BuildOptions;
    TArray<FText> WarningMessages;
    TArray<FName> WarningNames;
    
    bool bBuildSuccess = MeshUtilities.BuildSkeletalMesh(
        *LODModel,
        SkeletalMesh->GetPathName(),
        TargetSkeleton->GetReferenceSkeleton(),
        LODInfluences,
        LODWedges,
        LODFaces,
        LODPoints,
        ImportData.PointToRawMap,
        BuildOptions,
        &WarningMessages,
        &WarningNames
    );

    if (!bBuildSuccess)
    {
        Result.ErrorMessage = TEXT("Failed to build skeletal mesh");
        return Result;
    }

    // Mark package as dirty
    Package->MarkPackageDirty();

    // Register asset
    FAssetRegistryModule::GetRegistry().AssetCreated(SkeletalMesh);

    Result.bSuccess = true;
    Result.BuiltMesh = SkeletalMesh;
    return Result;
}