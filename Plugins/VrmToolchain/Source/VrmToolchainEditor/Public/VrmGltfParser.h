#pragma once

#include "CoreMinimal.h"
#include "VrmGltfTypes.h"

class VRMTOOLCHAINEDITOR_API FVrmGltfParser
{
public:
    // Parse JSON string (editor-only testable) and extract skeleton nodes
    static bool ExtractSkeletonFromGltfJsonString(const FString& JsonString, FVrmGltfSkeleton& OutSkeleton, FString& OutError);

    // High-level helper: read GLB JSON chunk from disk and then parse
    static bool ExtractSkeletonFromGlbFile(const FString& FilePath, FVrmGltfSkeleton& OutSkeleton, FString& OutError);
};
