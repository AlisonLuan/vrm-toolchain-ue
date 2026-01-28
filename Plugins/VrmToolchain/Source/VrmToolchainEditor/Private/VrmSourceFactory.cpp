#include "VrmSourceFactory.h"

#include "VrmToolchain/VrmSourceAsset.h"

// Runtime-side types we create/populate:
#include "VrmToolchain/VrmMetadata.h"
#include "VrmToolchain/VrmMetadataAsset.h"

#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "VrmToolchainEditor.h"
#include "UObject/UnrealType.h"
#include "UObject/SoftObjectPtr.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorSubsystem.h"
#include "Subsystems/ImportSubsystem.h"

// Diagnostic scanner: recursively inspect object and container properties inside a package
// and report any property that references the CDO's AssetImportData (which causes SavePackage to fail).
// When bAllowNative is true, properties declared in native classes will also be inspected (used for UAssetImportData instances).
static void ScanPropertyForCDOAssetImportData(const FProperty* Property, const void* ContainerPtr, const UAssetImportData* CDO, const FString& OwnerPath, const FString& PropPath, TSet<const void*>& Visited, bool bAllowNative = false);
static void ScanStructProperties(const UStruct* Struct, const void* StructMemory, const UAssetImportData* CDO, const FString& OwnerPath, const FString& BasePropPath, TSet<const void*>& Visited, bool bAllowNative = false)
{
    for (TFieldIterator<FProperty> It(Struct); It; ++It)
    {
        FProperty* Prop = *It;
        const void* PropPtr = Prop->ContainerPtrToValuePtr<void>(StructMemory);
        ScanPropertyForCDOAssetImportData(Prop, PropPtr, CDO, OwnerPath, BasePropPath.IsEmpty() ? Prop->GetName() : (BasePropPath + TEXT(".") + Prop->GetName()), Visited, bAllowNative);
    }
}

