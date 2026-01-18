#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoreMinimal.h"
#include "VrmToolchainWrapper.h"
#include "VrmToolchainBPLibrary.generated.h"

UCLASS()
class UVrmToolchainBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Validates a vrm/glb file using the staged VRM SDK (developer tool). Returns true if valid.
    UFUNCTION(BlueprintCallable, Category = "VRM Toolchain")
    static bool ValidateVrmFile(const FString& FilePath, FString& OutStatus);
};