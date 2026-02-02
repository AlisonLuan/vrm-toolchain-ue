#include "VrmToolchainBulkActions.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "VrmToolchain/VrmMetaAsset.h"
#include "VrmMetaFeatureDetection.h"

static bool VrmToolchain_CanShowUi()
{
	return !IsRunningCommandlet() && !GIsAutomationTesting && !FApp::IsUnattended();
}

void FVrmToolchainBulkActions::RecomputeAllImportReports()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName("/Game"));
	Filter.ClassPaths.Add(UVrmMetaAsset::StaticClass()->GetClassPathName());

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssets(Filter, Assets);

	int32 Total = Assets.Num();
	int32 Changed = 0;
	int32 Skipped = 0;
	int32 Failed = 0;

	FScopedSlowTask SlowTask(Total, FText::FromString("Recomputing import reports..."));
	SlowTask.MakeDialog();

	for (const FAssetData& AssetData : Assets)
	{
		SlowTask.EnterProgressFrame();

		if (!AssetData.IsValid())
		{
			Failed++;
			continue;
		}

		UVrmMetaAsset* Meta = Cast<UVrmMetaAsset>(AssetData.GetAsset());
		if (!Meta)
		{
			Skipped++;
			continue;
		}

		Meta->Modify();

		VrmMetaDetection::FVrmMetaFeatures Features;
		Features.SpecVersion = Meta->SpecVersion;
		Features.bHasHumanoid = Meta->bHasHumanoid;
		Features.bHasSpringBones = Meta->bHasSpringBones;
		Features.bHasBlendShapesOrExpressions = Meta->bHasBlendShapesOrExpressions;
		Features.bHasThumbnail = Meta->bHasThumbnail;

		const VrmMetaDetection::FVrmImportReport Report =
			VrmMetaDetection::BuildImportReport(Features);

		const bool bChanged =
			(Meta->ImportSummary != Report.Summary) ||
			(Meta->ImportWarnings != Report.Warnings);

		Meta->ImportSummary = Report.Summary;
		Meta->ImportWarnings = Report.Warnings;

		if (bChanged)
		{
			Changed++;
		}

		Meta->MarkPackageDirty();
		Meta->PostEditChange();
	}

	if (VrmToolchain_CanShowUi())
	{
		FText Msg = FText::Format(
			NSLOCTEXT("VrmToolchain", "RecomputeAllDoneFmt",
				"Recomputed {0}/{1} VRM Meta Assets (skipped {2}, failed {3})."),
			FText::AsNumber(Changed),
			FText::AsNumber(Total),
			FText::AsNumber(Skipped),
			FText::AsNumber(Failed));

		FNotificationInfo Info(Msg);
		Info.ExpireDuration = 6.0f;
		Info.bUseLargeFont = false;
		Info.bFireAndForget = true;

		FSlateNotificationManager::Get().AddNotification(Info);
	}
}
