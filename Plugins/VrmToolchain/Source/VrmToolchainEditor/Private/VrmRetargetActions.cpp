// Copyright Epic Games, Inc. All Rights Reserved.

#include "VrmRetargetActions.h"
#include "VrmRetargetScaffoldGenerator.h"
#include "VrmToolchainEditor.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Engine/SkeletalMesh.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor.h"

static TSharedPtr<FExtender> ContentBrowserExtender;
static FDelegateHandle ContentBrowserExtenderDelegateHandle;

void FVrmRetargetActions::RegisterMenuExtensions()
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

	CBMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateLambda(
		[](const TArray<FAssetData>& SelectedAssets)
		{
			TSharedRef<FExtender> Extender = MakeShared<FExtender>();

			// Check if any selected asset is a skeletal mesh
			bool bHasSkeletalMesh = false;
			for (const FAssetData& Asset : SelectedAssets)
			{
				if (Asset.AssetClassPath == USkeletalMesh::StaticClass()->GetClassPathName())
				{
					bHasSkeletalMesh = true;
					break;
				}
			}

			if (bHasSkeletalMesh)
			{
				Extender->AddMenuExtension(
					"CommonAssetActions",
					EExtensionHook::After,
					nullptr,
					FMenuExtensionDelegate::CreateLambda([SelectedAssets](FMenuBuilder& MenuBuilder)
					{
						MenuBuilder.AddMenuEntry(
							FText::FromString(TEXT("Create Retarget Scaffolding")),
							FText::FromString(TEXT("Generate IKRig and IK Retargeter assets for retargeting")),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateLambda([SelectedAssets]()
								{
									TArray<USkeletalMesh*> SelectedMeshes;
									for (const FAssetData& Asset : SelectedAssets)
									{
										if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Asset.GetAsset()))
										{
											SelectedMeshes.Add(Mesh);
										}
									}
									ExecuteRetargetScaffold(SelectedMeshes);
								}),
								FCanExecuteAction::CreateStatic(&FVrmRetargetActions::CanExecuteRetargetScaffold)
							)
						);
					})
				);
			}

			return Extender;
		}
	));

	ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();
}

void FVrmRetargetActions::UnregisterMenuExtensions()
{
	if (ContentBrowserExtenderDelegateHandle.IsValid())
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
		CBMenuExtenderDelegates.RemoveAll([](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
		{
			return Delegate.GetHandle() == ContentBrowserExtenderDelegateHandle;
		});
	}
}

void FVrmRetargetActions::ExecuteRetargetScaffold(TArray<USkeletalMesh*> SelectedMeshes)
{
	if (SelectedMeshes.Num() == 0)
	{
		UE_LOG(LogVrmToolchainEditor, Warning, TEXT("No skeletal meshes selected"));
		return;
	}

	// For now, process first selected mesh
	USkeletalMesh* SourceMesh = SelectedMeshes[0];

	// TODO: Show dialog to select target skeleton
	// For MVP, just create source IKRig without target
	FVrmRetargetScaffoldGenerator::FScaffoldResult Result =
		FVrmRetargetScaffoldGenerator::GenerateRetargetScaffolding(SourceMesh, nullptr, false);

	if (Result.bSuccess)
	{
		UE_LOG(LogVrmToolchainEditor, Display, TEXT("Retarget scaffolding created successfully"));
		
		// Log warnings if any
		for (const FString& Warning : Result.Warnings)
		{
			UE_LOG(LogVrmToolchainEditor, Warning, TEXT("%s"), *Warning);
		}
	}
	else
	{
		UE_LOG(LogVrmToolchainEditor, Error, TEXT("Failed to create retarget scaffolding: %s"), *Result.ErrorMessage);
	}
}

bool FVrmRetargetActions::CanExecuteRetargetScaffold()
{
	return true;
}
