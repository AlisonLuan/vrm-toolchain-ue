#include "VrmSourceFactory.h"

#include "VrmToolchain/VrmSourceAsset.h"
#include "VrmAssetNaming.h"
#include "VrmMetaFeatureDetection.h"
#include "VrmMetaAssetRecomputeHelper.h"
#include "VrmConversionService.h"
#include "VrmImportOptions.h"

// Runtime-side types we create/populate:
#include "VrmToolchain/VrmMetadata.h"
#include "VrmToolchain/VrmMetadataAsset.h"

#include "EditorFramework/AssetImportData.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "VrmToolchainEditor.h"
#include "VrmMetaAssetImportReportHelper.h"
#include "UObject/UnrealType.h"
#include "UObject/SoftObjectPtr.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "EditorSubsystem.h"
#include "Subsystems/ImportSubsystem.h"
#include "VrmToolchain/VrmMetaAsset.h"
#include "Logging/MessageLog.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "HAL/PlatformApplicationMisc.h"
#include "PropertyEditorModule.h"
#include "Widgets/SWindow.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "VrmToolchain"

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

bool UVrmSourceFactory::ConfigureProperties()
{
    // Create transient import options object
    ImportOptions = NewObject<UVrmImportOptions>(GetTransientPackage(), NAME_None, RF_Transient);
    if (!ImportOptions)
    {
        UE_LOG(LogVrmToolchainEditor, Warning, TEXT("UVrmSourceFactory::ConfigureProperties: Failed to create UVrmImportOptions"));
        return false;
    }

    // Show modal dialog with property details
    // Use FPropertyEditorModule to display options in a details panel within a modal window
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bAllowSearch = false;
    DetailsViewArgs.bLockable = false;
    DetailsViewArgs.bUpdatesFromSelection = false;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

    TSharedRef<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    DetailsView->SetObject(ImportOptions);

    // Create a modal window to hold the details view
    TSharedRef<SWindow> DialogWindow = SNew(SWindow)
        .Title(LOCTEXT("VrmImportOptionsTitle", "VRM Import Options"))
        .ClientSize(FVector2D(500.0f, 400.0f))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        [
            DetailsView
        ];

    // Show the dialog modally - user closes it to confirm and continue with import
    TSharedPtr<SWindow> ParentWindow = nullptr;
    if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
    {
        FSlateApplication::Get().FindBestParentWindowForDialogs(ParentWindow);
    }

    GEditor->EditorAddModalWindow(DialogWindow);

    // Return true to continue with the import using the configured options
    return true;
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

    const EObjectFlags AssetFlags = Flags | RF_Public | RF_Standalone | RF_Transactional;

    // Extract base name from the incoming asset name (strip path if present)
    FString BaseName = InName.ToString();
    
    // Derive folder path from InParent (the package)
    FString ParentPath = InParent->GetName();
    FString FolderPath = ParentPath.Contains(TEXT("/")) 
        ? ParentPath.Left(ParentPath.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd))
        : TEXT("/Game/VRM_");
    
    // Generate package path and asset name with "_VrmSource" suffix (both must match)
    FString FullPackagePath = FVrmAssetNaming::MakeVrmSourcePackagePath(FolderPath, BaseName);
    FString AssetName = FVrmAssetNaming::MakeVrmSourceAssetName(BaseName);
    
    // Create the package with the suffixed name
    UPackage* Package = CreatePackage(*FullPackagePath);
    if (!Package)
    {
        Warn->Logf(ELogVerbosity::Error, TEXT("VRM import: failed to create package: %s"), *FullPackagePath);
        return nullptr;
    }
    
    Package->FullyLoad();
    
    // Create Source asset with matching name (package name == asset name for CB visibility)
    UVrmSourceAsset* Source = NewObject<UVrmSourceAsset>(
        Package, UVrmSourceAsset::StaticClass(), *AssetName, AssetFlags);

