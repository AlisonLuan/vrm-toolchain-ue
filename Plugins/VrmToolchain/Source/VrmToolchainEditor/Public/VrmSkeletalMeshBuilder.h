#pragma once

#include "CoreMinimal.h"
#include "VrmGlbAccessorReader.h"

/**
 * Builds skeletal meshes using MeshUtilities from decoded GLB accessor data
 */
class FVrmSkeletalMeshBuilder
{
public:
    /** Result of mesh building operations */
    struct FBuildResult
    {
        bool bSuccess = false;
        FString ErrorMessage;
        USkeletalMesh* BuiltMesh = nullptr;
    };

    /**
     * Build a skinned skeletal mesh from GLB accessor data
     * @param AccessorReader The reader containing decoded GLB data
     * @param TargetSkeleton The skeleton to assign to the mesh
     * @param JointOrdinalToBoneIndex Mapping from joint ordinals to bone indices
     * @param PackageName Package name for the new mesh asset
     * @param AssetName Asset name for the new mesh asset
     * @return Build result with success/failure and the created mesh
     */
    static FBuildResult BuildLod0SkinnedPrimitive(
        const FVrmGlbAccessorReader& AccessorReader,
        USkeleton* TargetSkeleton,
        const TMap<int32, int32>& JointOrdinalToBoneIndex,
        const FString& PackageName,
        const FString& AssetName);
};