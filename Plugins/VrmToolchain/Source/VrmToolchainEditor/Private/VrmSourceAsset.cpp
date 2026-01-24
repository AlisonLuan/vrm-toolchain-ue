#include "VrmSourceAsset.h"

#include "EditorFramework/AssetImportData.h"

UVrmSourceAsset::UVrmSourceAsset()
{
    // Use NewObject to create instanced import data for UObject assets (safer than CreateDefaultSubobject)
    AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
}
