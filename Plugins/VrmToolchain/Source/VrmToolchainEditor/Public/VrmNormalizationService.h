#pragma once

#include "CoreMinimal.h"

/**
 * Result structure for VRM normalization operations
 */
struct FVrmNormalizationResult
{
	/** Whether the operation succeeded */
	bool bSuccess = false;

	/** Path to the normalized output file (if successful) */
	FString OutputFilePath;

	/** Path to the report file (if successful) */
	FString ReportFilePath;

	/** Error message (if failed) */
	FString ErrorMessage;

	/** Human-readable summary of what was done */
	FString Summary;
};

/**
 * Options for VRM normalization operations
 */
struct FVrmNormalizationOptions
{
	/** Where to write the output file (if empty, determined by settings) */
	FString OutputPath;

	/** Where to write the report file (if empty, auto-determined) */
	FString ReportPath;

	/** Whether to overwrite existing files */
	bool bOverwrite = false;
};

/**
 * Service class for normalizing VRM files
 */
class VRMTOOLCHAINEDITOR_API FVrmNormalizationService
{
public:
	/**
	 * Normalize a VRM file
	 * @param InPath Source VRM/GLB file path
	 * @param Options Normalization options (output paths, overwrite policy, etc.)
	 * @return Result with output paths or error information
	 */
	static FVrmNormalizationResult NormalizeVrmFile(const FString& InPath, const FVrmNormalizationOptions& Options);

	/**
	 * Build output file path based on settings and input path
	 * @param InPath Source file path
	 * @param OutPath Output file path
	 * @param OutReportPath Output report path
	 * @return true if paths were successfully determined
	 */
	static bool BuildOutputPaths(const FString& InPath, FString& OutPath, FString& OutReportPath);

	/**
	 * Validate that a file path is a supported VRM/GLB file
	 * @param FilePath File path to validate
	 * @param OutErrorMessage Error message if validation fails
	 * @return true if file is valid
	 */
	static bool ValidateSourceFile(const FString& FilePath, FString& OutErrorMessage);

private:
	/**
	 * Perform the actual normalization using SDK library
	 * @param InPath Source file path
	 * @param OutPath Destination file path
	 * @param OutReport Report data structure
	 * @param OutErrorMessage Error message if operation fails
	 * @return true if normalization succeeded
	 */
	static bool PerformNormalization(const FString& InPath, const FString& OutPath, TSharedPtr<FJsonObject>& OutReport, FString& OutErrorMessage);

	/**
	 * Write a JSON report file
	 * @param ReportPath Path to write report
	 * @param Report JSON report data
	 * @return true if report was written successfully
	 */
	static bool WriteReportFile(const FString& ReportPath, const TSharedPtr<FJsonObject>& Report);
};
