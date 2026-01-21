#include "VrmNormalizationService.h"
#include "VrmNormalizationSettings.h"
#include "VrmToolchainEditor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// Validate that a file path is a supported VRM/GLB file
bool FVrmNormalizationService::ValidateSourceFile(const FString& FilePath, FString& OutErrorMessage)
{
	OutErrorMessage.Reset();

	// Check extension first (before file existence check)
	const FString Extension = FPaths::GetExtension(FilePath, false).ToLower();
	if (Extension != TEXT("vrm") && Extension != TEXT("glb"))
	{
		OutErrorMessage = FString::Printf(TEXT("Unsupported file extension: .%s (expected .vrm or .glb)"), *Extension);
		return false;
	}

	// Check file exists
	if (!FPaths::FileExists(FilePath))
	{
		OutErrorMessage = FString::Printf(TEXT("Source file does not exist: %s"), *FilePath);
		return false;
	}

	return true;
}

// Build output file paths based on settings
bool FVrmNormalizationService::BuildOutputPaths(const FString& InPath, FString& OutPath, FString& OutReportPath)
{
	const UVrmNormalizationSettings* Settings = GetDefault<UVrmNormalizationSettings>();
	if (!Settings)
	{
		return false;
	}

	const FString BaseFileName = FPaths::GetBaseFilename(InPath);
	const FString Extension = FPaths::GetExtension(InPath, true); // Include dot
	const FString NormalizedName = BaseFileName + Settings->NormalizedSuffix + Extension;

	if (Settings->OutputLocation == EVrmNormalizationOutputLocation::NextToSource)
	{
		// Place next to source file
		const FString SourceDir = FPaths::GetPath(InPath);
		OutPath = FPaths::Combine(SourceDir, NormalizedName);
		OutReportPath = FPaths::Combine(SourceDir, BaseFileName + Settings->NormalizedSuffix + TEXT(".report.json"));
	}
	else // SavedDirectory
	{
		// Place in Saved/VrmToolchain/Normalized/
		const FString SavedDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("VrmToolchain"), TEXT("Normalized"));
		OutPath = FPaths::Combine(SavedDir, NormalizedName);
		OutReportPath = FPaths::Combine(SavedDir, BaseFileName + Settings->NormalizedSuffix + TEXT(".report.json"));
	}

	return true;
}

// Write a JSON report file
bool FVrmNormalizationService::WriteReportFile(const FString& ReportPath, const TSharedPtr<FJsonObject>& Report)
{
	if (!Report.IsValid())
	{
		return false;
	}

	// Ensure directory exists
	const FString ReportDir = FPaths::GetPath(ReportPath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*ReportDir))
	{
		if (!PlatformFile.CreateDirectoryTree(*ReportDir))
		{
			UE_LOG(LogVrmToolchainEditor, Error, TEXT("Failed to create report directory: %s"), *ReportDir);
			return false;
		}
	}

	// Serialize to JSON string
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(Report.ToSharedRef(), Writer))
	{
		UE_LOG(LogVrmToolchainEditor, Error, TEXT("Failed to serialize report to JSON"));
		return false;
	}

	// Write to file
	if (!FFileHelper::SaveStringToFile(JsonString, *ReportPath))
	{
		UE_LOG(LogVrmToolchainEditor, Error, TEXT("Failed to write report file: %s"), *ReportPath);
		return false;
	}

	return true;
}

