#pragma once

#include "CoreMinimal.h"
#include "VrmToolchain/VrmMetadata.h"
#include "VrmMetaAsset.generated.h"

UCLASS(BlueprintType)
class VRMTOOLCHAIN_API UVrmMetaAsset : public UObject
{
	GENERATED_BODY()

public:
	// Detected VRM spec version
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM")
	EVrmVersion SpecVersion = EVrmVersion::Unknown;

	// Minimal presence flags
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM")
	bool bHasHumanoid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM")
	bool bHasSpringBones = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM")
	bool bHasBlendShapesOrExpressions = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM")
	bool bHasThumbnail = false;

	// Convenience copy of source filename (not bulk data)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VRM")
	FString SourceFilename;
};