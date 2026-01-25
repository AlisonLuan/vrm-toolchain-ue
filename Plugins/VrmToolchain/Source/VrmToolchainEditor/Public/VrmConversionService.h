#pragma once

#include "CoreMinimal.h"
#include "VrmGltfTypes.h"

class UVrmSourceAsset;
class USkeletalMesh;
class USkeleton;

struct FVrmConvertOptions
{
	bool bOverwriteExisting = false;
	// When true, attempt to parse the source GLB/VRM and apply the skeleton to generated assets
	bool bApplyGltfSkeleton = true; // B1.1: default on
};

class VRMTOOLCHAINEDITOR_API FVrmConversionService
{
public:
	/**
	 * Creates generated output assets for a UVrmSourceAsset in a deterministic subfolder.
	 * Currently creates placeholder Skeleton + SkeletalMesh and attaches metadata.
	 */
	static bool ConvertSourceToPlaceholderSkeletalMesh(
		UVrmSourceAsset* Source,
		const FVrmConvertOptions& Options,
		USkeletalMesh*& OutSkeletalMesh,
		USkeleton*& OutSkeleton,
		FString& OutError);

private:
	static bool DeriveGeneratedPaths(UVrmSourceAsset* Source, FString& OutFolderPath, FString& OutBaseName, FString& OutError);

public:
	// Exposed for tests: apply a parsed GLTF skeleton to generated assets (editor-only)
#if WITH_EDITOR
	// NOTE: Implementation is deferred to a follow-up PR; stub exists to avoid build failures
	static bool ApplyGltfSkeletonToAssets(const FVrmGltfSkeleton& GltfSkel, USkeleton* TargetSkeleton, USkeletalMesh* TargetMesh, FString& OutError);
#endif
};
