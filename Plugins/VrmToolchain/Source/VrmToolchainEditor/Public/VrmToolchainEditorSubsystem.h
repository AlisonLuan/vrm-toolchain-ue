// Copyright (c) 2024-2026 VRM Consortium; Copyright (c) 2025 Aliso Softworks LLC. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "VrmToolchainEditorSubsystem.generated.h"

class FVrmSourceAssetEditorToolkit;
class IToolkitHost;

/**
 * Editor subsystem that manages VRM Source Asset editor toolkits.
 * Maintains strong references to keep editors alive while windows are open.
 */
UCLASS()
class VRMTOOLCHAINEDITOR_API UVrmToolchainEditorSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * Opens a VRM Source Asset editor and maintains a strong reference to keep it alive.
	 * @param InObjects - Assets to edit (should be UVrmSourceAsset instances)
	 * @param EditWithinLevelEditor - Optional level editor host
	 */
	void OpenVrmSourceAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor);

	/**
	 * Unregisters a toolkit when it closes, removing the strong reference.
	 * Called by the toolkit itself when closing.
	 * @param Toolkit - The toolkit to unregister
	 */
	void UnregisterToolkit(TSharedRef<FVrmSourceAssetEditorToolkit> Toolkit);

private:
	/** Active VRM Source Asset editor toolkits (strong references keep them alive) */
	TArray<TSharedRef<FVrmSourceAssetEditorToolkit>> OpenVrmSourceToolkits;
};
