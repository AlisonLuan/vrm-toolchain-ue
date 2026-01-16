// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * VRM Validator
 * Provides validation functionality for VRM files
 */
class VRMTOOLCHAINEDITOR_API FVrmValidator
{
public:
	/**
	 * Validate a VRM file
	 * @param FilePath Path to the VRM file to validate
	 * @param OutErrors Array to receive error messages
	 * @param OutWarnings Array to receive warning messages
	 * @return true if validation passed (may have warnings but no errors)
	 */
	static bool ValidateFile(const FString& FilePath, TArray<FString>& OutErrors, TArray<FString>& OutWarnings);

	/**
	 * Quick validation check (file existence and extension only)
	 * @param FilePath Path to check
	 * @return true if file exists and has .vrm extension
	 */
	static bool QuickValidate(const FString& FilePath);

	/**
	 * Deep validation using VRM SDK
	 * @param FilePath Path to the VRM file to validate
	 * @param OutErrors Array to receive error messages
	 * @param OutWarnings Array to receive warning messages
	 * @return true if deep validation passed
	 */
	static bool DeepValidate(const FString& FilePath, TArray<FString>& OutErrors, TArray<FString>& OutWarnings);
};
