#include "VrmMetaAssetRecomputeHelper.h"

#include "VrmToolchain/VrmMetaAsset.h"
#include "VrmMetaFeatureDetection.h"
#include "VrmToolchainEditor.h"

DEFINE_LOG_CATEGORY_STATIC(LogVrmRecompute, Log, All);

namespace VrmMetaAssetRecomputeHelper
{

FVrmRecomputeMetaResult RecomputeSingleMetaAsset(UVrmMetaAsset* Meta)
{
	FVrmRecomputeMetaResult Result;

	if (!Meta)
	{
		Result.bFailed = true;
		Result.Error = TEXT("Null meta asset");
		return Result;
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
		Result.bChanged = true;
	}

	// Diagnostic log
	const FString SpecStr = (Features.SpecVersion == EVrmVersion::VRM0) ? TEXT("vrm0") : (Features.SpecVersion == EVrmVersion::VRM1) ? TEXT("vrm1") : TEXT("unknown");
	UE_LOG(LogVrmToolchainEditor, Display, TEXT("RecomputeHelper: Meta=%s spec=%s humanoid=%d spring=%d blendOrExpr=%d thumb=%d summaryLen=%d warningsLen=%d changed=%d failed=%d"),
		*Meta->GetPathName(), *SpecStr, Features.bHasHumanoid ? 1 : 0, Features.bHasSpringBones ? 1 : 0, Features.bHasBlendShapesOrExpressions ? 1 : 0, Features.bHasThumbnail ? 1 : 0,
		Report.Summary.Len(), Report.Warnings.Num(), bChanged ? 1 : 0, Result.bFailed ? 1 : 0);

	return Result;
}

} // namespace VrmMetaAssetRecomputeHelper
