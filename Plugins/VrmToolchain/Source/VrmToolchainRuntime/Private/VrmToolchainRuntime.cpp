// Copyright Epic Games, Inc. All Rights Reserved.

#include "VrmToolchainRuntime.h"
#include "VrmSdkWrapper.h"

#define LOCTEXT_NAMESPACE "FVrmToolchainRuntimeModule"

DEFINE_LOG_CATEGORY(LogVrmToolchain);

void FVrmToolchainRuntimeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// Initialize VRM SDK wrapper
	FVrmSdkWrapper::Initialize();
	
	UE_LOG(LogVrmToolchain, Log, TEXT("VrmToolchainRuntime module started"));
}

void FVrmToolchainRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
	// Shutdown VRM SDK wrapper
	FVrmSdkWrapper::Shutdown();
	
	UE_LOG(LogVrmToolchain, Log, TEXT("VrmToolchainRuntime module shutdown"));
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVrmToolchainRuntimeModule, VrmToolchainRuntime)