// Perform the actual normalization using SDK library
bool FVrmNormalizationService::PerformNormalization(const FString& InPath, const FString& OutPath, TSharedPtr<FJsonObject>& OutReport, FString& OutErrorMessage)
{
	OutReport = MakeShared<FJsonObject>();
	OutErrorMessage.Reset();

	// Read source file
	TArray<uint8> SourceData;
	if (!FFileHelper::LoadFileToArray(SourceData, *InPath))
	{
		OutErrorMessage = FString::Printf(TEXT("Failed to read source file: %s"), *InPath);
		return false;
	}

	// TODO: Integrate vrm_normalizers.lib to perform actual normalization.
	// This is a placeholder stub demonstrating the end-to-end flow.
	// See Plugins/VrmToolchain/README.md "VRM Normalization Feature" for details.
	// Expected integration points:
	// 1. Parse VRM/GLB using vrm_glb_parser API
	// 2. Apply normalization rules via vrm_normalizers API
	// 3. Serialize normalized output back to VRM/GLB format
	// 4. Populate OutReport with detailed change descriptions
	// For now, implement a basic stub that copies the file and generates a report.
	// The stub serves as a working placeholder demonstrating the end-to-end flow.
	// NOTE: This stub loads the entire file into memory, which is acceptable for the placeholder.
	// The actual SDK integration will handle large files more efficiently.
	
	// Ensure output directory exists
	const FString OutDir = FPaths::GetPath(OutPath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*OutDir))
	{
		if (!PlatformFile.CreateDirectoryTree(*OutDir))
		{
			OutErrorMessage = FString::Printf(TEXT("Failed to create output directory: %s"), *OutDir);
			return false;
		}
	}

	// Write normalized output (currently just a copy)
	if (!FFileHelper::SaveArrayToFile(SourceData, *OutPath))
	{
		OutErrorMessage = FString::Printf(TEXT("Failed to write output file: %s"), *OutPath);
		return false;
	}

	// Build report
	OutReport->SetStringField(TEXT("source_file"), InPath);
	OutReport->SetStringField(TEXT("output_file"), OutPath);
	OutReport->SetStringField(TEXT("timestamp"), FDateTime::UtcNow().ToIso8601());
	OutReport->SetNumberField(TEXT("source_size_bytes"), SourceData.Num());
	OutReport->SetBoolField(TEXT("success"), true);

	// Record changes (currently none, as we're just copying)
	TArray<TSharedPtr<FJsonValue>> ChangesArray;
	OutReport->SetArrayField(TEXT("changes"), ChangesArray);
	OutReport->SetStringField(TEXT("summary"), TEXT("File validated and copied (normalization stub)"));

	return true;
}

// Main normalization entry point
FVrmNormalizationResult FVrmNormalizationService::NormalizeVrmFile(const FString& InPath, const FVrmNormalizationOptions& Options)
{
	FVrmNormalizationResult Result;

	// Validate source file
	FString ValidationError;
	if (!ValidateSourceFile(InPath, ValidationError))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = ValidationError;
		UE_LOG(LogVrmToolchainEditor, Error, TEXT("VRM normalization failed: %s"), *ValidationError);
		return Result;
	}

	// Determine output paths
	FString OutputPath = Options.OutputPath;
	FString ReportPath = Options.ReportPath;

	if (OutputPath.IsEmpty() || ReportPath.IsEmpty())
	{
		if (!BuildOutputPaths(InPath, OutputPath, ReportPath))
		{
			Result.bSuccess = false;
			Result.ErrorMessage = TEXT("Failed to determine output paths");
			UE_LOG(LogVrmToolchainEditor, Error, TEXT("VRM normalization failed: Failed to determine output paths"));
			return Result;
		}
	}

	// Check if output already exists and handle overwrite policy
	if (FPaths::FileExists(OutputPath))
	{
		// Only allow overwrite if explicitly enabled via options AND settings permit it
		if (!Options.bAllowOverwrite)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Output file already exists: %s (overwrite not allowed by options)"), *OutputPath);
			UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VRM normalization skipped: %s"), *Result.ErrorMessage);
			return Result;
		}

		// Check settings to see if user has enabled overwriting
		const UVrmNormalizationSettings* Settings = GetDefault<UVrmNormalizationSettings>();
		if (!Settings || !Settings->bOverwriteWithoutPrompt)
		{
			Result.bSuccess = false;
			Result.ErrorMessage = FString::Printf(TEXT("Output file already exists: %s (enable 'Overwrite Without Prompt' in settings to allow)"), *OutputPath);
			UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VRM normalization skipped: %s"), *Result.ErrorMessage);
			return Result;
		}
	}

	// Perform normalization
	TSharedPtr<FJsonObject> Report;
	FString NormalizationError;
	if (!PerformNormalization(InPath, OutputPath, Report, NormalizationError))
	{
		Result.bSuccess = false;
		Result.ErrorMessage = NormalizationError;
		UE_LOG(LogVrmToolchainEditor, Error, TEXT("VRM normalization failed: %s"), *NormalizationError);
		return Result;
	}

	// Write report file
	if (!WriteReportFile(ReportPath, Report))
	{
		// Non-fatal: normalization succeeded but report writing failed
		UE_LOG(LogVrmToolchainEditor, Warning, TEXT("VRM normalization succeeded but report writing failed: %s"), *ReportPath);
	}

	// Success
	Result.bSuccess = true;
	Result.OutputFilePath = OutputPath;
	Result.ReportFilePath = ReportPath;
	Result.Summary = Report.IsValid() ? Report->GetStringField(TEXT("summary")) : TEXT("Normalization completed");

	UE_LOG(LogVrmToolchainEditor, Display, TEXT("VRM normalization succeeded: %s -> %s"), *InPath, *OutputPath);

	return Result;
}
