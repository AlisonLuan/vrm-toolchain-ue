#include "Misc/AutomationTest.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "VrmAssetNaming.h"
#include "VrmToolchain/VrmMetaAsset.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmMetaNamingContractTest, "VrmToolchain.Naming.Contract.MetaAsset", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmMetaNamingContractTest::RunTest(const FString& Parameters)
{
	const FString Base = TEXT("teu.cu weird name");
	const FString AssetName = FVrmAssetNaming::MakeVrmMetaAssetName(Base);
	const FString PackagePath = FVrmAssetNaming::MakeVrmMetaPackagePath(TEXT("/Game/VRM"), Base);

	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		AddError(TEXT("Failed to create package for meta asset"));
		return false;
	}
	Package->FullyLoad();

	UVrmMetaAsset* Asset = NewObject<UVrmMetaAsset>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
	if (!Asset)
	{
		AddError(TEXT("Failed to create UVrmMetaAsset instance"));
		return false;
	}

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	ARM.Get().AssetCreated(Asset);

	Asset->MarkPackageDirty();
	Asset->PostEditChange();

	IAssetRegistry& AR = ARM.Get();
	TArray<FAssetData> Assets;
	AR.GetAssetsByPath(FName(TEXT("/Game/VRM")), Assets, true);

	FAssetData Found;
	for (const FAssetData& D : Assets)
	{
		if (D.AssetName.ToString() == AssetName)
		{
			Found = D;
			break;
		}
	}

	TestTrue(TEXT("Meta asset was registered in AssetRegistry"), Found.IsValid());
	if (!Found.IsValid())
	{
		for (const FAssetData& D : Assets)
		{
			AddInfo(FString::Printf(TEXT("Found asset: %s (ObjectPath=%s)"), *D.AssetName.ToString(), *D.GetSoftObjectPath().ToString()));
		}
		return false;
	}

	const FString PkgName = Found.PackageName.ToString();
	const FString PkgShort = FPackageName::GetShortName(PkgName);

	AddInfo(FString::Printf(TEXT("Package short actual='%s' expected='%s'"), *PkgShort, *AssetName));
	TestEqual(TEXT("Package short == AssetName"), PkgShort, AssetName);

	AddInfo(FString::Printf(TEXT("Package path actual='%s' expected='%s'"), *PkgName, *PackagePath));
	TestEqual(TEXT("Package path matches"), PkgName, PackagePath);

	const FString ExpectedObjectPath = PackagePath + TEXT(".") + AssetName;
	AddInfo(FString::Printf(TEXT("ObjectPath actual='%s' expected='%s'"), *Found.GetSoftObjectPath().ToString(), *ExpectedObjectPath));
	TestEqual(TEXT("ObjectPath matches"), Found.GetSoftObjectPath().ToString(), ExpectedObjectPath);

	Package->SetDirtyFlag(false);
	return true;
}