static void ScanPropertyForCDOAssetImportData(const FProperty* Property, const void* ContainerPtr, const UAssetImportData* CDO, const FString& OwnerPath, const FString& PropPath, TSet<const void*>& Visited, bool bAllowNative)
{
    // Safety guards: avoid runaway recursion and operating on many objects after corruption
    if (!Property || !ContainerPtr) return;
    if (Visited.Num() > 2000) // defensive limit
    {
        UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VrmSourceFactory: Scan aborted early (visited limit) Owner=%s Property=%s"), *OwnerPath, *PropPath);
        return;
    }
    // If this property's owner class is native and we're not allowed to inspect native members, skip it.
    if (!bAllowNative && Property->GetOwner<UClass>() && Property->GetOwner<UClass>()->IsNative())
    {
        return;
    }

    // Direct object pointers
    if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
    {
        UObject* Val = ObjProp->GetObjectPropertyValue_InContainer(ContainerPtr);
        if (Val == CDO)
        {
            UE_LOG(LogVrmToolchainEditor, Error, TEXT("VrmSourceFactory: Found illegal CDO AssetImportData reference! Owner=%s Property=%s Value=%p"), *OwnerPath, *PropPath, Val);
        }

        if (Val && Val->IsValidLowLevel() && !Val->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
        {
            if (!Visited.Contains(Val))
            {
                Visited.Add(Val);
                for (TFieldIterator<FProperty> It(Val->GetClass()); It; ++It)
                {
                    FProperty* P = *It;
                    const void* Ptr = P->ContainerPtrToValuePtr<void>(Val);
                    if (Ptr)
                    {
                        const FString NewPropPath = PropPath + TEXT(".") + P->GetName();
                        bool bChildAllowNative = bAllowNative || Val->IsA(UAssetImportData::StaticClass());
                        ScanPropertyForCDOAssetImportData(P, Ptr, CDO, OwnerPath + TEXT(".") + Val->GetName(), NewPropPath, Visited, bChildAllowNative);
                    }
                    else
                    {
                        UE_LOG(LogVrmToolchainEditor, Verbose, TEXT("VrmSourceFactory: Skipping null property pointer '%s' on '%s'"), *P->GetName(), *Val->GetPathName());
                    }
                }
            }
        }
    }
    // Array/Map/Set recursion
    else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
    {
        FScriptArrayHelper Helper(ArrayProp, ContainerPtr);
        int32 Num = Helper.Num();
        if (Num < 0 || Num > 10000)
        {
            UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VrmSourceFactory: Unreasonable array length %d for property '%s' on %s; skipping."), Num, *PropPath, *OwnerPath);
        }
        else
        {
            for (int32 i = 0; i < Num; ++i)
            {
                const void* ElemPtr = Helper.GetRawPtr(i);
                const FString ElemPath = FString::Printf(TEXT("%s[%d]"), *PropPath, i);
                ScanPropertyForCDOAssetImportData(ArrayProp->Inner, ElemPtr, CDO, OwnerPath, ElemPath, Visited, bAllowNative);
            }
        }
    }
    else if (const FMapProperty* MapProp = CastField<FMapProperty>(Property))
    {
        FScriptMapHelper MapHelper(MapProp, ContainerPtr);
        int32 Num = MapHelper.Num();
        if (Num < 0 || Num > 10000)
        {
            UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VrmSourceFactory: Unreasonable map length %d for property '%s' on %s; skipping."), Num, *PropPath, *OwnerPath);
        }
        else
        {
            for (int32 i = 0; i < Num; ++i)
            {
                const void* KeyPtr = MapHelper.GetKeyPtr(i);
                ScanPropertyForCDOAssetImportData(MapProp->KeyProp, KeyPtr, CDO, OwnerPath, PropPath + TEXT("[key]"), Visited, bAllowNative);
                const void* ValuePtr = MapHelper.GetValuePtr(i);
                ScanPropertyForCDOAssetImportData(MapProp->ValueProp, ValuePtr, CDO, OwnerPath, PropPath + TEXT("[value]"), Visited, bAllowNative);
            }
        }
    }
    else if (const FSetProperty* SetProp = CastField<FSetProperty>(Property))
    {
        FScriptSetHelper SetHelper(SetProp, ContainerPtr);
        int32 Num = SetHelper.Num();
        if (Num < 0 || Num > 10000)
        {
            UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VrmSourceFactory: Unreasonable set length %d for property '%s' on %s; skipping."), Num, *PropPath, *OwnerPath);
        }
        else
        {
            for (int32 i = 0; i < Num; ++i)
            {
                const void* ElemPtr = SetHelper.GetElementPtr(i);
                ScanPropertyForCDOAssetImportData(SetProp->ElementProp, ElemPtr, CDO, OwnerPath, FString::Printf(TEXT("%s[%d]"), *PropPath, i), Visited, bAllowNative);
            }
        }
    }
    // Structs: descend
    else if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
    {
        // Special-case: check SoftObjectPath content for textual matches (guarded)
        if (StructProp->Struct == FSoftObjectPath::StaticStruct())
        {
            const FSoftObjectPath* SoftPtr = StructProp->ContainerPtrToValuePtr<FSoftObjectPath>(ContainerPtr);
            if (SoftPtr)
            {
                const FString CdoPath = CDO->GetPathName();
                const FString SoftStr = SoftPtr->ToString();
                if (SoftStr.Contains(CdoPath) || SoftStr.Contains(CDO->GetName()))
                {
                    UE_LOG(LogVrmToolchainEditor, Error, TEXT("VrmSourceFactory: Found textual reference to CDO AssetImportData via SoftObjectPath! Owner=%s Property=%s Value='%s'"), *OwnerPath, *PropPath, *SoftStr);
                }
            }
        }
        const void* StructPtr = ContainerPtr;
        ScanStructProperties(StructProp->Struct, StructPtr, CDO, OwnerPath, PropPath, Visited, bAllowNative);
    }
    // FName / FString textual checks (catch indirect references stored as names/strings)
    else if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
        FName Val = NameProp->GetPropertyValue_InContainer(ContainerPtr);
        const FString S = Val.ToString();
        const FString CdoPath = CDO->GetPathName();
        if (S.Contains(CdoPath) || S.Contains(CDO->GetName()))
        {
            UE_LOG(LogVrmToolchainEditor, Error, TEXT("VrmSourceFactory: Found textual reference to CDO (Name) Owner=%s Property=%s Value='%s'"), *OwnerPath, *PropPath, *S);
        }
    }
    else if (const FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
        FString Val = StrProp->GetPropertyValue_InContainer(ContainerPtr);
        const FString CdoPath = CDO->GetPathName();
        if (Val.Contains(CdoPath) || Val.Contains(CDO->GetName()))
        {
            UE_LOG(LogVrmToolchainEditor, Error, TEXT("VrmSourceFactory: Found textual reference to CDO (String) Owner=%s Property=%s Value='%s'"), *OwnerPath, *PropPath, *Val);
        }
    }
}

