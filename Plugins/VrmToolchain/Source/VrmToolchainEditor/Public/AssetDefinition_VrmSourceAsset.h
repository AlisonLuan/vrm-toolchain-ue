#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "VrmSourceAsset.h"
#include "AssetDefinition_VrmSourceAsset.generated.h"

/**
 * Asset Definition for UVrmSourceAsset - enables Content Browser display and actions
 */
UCLASS()
class VRMTOOLCHAINEDITOR_API UAssetDefinition_VrmSourceAsset : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	// UAssetDefinition interface
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_VrmSource", "VRM Source"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(63, 126, 255)); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UVrmSourceAsset::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const auto Categories = { EAssetCategoryPaths::Misc };
		return Categories;
	}

	// Don't override OpenAssets - let base class handle it with generic property editor
};
