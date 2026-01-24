#include "VrmSourceAsset.h"

#include "EditorFramework/AssetImportData.h"

UVrmSourceAsset::UVrmSourceAsset()
{
    AssetImportData = CreateDefaultSubobject<UAssetImportData>(TEXT("AssetImportData"));
}
