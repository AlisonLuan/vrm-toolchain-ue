#include "VrmMetadata.h"
#include "VrmToolchain.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

// GLB file format constants
static const uint32 GLB_MAGIC = 0x46546C67; // "glTF" in little-endian
static const uint32 GLB_VERSION_2 = 2;
static const uint32 GLB_CHUNK_TYPE_JSON = 0x4E4F534A; // "JSON" in little-endian
static const uint32 GLB_CHUNK_TYPE_BIN = 0x004E4942;  // "BIN\0" in little-endian

struct FGlbHeader
{
	uint32 Magic;
	uint32 Version;
	uint32 Length;
};

struct FGlbChunkHeader
{
	uint32 Length;
	uint32 Type;
};

bool FVrmParser::ReadGlbJsonChunkFromMemory(const uint8* Data, int64 DataSize, FString& OutJsonString)
{
	if (!Data || DataSize < sizeof(FGlbHeader))
	{
		UE_LOG(LogVrmToolchain, Warning, TEXT("Invalid GLB data: insufficient size for header"));
		return false;
	}

	// Read and validate GLB header
	const FGlbHeader* Header = reinterpret_cast<const FGlbHeader*>(Data);
	if (Header->Magic != GLB_MAGIC)
	{
		UE_LOG(LogVrmToolchain, Warning, TEXT("Invalid GLB file: incorrect magic number"));
		return false;
	}

	if (Header->Version != GLB_VERSION_2)
	{
		UE_LOG(LogVrmToolchain, Warning, TEXT("Unsupported GLB version: %u (expected 2)"), Header->Version);
		return false;
	}

	if (Header->Length > DataSize)
	{
		UE_LOG(LogVrmToolchain, Warning, TEXT("Invalid GLB file: header length (%u) exceeds data size (%lld)"), Header->Length, DataSize);
		return false;
	}

	// Read chunks
	int64 Offset = sizeof(FGlbHeader);
	while (Offset + sizeof(FGlbChunkHeader) <= DataSize)
	{
		// Ensure proper alignment for chunk header (GLB chunks should be 4-byte aligned)
		if (Offset % 4 != 0)
		{
			UE_LOG(LogVrmToolchain, Warning, TEXT("Invalid GLB chunk alignment at offset %lld"), Offset);
			return false;
		}

		const FGlbChunkHeader* ChunkHeader = reinterpret_cast<const FGlbChunkHeader*>(Data + Offset);
		Offset += sizeof(FGlbChunkHeader);

		// Check for integer overflow and bounds
		uint32 ChunkLength = ChunkHeader->Length;
		if (ChunkLength > static_cast<uint64>(DataSize) || 
		    Offset > DataSize - ChunkLength)
		{
			UE_LOG(LogVrmToolchain, Warning, TEXT("Invalid GLB chunk: length (%u) exceeds file size"), ChunkLength);
			return false;
		}

		if (ChunkHeader->Type == GLB_CHUNK_TYPE_JSON)
		{
			// Found JSON chunk
			const uint8* JsonData = Data + Offset;
			OutJsonString = FString(ChunkLength, UTF8_TO_TCHAR(reinterpret_cast<const char*>(JsonData)));
			return true;
		}

		// Skip to next chunk - check for overflow before advancing
		if (Offset > DataSize - ChunkLength)
		{
			UE_LOG(LogVrmToolchain, Warning, TEXT("Invalid GLB chunk: would overflow when advancing offset"));
			return false;
		}
		Offset += ChunkLength;
	}

	UE_LOG(LogVrmToolchain, Warning, TEXT("GLB file does not contain a JSON chunk"));
	return false;
}

bool FVrmParser::ReadGlbJsonChunk(const FString& FilePath, FString& OutJsonString)
{
	// Read the entire file into memory
	TArray<uint8> FileData;
	if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
	{
		UE_LOG(LogVrmToolchain, Warning, TEXT("Failed to read file: %s"), *FilePath);
		return false;
	}

	return ReadGlbJsonChunkFromMemory(FileData.GetData(), FileData.Num(), OutJsonString);
}

EVrmVersion FVrmParser::DetectVrmVersion(const FString& FilePath)
{
	FString JsonString;
	if (!ReadGlbJsonChunk(FilePath, JsonString))
	{
		return EVrmVersion::Unknown;
	}

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogVrmToolchain, Warning, TEXT("Failed to parse JSON from GLB file"));
		return EVrmVersion::Unknown;
	}

	// Check for extensions object
	const TSharedPtr<FJsonObject>* ExtensionsObject;
	if (!JsonObject->TryGetObjectField(TEXT("extensions"), ExtensionsObject))
	{
		UE_LOG(LogVrmToolchain, Verbose, TEXT("No extensions field found in JSON"));
		return EVrmVersion::Unknown;
	}

	// Check for VRM1 (VRMC_vrm extension)
	if ((*ExtensionsObject)->HasField(TEXT("VRMC_vrm")))
	{
		return EVrmVersion::VRM1;
	}

	// Check for VRM0 (VRM extension)
	if ((*ExtensionsObject)->HasField(TEXT("VRM")))
	{
		return EVrmVersion::VRM0;
	}

	return EVrmVersion::Unknown;
}