#if WITH_EDITOR
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: Created VRM Source Asset"));
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("  Package Path: %s"), *Package->GetName());
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("  Asset Name:   %s"), *Source->GetName());
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("  Object Path:  %s"), *Source->GetPathName());
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("  Outer Name:   %s"), *Source->GetOuter()->GetName());
    
    // Verify name matching (critical for Content Browser visibility)
    FString ShortPackageName = FPackageName::GetShortName(Package->GetName());
    if (ShortPackageName != Source->GetName())
    {
        UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VrmSourceFactory: Name mismatch detected! PackageShortName='%s' AssetName='%s' - Content Browser may not show asset! FullPackageName='%s' ObjectPath='%s' Expected pattern: /Game/VRM/<Name>_VrmSource.<Name>_VrmSource"), 
            *ShortPackageName, *Source->GetName(), *Package->GetName(), *Source->GetPathName());
    }

    // Editor-only invariant guard: ensure package short name equals asset name
    ensureMsgf(ShortPackageName == Source->GetName(),
        TEXT("VrmSourceFactory: Naming invariant violated! FullPackage='%s' PackageShortName='%s' AssetName='%s' ObjectPath='%s' Expected pattern: /Game/VRM/<Name>_VrmSource.<Name>_VrmSource"),
        *Package->GetName(), *ShortPackageName, *Source->GetName(), *Source->GetPathName());
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

    // --- VRM meta asset (lightweight) ---
    // Parse JSON chunk minimally to detect features. Use bytes if still available in memory.

    FString JsonStr;
    const TArray<uint8>& SourceBytesRef = Source->GetSourceBytes();
    if (SourceBytesRef.Num() > 0 && FVrmParser::ReadGlbJsonChunkFromMemory(SourceBytesRef.GetData(), SourceBytesRef.Num(), JsonStr))
    {
        // Parse VRM metadata features from JSON
        VrmMetaDetection::FVrmMetaFeatures Features = VrmMetaDetection::ParseMetaFeaturesFromJson(JsonStr);
        EVrmVersion MetaVer = Features.SpecVersion;
        bool bHasHumanoid = Features.bHasHumanoid;
        bool bHasSpring = Features.bHasSpringBones;
        bool bHasBlendOrExpr = Features.bHasBlendShapesOrExpressions;
        bool bHasThumb = Features.bHasThumbnail;

        // Log metadata detection diagnostics (canonical single-line format)
        FString DiagnosticsStr = VrmMetaDetection::FormatMetaFeaturesForDiagnostics(Features);
        UE_LOG(LogVrmToolchainEditor, Verbose, TEXT("VrmSourceFactory: VRM meta detection - file=%s %s"), *FPaths::GetCleanFilename(Filename), *DiagnosticsStr);

        // Detect parse failure or missing VRM extensions
        const bool bJsonBlank = JsonStr.TrimStartAndEnd().IsEmpty();
        const bool bLooksNonVrm = (MetaVer == EVrmVersion::Unknown) && !bHasHumanoid && !bHasSpring && !bHasBlendOrExpr && !bHasThumb;

        if (bJsonBlank)
        {
            UE_LOG(LogVrmToolchainEditor, Verbose, TEXT("VrmSourceFactory: VRM meta detection - missing/blank JSON chunk"));
        }
        else if (bLooksNonVrm)
        {
            UE_LOG(LogVrmToolchainEditor, Verbose, TEXT("VrmSourceFactory: VRM meta detection - no VRM extensions detected"));
        }

        // Create the meta asset in its own package
        const FString BaseForMeta = FVrmAssetNaming::StripKnownSuffixes(AssetName);
        const FString MetaAssetName = FVrmAssetNaming::MakeVrmMetaAssetName(BaseForMeta);
        const FString MetaPackagePath = FVrmAssetNaming::MakeVrmMetaPackagePath(FolderPath, BaseForMeta);

        UPackage* MetaPackage = CreatePackage(*MetaPackagePath);
        if (MetaPackage)
        {
            MetaPackage->FullyLoad();
            UVrmMetaAsset* Meta = NewObject<UVrmMetaAsset>(MetaPackage, *MetaAssetName, AssetFlags);
            if (Meta)
            {
                // Apply detected features to meta asset (single source of truth)
                VrmMetaDetection::ApplyFeaturesToMetaAsset(Meta, Features);
                Meta->SourceFilename = Filename;

                // Populate import report using recompute helper (single code path)
#if WITH_EDITORONLY_DATA
                {
                    using namespace VrmMetaAssetRecomputeHelper;
                    FVrmRecomputeMetaResult RecomputeResult = RecomputeSingleMetaAsset(Meta);
                    UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: Recompute helper ran - changed=%d failed=%d"),
                        RecomputeResult.bChanged ? 1 : 0, RecomputeResult.bFailed ? 1 : 0);
                }
#endif

#if WITH_EDITOR
                // Optional: toast when warnings exist (UX only; skipped in automation/commandlets/headless)
                if (!IsRunningCommandlet() && !GIsAutomationTesting && !FApp::IsUnattended())
                {
                    // Sync Content Browser to show freshly imported/recomputed meta asset
                    if (GEditor)
                    {
                        TArray<UObject*> Objects{Meta};
                        GEditor->SyncBrowserToObjects(Objects);
                    }

#if WITH_EDITORONLY_DATA
                    const int32 WarningCount = Meta->ImportWarnings.Num();
                    if (WarningCount > 0)
                    {
                        const FString Title = FString::Printf(TEXT("VRM imported with %d warning(s)"), WarningCount);

                        // Build copy payload using shared formatter
                        const FString CopyText = FVrmMetaAssetImportReportHelper::BuildCopyText(Meta->ImportSummary, Meta->ImportWarnings);

                        FNotificationInfo Info(FText::FromString(Title));
                        Info.SubText = FText::FromString(TEXT("See Meta asset Details → Import Report"));
                        Info.bFireAndForget = true;
                        Info.ExpireDuration = 6.0f;
                        Info.bUseLargeFont = false;

                        // Button: Copy report
                        Info.ButtonDetails.Add(FNotificationButtonInfo(
                            FText::FromString(TEXT("Copy Report")),
                            FText::FromString(TEXT("Copy summary and warnings to clipboard")),
                            FSimpleDelegate::CreateLambda([CopyText]()
                            {
                                FPlatformApplicationMisc::ClipboardCopy(*CopyText);
                            })
                        ));

                        FSlateNotificationManager::Get().AddNotification(Info);
                    }
#endif // WITH_EDITORONLY_DATA
                }
#endif // WITH_EDITOR

#if WITH_EDITOR
                // Optional: user-facing log (non-gating)
                FMessageLog Log(TEXT("VrmToolchain"));
#if WITH_EDITORONLY_DATA
                Log.Info(FText::FromString(Meta->ImportSummary));
                for (const FString& W : Meta->ImportWarnings)
                {
                    Log.Warning(FText::FromString(W));
                }
#else
                Log.Info(FText::FromString(TEXT("Imported VRM")));
#endif

                UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: Created VRM Meta Asset"));
                UE_LOG(LogVrmToolchainEditor, Display, TEXT("  Package Path: %s"), *MetaPackage->GetName());
                UE_LOG(LogVrmToolchainEditor, Display, TEXT("  Asset Name:   %s"), *Meta->GetName());
                UE_LOG(LogVrmToolchainEditor, Display, TEXT("  Object Path:  %s"), *Meta->GetPathName());
                UE_LOG(LogVrmToolchainEditor, Display, TEXT("  Outer Name:   %s"), *Meta->GetOuter()->GetName());
                
                FAssetRegistryModule& ARM2 = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
                ARM2.Get().AssetCreated(Meta);
                MetaPackage->MarkPackageDirty();
                Meta->PostEditChange();
#endif
            }
        }
    }
    else
    {
        UE_LOG(LogVrmToolchainEditor, Verbose, TEXT("VrmSourceFactory: No JSON chunk found for metadata detection"));
    }

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
    // Mark package dirty for proper saving
    Source->GetOutermost()->MarkPackageDirty();

    // Notify Asset Registry BEFORE any UI operations (critical ordering)
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    AssetRegistryModule.Get().AssetCreated(Source);
    
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: AssetCreated notification sent"));

    // PostEditChange to finalize property initialization (call AFTER AssetCreated)
    Source->PostEditChange();
    
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: PostEditChange completed"));

    // Sync Content Browser UI to the newly created asset for immediate visibility
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    IContentBrowserSingleton& ContentBrowser = ContentBrowserModule.Get();
    TArray<FAssetData> AssetsToSync;
    AssetsToSync.Add(FAssetData(Source));
    ContentBrowser.SyncBrowserToAssets(AssetsToSync);
    
    UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: Content Browser synced"));

    // Broadcast import completion to subsystems (UI updates, automations, etc.)
    if (GEditor)
    {
        UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>();
        if (ImportSubsystem)
        {
            ImportSubsystem->BroadcastAssetPostImport(this, Source);
            UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: BroadcastAssetPostImport completed"));
        }
    }

    // PR-21: Optional auto-generation of SkeletalMesh + Skeleton
    // Check if user enabled auto-generation in the import dialog
    const bool bShouldAutoGenerate = ImportOptions && ImportOptions->bAutoCreateSkeletalMesh;
    
    if (bShouldAutoGenerate)
    {
        UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: Auto-generation enabled, creating SkeletalMesh and Skeleton..."));
        
        USkeletalMesh* GeneratedMesh = nullptr;
        USkeleton* GeneratedSkeleton = nullptr;
        FString ConversionError;
        
		FVrmConvertOptions ConvertOptions = FVrmConversionService::MakeDefaultConvertOptions();
		ConvertOptions.bApplyGltfSkeleton = ImportOptions->bApplyGltfSkeleton;  // Allow user override from dialog
		
		const bool bConversionSuccess = FVrmConversionService::ConvertSourceToPlaceholderSkeletalMesh(
			Source, ConvertOptions, GeneratedMesh, GeneratedSkeleton, ConversionError);
        
        if (bConversionSuccess && GeneratedMesh && GeneratedSkeleton)
        {
            UE_LOG(LogVrmToolchainEditor, Display, TEXT("VrmSourceFactory: Auto-generated SkeletalMesh '%s' and Skeleton '%s'"),
                *GeneratedMesh->GetName(), *GeneratedSkeleton->GetName());
            
            // Show notification only in interactive mode (skip commandlet/automation/unattended)
            if (!IsRunningCommandlet() && !GIsAutomationTesting && !FApp::IsUnattended())
            {
                FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("VRM Import: Created %s, %s, %s"),
                    *Source->GetName(), *GeneratedSkeleton->GetName(), *GeneratedMesh->GetName())));
                Info.ExpireDuration = 5.0f;
                Info.bFireAndForget = true;
                FSlateNotificationManager::Get().AddNotification(Info);
            }
        }
        else
        {
            // Conversion failed, but import should still succeed for Source+Meta
            const FString ErrorMsg = FString::Printf(TEXT("VRM Import: Created %s, but auto-generation of SkeletalMesh failed: %s"),
                *Source->GetName(), *ConversionError);
            
            UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VrmSourceFactory: %s"), *ErrorMsg);
            
            // Add warning to Source asset for user visibility
            if (Source)
            {
                Source->ImportWarnings.Add(FString::Printf(TEXT("Auto-generation failed: %s"), *ConversionError));
                Source->MarkPackageDirty();
            }
            
            // Show notification only in interactive mode
            if (!IsRunningCommandlet() && !GIsAutomationTesting && !FApp::IsUnattended())
            {
                FNotificationInfo Info(FText::FromString(ErrorMsg));
                Info.ExpireDuration = 7.0f;
                Info.bFireAndForget = true;
                FSlateNotificationManager::Get().AddNotification(Info);
            }
            
            // Log to MessageLog for user reference
            FMessageLog("VrmToolchain").Warning(FText::FromString(ErrorMsg));
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

#undef LOCTEXT_NAMESPACE
