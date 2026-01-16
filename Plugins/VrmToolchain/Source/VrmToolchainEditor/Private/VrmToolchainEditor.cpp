// Copyright Epic Games, Inc. All Rights Reserved.

#include "VrmToolchainEditor.h"
#include "VrmSdkWrapper.h"

#define LOCTEXT_NAMESPACE "FVrmToolchainEditorModule"

void FVrmToolchainEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	// Initialize VRM SDK wrapper
	FVrmSdkWrapper::Initialize();

	// Register asset tools
	RegisterAssetTools();

	UE_LOG(LogTemp, Log, TEXT("VrmToolchainEditor module started"));
}

void FVrmToolchainEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	// Unregister asset tools
	UnregisterAssetTools();

	// Shutdown VRM SDK wrapper
	FVrmSdkWrapper::Shutdown();

	UE_LOG(LogTemp, Log, TEXT("VrmToolchainEditor module shutdown"));
}

void FVrmToolchainEditorModule::RegisterAssetTools()
{
	// TODO: Register custom asset actions for VRM files
	// This is where we would register importers, validators, and other asset tools
}

void FVrmToolchainEditorModule::UnregisterAssetTools()
{
	// TODO: Unregister custom asset actions
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVrmToolchainEditorModule, VrmToolchainEditor)
