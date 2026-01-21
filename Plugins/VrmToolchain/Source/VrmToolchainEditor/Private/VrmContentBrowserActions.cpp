#include "VrmContentBrowserActions.h"
#include "VrmNormalizationService.h"
#include "VrmToolchainEditor.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Engine/SkeletalMesh.h"
#include "EditorFramework/AssetImportData.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "ToolMenus.h"

FDelegateHandle FVrmContentBrowserActions::ContentBrowserExtenderHandle;

void FVrmContentBrowserActions::RegisterMenuExtensions()
{
	// Register asset context menu extender
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

	CBMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&FVrmContentBrowserActions::CreateAssetContextMenuExtender));
	ContentBrowserExtenderHandle = CBMenuExtenderDelegates.Last().GetHandle();

	UE_LOG(LogVrmToolchainEditor, Log, TEXT("VRM Content Browser actions registered"));
}

void FVrmContentBrowserActions::UnregisterMenuExtensions()
{
	if (ContentBrowserExtenderHandle.IsValid())
	{
		FContentBrowserModule* ContentBrowserModule = FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser"));
		if (ContentBrowserModule)
		{
			TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule->GetAllAssetViewContextMenuExtenders();
			FDelegateHandle HandleToRemove = ContentBrowserExtenderHandle;
			CBMenuExtenderDelegates.RemoveAll([HandleToRemove](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
			{
				return Delegate.GetHandle() == HandleToRemove;
			});
		}

		ContentBrowserExtenderHandle.Reset();
		UE_LOG(LogVrmToolchainEditor, Log, TEXT("VRM Content Browser actions unregistered"));
	}
}

TSharedRef<FExtender> FVrmContentBrowserActions::CreateAssetContextMenuExtender(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	// Only add menu if action is available for selected assets
	if (CanNormalizeVrm(SelectedAssets))
	{
		Extender->AddMenuExtension(
			"GetAssetActions",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateLambda([SelectedAssets](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuSeparator();
				MenuBuilder.BeginSection("VrmToolchain", NSLOCTEXT("VrmToolchain", "VrmToolchainSection", "VRM Toolchain"));
				{
					FUIAction UIAction(
						FExecuteAction::CreateLambda([SelectedAssets]() { ExecuteNormalizeVrm(SelectedAssets); }),
						FCanExecuteAction::CreateLambda([SelectedAssets]() { return CanNormalizeVrm(SelectedAssets); })
					);
					MenuBuilder.AddMenuEntry(
						UIAction,
						SNew(STextBlock)
							.Text(NSLOCTEXT("VrmToolchain", "NormalizeSourceVRM", "Normalize Source VRM"))
							.ToolTipText(NSLOCTEXT("VrmToolchain", "NormalizeSourceVRMTooltip", "Normalize the source VRM/GLB file and write a cleaned copy"))
					);
				}
				MenuBuilder.EndSection();
			})
		);
	}

	return Extender;
}

bool FVrmContentBrowserActions::CanNormalizeVrm(const TArray<FAssetData>& SelectedAssets)
{
	// Currently only support single selection
	if (SelectedAssets.Num() != 1)
	{
		return false;
	}

	const FAssetData& AssetData = SelectedAssets[0];

	// Check if this is a skeletal mesh (without loading the asset)
	if (AssetData.AssetClassPath != USkeletalMesh::StaticClass()->GetClassPathName())
	{
		return false;
	}

	// Check if import data exists by reading asset registry tags
	// This avoids loading the full asset and potentially freezing the UI
	FString ImportDataJson;
	if (AssetData.GetTagValue(UAssetImportData::SourceFileTagName(), ImportDataJson))
	{
		// If import data tag exists, assume we can normalize (detailed validation happens in ExecuteNormalizeVrm)
		return !ImportDataJson.IsEmpty();
	}

	// Fall back to full validation if tag is not available
	// This may load the asset but ensures we don't miss valid meshes
	FString SourcePath;
	return GetSourceFilePathFromAsset(AssetData, SourcePath);
}

bool FVrmContentBrowserActions::GetSourceFilePathFromAsset(const FAssetData& AssetData, FString& OutSourcePath)
{
	OutSourcePath.Reset();

	// Load the asset to access import data
	UObject* Asset = AssetData.GetAsset();
	if (!Asset)
	{
		return false;
	}

	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset);
	if (!SkeletalMesh)
	{
		return false;
	}

	// Get asset import data
	UAssetImportData* ImportData = SkeletalMesh->GetAssetImportData();
	if (!ImportData)
	{
		return false;
	}

	// Get the first source file path (with exception handling for malformed import data)
	if (ImportData->GetSourceFileCount() == 0)
	{
		return false;
	}

	FString SourceFilePath;
	try
	{
		SourceFilePath = ImportData->GetSourceFile(0);
	}
	catch (...)
	{
		UE_LOG(LogVrmToolchainEditor, Warning, TEXT("Exception caught while reading source file from import data"));
		return false;
	}

	if (SourceFilePath.IsEmpty())
	{
		return false;
	}

	// Ensure it's a valid absolute path
	if (FPaths::IsRelative(SourceFilePath))
	{
		// Convert relative path to absolute
		SourceFilePath = FPaths::ConvertRelativePathToFull(SourceFilePath);
	}

	// Validate the file exists and has the right extension
	FString ValidationError;
	if (!FVrmNormalizationService::ValidateSourceFile(SourceFilePath, ValidationError))
	{
		return false;
	}

	OutSourcePath = SourceFilePath;
	return true;
}

void FVrmContentBrowserActions::ExecuteNormalizeVrm(const TArray<FAssetData>& SelectedAssets)
{
	if (SelectedAssets.Num() != 1)
	{
		return;
	}

	const FAssetData& AssetData = SelectedAssets[0];

	// Get source file path
	FString SourcePath;
	if (!GetSourceFilePathFromAsset(AssetData, SourcePath))
	{
		FNotificationInfo Info(NSLOCTEXT("VrmToolchain", "NormalizeFailedNoSource", "Failed to resolve source VRM file path from asset"));
		Info.ExpireDuration = 5.0f;
		Info.bUseLargeFont = false;
		Info.bFireAndForget = true;
		FSlateNotificationManager::Get().AddNotification(Info);

		UE_LOG(LogVrmToolchainEditor, Error, TEXT("Failed to resolve source file path from asset: %s"), *AssetData.AssetName.ToString());
		return;
	}

	// Perform normalization
	FVrmNormalizationOptions Options;
	Options.bAllowOverwrite = true;

	FVrmNormalizationResult Result = FVrmNormalizationService::NormalizeVrmFile(SourcePath, Options);

	// Show notification
	FNotificationInfo Info(FText::GetEmpty());
	Info.ExpireDuration = 5.0f;
	Info.bUseLargeFont = false;
	Info.bFireAndForget = true;

	if (Result.bSuccess)
	{
		Info.Text = FText::Format(
			NSLOCTEXT("VrmToolchain", "NormalizeSuccess", "VRM normalized successfully:\n{0}\n\nReport: {1}"),
			FText::FromString(Result.OutputFilePath),
			FText::FromString(Result.ReportFilePath)
		);
		Info.bUseSuccessFailIcons = true;
	}
	else
	{
		Info.Text = FText::Format(
			NSLOCTEXT("VrmToolchain", "NormalizeFailed", "VRM normalization failed:\n{0}"),
			FText::FromString(Result.ErrorMessage)
		);
		Info.bUseSuccessFailIcons = true;
	}

	FSlateNotificationManager::Get().AddNotification(Info);
}
