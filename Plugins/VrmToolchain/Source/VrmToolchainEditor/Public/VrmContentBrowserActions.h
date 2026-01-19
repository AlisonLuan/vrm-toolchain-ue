#pragma once

#include "CoreMinimal.h"

/**
 * Handles Content Browser actions for VRM normalization
 */
class VRMTOOLCHAINEDITOR_API FVrmContentBrowserActions
{
public:
	/** Register Content Browser context menu extensions */
	static void RegisterMenuExtensions();

	/** Unregister Content Browser context menu extensions */
	static void UnregisterMenuExtensions();

private:
	/** Create the menu extender for skeletal mesh assets */
	static TSharedRef<FExtender> CreateAssetContextMenuExtender(const TArray<FAssetData>& SelectedAssets);

	/** Check if the Normalize VRM action should be visible */
	static bool CanNormalizeVrm(const TArray<FAssetData>& SelectedAssets);

	/** Execute the Normalize VRM action */
	static void ExecuteNormalizeVrm(const TArray<FAssetData>& SelectedAssets);

	/** Resolve source file path from asset import data */
	static bool GetSourceFilePathFromAsset(const FAssetData& AssetData, FString& OutSourcePath);

	/** Delegate handle for menu extender */
	static FDelegateHandle ContentBrowserExtenderHandle;
};
