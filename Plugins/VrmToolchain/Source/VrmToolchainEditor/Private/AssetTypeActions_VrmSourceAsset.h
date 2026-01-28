#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "VrmToolchain/VrmSourceAsset.h"
#include "VrmSourceAssetEditorToolkit.h"

/**
 * Classic AssetTypeActions for UVrmSourceAsset
 * Provides Content Browser visibility and opens custom filtered asset editor
 */
class FAssetTypeActions_VrmSourceAsset : public FAssetTypeActions_Base
{
public:
	// FAssetTypeActions_Base interface
	virtual FText GetName() const override { return NSLOCTEXT("VrmToolchain", "VrmSourceAssetTypeActions", "VRM Source"); }
	virtual FColor GetTypeColor() const override { return FColor(63, 126, 255); }
	virtual UClass* GetSupportedClass() const override { return UVrmSourceAsset::StaticClass(); }
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }

	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override
	{
		EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
		
		// Use custom toolkit with filtered Details panel (hides SourceBytes to prevent UI freeze)
		TSharedRef<FVrmSourceAssetEditorToolkit> EditorToolkit = MakeShared<FVrmSourceAssetEditorToolkit>();
		EditorToolkit->InitEditor(Mode, EditWithinLevelEditor, InObjects);
	}
};
