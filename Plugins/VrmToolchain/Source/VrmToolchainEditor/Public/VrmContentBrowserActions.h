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

	/**
	 * Resolve the source file path from asset import data.
	 *
	 * @param AssetData     The asset whose import data should be inspected.
	 * @param OutSourcePath Populated with the validated source file path on success.
	 *
	 * @return true if a source file path was found, the file exists on disk,
	 *         has a .vrm or .glb extension, and OutSourcePath was populated
	 *         with the validated absolute path; false otherwise.
	 */
	static bool GetSourceFilePathFromAsset(const FAssetData& AssetData, FString& OutSourcePath);

	/** Delegate handle for menu extender */
	static FDelegateHandle ContentBrowserExtenderHandle;
};
