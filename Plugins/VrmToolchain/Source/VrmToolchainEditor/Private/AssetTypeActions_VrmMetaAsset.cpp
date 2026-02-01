#include "AssetTypeActions_VrmMetaAsset.h"

#include "VrmToolchain/VrmMetaAsset.h"
#include "VrmMetaAssetImportReportHelper.h"
#include "VrmMetaFeatureDetection.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Misc/App.h"

#define LOCTEXT_NAMESPACE "VrmMetaAssetTypeActions"

FText FAssetTypeActions_VrmMetaAsset::GetName() const
{
	return LOCTEXT("VrmMetaAssetTypeName", "VRM Meta");
}

FColor FAssetTypeActions_VrmMetaAsset::GetTypeColor() const
{
	return FColor(200, 100, 255);
}

UClass* FAssetTypeActions_VrmMetaAsset::GetSupportedClass() const
{
	return UVrmMetaAsset::StaticClass();
}

uint32 FAssetTypeActions_VrmMetaAsset::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

FString FAssetTypeActions_VrmMetaAsset::SanitizeForClipboardSingleLine(const FString& In)
{
	return FVrmMetaAssetImportReportHelper::SanitizeForClipboardSingleLine(In);
}

UVrmMetaAsset* FAssetTypeActions_VrmMetaAsset::GetSingleMeta(const TArray<UObject*>& Objects)
{
	UVrmMetaAsset* Result = nullptr;
	for (UObject* Obj : Objects)
	{
		if (UVrmMetaAsset* Meta = Cast<UVrmMetaAsset>(Obj))
		{
			if (Result != nullptr)
			{
				return nullptr; // more than one
			}
			Result = Meta;
		}
	}
	return Result;
}

void FAssetTypeActions_VrmMetaAsset::CollectMetas(const TArray<UObject*>& Objects, TArray<UVrmMetaAsset*>& OutMetas)
{
	for (UObject* Obj : Objects)
	{
		if (UVrmMetaAsset* Meta = Cast<UVrmMetaAsset>(Obj))
		{
			OutMetas.Add(Meta);
		}
	}
}

void FAssetTypeActions_VrmMetaAsset::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

#if WITH_EDITORONLY_DATA
	TArray<UVrmMetaAsset*> Metas;
	CollectMetas(InObjects, Metas);
	if (Metas.Num() == 0)
	{
		return;
	}

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("ImportReportSection", "Import Report"));

	// For Summary/Warnings, only enable for single-selection to avoid ambiguity.
	const bool bSingle = (Metas.Num() == 1);
	TArray<UObject*> ObjectsCopy = InObjects;  // Copy for lambda capture

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CopySummary", "Copy Summary"),
		LOCTEXT("CopySummaryTooltip", "Copy import summary to clipboard (single selection)"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this, ObjectsCopy]() { CopyImportSummary(ObjectsCopy); }),
			FCanExecuteAction::CreateLambda([bSingle]() { return bSingle; })
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("CopyWarnings", "Copy Warnings"),
		LOCTEXT("CopyWarningsTooltip", "Copy import warnings to clipboard (single selection)"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this, ObjectsCopy]() { CopyImportWarnings(ObjectsCopy); }),
			FCanExecuteAction::CreateLambda([bSingle]() { return bSingle; })
		)
	);

	// Full report can support multi-select safely (include separators).
	MenuBuilder.AddMenuEntry(
		LOCTEXT("CopyFullReport", "Copy Full Report"),
		LOCTEXT("CopyFullReportTooltip", "Copy summary + warnings to clipboard (supports multi-selection)"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, ObjectsCopy]() { CopyFullImportReport(ObjectsCopy); }))
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("RecomputeReport", "Recompute Import Report"),
		LOCTEXT("RecomputeReportTooltip", "Rebuild ImportSummary/Warnings from stored feature flags (useful after upgrading)"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda([this, ObjectsCopy]() { RecomputeImportReport(ObjectsCopy); })
		)
	);

	MenuBuilder.EndSection();
#endif
}

void FAssetTypeActions_VrmMetaAsset::CopyImportSummary(const TArray<UObject*>& Objects)
{
#if WITH_EDITORONLY_DATA
	if (UVrmMetaAsset* Meta = GetSingleMeta(Objects))
	{
		const FString Text = SanitizeForClipboardSingleLine(Meta->ImportSummary);
		FPlatformApplicationMisc::ClipboardCopy(*Text);
	}
#endif
}

void FAssetTypeActions_VrmMetaAsset::CopyImportWarnings(const TArray<UObject*>& Objects)
{
#if WITH_EDITORONLY_DATA
	if (UVrmMetaAsset* Meta = GetSingleMeta(Objects))
	{
		FString Text;
		if (Meta->ImportWarnings.Num() == 0)
		{
			Text = TEXT("(no warnings)");
		}
		else
		{
			for (const FString& W : Meta->ImportWarnings)
			{
				Text += TEXT("- ") + SanitizeForClipboardSingleLine(W) + TEXT("\n");
			}
		}
		FPlatformApplicationMisc::ClipboardCopy(*Text);
	}
#endif
}

void FAssetTypeActions_VrmMetaAsset::CopyFullImportReport(const TArray<UObject*>& Objects)
{
#if WITH_EDITORONLY_DATA
	TArray<UVrmMetaAsset*> Metas;
	CollectMetas(Objects, Metas);
	if (Metas.Num() == 0)
	{
		return;
	}

	FString Out;
	for (int32 i = 0; i < Metas.Num(); ++i)
	{
		UVrmMetaAsset* Meta = Metas[i];

		// Separator for multi-select
		if (i > 0)
		{
			Out += TEXT("\n----------------------------------------\n");
		}

		Out += SanitizeForClipboardSingleLine(Meta->ImportSummary) + TEXT("\n");

		if (Meta->ImportWarnings.Num() > 0)
		{
			Out += TEXT("Warnings:\n");
			for (const FString& W : Meta->ImportWarnings)
			{
				Out += TEXT("- ") + SanitizeForClipboardSingleLine(W) + TEXT("\n");
			}
		}
		else
		{
			Out += TEXT("(no warnings)\n");
		}
	}

	FPlatformApplicationMisc::ClipboardCopy(*Out);
#endif
}

void FAssetTypeActions_VrmMetaAsset::RecomputeImportReport(const TArray<UObject*>& Objects)
{
#if WITH_EDITORONLY_DATA
	TArray<UVrmMetaAsset*> Metas;
	CollectMetas(Objects, Metas);
	if (Metas.Num() == 0)
	{
		return;
	}

	// Optional: keep any UI notifications out of automation/unattended contexts
	const bool bInteractive =
		!IsRunningCommandlet() && !GIsAutomationTesting && !FApp::IsUnattended();

	int32 UpdatedCount = 0;

	for (UVrmMetaAsset* Meta : Metas)
	{
		if (!Meta)
		{
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

		Meta->ImportSummary = Report.Summary;
		Meta->ImportWarnings = Report.Warnings;

		Meta->MarkPackageDirty();
		Meta->PostEditChange();

		++UpdatedCount;
	}

	// Optional toast (only in interactive contexts)
	if (bInteractive && UpdatedCount > 0)
	{
		FNotificationInfo Info(FText::FromString(
			FString::Printf(TEXT("Recomputed import report for %d meta asset(s)"), UpdatedCount)
		));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
#endif
}

#undef LOCTEXT_NAMESPACE
