// Copyright (c) 2024-2026 VRM Consortium; Copyright (c) 2025 Aliso Softworks LLC. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/**
 * Naming utilities for VRM source assets.
 * Enforces the standard naming convention where VRM source assets use a "_VrmSource" suffix
 * to distinguish the asset object name from its package name.
 *
 * Example:
 *   Package path:  /Game/VRM_/teucu
 *   Asset name:    teucu_VrmSource
 *   Object path:   /Game/VRM_/teucu.teucu_VrmSource
 */
struct VRMTOOLCHAINEDITOR_API FVrmNaming
{
	/** Standard suffix for VRM source asset object names */
	static constexpr const TCHAR* VrmSourceSuffix = TEXT("_VrmSource");

	/**
	 * Generates the standard VRM source asset name by appending the suffix.
	 * @param BaseName - The base name (typically the package name without path)
	 * @return The asset name with "_VrmSource" suffix appended
	 */
	static FString MakeVrmSourceAssetName(const FString& BaseName)
	{
		return FString::Printf(TEXT("%s%s"), *BaseName, VrmSourceSuffix);
	}

	/**
	 * Generates the standard VRM source asset name by appending the suffix.
	 * @param BaseName - The base name as FName
	 * @return The asset name with "_VrmSource" suffix appended as FName
	 */
	static FName MakeVrmSourceAssetFName(const FName& BaseName)
	{
		return FName(*MakeVrmSourceAssetName(BaseName.ToString()));
	}

	/**
	 * Checks if a name follows the VRM source asset naming convention.
	 * @param Name - The name to check
	 * @return true if the name ends with "_VrmSource" suffix
	 */
	static bool IsVrmSourceAssetName(const FString& Name)
	{
		return Name.EndsWith(VrmSourceSuffix);
	}

	/**
	 * Strips the "_VrmSource" suffix if present, otherwise returns the input unchanged.
	 * @param Name - The name to strip
	 * @return The name without the suffix, or the original name if suffix wasn't present
	 */
	static FString StripVrmSourceSuffix(const FString& Name)
	{
		if (Name.EndsWith(VrmSourceSuffix))
		{
			return Name.LeftChop(FCString::Strlen(VrmSourceSuffix));
		}
		return Name;
	}

	/**
	 * Builds a full object path from package path and base name.
	 * @param PackagePath - The package path (e.g., "/Game/VRM_/teucu")
	 * @param BaseName - The base name (e.g., "teucu")
	 * @return Full object path (e.g., "/Game/VRM_/teucu.teucu_VrmSource")
	 */
	static FString MakeVrmSourceObjectPath(const FString& PackagePath, const FString& BaseName)
	{
		return FString::Printf(TEXT("%s.%s"), *PackagePath, *MakeVrmSourceAssetName(BaseName));
	}
};
