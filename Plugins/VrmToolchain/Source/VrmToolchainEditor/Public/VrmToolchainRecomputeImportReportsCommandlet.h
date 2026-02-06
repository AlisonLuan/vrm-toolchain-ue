#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "VrmToolchainRecomputeImportReportsCommandlet.generated.h"

/**
 * Commandlet to bulk recompute VRM meta import reports.
 * 
 * Invocation: -run=VrmToolchainRecomputeImportReports
 * 
 * Enumerates UVrmMetaAsset via AssetRegistry with FARFilter (default root /Game, optional -Root=).
 * For each meta asset:
 *   - Build FVrmMetaFeatures from stored fields on UVrmMetaAsset (SpecVersion and bHas* flags).
 *   - Call VrmMetaDetection::BuildImportReport(Features).
 *   - If changed: Meta.Modify(); set ImportSummary/ImportWarnings; MarkPackageDirty(); PostEditChange();
 * 
 * Flags:
 *   -Save               : Save changed packages after recompute
 *   -FailOnChanges      : Exit with nonzero code if any assets changed
 *   -FailOnFailed       : Exit with nonzero code if any assets failed (default true)
 *   -NoFailOnFailed     : Override FailOnFailed to allow failures without error exit
 *   -Root=/Game/MyPath  : Override default /Game root path
 * 
 * No UI notifications; pure logging.
 */
UCLASS()
class UVrmToolchainRecomputeImportReportsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UVrmToolchainRecomputeImportReportsCommandlet();

	virtual int32 Main(const FString& Params) override;
};
