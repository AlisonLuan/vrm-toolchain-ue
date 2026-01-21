// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class USkeletalMesh;

/**
 * UI action handler for retarget scaffolding workflow
 */
class FVrmRetargetActions
{
public:
	/** Register content browser context menu extensions */
	static void RegisterMenuExtensions();

	/** Unregister content browser context menu extensions */
	static void UnregisterMenuExtensions();

private:
	/** Execute retarget scaffold generation on selected assets */
	static void ExecuteRetargetScaffold(TArray<USkeletalMesh*> SelectedMeshes);

	/** Can we execute retarget scaffold generation? */
	static bool CanExecuteRetargetScaffold();
};
