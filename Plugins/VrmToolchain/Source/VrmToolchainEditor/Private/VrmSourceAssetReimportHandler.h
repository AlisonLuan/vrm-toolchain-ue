#pragma once
#include "CoreMinimal.h"
#include "EditorReimportHandler.h"

class UVrmSourceAsset;

/**
 * Enables right-click Reimport for UVrmSourceAsset.
 */
class FVrmSourceAssetReimportHandler final : public FReimportHandler
{
public:
    virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
    virtual int32 GetPriority() const override { return 100; }

private:
    static bool TryGetReimportFilename(UVrmSourceAsset* Source, FString& OutFilename);
    static bool RefreshFromFile(UVrmSourceAsset* Source, const FString& Filename, FString& OutError);
};