FVrmMetadata FVrmParser::ExtractVrmMetadata(const FString& FilePath)
{
	FVrmMetadata Metadata;

	FString JsonString;
	if (!ReadGlbJsonChunk(FilePath, JsonString))
	{
		return Metadata;
	}

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogVrmToolchain, Warning, TEXT("Failed to parse JSON from GLB file"));
		return Metadata;
	}

	// Check for extensions object
	const TSharedPtr<FJsonObject>* ExtensionsObject;
	if (!JsonObject->TryGetObjectField(TEXT("extensions"), ExtensionsObject))
	{
		return Metadata;
	}

	// Try VRM1 first
	const TSharedPtr<FJsonObject>* VrmcVrmObject;
	if ((*ExtensionsObject)->TryGetObjectField(TEXT("VRMC_vrm"), VrmcVrmObject))
	{
		Metadata.Version = EVrmVersion::VRM1;

		// Extract VRM1 metadata
		const TSharedPtr<FJsonObject>* MetaObject;
		if ((*VrmcVrmObject)->TryGetObjectField(TEXT("meta"), MetaObject))
		{
			// Name
			FString Name;
			if ((*MetaObject)->TryGetStringField(TEXT("name"), Name))
			{
				Metadata.Name = Name;
			}

			// Version
			FString Version;
			if ((*MetaObject)->TryGetStringField(TEXT("version"), Version))
			{
				Metadata.ModelVersion = Version;
			}

			// Authors
			const TArray<TSharedPtr<FJsonValue>>* AuthorsArray;
			if ((*MetaObject)->TryGetArrayField(TEXT("authors"), AuthorsArray))
			{
				for (const TSharedPtr<FJsonValue>& AuthorValue : *AuthorsArray)
				{
					FString Author;
					if (AuthorValue->TryGetString(Author))
					{
						Metadata.Authors.Add(Author);
					}
				}
			}

			// Copyright
			FString CopyrightInfo;
			if ((*MetaObject)->TryGetStringField(TEXT("copyrightInformation"), CopyrightInfo))
			{
				Metadata.Copyright = CopyrightInfo;
			}

			// License - in VRM1 this is under licenseUrl
			FString LicenseUrl;
			if ((*MetaObject)->TryGetStringField(TEXT("licenseUrl"), LicenseUrl))
			{
				Metadata.License = LicenseUrl;
			}
		}
	}
	// Try VRM0
	else if ((*ExtensionsObject)->TryGetObjectField(TEXT("VRM"), VrmcVrmObject))
	{
		Metadata.Version = EVrmVersion::VRM0;

		// Extract VRM0 metadata
		const TSharedPtr<FJsonObject>* MetaObject;
		if ((*VrmcVrmObject)->TryGetObjectField(TEXT("meta"), MetaObject))
		{
			// Title (VRM0 uses "title" instead of "name")
			FString Title;
			if ((*MetaObject)->TryGetStringField(TEXT("title"), Title))
			{
				Metadata.Name = Title;
			}

			// Version
			FString Version;
			if ((*MetaObject)->TryGetStringField(TEXT("version"), Version))
			{
				Metadata.ModelVersion = Version;
			}

			// Author (VRM0 uses "author" string instead of "authors" array)
			FString Author;
			if ((*MetaObject)->TryGetStringField(TEXT("author"), Author))
			{
				Metadata.Authors.Add(Author);
			}

			// Contact Information (as additional author info)
			FString ContactInfo;
			if ((*MetaObject)->TryGetStringField(TEXT("contactInformation"), ContactInfo))
			{
				if (!ContactInfo.IsEmpty())
				{
					Metadata.Copyright = ContactInfo;
				}
			}

			// License Name
			FString OtherLicenseUrl;
			if ((*MetaObject)->TryGetStringField(TEXT("otherLicenseUrl"), OtherLicenseUrl))
			{
				Metadata.License = OtherLicenseUrl;
			}
			else
			{
				// Try to get license name as fallback
				FString LicenseName;
				if ((*MetaObject)->TryGetStringField(TEXT("licenseName"), LicenseName))
				{
					Metadata.License = LicenseName;
				}
			}
		}
	}

	return Metadata;
}
