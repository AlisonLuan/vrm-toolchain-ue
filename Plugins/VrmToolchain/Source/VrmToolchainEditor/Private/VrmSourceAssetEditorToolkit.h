#pragma once

#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

class UVrmSourceAsset;
class IDetailsView;
class FTabManager;
class UVrmToolchainEditorSubsystem;

/**
 * Minimal custom asset editor for UVrmSourceAsset.
 * Uses a filtered DetailsView to hide heavy properties (SourceBytes) that would freeze the UI.
 */
class FVrmSourceAssetEditorToolkit : public FAssetEditorToolkit
{
public:
	virtual ~FVrmSourceAssetEditorToolkit() override;

	/**
	 * Initialize the editor with the given VRM source assets.
	 * @param Mode Standalone or WorldCentric
	 * @param InitToolkitHost Optional parent toolkit host
	 * @param InObjects The UVrmSourceAsset objects to edit
	 * @param InOwningSubsystem The subsystem managing this toolkit's lifetime
	 */
	void InitEditor(
		const EToolkitMode::Type Mode,
		const TSharedPtr<IToolkitHost>& InitToolkitHost,
		const TArray<UObject*>& InObjects,
		UVrmToolchainEditorSubsystem* InOwningSubsystem);

	// Additional helpers
	bool IsEditingObject(const UObject* Obj) const;
	void BringToolkitToFront();

	// FAssetEditorToolkit interface
	virtual FName GetToolkitFName() const override { return FName("VrmSourceAssetEditor"); }
	virtual FText GetBaseToolkitName() const override { return NSLOCTEXT("VrmToolchain", "VrmSourceAssetEditorToolkit", "VRM Source Editor"); }
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("VrmSourceAsset"); }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.25f, 0.5f, 1.0f, 0.5f); }

	// Tab spawner registration
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

protected:
	// Called when the toolkit is being closed - notify subsystem to release reference
	virtual bool OnRequestClose(EAssetEditorCloseReason InCloseReason) override;

private:
	/** Create the Details panel with property filtering */
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);

	/** The objects being edited */
	TArray<TWeakObjectPtr<UVrmSourceAsset>> EditingAssets;

	/** The filtered details view */
	TSharedPtr<IDetailsView> DetailsView;

	/** The subsystem that owns this toolkit (for unregistration on close) */
	TWeakObjectPtr<UVrmToolchainEditorSubsystem> OwningSubsystem;

	/** Weak reference to the tab manager used to spawn our Details tab (if available) */
	TWeakPtr<FTabManager> WeakTabManager;

	/** Tab ID for the details panel */
	static const FName DetailsTabId;
};
