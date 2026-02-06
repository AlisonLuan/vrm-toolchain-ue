#include "VrmMetaAssetRecomputeHelper.h"

#include "VrmToolchain/VrmMetaAsset.h"
#include "VrmMetaFeatureDetection.h"

namespace VrmMetaAssetRecomputeHelper
{

ERecomputeResult RecomputeSingleMetaAsset(UVrmMetaAsset* Meta)
{
	if (!Meta)
	{
		return ERecomputeResult::Failed;
	}

	// Build features from stored fields on the asset
	VrmMetaDetection::FVrmMetaFeatures Features;
	Features.SpecVersion = Meta->SpecVersion;
	Features.bHasHumanoid = Meta->bHasHumanoid;
	Features.bHasSpringBones = Meta->bHasSpringBones;
	Features.bHasBlendShapesOrExpressions = Meta->bHasBlendShapesOrExpressions;
	Features.bHasThumbnail = Meta->bHasThumbnail;

	// Build deterministic import report
	const VrmMetaDetection::FVrmImportReport Report =
		VrmMetaDetection::BuildImportReport(Features);

	// Check if changed
	const bool bChanged =
		(Meta->ImportSummary != Report.Summary) ||
		(Meta->ImportWarnings != Report.Warnings);

	if (bChanged)
	{
		Meta->Modify();
		Meta->ImportSummary = Report.Summary;
		Meta->ImportWarnings = Report.Warnings;
		Meta->MarkPackageDirty();
		Meta->PostEditChange();
		return ERecomputeResult::Success;
	}

	return ERecomputeResult::Unchanged;
}

} // namespace VrmMetaAssetRecomputeHelper
