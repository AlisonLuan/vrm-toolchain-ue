#pragma once

#include "CoreMinimal.h"
#include "VrmToolchain/VrmMetadata.h"

// Forward declaration (global scope to avoid namespace confusion)
class UVrmMetaAsset;

namespace VrmMetaDetection
{
	/**
	 * Result struct from parsing VRM metadata features from a JSON chunk.
	 * Represents detected SpecVersion and feature flags extracted from a GLB JSON chunk.
	 */
	struct FVrmMetaFeatures
    {
        /** Detected VRM version (VRM0, VRM1, or Unknown) */
        EVrmVersion SpecVersion = EVrmVersion::Unknown;

        /** Whether the model has a humanoid armature definition */
        bool bHasHumanoid = false;

        /** Whether the model has spring bone physics definitions */
        bool bHasSpringBones = false;

        /** Whether the model has blend shapes (VRM0) or expressions (VRM1) */
        bool bHasBlendShapesOrExpressions = false;

        /** Whether the model has an embedded thumbnail */
        bool bHasThumbnail = false;
    };

    /**
     * Parse VRM metadata features from a JSON string extracted from a GLB file.
     * 
     * Examines the JSON structure for VRM0 (extensions.VRM) or VRM1 (extensions.VRMC_vrm)
     * indicators and detects the presence of feature fields.
     * 
     * Safely handles malformed JSON (missing fields, non-object types) by returning
     * conservative defaults (Unknown version, all flags false).
     * 
     * @param JsonStr The JSON string extracted from a GLB file's JSON chunk
     * @return FVrmMetaFeatures struct with detected SpecVersion and feature flags.
     *         On parse failure (invalid JSON), all fields are set to Unknown/false (conservative defaults).
     */
    FVrmMetaFeatures ParseMetaFeaturesFromJson(const FString& JsonStr);

    /**
     * Format detected VRM metadata features into a compact, deterministic single-line diagnostic string.
     * 
     * Output format: "spec=vrm0 humanoid=1 spring=1 blendOrExpr=1 thumb=0"
     * - SpecVersion is one of: vrm0, vrm1, unknown
     * - Feature flags are 0 or 1
     * - Uses stable keys for consistent parsing and logging
     * 
     * @param Features The FVrmMetaFeatures struct to format
     * @return A compact diagnostic string suitable for logging
     */
    FString FormatMetaFeaturesForDiagnostics(const FVrmMetaFeatures& Features);

    /**
     * Apply detected VRM features to a meta asset.
     * 
     * Assigns the 5 feature fields from the detected features struct to the meta asset.
     * This is the single source of truth for how detection results map to meta asset fields.
     * 
     * If Meta is nullptr, does nothing (safe no-op).
     * 
     * @param Meta The UVrmMetaAsset to assign fields to
     * @param Features The FVrmMetaFeatures struct to assign from
     */
    void ApplyFeaturesToMetaAsset(UVrmMetaAsset* Meta, const FVrmMetaFeatures& Features);

    /**
     * Deterministic import summary formatter.
     * 
     * Produces a stable summary string suitable for MessageLog display,
     * encoding the VRM version and feature flags in human-readable format.
     * 
     * Output format: "Imported VRM (spec=vrm0) humanoid=1 spring=1 blendOrExpr=1 thumb=0"
     * 
     * @param Features The detected features to summarize
     * @return A formatted summary string (deterministic, no timestamps)
     */
    FString FormatImportSummary(const FVrmMetaFeatures& Features);

	/**
	 * Report struct containing import summary and warnings.
	 * Suitable for storing on UVrmMetaAsset and displaying in MessageLog.
	 */
	struct FVrmImportReport
	{
		FString Summary;
		TArray<FString> Warnings;
	};

	/**
	 * Build a deterministic import report from detected features.
	 * 
	 * Generates both a human-readable summary and a list of warnings
	 * (e.g., "Missing humanoid definition") for missing features.
	 * 
	 * Output is stable and suitable for CI assertions.
	 * 
	 * @param Features The detected VRM features
	 * @return Report struct with Summary and Warnings
	 */
	FVrmImportReport BuildImportReport(const FVrmMetaFeatures& Features);

	/**
	 * Convert EVrmVersion enum to stable string representation.
	 * 
	 * @param Version The version to convert
	 * @return "vrm0", "vrm1", or "unknown"
	 */
	FString VrmVersionToStableString(EVrmVersion Version);

} // namespace VrmMetaDetection
