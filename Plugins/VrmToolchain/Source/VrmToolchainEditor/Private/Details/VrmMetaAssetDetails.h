#pragma once

#include "IDetailCustomization.h"

/**
 * Details panel customization for UVrmMetaAsset.
 * Displays Import Report section with summary and warnings.
 */
class FVrmMetaAssetDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	static FString BuildCopyText(const FString& Summary, const TArray<FString>& Warnings);
};
