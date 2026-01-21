#pragma once

#include "CoreMinimal.h"

class UVrmMetadataAsset;
struct FVrmMetadata;
struct FVrmSkeletonCoverage;

/**
 * Editor-only extensions to FVrmSdkFacade for asset manipulation and metadata storage.
 * Only available in the editor; not available in packaged games.
 */
class VRMTOOLCHAINEDITOR_API FVrmSdkFacadeEditor
{
public:
    /** Find or create VRM metadata user data and assign the provided metadata. Returns the user data object or nullptr. */
    static UVrmMetadataAsset* UpsertVrmMetadata(UObject* Object, const FVrmMetadata& Metadata);

    /** Compute skeleton coverage from a list of bone names (testable, deterministic helper) */
    static FVrmSkeletonCoverage ComputeSkeletonCoverage(const TArray<FName>& BoneNames);
};
