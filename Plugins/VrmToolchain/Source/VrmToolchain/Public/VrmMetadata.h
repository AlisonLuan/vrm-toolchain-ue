#pragma once

#include "CoreMinimal.h"
#include "VrmMetadata.generated.h"

/**
 * VRM version enumeration
 */
UENUM(BlueprintType)
enum class EVrmVersion : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	VRM0 UMETA(DisplayName = "VRM 0.x"),
	VRM1 UMETA(DisplayName = "VRM 1.0")
};

/**
 * VRM metadata structure containing core metadata fields
 */
USTRUCT(BlueprintType)
struct FVrmMetadata
{
	GENERATED_BODY()

	/** VRM version detected */
	UPROPERTY(BlueprintReadOnly, Category = "VRM Metadata")
	EVrmVersion Version = EVrmVersion::Unknown;

	/** Model name */
	UPROPERTY(BlueprintReadOnly, Category = "VRM Metadata")
	FString Name;

	/** Model version string */
	UPROPERTY(BlueprintReadOnly, Category = "VRM Metadata")
	FString ModelVersion;

	/** Authors list */
	UPROPERTY(BlueprintReadOnly, Category = "VRM Metadata")
	TArray<FString> Authors;

	/** Copyright information */
	UPROPERTY(BlueprintReadOnly, Category = "VRM Metadata")
	FString Copyright;

	/** License information */
	UPROPERTY(BlueprintReadOnly, Category = "VRM Metadata")
	FString License;

	FVrmMetadata()
		: Version(EVrmVersion::Unknown)
	{
	}
};

/**
 * VRM file parsing and metadata extraction utilities
 */
class VRMTOOLCHAIN_API FVrmParser
{
public:
	/**
	 * Detects the VRM version from a .vrm or .glb file
	 * @param FilePath Path to the VRM/GLB file
	 * @return The detected VRM version (Unknown, VRM0, or VRM1)
	 */
	static EVrmVersion DetectVrmVersion(const FString& FilePath);

	/**
	 * Extracts VRM metadata from a .vrm or .glb file
	 * @param FilePath Path to the VRM/GLB file
	 * @return A struct with populated metadata fields (empty if parsing fails)
	 */
	static FVrmMetadata ExtractVrmMetadata(const FString& FilePath);

	/**
	 * Reads the GLB file and extracts the JSON chunk
	 * @param FilePath Path to the GLB file
	 * @param OutJsonString The extracted JSON string
	 * @return True if the JSON chunk was successfully extracted
	 */
	static bool ReadGlbJsonChunk(const FString& FilePath, FString& OutJsonString);

	/**
	 * Reads the GLB file from memory and extracts the JSON chunk
	 * @param Data Pointer to GLB file data in memory
	 * @param DataSize Size of the GLB file data
	 * @param OutJsonString The extracted JSON string
	 * @return True if the JSON chunk was successfully extracted
	 */
	static bool ReadGlbJsonChunkFromMemory(const uint8* Data, int64 DataSize, FString& OutJsonString);
};
