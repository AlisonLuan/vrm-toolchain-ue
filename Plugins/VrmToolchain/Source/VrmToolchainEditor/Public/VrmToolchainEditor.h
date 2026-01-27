#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FVrmToolchainEditorModule : public IModuleInterface
{
public:
    static inline FVrmToolchainEditorModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FVrmToolchainEditorModule>("VrmToolchainEditor");
    }

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    // Returns true when the optional developer validation tool is available on disk.
    bool IsValidationToolAvailable() const { return bValidationCliAvailable; }

    // Returns the path to vrm_validate.exe if available, empty string otherwise.
    FString GetValidationToolPath() const { return ValidationCliExePath; }

private:
    // Track whether the optional CLI validation tool is present and its path.
    bool bValidationCliAvailable = false;
    FString ValidationCliExePath;
};

// Dedicated log category for the editor module
DECLARE_LOG_CATEGORY_EXTERN(LogVrmToolchainEditor, Log, All);
