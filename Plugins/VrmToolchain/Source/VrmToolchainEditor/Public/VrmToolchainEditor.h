// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVrmToolchainEditor, Log, All);

/**
 * VRM Toolchain Editor Module
 * Provides editor-time functionality for VRM import/export
 */
class FVrmToolchainEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Register asset actions */
	void RegisterAssetTools();

	/** Unregister asset actions */
	void UnregisterAssetTools();
};
