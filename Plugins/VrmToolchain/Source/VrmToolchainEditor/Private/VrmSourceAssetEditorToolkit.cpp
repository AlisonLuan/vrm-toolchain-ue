#include "VrmSourceAssetEditorToolkit.h"

#include "VrmToolchain/VrmSourceAsset.h"
#include "VrmToolchainEditorSubsystem.h"
#include "VrmToolchainEditor.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "VrmSourceAssetEditorToolkit"

const FName FVrmSourceAssetEditorToolkit::DetailsTabId(TEXT("VrmSourceAssetEditor_Details"));

FVrmSourceAssetEditorToolkit::~FVrmSourceAssetEditorToolkit()
{
}

void FVrmSourceAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	// Call parent first
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	// Cache the tab manager weakly for use in BringToolkitToFront if GetTabManager() is not available
	WeakTabManager = InTabManager;

	UE_LOG(LogVrmToolchainEditor, Display, TEXT("[VrmSourceAssetEditor] RegisterTabSpawners DetailsTabId=%s"), *DetailsTabId.ToString());

	// Register our details tab
	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FVrmSourceAssetEditorToolkit::SpawnDetailsTab))
		.SetDisplayName(LOCTEXT("DetailsTabLabel", "Details"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FVrmSourceAssetEditorToolkit::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	// Unregister our details tab
	InTabManager->UnregisterTabSpawner(DetailsTabId);

	// Call parent
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
}

void FVrmSourceAssetEditorToolkit::InitEditor(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	const TArray<UObject*>& InObjects,
	UVrmToolchainEditorSubsystem* InOwningSubsystem)
{
	// Store owning subsystem for cleanup on close
	this->OwningSubsystem = InOwningSubsystem;

	// Store editing assets
	EditingAssets.Reset();
	for (UObject* Obj : InObjects)
	{
		if (UVrmSourceAsset* VrmAsset = Cast<UVrmSourceAsset>(Obj))
		{
			EditingAssets.Add(VrmAsset);

			// Log asset info for diagnostics
#if WITH_EDITORONLY_DATA
			UE_LOG(LogVrmToolchainEditor, Log, TEXT("[VrmSourceAssetEditor] Opening: %s (SourceBytes: %d bytes)"),
				*VrmAsset->GetPathName(),
				VrmAsset->GetSourceBytes().Num());
#else
			UE_LOG(LogVrmToolchainEditor, Log, TEXT("[VrmSourceAssetEditor] Opening: %s"),
				*VrmAsset->GetPathName());
#endif
		}
	}

	// Define the layout - just a single details tab
	const FString LayoutNameStr = TEXT("Standalone_VrmSourceAssetEditor_v2");
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout(*LayoutNameStr)
		->AddArea
		(
			FTabManager::NewPrimaryArea()
				->SetOrientation(Orient_Vertical)
				->Split
				(
					FTabManager::NewStack()
						->AddTab(DetailsTabId, ETabState::OpenedTab)
						->SetHideTabWell(true)
				)
			);

	UE_LOG(LogVrmToolchainEditor, Display, TEXT("[VrmSourceAssetEditor] InitEditor: DetailsTabId=%s, LayoutName=%s (expected 'Standalone_VrmSourceAssetEditor_v1')"), *DetailsTabId.ToString(), *LayoutNameStr);

	// Initialize the asset editor - RegisterTabSpawners will be called automatically
	FAssetEditorToolkit::InitAssetEditor(
		Mode,
		InitToolkitHost,
		FName("VrmSourceAssetEditorApp"),
		Layout,
		/*bCreateDefaultStandaloneMenu=*/ true,
		/*bCreateDefaultToolbar=*/ true,
		InObjects);
}

TSharedRef<SDockTab> FVrmSourceAssetEditorToolkit::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	// Create the details view with property filtering
	FPropertyEditorModule* PropertyEditorModulePtr = FModuleManager::Get().GetModulePtr<FPropertyEditorModule>("PropertyEditor");
	if (!PropertyEditorModulePtr)
	{
		// Fall back to loading (rare) - prefer GetModulePtr so tests can stub or avoid load
		PropertyEditorModulePtr = &FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	}

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	TSharedPtr<IDetailsView> LocalDetailsView = PropertyEditorModulePtr->CreateDetailView(DetailsViewArgs);
	if (!LocalDetailsView.IsValid())
	{
		UE_LOG(LogVrmToolchainEditor, Warning, TEXT("[VrmSourceAssetEditor] SpawnDetailsTab: Failed to create DetailsView - returning error widget"));

		return SNew(SDockTab)
			.Label(LOCTEXT("DetailsTabLabel", "Details"))
			[
				SNew(STextBlock).Text(LOCTEXT("DetailsFailed", "Failed to create Details view"))
			];
	}

	// Store the valid details view
	DetailsView = LocalDetailsView;

	// Hard-filter properties: hide heavy/debug fields that would freeze the UI
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda(
		[](const FPropertyAndParent& PropertyAndParent) -> bool
		{
			const FName PropertyName = PropertyAndParent.Property.GetFName();

			// Always hide these heavy/internal properties
			static const TSet<FName> HiddenProperties = {
				FName(TEXT("SourceBytes")),          // ~15MB TArray<uint8> - causes freeze
				FName(TEXT("ValidationJsonDump")),   // Debug field if present
				FName(TEXT("RawGltfJson")),          // Debug field if present
			};

			if (HiddenProperties.Contains(PropertyName))
			{
				return false;
			}

			return true;
		}));

	// Set the objects to edit
	TArray<UObject*> ObjectsToEdit;
	for (const TWeakObjectPtr<UVrmSourceAsset>& WeakAsset : EditingAssets)
	{
		if (UVrmSourceAsset* Asset = WeakAsset.Get())
		{
			ObjectsToEdit.Add(Asset);
		}
	}
	DetailsView->SetObjects(ObjectsToEdit);

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabLabel", "Details"))
		[
			DetailsView.ToSharedRef()
		];
}

