// Copyright Epic Games, Inc. All Rights Reserved.

#include "VrmSdkWrapper.h"
#include "VrmToolchainRuntime.h"
#include "HAL/PlatformAtomics.h"

static volatile int32 GVrmSdkInitialized = 0;

bool FVrmSdkWrapper::Initialize()
{
	if (FPlatformAtomics::InterlockedCompareExchange(&GVrmSdkInitialized, 1, 0) != 0)
	{
		return true;
	}

	// TODO: Initialize VRM SDK when available
	// For now, just mark as initialized

	UE_LOG(LogVrmToolchain, Log, TEXT("VRM SDK Wrapper initialized"));
	return true;
}

void FVrmSdkWrapper::Shutdown()
{
	if (FPlatformAtomics::InterlockedCompareExchange(&GVrmSdkInitialized, 0, 1) != 1)
	{
		return;
	}

	// TODO: Cleanup VRM SDK resources when available

	UE_LOG(LogVrmToolchain, Log, TEXT("VRM SDK Wrapper shutdown"));
}

bool FVrmSdkWrapper::IsAvailable()
{
	return FPlatformAtomics::AtomicRead(&GVrmSdkInitialized) != 0;
}

FString FVrmSdkWrapper::GetVersion()
{
	if (!IsAvailable())
	{
		return FString();
	}

	// TODO: Get actual version from VRM SDK
	return TEXT("1.0.0-stub");
}

bool FVrmSdkWrapper::ValidateVrmFile(const FString& FilePath, TArray<FString>& OutErrors)
{
	if (!IsAvailable())
	{
		OutErrors.Add(TEXT("VRM SDK is not initialized"));
		return false;
	}

	// TODO: Implement actual validation using VRM SDK
	// For now, just check if the file exists and has .vrm extension
	if (!FPaths::FileExists(FilePath))
	{
		OutErrors.Add(FString::Printf(TEXT("File does not exist: %s"), *FilePath));
		return false;
	}

	if (!FilePath.EndsWith(TEXT(".vrm"), ESearchCase::IgnoreCase))
	{
		OutErrors.Add(TEXT("File does not have .vrm extension"));
		return false;
	}

	return true;
}
