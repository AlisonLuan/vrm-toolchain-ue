// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "VrmImporter.generated.h"

/**
 * Factory for importing VRM files
 */
UCLASS()
class VRMTOOLCHAINEDITOR_API UVrmImporterFactory : public UFactory
{
	GENERATED_BODY()

public:
	UVrmImporterFactory();

	// UFactory interface
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	virtual bool FactoryCanImport(const FString& Filename) override;
	// End of UFactory interface
};
