#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "VrmToolchain/VrmSourceAsset.h"
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
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("VrmToolchain", "VrmSourceAsset", "VRM Source"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(63, 126, 255)); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UVrmSourceAsset::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		// Use standard Misc category so Content Browser filters work correctly
		static const FAssetCategoryPath Categories[] = { EAssetCategoryPaths::Misc };
		return Categories;
	}

	// Don't override OpenAssets - let base class handle it with generic property editor
};
