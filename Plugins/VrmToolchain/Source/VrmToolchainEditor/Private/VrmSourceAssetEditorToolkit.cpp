#include "VrmSourceAssetEditorToolkit.h"

#include "VrmToolchain/VrmSourceAsset.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "VrmSourceAssetEditorToolkit"

const FName FVrmSourceAssetEditorToolkit::DetailsTabId(TEXT("VrmSourceAssetEditor_Details"));

FVrmSourceAssetEditorToolkit::~FVrmSourceAssetEditorToolkit()
{
}

void FVrmSourceAssetEditorToolkit::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	// Call parent first
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

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
	const TArray<UObject*>& InObjects)
{
	// Store editing assets
	EditingAssets.Reset();
	for (UObject* Obj : InObjects)
	{
		if (UVrmSourceAsset* VrmAsset = Cast<UVrmSourceAsset>(Obj))
		{
			EditingAssets.Add(VrmAsset);

			// Log asset info for diagnostics
#if WITH_EDITORONLY_DATA
			UE_LOG(LogTemp, Log, TEXT("[VrmSourceAssetEditor] Opening: %s (SourceBytes: %d bytes)"),
				*VrmAsset->GetPathName(),
				VrmAsset->GetSourceBytes().Num());
#else
			UE_LOG(LogTemp, Log, TEXT("[VrmSourceAssetEditor] Opening: %s"),
				*VrmAsset->GetPathName());
#endif
		}
	}

	// Define the layout - just a single details tab
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("Standalone_VrmSourceAssetEditor_v2")
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
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

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

#undef LOCTEXT_NAMESPACE
