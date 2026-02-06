#pragma once

#include "CoreMinimal.h"

class UVrmMetaAsset;

/**
 * Shared helper for recomputing import reports on VRM meta assets.
 * Used by both the interactive bulk action (PR-19) and the commandlet (PR-20).
 * Single source of truth for the deterministic recompute logic.
 */
namespace VrmMetaAssetRecomputeHelper
{
	/**
	 * Result of a single meta asset recompute operation.
	 */
	struct FVrmRecomputeMetaResult
	{
		bool bChanged = false;   // Report was updated
		bool bFailed = false;    // Asset was null or invalid
		FString Error;           // Error message if failed
	};

	/**
	 * Recompute import report for a single VRM meta asset.
	 * 
	 * Builds FVrmMetaFeatures from the stored fields on the asset (SpecVersion and bHas* flags).
	 * Calls VrmMetaDetection::BuildImportReport(Features) to generate the report.
	 * If changed, calls Modify(), updates ImportSummary/ImportWarnings, and MarkPackageDirty().
	 * 
	 * @param Meta The meta asset to recompute (may be nullptr)
	 * @return Result with bChanged/bFailed flags and optional error message
	 */
	FVrmRecomputeMetaResult RecomputeSingleMetaAsset(UVrmMetaAsset* Meta);

} // namespace VrmMetaAssetRecomputeHelper
