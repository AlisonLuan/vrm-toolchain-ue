#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "VrmAssetNaming.h"
#include "VrmToolchain/VrmSourceAsset.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmNamingContractSourceAssetTest, "VrmToolchain.Naming.Contract.SourceAsset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmNamingContractSourceAssetTest::RunTest(const FString& Parameters)
{
	// Base name with tricky characters
	const FString Base = TEXT("teu.cu weird name");

	// Use the canonical naming helpers
	const FString AssetName = FVrmAssetNaming::MakeVrmSourceAssetName(Base);
	const FString PackagePath = FVrmAssetNaming::MakeVrmSourcePackagePath(TEXT("/Game/VRM"), Base);

	// Create package and asset (editor-only, in-memory)
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		AddError(TEXT("Failed to create package"));
		return false;
	}
	Package->FullyLoad();

	UVrmSourceAsset* Asset = NewObject<UVrmSourceAsset>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	if (!Asset)
	{
		AddError(TEXT("Failed to create UVrmSourceAsset instance"));
		return false;
	}

	// Register with Asset Registry
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	ARM.Get().AssetCreated(Asset);

	// Mark dirty and finalize
	Asset->MarkPackageDirty();
	Asset->PostEditChange();

	// Query assets under /Game/VRM
	IAssetRegistry& AR = ARM.Get();
	TArray<FAssetData> Assets;
	AR.GetAssetsByPath(FName(TEXT("/Game/VRM")), Assets, true);

	// Find our asset
	FAssetData Found;
	for (const FAssetData& D : Assets)
	{
		if (D.AssetName.ToString() == AssetName)
		{
			Found = D;
			break;
		}
	}

	TestTrue(TEXT("Asset was registered in AssetRegistry"), Found.IsValid());
	if (!Found.IsValid())
	{
		// Asset not found - dump summary and fail with clear diagnostics
		AddError(FString::Printf(TEXT("VrmNamingContractTests: Asset NOT FOUND. Expected PackagePath='%s' AssetName='%s' ExpectedObjectPath='%s' AssetsReturned=%d"),
			*PackagePath, *AssetName, *FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName), Assets.Num()));

		// Optionally dump assets for debugging
		for (const FAssetData& D : Assets)
		{
			AddInfo(FString::Printf(TEXT("Found asset: %s (ObjectPath=%s)"), *D.AssetName.ToString(), *D.GetSoftObjectPath().ToString()));
		}

		return false;
	}

	const FString PkgName = Found.PackageName.ToString();
	const FString PkgShort = FPackageName::GetShortName(PkgName);

	// Log actual vs expected before assertions to aid debugging
	AddInfo(FString::Printf(TEXT("Package short actual='%s' expected='%s'"), *PkgShort, *AssetName));
	TestEqual(TEXT("Package short == AssetName"), PkgShort, AssetName);

	AddInfo(FString::Printf(TEXT("Package path actual='%s' expected='%s'"), *PkgName, *PackagePath));
	TestEqual(TEXT("Package path matches"), PkgName, PackagePath);

	const FString ExpectedObjectPath = PackagePath + TEXT(".") + AssetName;
	AddInfo(FString::Printf(TEXT("ObjectPath actual='%s' expected='%s'"), *Found.GetSoftObjectPath().ToString(), *ExpectedObjectPath));
	TestEqual(TEXT("ObjectPath matches"), Found.GetSoftObjectPath().ToString(), ExpectedObjectPath);

	// Cleanup: clear dirty flag to avoid leaving package dirty
	Package->SetDirtyFlag(false);

	return true;
}
