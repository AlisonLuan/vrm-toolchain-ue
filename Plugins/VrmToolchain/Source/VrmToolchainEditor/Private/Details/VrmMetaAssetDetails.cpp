#include "Details/VrmMetaAssetDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"

#include "VrmToolchain/VrmMetaAsset.h"
#include "VrmMetaAssetImportReportHelper.h"
#include "HAL/PlatformApplicationMisc.h"
#include "UObject/WeakObjectPtrTemplates.h"

TSharedRef<IDetailCustomization> FVrmMetaAssetDetails::MakeInstance()
{
	return MakeShared<FVrmMetaAssetDetails>();
}

static UVrmMetaAsset* GetSingleMetaAsset(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() == 1)
	{
		return Cast<UVrmMetaAsset>(Objects[0].Get());
	}
	return nullptr;
}

FString FVrmMetaAssetDetails::BuildCopyText(const FString& Summary, const TArray<FString>& Warnings)
{
	return FVrmMetaAssetImportReportHelper::BuildCopyText(Summary, Warnings);
}

void FVrmMetaAssetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	UVrmMetaAsset* Meta = GetSingleMetaAsset(DetailBuilder);
	if (!Meta)
	{
		return;
	}

	IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory(
		"Import Report",
		FText::FromString(TEXT("Import Report")),
		ECategoryPriority::Important
	);

#if WITH_EDITORONLY_DATA
	const FString Summary = Meta->ImportSummary;
	const TArray<FString> Warnings = Meta->ImportWarnings;
#else
	const FString Summary;
	const TArray<FString> Warnings;
#endif

	// Summary (wrapped, read-only)
	Cat.AddCustomRow(FText::FromString(TEXT("ImportSummary")))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Summary")))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(450.f)
	[
		SNew(STextBlock)
		.Text(FText::FromString(Summary.IsEmpty() ? TEXT("(empty)") : Summary))
		.AutoWrapText(true)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	// Warnings header/count
	if (Warnings.Num() == 0)
	{
		Cat.AddCustomRow(FText::FromString(TEXT("ImportWarnings")))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Warnings")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("(none)")))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];
	}
	else
	{
		// Add warning items as separate rows (cleaner in Details panel)
		for (int32 i = 0; i < Warnings.Num(); ++i)
		{
			FString Clean = Warnings[i];
			Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
			Clean.ReplaceInline(TEXT("\n"), TEXT(" "));

			// First warning gets the "Warnings" label; rest are indented
			if (i == 0)
			{
				Cat.AddCustomRow(FText::FromString(TEXT("ImportWarning_0")))
				.NameContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Warnings")))
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
				.ValueContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("• ") + Clean))
					.AutoWrapText(true)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				];
			}
			else
			{
				Cat.AddCustomRow(FText::FromString(FString::Printf(TEXT("ImportWarning_%d"), i)))
				.WholeRowContent()
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("    • %s"), *Clean)))
					.AutoWrapText(true)
					.Font(IDetailLayoutBuilder::GetDetailFont())
				];
			}
		}
	}

	// Copy report button
	Cat.AddCustomRow(FText::FromString(TEXT("CopyImportReport")))
	.WholeRowContent()
	.HAlign(HAlign_Left)
	[
		SNew(SButton)
		.Text(FText::FromString(TEXT("Copy Import Report")))
		.OnClicked_Lambda([Summary, Warnings]()
		{
			const FString CopyText = BuildCopyText(Summary, Warnings);
			FPlatformApplicationMisc::ClipboardCopy(*CopyText);
			return FReply::Handled();
		})
	];
}
