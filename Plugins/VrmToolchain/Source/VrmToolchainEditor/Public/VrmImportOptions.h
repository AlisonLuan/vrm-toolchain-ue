#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VrmImportOptions.generated.h"

/**
 * VRM import options shown in the import dialog when importing .vrm/.glb files.
 * These are displayed via ConfigureProperties() modal in UVrmSourceFactory.
 */
UCLASS()
class VRMTOOLCHAINEDITOR_API UVrmImportOptions : public UObject
{
    GENERATED_BODY()

public:
    UVrmImportOptions();

    /** When enabled, automatically generates USkeleton and USkeletalMesh assets after import. */
    UPROPERTY(EditAnywhere, Category = "VRM Import", meta = (DisplayName = "Create SkeletalMesh (Experimental)"))
    bool bAutoCreateSkeletalMesh = false;

    /** When enabled, attempts to parse and apply the skeleton from the GLB/VRM file. */
    UPROPERTY(EditAnywhere, Category = "VRM Import", meta = (DisplayName = "Apply GLTF Skeleton", EditCondition = "bAutoCreateSkeletalMesh"))
    bool bApplyGltfSkeleton = true;
};
