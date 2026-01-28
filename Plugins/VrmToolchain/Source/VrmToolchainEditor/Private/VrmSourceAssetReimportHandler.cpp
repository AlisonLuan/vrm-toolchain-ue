#include "VrmSourceAssetReimportHandler.h"

#include "VrmToolchain/VrmSourceAsset.h"

#include "VrmToolchain/VrmMetadata.h"
#include "VrmToolchain/VrmMetadataAsset.h"

#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "VrmToolchainEditor.h"
#endif

bool FVrmSourceAssetReimportHandler::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    OutFilenames.Reset();

    UVrmSourceAsset* Source = Cast<UVrmSourceAsset>(Obj);
    if (!Source)
    {
        return false;
    }

    FString Filename;
    if (!TryGetReimportFilename(Source, Filename))
    {
        return false;
    }

    OutFilenames.Add(Filename);
    return true;
}

void FVrmSourceAssetReimportHandler::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    UVrmSourceAsset* Source = Cast<UVrmSourceAsset>(Obj);
    if (!Source || NewReimportPaths.Num() != 1)
    {
        return;
    }

    const FString& NewPath = NewReimportPaths[0];

    // Prefer storing explicit SourceFilename (more stable than import data)
    Source->SourceFilename = NewPath;

    if (Source->AssetImportData)
    {
        Source->AssetImportData->Update(NewPath);
    }

    Source->MarkPackageDirty();
}

EReimportResult::Type FVrmSourceAssetReimportHandler::Reimport(UObject* Obj)
{
    UVrmSourceAsset* Source = Cast<UVrmSourceAsset>(Obj);
    if (!Source)
    {
        return EReimportResult::Failed;
    }

    FString Filename;
    if (!TryGetReimportFilename(Source, Filename))
    {
#if WITH_EDITOR
        UE_LOG(LogVrmToolchainEditor, Warning, TEXT("Reimport failed: no filename for %s"), *Source->GetPathName());
#endif
        return EReimportResult::Failed;
    }

    FString Error;
    if (!RefreshFromFile(Source, Filename, Error))
    {
#if WITH_EDITOR
        UE_LOG(LogVrmToolchainEditor, Warning, TEXT("Reimport failed: %s"), *Error);
#endif
        return EReimportResult::Failed;
    }

    return EReimportResult::Succeeded;
}

bool FVrmSourceAssetReimportHandler::TryGetReimportFilename(UVrmSourceAsset* Source, FString& OutFilename)
{
    OutFilename.Reset();

    if (!Source)
    {
        return false;
    }

    if (!Source->SourceFilename.IsEmpty())
    {
        OutFilename = Source->SourceFilename;
        return true;
    }

    if (Source->AssetImportData)
    {
        const FString First = Source->AssetImportData->GetFirstFilename();
        if (!First.IsEmpty())
        {
            OutFilename = First;
            return true;
        }
    }

    return false;
}

bool FVrmSourceAssetReimportHandler::RefreshFromFile(UVrmSourceAsset* Source, const FString& Filename, FString& OutError)
{
    OutError.Reset();

    if (!Source)
    {
        OutError = TEXT("Source asset is null.");
        return false;
    }

    if (!FPaths::FileExists(Filename))
    {
        OutError = FString::Printf(TEXT("File does not exist: %s"), *Filename);
        return false;
    }

    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Filename))
    {
        OutError = FString::Printf(TEXT("Failed to read file: %s"), *Filename);
        return false;
    }

#if WITH_EDITORONLY_DATA
    Source->SetSourceBytes(MoveTemp(Bytes));
#endif

    Source->SourceFilename = Filename;

    if (Source->AssetImportData)
    {
        Source->AssetImportData->Update(Filename);
    }

    // Update sibling metadata asset if present
    UVrmMetadataAsset* MetaAsset = Source->Descriptor;
    if (MetaAsset)
    {
        const FVrmMetadata Parsed = FVrmParser::ExtractVrmMetadata(Filename);

        MetaAsset->SpecVersion = Parsed.Version;
        MetaAsset->Metadata.Title       = Parsed.Name;
        MetaAsset->Metadata.Version     = Parsed.ModelVersion;
        MetaAsset->Metadata.Author      = FString::Join(Parsed.Authors, TEXT(", "));
        MetaAsset->Metadata.LicenseName = Parsed.License;

        MetaAsset->Metadata.ContactInformation.Empty();
        MetaAsset->Metadata.Reference.Empty();

        MetaAsset->MarkPackageDirty();
    }
    else
    {
        // Non-fatal: allow reimport to succeed even if descriptor is missing
        Source->ImportWarnings.Add(TEXT("Reimport: Descriptor metadata asset was null; bytes/import data refreshed only."));
    }

    Source->MarkPackageDirty();
    return true;
}
