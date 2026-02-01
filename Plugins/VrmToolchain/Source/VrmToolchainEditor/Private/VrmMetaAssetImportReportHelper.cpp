#include "VrmMetaAssetImportReportHelper.h"

FString FVrmMetaAssetImportReportHelper::SanitizeForClipboardSingleLine(const FString& In)
{
	FString Out = In;
	Out.ReplaceInline(TEXT("\r"), TEXT(" "));
	Out.ReplaceInline(TEXT("\n"), TEXT(" "));
	Out.TrimStartAndEndInline();
	return Out;
}

FString FVrmMetaAssetImportReportHelper::BuildCopyText(const FString& Summary, const TArray<FString>& Warnings)
{
	FString Out = Summary;

	if (Warnings.Num() > 0)
	{
		Out += TEXT("\n\nWarnings:");
		for (const FString& W : Warnings)
		{
			// Single-line guarantee, but keep it defensive
			FString Clean = W;
			Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
			Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
			Out += TEXT("\n- ") + Clean;
		}
	}
	return Out;
}
