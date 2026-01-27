#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/SoftObjectPtr.h"

#if WITH_EDITOR
  #if __has_include("AssetRegistry/AssetRegistryTagsContext.h")
    #include "AssetRegistry/AssetRegistryTagsContext.h"
    #define VRM_HAS_TAGS_CONTEXT 1
  #else
    #define VRM_HAS_TAGS_CONTEXT 0
  #endif

  #include "Misc/Paths.h"

  // Forward declare the tag type so we can use it in signatures without forcing an include here
  struct FAssetRegistryTag;
#endif

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
    UVrmMetadataAsset* Descriptor;

    /** Convenience: VRM spec major version (0 = VRM0, 1 = VRM1, -1 = Unknown) */
    UPROPERTY(VisibleAnywhere, Category="VRM")
    int32 VrmSpecVersionMajor = -1;

    /** Convenience: detected file extension (lowercase, e.g. "vrm" or "glb") */
    UPROPERTY(VisibleAnywhere, Category="VRM")
    FString DetectedVrmExtension;

#if WITH_EDITOR
    // Expose useful tags to the Asset Registry so filters and searches can see VRM metadata without loading the whole asset.
  #if VRM_HAS_TAGS_CONTEXT
    virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
  #endif
    virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;

    // Editor-only: deterministically compute VRM spec version from JSON (files[0].vrm.version)
    // No-op if already set; call EditorRecomputeSpecVersionForce() to force re-computation.
    UFUNCTION(CallInEditor, BlueprintCallable, Category="VRM")
    bool EditorRecomputeSpecVersion();

    // Editor-only: force re-computation of VRM spec version from JSON (files[0].vrm.version)
    // Always runs vrm_validate.exe, ignoring cached value.
    UFUNCTION(CallInEditor, BlueprintCallable, Category="VRM")
    bool EditorRecomputeSpecVersionForce();

    // Editor-only convenience: backfill derived fields (callable from Python/Editor)
    UFUNCTION(CallInEditor, BlueprintCallable, Category="VRM")
    void EditorBackfillDerivedFields();
#endif
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
