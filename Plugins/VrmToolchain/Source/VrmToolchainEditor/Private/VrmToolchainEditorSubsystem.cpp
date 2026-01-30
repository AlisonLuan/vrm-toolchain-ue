// Copyright (c) 2024-2026 VRM Consortium; Copyright (c) 2025 Aliso Softworks LLC. All Rights Reserved.
#include "VrmToolchainEditorSubsystem.h"
#include "VrmSourceAssetEditorToolkit.h"
#include "VrmToolchain/VrmSourceAsset.h"
#include "VrmToolchainEditor.h"

void UVrmToolchainEditorSubsystem::OpenVrmSourceAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	if (InObjects.Num() == 0)
	{
		UE_LOG(LogVrmToolchainEditor, Warning, TEXT("[VrmToolchainEditorSubsystem] OpenVrmSourceAssetEditor called with no objects"));
		return;
	}

	// Extract the first UVrmSourceAsset to support deduplication (reuse existing editor if present)
	UVrmSourceAsset* FirstAsset = nullptr;
	for (UObject* Obj : InObjects)
	{
		if (UVrmSourceAsset* A = Cast<UVrmSourceAsset>(Obj))
		{
			FirstAsset = A;
			break;
		}
	}

	if (FirstAsset)
	{
		// Search for an existing toolkit editing the same asset
		for (const TSharedRef<FVrmSourceAssetEditorToolkit>& Toolkit : OpenVrmSourceToolkits)
		{
			if (Toolkit->IsEditingObject(FirstAsset))
			{
				// Reuse: bring to front and return without creating another toolkit
				Toolkit->BringToolkitToFront();
				UE_LOG(LogVrmToolchainEditor, Display, TEXT("[VrmToolchainEditorSubsystem] Reusing existing editor for %s"), *FirstAsset->GetPathName());
				return;
			}
		}
	}

	// Create the toolkit with a strong reference
	TSharedRef<FVrmSourceAssetEditorToolkit> EditorToolkit = MakeShared<FVrmSourceAssetEditorToolkit>();
	
	// Store strong reference to keep toolkit alive
	OpenVrmSourceToolkits.Add(EditorToolkit);
	
	UE_LOG(LogVrmToolchainEditor, Display, TEXT("[VrmToolchainEditorSubsystem] Opening VRM Source Asset editor (%d toolkits now open)"), 
		OpenVrmSourceToolkits.Num());

	// Initialize the editor (this will register tabs and open the window)
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	EditorToolkit->InitEditor(Mode, EditWithinLevelEditor, InObjects, this);
}

void UVrmToolchainEditorSubsystem::UnregisterToolkit(TSharedRef<FVrmSourceAssetEditorToolkit> Toolkit)
{
	// Remove by pointer comparison (find the exact same shared reference)
	const int32 RemovedCount = OpenVrmSourceToolkits.RemoveAll([&Toolkit](const TSharedRef<FVrmSourceAssetEditorToolkit>& Item)
	{
		return &Item.Get() == &Toolkit.Get();
	});

	if (RemovedCount > 0)
	{
		UE_LOG(LogVrmToolchainEditor, Display, TEXT("[VrmToolchainEditorSubsystem] Closed VRM Source Asset editor (%d toolkits remaining)"), 
			OpenVrmSourceToolkits.Num());
	}
	else
	{
		UE_LOG(LogVrmToolchainEditor, Warning, TEXT("[VrmToolchainEditorSubsystem] Attempted to unregister toolkit that wasn't tracked"));
	}
}
