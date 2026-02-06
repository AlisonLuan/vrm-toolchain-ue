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
	enum class ERecomputeResult : uint8
	{
		Success,     // Report was updated
		Unchanged,   // Report was already up-to-date
		Failed       // Asset was null or invalid
	};

	/**
	 * Recompute import report for a single VRM meta asset.
	 * 
	 * Builds FVrmMetaFeatures from the stored fields on the asset (SpecVersion and bHas* flags).
	 * Calls VrmMetaDetection::BuildImportReport(Features) to generate the report.
	 * If changed, calls Modify(), updates ImportSummary/ImportWarnings, and MarkPackageDirty().
	 * 
	 * @param Meta The meta asset to recompute (may be nullptr)
	 * @return Result indicating success, unchanged, or failed
	 */
	ERecomputeResult RecomputeSingleMetaAsset(UVrmMetaAsset* Meta);

} // namespace VrmMetaAssetRecomputeHelper