bool FVrmSourceAssetEditorToolkit::OnRequestClose(EAssetEditorCloseReason InCloseReason)
{
	UE_LOG(LogVrmToolchainEditor, Display, TEXT("[VrmSourceAssetEditor] OnRequestClose reason=%d"), (int32)InCloseReason);

	// Notify the subsystem to release the strong reference BEFORE we start closing
	// This is safer than OnToolkitHostingFinished because the toolkit is still valid here
	if (UVrmToolchainEditorSubsystem* Subsystem = OwningSubsystem.Get())
	{
		// Use AsShared() while we're still valid
		Subsystem->UnregisterToolkit(StaticCastSharedRef<FVrmSourceAssetEditorToolkit>(AsShared()));
		
		// Clear the weak pointer to avoid double-unregister
		OwningSubsystem.Reset();
	}

	// Call parent to proceed with closing
	return FAssetEditorToolkit::OnRequestClose(InCloseReason);
}

bool FVrmSourceAssetEditorToolkit::IsEditingObject(const UObject* Obj) const
{
	if (!Obj)
	{
		return false;
	}

	for (const TWeakObjectPtr<UVrmSourceAsset>& WeakAsset : EditingAssets)
	{
		if (WeakAsset.IsValid() && WeakAsset.Get() == Obj)
		{
			return true;
		}
	}

	return false;
}

void FVrmSourceAssetEditorToolkit::BringToolkitToFront()
{
	// Prefer the toolkit's TabManager if available
	TSharedPtr<FTabManager> LocalTabManager;

	// Try to use GetTabManager() when available (the toolkit's own manager)
	LocalTabManager = GetTabManager();
	if (LocalTabManager.IsValid())
	{
		LocalTabManager->TryInvokeTab(DetailsTabId);
		return;
	}

	// Fall back to the cached weak tab manager if present
	if (TSharedPtr<FTabManager> Pinned = WeakTabManager.Pin())
	{
		Pinned->TryInvokeTab(DetailsTabId);
	}
}
