#pragma once

#include "CoreMinimal.h"

class UVrmSourceAsset;
class USkeletalMesh;
class USkeleton;

struct FVrmConvertOptions
{
	bool bOverwriteExisting = false;
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
};
