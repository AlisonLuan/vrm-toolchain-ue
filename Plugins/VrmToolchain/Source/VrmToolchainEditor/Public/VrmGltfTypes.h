#pragma once

#include "CoreMinimal.h"
#include "VrmGltfTypes.generated.h"

USTRUCT()
struct FVrmGltfBone
{
    GENERATED_BODY()

    UPROPERTY()
    FName Name;

    UPROPERTY()
    int32 ParentIndex = INDEX_NONE;

    UPROPERTY()
    FTransform LocalTransform = FTransform::Identity;
};

USTRUCT()
struct FVrmGltfSkeleton
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FVrmGltfBone> Bones;
};
