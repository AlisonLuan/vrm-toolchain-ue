// Copyright Epic Games, Inc. All Rights Reserved.

#include "VrmSdkWrapper.h"

bool FVrmSdkWrapper::bIsInitialized = false;

bool FVrmSdkWrapper::Initialize()
{
	if (bIsInitialized)
	{
		return true;
	}

	// TODO: Initialize VRM SDK when available
	// For now, just mark as initialized
	bIsInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("VRM SDK Wrapper initialized"));
	return true;
}

void FVrmSdkWrapper::Shutdown()
{
	if (!bIsInitialized)
	{
		return;
	}

	// TODO: Cleanup VRM SDK resources when available
	bIsInitialized = false;

	UE_LOG(LogTemp, Log, TEXT("VRM SDK Wrapper shutdown"));
}

bool FVrmSdkWrapper::IsAvailable()
{
	return bIsInitialized;
}

FString FVrmSdkWrapper::GetVersion()
{
	if (!bIsInitialized)
	{
		return FString();
	}

	// TODO: Get actual version from VRM SDK
	return TEXT("1.0.0-stub");
}

bool FVrmSdkWrapper::ValidateVrmFile(const FString& FilePath, TArray<FString>& OutErrors)
{
	if (!bIsInitialized)
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

	if (!FilePath.EndsWith(TEXT(".vrm")))
	{
		OutErrors.Add(TEXT("File does not have .vrm extension"));
		return false;
	}

	return true;
}
