// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * VRM SDK API Wrapper
 * Provides a minimal public API wrapper around the vrm-sdk functionality
 */
class VRMTOOLCHAINRUNTIME_API FVrmSdkWrapper
{
public:
	/**
	 * Initialize the VRM SDK
	 * @return true if initialization was successful
	 */
	static bool Initialize();

	/**
	 * Shutdown the VRM SDK
	 */
	static void Shutdown();

	/**
	 * Check if the VRM SDK is available
	 * @return true if the SDK is loaded and available
	 */
	static bool IsAvailable();

	/**
	 * Get the VRM SDK version string
	 * @return Version string or empty if SDK is not available
	 */
	static FString GetVersion();

	/**
	 * Validate a VRM file
	 * @param FilePath Path to the VRM file to validate
	 * @param OutErrors Array to receive error messages if validation fails
	 * @return true if the file is a valid VRM file
	 */
	static bool ValidateVrmFile(const FString& FilePath, TArray<FString>& OutErrors);

private:
	static bool bIsInitialized;
};
