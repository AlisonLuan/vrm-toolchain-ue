#include "VrmToolchainRecomputeImportReportsCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "VrmToolchain/VrmMetaAsset.h"
#include "VrmMetaAssetRecomputeHelper.h"

UVrmToolchainRecomputeImportReportsCommandlet::UVrmToolchainRecomputeImportReportsCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 UVrmToolchainRecomputeImportReportsCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Log, TEXT("VrmToolchainRecomputeImportReportsCommandlet starting..."));

	// Parse flags using Unreal-standard FParse
	bool bSave = FParse::Param(*Params, TEXT("Save"));
	bool bFailOnChanges = FParse::Param(*Params, TEXT("FailOnChanges"));
	bool bFailOnFailed = !FParse::Param(*Params, TEXT("NoFailOnFailed")); // default true
	FString RootPath = TEXT("/Game");
	FParse::Value(*Params, TEXT("Root="), RootPath);

	if (bSave)
	{
		UE_LOG(LogTemp, Log, TEXT("  -Save flag detected: will save changed packages"));
	}

	if (bFailOnChanges)
	{
		UE_LOG(LogTemp, Log, TEXT("  -FailOnChanges flag detected"));
	}

	if (!bFailOnFailed)
	{
		UE_LOG(LogTemp, Log, TEXT("  -NoFailOnFailed flag detected"));
	}

	UE_LOG(LogTemp, Log, TEXT("  -Root=%s"), *RootPath);

	// Enumerate VRM meta assets via AssetRegistry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.PackagePaths.Add(FName(*RootPath));
	Filter.ClassPaths.Add(UVrmMetaAsset::StaticClass()->GetClassPathName());

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssets(Filter, Assets);

	int32 Total = Assets.Num();
	int32 Changed = 0;
	int32 Skipped = 0;
	int32 Failed = 0;

	UE_LOG(LogTemp, Log, TEXT("Found %d VRM Meta Assets in %s"), Total, *RootPath);

	TArray<UPackage*> PackagesToSave;

	for (const FAssetData& AssetData : Assets)
	{
		if (!AssetData.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid AssetData for %s"), *AssetData.PackageName.ToString());
			Failed++;
			continue;
		}

		UVrmMetaAsset* Meta = Cast<UVrmMetaAsset>(AssetData.GetAsset());
		if (!Meta)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load meta asset: %s"), *AssetData.PackageName.ToString());
			Failed++;
			continue;
		}

		using namespace VrmMetaAssetRecomputeHelper;
		FVrmRecomputeMetaResult Result = RecomputeSingleMetaAsset(Meta);

		if (Result.bFailed)
		{
			UE_LOG(LogTemp, Warning, TEXT("  [Failed] %s: %s"), *AssetData.PackageName.ToString(), *Result.Error);
			Failed++;
		}
		else if (Result.bChanged)
		{
			UE_LOG(LogTemp, Display, TEXT("  [Changed] %s"), *AssetData.PackageName.ToString());
			Changed++;
			if (bSave)
			{
				PackagesToSave.AddUnique(Meta->GetPackage());
			}
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("  [Unchanged] %s"), *AssetData.PackageName.ToString());
			Skipped++;
		}
	}

	// Save packages if requested
	if (bSave && PackagesToSave.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Saving %d changed package(s)..."), PackagesToSave.Num());

		for (UPackage* Package : PackagesToSave)
		{
			if (!Package)
			{
				continue;
			}

			const FString PackageFileName = FPackageName::LongPackageNameToFilename(
				Package->GetName(),
				FPackageName::GetAssetPackageExtension()
			);

			FSavePackageArgs SaveArgs;
			SaveArgs.TopLevelFlags = RF_Standalone;
			SaveArgs.SaveFlags = SAVE_NoError;

			if (UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs))
			{
				UE_LOG(LogTemp, Display, TEXT("  Saved: %s"), *Package->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("  Failed to save: %s"), *Package->GetName());
			}
		}
	}

	// Summary
	UE_LOG(LogTemp, Log, TEXT("Recompute complete:"));
	UE_LOG(LogTemp, Log, TEXT("  Total: %d"), Total);
	UE_LOG(LogTemp, Log, TEXT("  Changed: %d"), Changed);
	UE_LOG(LogTemp, Log, TEXT("  Unchanged: %d"), Skipped);
	UE_LOG(LogTemp, Log, TEXT("  Failed: %d"), Failed);

	// Determine exit code
	int32 ExitCode = 0;

	if (bFailOnFailed && Failed > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FailOnFailed: %d asset(s) failed, exiting with error code 1"), Failed);
		ExitCode = 1;
	}

	if (bFailOnChanges && Changed > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FailOnChanges: %d asset(s) changed, exiting with error code 2"), Changed);
		ExitCode = 2;
	}

	return ExitCode;
}
