#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "VrmSourceFactory.generated.h"

/**
 * Factory that imports .vrm/.glb files into a UVrmSourceAsset and a sibling UVrmMetadataAsset.
 *
 * The UVrmSourceAsset is returned as the “primary” imported asset; the UVrmMetadataAsset is created
 * in the same package and referenced via UVrmSourceAsset::Descriptor.
 */
UCLASS()
class VRMTOOLCHAINEDITOR_API UVrmSourceFactory : public UFactory
{
    GENERATED_BODY()

public:
    UVrmSourceFactory();

    virtual bool FactoryCanImport(const FString& Filename) override;

    virtual UObject* FactoryCreateFile(
        UClass* InClass,
        UObject* InParent,
        FName InName,
        EObjectFlags Flags,
        const FString& Filename,
        const TCHAR* Parms,
        FFeedbackContext* Warn,
        bool& bOutOperationCanceled) override;
};
