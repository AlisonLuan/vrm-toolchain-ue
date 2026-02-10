#include "VrmToolchainBulkActions.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "VrmToolchain/VrmMetaAsset.h"
#include "VrmMetaAssetRecomputeHelper.h"

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

	TArray<UObject*> RecomputedAssets; // Track for Content Browser sync

	FScopedSlowTask SlowTask(Total, FText::FromString("Recomputing import reports..."));
	SlowTask.MakeDialog(true);

	for (const FAssetData& AssetData : Assets)
	{
		SlowTask.EnterProgressFrame(1.0f, FText::FromString("Recomputing import reports..."));

		if (SlowTask.ShouldCancel())
		{
			break;
		}

		if (!AssetData.IsValid())
		{
			Failed++;
			continue;
		}

		UVrmMetaAsset* Meta = Cast<UVrmMetaAsset>(AssetData.GetAsset());
		if (!Meta)
		{
			Failed++;
			continue;
		}

		using namespace VrmMetaAssetRecomputeHelper;
		FVrmRecomputeMetaResult Result = RecomputeSingleMetaAsset(Meta);

		if (Result.bFailed)
		{
			Failed++;
		}
		else if (Result.bChanged)
		{
			Changed++;
			RecomputedAssets.Add(Meta);
		}
		else
		{
			Skipped++;
		}
	}

	if (VrmToolchain_CanShowUi())
	{
		// Sync Content Browser to show recomputed assets
		if (GEditor && RecomputedAssets.Num() > 0)
		{
			GEditor->SyncBrowserToObjects(RecomputedAssets);
		}

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
