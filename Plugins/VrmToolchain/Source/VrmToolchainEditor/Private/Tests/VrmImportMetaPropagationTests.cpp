#if WITH_DEV_AUTOMATION_TESTS

#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Modules/ModuleManager.h"
#include "VrmToolchain/VrmMetaAsset.h"

/**
 * Helper: Resolve VRM fixture file path relative to plugin base directory
 */
static FString GetFixturePath(const FString& FixtureName)
{
	TSharedPtr<IPlugin> VrmPlugin = IPluginManager::Get().FindPlugin(TEXT("VrmToolchain"));
	if (!VrmPlugin)
	{
		return FString();
	}
	return FPaths::Combine(VrmPlugin->GetBaseDir(), TEXT("Resources/TestData/Fixtures"), FixtureName);
}

/**
 * Helper: Create a unique destination path for import artifacts
 */
static FString GetUniqueImportDestination()
{
	const FString UniqueSuffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	return FString::Printf(TEXT("/Game/Developers/VrmToolchainTest/ImportMetaPropagation_%s"), *UniqueSuffix);
}

/**
 * Helper: Find the first UVrmMetaAsset in a package path (searched recursively)
 * Does NOT rely on naming; selects by class type
 */
static UVrmMetaAsset* FindMetaAssetInPath(const FString& PackagePath)
{
	IAssetRegistry& AssetRegistry = FAssetRegistryModule::GetRegistry();
	TArray<FAssetData> Assets;
	AssetRegistry.GetAssetsByPath(*PackagePath, Assets, /*bRecursive*/ true);

	for (const FAssetData& AssetData : Assets)
	{
		// Check if this asset's class matches UVrmMetaAsset
		if (AssetData.AssetClassPath == UVrmMetaAsset::StaticClass()->GetClassPathName())
		{
			// Load the asset
			UObject* LoadedAsset = AssetData.GetAsset();
			if (UVrmMetaAsset* MetaAsset = Cast<UVrmMetaAsset>(LoadedAsset))
			{
				return MetaAsset;
			}
		}
	}

	return nullptr;
}

// ==============================================================================
// Test: VRM0 import via actual UE import pipeline produces correct meta asset
// ==============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVrmImportMetaPropagation_VRM0AllFeatures,
	"VrmToolchain.ImportMetaPropagation.VRM0AllFeatures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmImportMetaPropagation_VRM0AllFeatures::RunTest(const FString& Parameters)
{
	// Resolve fixture file path using plugin base directory
	FString FixturePath = GetFixturePath(TEXT("AvatarSample_VRM0.vrm"));
	if (!FPaths::FileExists(FixturePath))
	{
		AddError(FString::Printf(TEXT("VRM0 fixture not found at: %s"), *FixturePath));
		return false;
	}

	// Create unique destination folder for this test run
	FString DestinationPath = GetUniqueImportDestination();

	// Create import task
	UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
	if (!ImportTask)
	{
		AddError(TEXT("Failed to create UAssetImportTask"));
		return false;
	}

	ImportTask->Filename = FixturePath;
	ImportTask->DestinationPath = DestinationPath;
	ImportTask->DestinationName = FString();
	ImportTask->bAutomated = true;
	ImportTask->bReplaceExisting = true;
	ImportTask->bSave = false;  // Don't persist assets to disk

	// Execute import using AssetTools
	TArray<UAssetImportTask*> ImportTasks;
	ImportTasks.Add(ImportTask);

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	AssetToolsModule.Get().ImportAssetTasks(ImportTasks);

	// Brief delay to allow asset registry to update
	FPlatformProcess::Sleep(0.1f);

	// Scan destination for imported meta asset
	UVrmMetaAsset* MetaAsset = FindMetaAssetInPath(DestinationPath);
	if (!MetaAsset)
	{
		AddError(TEXT("No UVrmMetaAsset found after VRM0 import. Factory may not be wired correctly."));
		return false;
	}

	// Verify all meta fields propagated correctly from import
	TestEqual(TEXT("VRM0 SpecVersion"), MetaAsset->SpecVersion, EVrmVersion::VRM0);
	TestTrue(TEXT("VRM0 bHasHumanoid"), MetaAsset->bHasHumanoid);
	TestTrue(TEXT("VRM0 bHasSpringBones"), MetaAsset->bHasSpringBones);
	TestTrue(TEXT("VRM0 bHasBlendShapesOrExpressions"), MetaAsset->bHasBlendShapesOrExpressions);
	TestTrue(TEXT("VRM0 bHasThumbnail"), MetaAsset->bHasThumbnail);

	return true;
}

// ==============================================================================
// Test: VRM1 import via actual UE import pipeline produces correct meta asset
// ==============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVrmImportMetaPropagation_VRM1AllFeatures,
	"VrmToolchain.ImportMetaPropagation.VRM1AllFeatures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmImportMetaPropagation_VRM1AllFeatures::RunTest(const FString& Parameters)
{
	// Resolve fixture file path using plugin base directory
	FString FixturePath = GetFixturePath(TEXT("AvatarSample_VRM1.vrm"));
	if (!FPaths::FileExists(FixturePath))
	{
		AddError(FString::Printf(TEXT("VRM1 fixture not found at: %s"), *FixturePath));
		return false;
	}

	// Create unique destination folder for this test run
	FString DestinationPath = GetUniqueImportDestination();

	// Create import task
	UAssetImportTask* ImportTask = NewObject<UAssetImportTask>();
	if (!ImportTask)
	{
		AddError(TEXT("Failed to create UAssetImportTask"));
		return false;
	}

	ImportTask->Filename = FixturePath;
	ImportTask->DestinationPath = DestinationPath;
	ImportTask->DestinationName = FString();
	ImportTask->bAutomated = true;
	ImportTask->bReplaceExisting = true;
	ImportTask->bSave = false;  // Don't persist assets to disk

	// Execute import using AssetTools
	TArray<UAssetImportTask*> ImportTasks;
	ImportTasks.Add(ImportTask);

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	AssetToolsModule.Get().ImportAssetTasks(ImportTasks);

	// Brief delay to allow asset registry to update
	FPlatformProcess::Sleep(0.1f);

	// Scan destination for imported meta asset
	UVrmMetaAsset* MetaAsset = FindMetaAssetInPath(DestinationPath);
	if (!MetaAsset)
	{
		AddError(TEXT("No UVrmMetaAsset found after VRM1 import. Factory may not be wired correctly."));
		return false;
	}

	// Verify all meta fields propagated correctly from import
	TestEqual(TEXT("VRM1 SpecVersion"), MetaAsset->SpecVersion, EVrmVersion::VRM1);
	TestTrue(TEXT("VRM1 bHasHumanoid"), MetaAsset->bHasHumanoid);
	TestTrue(TEXT("VRM1 bHasSpringBones"), MetaAsset->bHasSpringBones);
	TestTrue(TEXT("VRM1 bHasBlendShapesOrExpressions"), MetaAsset->bHasBlendShapesOrExpressions);
	TestTrue(TEXT("VRM1 bHasThumbnail"), MetaAsset->bHasThumbnail);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
