// Copyright (c) 2024-2026 VRM Consortium; Copyright (c) 2025 Aliso Softworks LLC. All Rights Reserved.
#pragma once

// Naming contract: see Plugins/VrmToolchain/Docs/Naming.md

#include "CoreMinimal.h"

/**
 * Naming utilities for VRM source assets.
 * Enforces Unreal's Content Browser visibility requirement: package name must match asset name.
 * 
 * Convention:
 *   Package path:  /Game/VRM_/teucu_VrmSource
 *   Asset name:    teucu_VrmSource
 *   Object path:   /Game/VRM_/teucu_VrmSource.teucu_VrmSource
 * 
 * This ensures stable Content Browser visibility by maintaining name parity.
 */
struct VRMTOOLCHAINEDITOR_API FVrmAssetNaming
{
	/** Standard suffix for VRM source assets (applied to both package and asset names) */
	static constexpr const TCHAR* VrmSourceSuffix = TEXT("_VrmSource");
	/** Suffix for VRM meta assets */
	static constexpr const TCHAR* VrmMetaSuffix = TEXT("_VrmMeta");

	/**
	 * Generates the standard VRM meta asset/package name by appending the suffix.
	 * @param BaseName - The base name (e.g., "teucu")
	 * @return The name with "_VrmMeta" suffix (e.g., "teucu_VrmMeta")
	 */
	static FString MakeVrmMetaAssetName(const FString& BaseName);

	/**
	 * Generates the full package path for a VRM meta asset.
	 * @param FolderPath - The folder path (e.g., "/Game/VRM_")
	 * @param BaseName - The base name (e.g., "teucu")
	 * @return Full package path (e.g., "/Game/VRM_/teucu_VrmMeta")
	 */
	static FString MakeVrmMetaPackagePath(const FString& FolderPath, const FString& BaseName);

	/**
	 * Strip known suffixes (_VrmSource, _VrmMeta) returning base name
	 */
	static FString StripKnownSuffixes(const FString& Name);

	/**
	 * Generates the standard VRM source asset/package name by appending the suffix.
	 * @param BaseName - The base name (e.g., "teucu")
	 * @return The name with "_VrmSource" suffix (e.g., "teucu_VrmSource")
	 */
	static FString MakeVrmSourceAssetName(const FString& BaseName);

	/**
	 * Generates the full package path for a VRM source asset.
	 * @param FolderPath - The folder path (e.g., "/Game/VRM_")
	 * @param BaseName - The base name (e.g., "teucu")
	 * @return Full package path (e.g., "/Game/VRM_/teucu_VrmSource")
	 */
	static FString MakeVrmSourcePackagePath(const FString& FolderPath, const FString& BaseName);

	/**
	 * Strips the "_VrmSource" suffix if present, otherwise returns the input unchanged.
	 * @param Name - The name to strip
	 * @return The base name without suffix (e.g., "teucu_VrmSource" -> "teucu")
	 */
	static FString StripVrmSourceSuffix(const FString& Name);

	/**
	 * Checks if a name follows the VRM source asset naming convention.
	 * @param Name - The name to check
	 * @return true if the name ends with "_VrmSource" suffix
	 */
	static bool IsVrmSourceAssetName(const FString& Name);

	/**
	 * Sanitizes a base name for use in package/asset names.
	 * Replaces invalid characters (dots, spaces, etc.) with underscores.
	 * @param BaseName - The name to sanitize
	 * @return Sanitized name safe for use in Unreal asset paths
	 */
	static FString SanitizeBaseName(const FString& BaseName);
};
