// Copyright Epic Games, Inc. All Rights Reserved.

#include "VrmValidator.h"
#include "VrmSdkWrapper.h"
#include "HAL/FileManager.h"

bool FVrmValidator::ValidateFile(const FString& FilePath, TArray<FString>& OutErrors, TArray<FString>& OutWarnings)
{
	// First do quick validation
	if (!QuickValidate(FilePath))
	{
		OutErrors.Add(FString::Printf(TEXT("Quick validation failed for: %s"), *FilePath));
		return false;
	}

	// Then do deep validation
	return DeepValidate(FilePath, OutErrors, OutWarnings);
}

bool FVrmValidator::QuickValidate(const FString& FilePath)
{
	// Check if file exists
	if (!FPaths::FileExists(FilePath))
	{
		return false;
	}

	// Check if file has .vrm extension
	if (!FilePath.EndsWith(TEXT(".vrm")))
	{
		return false;
	}

	return true;
}

bool FVrmValidator::DeepValidate(const FString& FilePath, TArray<FString>& OutErrors, TArray<FString>& OutWarnings)
{
	// Use VRM SDK wrapper for validation
	if (!FVrmSdkWrapper::ValidateVrmFile(FilePath, OutErrors))
	{
		return false;
	}

	// TODO: Add more detailed validation checks using VRM SDK
	// For now, basic validation is sufficient

	// Check file size (warn if too large)
	int64 FileSize = IFileManager::Get().FileSize(*FilePath);
	if (FileSize > 100 * 1024 * 1024) // 100 MB
	{
		OutWarnings.Add(FString::Printf(TEXT("File is very large (%lld bytes). This may cause performance issues."), FileSize));
	}

	return true;
}