static void ScanPackageForCDOAssetImportData(UPackage* Package, const UAssetImportData* CDO)
{
    if (!Package || !CDO) return;
    TSet<const void*> Visited;
    for (TObjectIterator<UObject> It; It; ++It)
    {
        UObject* Obj = *It;
        if (Obj->GetOutermost() != Package) continue;
        FString OwnerPath = Obj->GetPathName();
        if (Visited.Contains(Obj)) continue;
        Visited.Add(Obj);
        bool bAllowNativeScan = Obj->IsA(UAssetImportData::StaticClass());
        for (TFieldIterator<FProperty> ItProp(Obj->GetClass()); ItProp; ++ItProp)
        {
            FProperty* Prop = *ItProp;
            const void* PropPtr = Prop->ContainerPtrToValuePtr<void>(Obj);
            ScanPropertyForCDOAssetImportData(Prop, PropPtr, CDO, OwnerPath, Prop->GetName(), Visited, bAllowNativeScan);
        }
    }
}
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

    // Create Source asset with same name as package (required for Content Browser visibility)
    // Previously used BaseName + "_VrmSource" but that causes package/asset name mismatch
    UVrmSourceAsset* Source = NewObject<UVrmSourceAsset>(
        InParent, UVrmSourceAsset::StaticClass(), InName, AssetFlags);

#if WITH_EDITOR
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: Created Source Name='%s' Path='%s' Outer='%s'"), *Source->GetName(), *Source->GetPathName(), *Source->GetOuter()->GetName());
#endif

    Source->SourceFilename = Filename;
    Source->ImportTime = FDateTime::UtcNow();

#if WITH_EDITORONLY_DATA
    Source->SetSourceBytes(MoveTemp(Bytes));
#endif

    // Create AssetImportData if needed (must be done after removing constructor initialization to avoid CDO reference)
    if (!Source->AssetImportData)
    {
        Source->AssetImportData = NewObject<UAssetImportData>(Source, UAssetImportData::StaticClass(), NAME_None, RF_Public | RF_Transactional);
#if WITH_EDITOR
        UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: Created AssetImportData (%p) for Source"), Source->AssetImportData.Get());
#endif
    }
    
    // Update import data with source filename
    Source->AssetImportData->Update(Filename);
#if WITH_EDITOR
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: Updated AssetImportData with filename '%s'"), *Filename);
#endif

    // Create sibling Metadata asset (runtime-safe descriptor)
    UVrmMetadataAsset* MetaAsset = NewObject<UVrmMetadataAsset>(Source, TEXT("Metadata"), AssetFlags);
    Source->Descriptor = MetaAsset;

    // Parse metadata (fail-soft): ExtractVrmMetadata provides Version and basic fields
    const FVrmMetadata Parsed = FVrmParser::ExtractVrmMetadata(Filename);

    MetaAsset->SpecVersion = Parsed.Version;

    // Map simple major version for quick queries: VRM0 => 0, VRM1 => 1, Unknown => -1
    int32 Major = -1;
    if (Parsed.Version == EVrmVersion::VRM0)
    {
        Major = 0;
    }
    else if (Parsed.Version == EVrmVersion::VRM1)
    {
        Major = 1;
    }
    Source->VrmSpecVersionMajor = Major;
    Source->DetectedVrmExtension = FPaths::GetExtension(Filename).ToLower();

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
    // Ensure immediate Content Browser visibility and asset registry updates
    Source->PostEditChange();
    FAssetRegistryModule::AssetCreated(Source);

    // Broadcast import completion to subsystems (UI updates, automations, etc.)
    if (GEditor)
    {
        UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>();
        if (ImportSubsystem)
        {
            ImportSubsystem->BroadcastAssetPostImport(this, Source);
        }
    }

    // DISABLED: Diagnostic scan causes crashes when accessing certain native properties
    // Run diagnostic scan to detect any references to the class default AssetImportData
    /*
    UAssetImportData* CDOAssetImportData = UVrmSourceAsset::StaticClass()->GetDefaultObject<UVrmSourceAsset>()->AssetImportData;
    if (CDOAssetImportData)
    {
        UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VrmSourceFactory: scanning package '%s' for references to CDO AssetImportData %p"), *Source->GetOutermost()->GetName(), CDOAssetImportData);
        ScanPackageForCDOAssetImportData(Source->GetOutermost(), CDOAssetImportData);
    }
    else
    {
        UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VrmSourceFactory: CDO AssetImportData is null"));
    }
    */

    UE_LOG(LogVrmToolchainEditor, Log, TEXT("Imported VRM Source: %s (%s)"),
        *Source->GetPathName(), *Filename);
#endif

    return Source;
}
