#pragma once

#include "CoreMinimal.h"

/**
 * Centralized helper for formatting meta asset import reports for clipboard/display.
 * Single source of truth to prevent drift across Details panel, toast, and context menu.
 */
class FVrmMetaAssetImportReportHelper
{
public:
	/**
	 * Sanitize a string for single-line clipboard output.
	 * Replaces newlines and carriage returns with spaces, trims whitespace.
	 * 
	 * @param In The string to sanitize
	 * @return Sanitized single-line string
	 */
	static FString SanitizeForClipboardSingleLine(const FString& In);

	/**
	 * Build a formatted copy text from summary + warnings.
	 * Produces deterministic, multi-line-safe format:
	 *   Summary
	 *   
	 *   Warnings:
	 *   - Warning 1
	 *   - Warning 2
	 * 
	 * @param Summary The import summary string
	 * @param Warnings Array of warning strings
	 * @return Formatted text suitable for clipboard
	 */
	static FString BuildCopyText(const FString& Summary, const TArray<FString>& Warnings);
};
