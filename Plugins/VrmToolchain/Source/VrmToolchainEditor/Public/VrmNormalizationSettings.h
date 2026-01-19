#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "VrmNormalizationSettings.generated.h"

/**
 * Output location strategy for normalized VRM files
 */
UENUM(BlueprintType)
enum class EVrmNormalizationOutputLocation : uint8
{
	/** Write normalized file next to the source file (e.g., <SourceDir>/<Name>.normalized.vrm) */
	NextToSource UMETA(DisplayName = "Next to Source"),
	
	/** Write normalized file to project Saved directory (e.g., Saved/VrmToolchain/Normalized/) */
	SavedDirectory UMETA(DisplayName = "Saved Directory")
};

/**
 * Editor settings for VRM normalization feature
 */
UCLASS(Config=EditorPerProjectUserSettings, meta=(DisplayName="VRM Normalization"))
class VRMTOOLCHAINEDITOR_API UVrmNormalizationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UVrmNormalizationSettings();

	/** Where to write normalized output files */
	UPROPERTY(Config, EditAnywhere, Category = "Output")
	EVrmNormalizationOutputLocation OutputLocation;

	/** Whether to overwrite existing normalized files without prompting */
	UPROPERTY(Config, EditAnywhere, Category = "Output")
	bool bOverwriteWithoutPrompt;

	/** Suffix to append to normalized filenames (before extension) */
	UPROPERTY(Config, EditAnywhere, Category = "Output")
	FString NormalizedSuffix;

	//~ Begin UDeveloperSettings Interface
	virtual FName GetCategoryName() const override;
	virtual FText GetSectionText() const override;
	//~ End UDeveloperSettings Interface
};
