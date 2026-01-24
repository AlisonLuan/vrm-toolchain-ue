#include "VrmSourceFactory.h"

#include "VrmSourceAsset.h"

// Runtime-side types we create/populate:
#include "VrmToolchain/VrmMetadata.h"
#include "VrmToolchain/VrmMetadataAsset.h"

#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_EDITOR
#include "VrmToolchainEditor.h"
#endif

UVrmSourceFactory::UVrmSourceFactory()
{
    bCreateNew = false;
    bEditorImport = true;
    bText = false;

    Formats.Add(TEXT("vrm;VRM Avatar"));
    Formats.Add(TEXT("glb;glTF Binary"));

    SupportedClass = UVrmSourceAsset::StaticClass();
}

bool UVrmSourceFactory::FactoryCanImport(const FString& Filename)
{
    const FString Ext = FPaths::GetExtension(Filename).ToLower();
    return Ext == TEXT("vrm") || Ext == TEXT("glb");
}

UObject* UVrmSourceFactory::FactoryCreateFile(
    UClass* InClass,
    UObject* InParent,
    FName InName,
    EObjectFlags Flags,
    const FString& Filename,
    const TCHAR* Parms,
    FFeedbackContext* Warn,
    bool& bOutOperationCanceled)
{
    bOutOperationCanceled = false;

    // Read bytes
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Filename))
    {
        Warn->Logf(ELogVerbosity::Error, TEXT("VRM import: failed to read file: %s"), *Filename);
        return nullptr;
    }

    const EObjectFlags AssetFlags = Flags | RF_Public | RF_Standalone;
    const FString BaseName = InName.ToString();

    // Create Source asset (primary return)
    const FString SourceName = BaseName + TEXT("_VrmSource");
    UVrmSourceAsset* Source = NewObject<UVrmSourceAsset>(
        InParent, UVrmSourceAsset::StaticClass(), *SourceName, AssetFlags);

    Source->SourceFilename = Filename;
    Source->ImportTime = FDateTime::UtcNow();

#if WITH_EDITORONLY_DATA
    Source->SourceBytes = Bytes;
#endif

    if (Source->AssetImportData)
    {
        Source->AssetImportData->Update(Filename);
    }

    // Create sibling Metadata asset (runtime-safe descriptor)
    const FString MetadataName = BaseName + TEXT("_Metadata");
    UVrmMetadataAsset* MetaAsset = NewObject<UVrmMetadataAsset>(
        InParent, UVrmMetadataAsset::StaticClass(), *MetadataName, AssetFlags);

    Source->Descriptor = MetaAsset;

    // Parse metadata (fail-soft): ExtractVrmMetadata provides Version and basic fields
    const FVrmMetadata Parsed = FVrmParser::ExtractVrmMetadata(Filename);

    MetaAsset->SpecVersion = Parsed.Version;

    // Conservative mapping FVrmMetadata -> FVrmMetadataRecord
    MetaAsset->Metadata.Title       = Parsed.Name;
    MetaAsset->Metadata.Version     = Parsed.ModelVersion;
    MetaAsset->Metadata.Author      = FString::Join(Parsed.Authors, TEXT(", "));
    MetaAsset->Metadata.LicenseName = Parsed.License;

    // Leave unsupported fields empty for now
    MetaAsset->Metadata.ContactInformation.Empty();
    MetaAsset->Metadata.Reference.Empty();

    if (MetaAsset->Metadata.Title.IsEmpty() && MetaAsset->Metadata.Author.IsEmpty())
    {
        Source->ImportWarnings.Add(TEXT("Metadata extraction yielded empty Name/Authors (file may be invalid or unsupported)."));
    }

    // Preserve extra parsed fields without changing runtime structs (optional, harmless)
    if (!Parsed.Copyright.IsEmpty())
    {
        Source->ImportWarnings.Add(FString::Printf(TEXT("Copyright: %s"), *Parsed.Copyright));
    }

    MetaAsset->MarkPackageDirty();
    Source->MarkPackageDirty();

#if WITH_EDITOR
    UE_LOG(LogVrmToolchainEditor, Log, TEXT("Imported VRM Source: %s (%s)"),
        *Source->GetPathName(), *Filename);
#endif

    return Source;
}
