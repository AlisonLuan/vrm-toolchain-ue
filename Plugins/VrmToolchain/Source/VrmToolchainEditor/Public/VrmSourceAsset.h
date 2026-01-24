#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPtr.h"
#include "VrmSourceAsset.generated.h"

class UAssetImportData;
class UVrmMetadataAsset;

/**
 * Editor-only “source of truth” asset for imported VRM/GLB files.
 *
 * Stores original bytes (editor-only), provenance (import data + filename),
 * and a soft link to the runtime-safe metadata descriptor (UVrmMetadataAsset).
 */
UCLASS(BlueprintType)
class VRMTOOLCHAINEDITOR_API UVrmSourceAsset : public UObject
{
    GENERATED_BODY()

public:
    UVrmSourceAsset();

    /** Standard UE import provenance to support reimport workflows. */
    UPROPERTY(VisibleAnywhere, Instanced, Category="Import")
    TObjectPtr<UAssetImportData> AssetImportData;

    /** Original imported filename (as given by the import pipeline). */
    UPROPERTY(VisibleAnywhere, Category="Import")
    FString SourceFilename;

    /** Import timestamp (UTC). */
    UPROPERTY(VisibleAnywhere, Category="Import")
    FDateTime ImportTime;

    /** Runtime-safe metadata descriptor created alongside this source asset. */
    UPROPERTY(VisibleAnywhere, Category="Import")
    TSoftObjectPtr<UVrmMetadataAsset> Descriptor;

    /** Non-fatal issues encountered during import/parsing. */
    UPROPERTY(VisibleAnywhere, Category="Import")
    TArray<FString> ImportWarnings;

    /** Fatal issues encountered during import (asset may still exist for debugging). */
    UPROPERTY(VisibleAnywhere, Category="Import")
    TArray<FString> ImportErrors;

#if WITH_EDITORONLY_DATA
    /** Raw source bytes (never cooked by default). */
    UPROPERTY(VisibleAnywhere, Category="Source")
    TArray<uint8> SourceBytes;
#endif
};
