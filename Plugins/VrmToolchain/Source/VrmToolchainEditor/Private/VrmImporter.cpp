// Copyright Epic Games, Inc. All Rights Reserved.

#include "VrmImporter.h"
#include "VrmSdkWrapper.h"
#include "Engine/StaticMesh.h"

UVrmImporterFactory::UVrmImporterFactory()
{
	bCreateNew = false;
	bEditAfterNew = false;
	bEditorImport = true;
	SupportedClass = UStaticMesh::StaticClass();

	Formats.Add(TEXT("vrm;VRM Model File"));
}

UObject* UVrmImporterFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	bOutOperationCanceled = false;

	// Validate the VRM file
	TArray<FString> Errors;
	if (!FVrmSdkWrapper::ValidateVrmFile(Filename, Errors))
	{
		for (const FString& Error : Errors)
		{
			if (Warn)
			{
				Warn->Logf(ELogVerbosity::Error, TEXT("VRM Import Error: %s"), *Error);
			}
		}
		return nullptr;
	}

	// TODO: Implement actual VRM import using the VRM SDK
	// For now, return nullptr to indicate import is not yet fully implemented
	if (Warn)
	{
		Warn->Logf(ELogVerbosity::Warning, TEXT("VRM import is not yet fully implemented. File validated successfully: %s"), *Filename);
	}

	return nullptr;
}

bool UVrmImporterFactory::FactoryCanImport(const FString& Filename)
{
	return Filename.EndsWith(TEXT(".vrm"), ESearchCase::IgnoreCase);
}
