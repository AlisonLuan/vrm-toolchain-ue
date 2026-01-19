#include "VrmNormalizationSettings.h"

UVrmNormalizationSettings::UVrmNormalizationSettings()
	: OutputLocation(EVrmNormalizationOutputLocation::NextToSource)
	, bOverwriteWithoutPrompt(true)
	, NormalizedSuffix(TEXT(".normalized"))
{
}

FName UVrmNormalizationSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FText UVrmNormalizationSettings::GetSectionText() const
{
	return NSLOCTEXT("VrmToolchain", "VrmNormalizationSettingsSection", "VRM Normalization");
}
