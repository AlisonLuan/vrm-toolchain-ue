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

	// Parse flags
	bool bSave = false;
	bool bFailOnChanges = false;
	bool bFailOnFailed = true; // default true
	FString RootPath = TEXT("/Game");

	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamsMap;
	ParseCommandLine(*Params, Tokens, Switches);

	if (Switches.Contains(TEXT("Save")))
	{
		bSave = true;
		UE_LOG(LogTemp, Log, TEXT("  -Save flag detected: will save changed packages"));
	}

	if (Switches.Contains(TEXT("FailOnChanges")))
	{
		bFailOnChanges = true;
		UE_LOG(LogTemp, Log, TEXT("  -FailOnChanges flag detected"));
	}

	if (Switches.Contains(TEXT("NoFailOnFailed")))
	{
		bFailOnFailed = false;
		UE_LOG(LogTemp, Log, TEXT("  -NoFailOnFailed flag detected"));
	}

	// Parse -Root= parameter
	for (const FString& Token : Tokens)
	{
		if (Token.StartsWith(TEXT("Root=")))
		{
			RootPath = Token.Mid(5); // Skip "Root="
			UE_LOG(LogTemp, Log, TEXT("  -Root=%s detected"), *RootPath);
			break;
		}
	}

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
		ERecomputeResult Result = RecomputeSingleMetaAsset(Meta);

		switch (Result)
		{
		case ERecomputeResult::Success:
			UE_LOG(LogTemp, Display, TEXT("  [Changed] %s"), *AssetData.PackageName.ToString());
			Changed++;
			if (bSave)
			{
				PackagesToSave.AddUnique(Meta->GetPackage());
			}
			break;

		case ERecomputeResult::Unchanged:
			UE_LOG(LogTemp, Verbose, TEXT("  [Unchanged] %s"), *AssetData.PackageName.ToString());
			Skipped++;
			break;

		case ERecomputeResult::Failed:
			UE_LOG(LogTemp, Warning, TEXT("  [Failed] %s"), *AssetData.PackageName.ToString());
			Failed++;
			break;
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

	if (bFailOnChanges && Changed > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FailOnChanges: %d asset(s) changed, exiting with error"), Changed);
		ExitCode = 1;
	}

	if (bFailOnFailed && Failed > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("FailOnFailed: %d asset(s) failed, exiting with error"), Failed);
		ExitCode = 1;
	}

	return ExitCode;
}
