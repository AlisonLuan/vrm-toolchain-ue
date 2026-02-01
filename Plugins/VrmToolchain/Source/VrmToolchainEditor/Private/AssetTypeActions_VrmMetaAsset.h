#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class UVrmMetaAsset;

class FAssetTypeActions_VrmMetaAsset : public FAssetTypeActions_Base
{
public:
	// FAssetTypeActions_Base
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;

private:
	static FString SanitizeForClipboardSingleLine(const FString& In);

	static UVrmMetaAsset* GetSingleMeta(const TArray<UObject*>& Objects);
	static void CollectMetas(const TArray<UObject*>& Objects, TArray<UVrmMetaAsset*>& OutMetas);

	void CopyImportSummary(const TArray<UObject*>& Objects);
	void CopyImportWarnings(const TArray<UObject*>& Objects);
	void CopyFullImportReport(const TArray<UObject*>& Objects);
};
