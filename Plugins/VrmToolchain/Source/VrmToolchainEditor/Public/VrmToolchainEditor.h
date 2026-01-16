// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

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
