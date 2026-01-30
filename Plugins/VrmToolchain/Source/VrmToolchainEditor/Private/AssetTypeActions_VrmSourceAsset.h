#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "VrmToolchain/VrmSourceAsset.h"
#include "VrmToolchainEditorSubsystem.h"

/**
 * Classic AssetTypeActions for UVrmSourceAsset
 * Provides Content Browser visibility and opens custom filtered asset editor via subsystem
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
		// Use the editor subsystem to manage toolkit lifetime (prevents premature destruction)
		if (GEditor)
		{
			if (UVrmToolchainEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UVrmToolchainEditorSubsystem>())
			{
				Subsystem->OpenVrmSourceAssetEditor(InObjects, EditWithinLevelEditor);
			}
		}
	}
};
