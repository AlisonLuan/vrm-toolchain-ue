#include "VrmMetadata.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

#if WITH_DEV_AUTOMATION_TESTS

// Helper function to get temp file path for tests
static FString GetTestTempFilePath(const FString& Filename)
{
	return FPaths::ProjectSavedDir() / TEXT("Temp") / Filename;
}

// Helper function to create a minimal GLB file in memory
static TArray<uint8> CreateSyntheticGlb(const FString& JsonContent)
{
	TArray<uint8> GlbData;

	// Convert JSON string to UTF8 bytes
	FTCHARToUTF8 JsonUtf8(*JsonContent);
	TArray<uint8> JsonBytes;
	JsonBytes.Append(reinterpret_cast<const uint8*>(JsonUtf8.Get()), JsonUtf8.Length());

	// Pad JSON chunk to 4-byte alignment
	while (JsonBytes.Num() % 4 != 0)
	{
		JsonBytes.Add(' ');
	}

	// GLB constants
	const uint32 GLB_MAGIC = 0x46546C67; // "glTF"
	const uint32 GLB_VERSION = 2;
	const uint32 GLB_CHUNK_TYPE_JSON = 0x4E4F534A; // "JSON"

	// Calculate total file length
	const uint32 HeaderSize = 12; // magic + version + length
	const uint32 ChunkHeaderSize = 8; // length + type
	const uint32 TotalLength = HeaderSize + ChunkHeaderSize + JsonBytes.Num();

	// Write GLB header
	GlbData.Reserve(TotalLength);
	GlbData.Append(reinterpret_cast<const uint8*>(&GLB_MAGIC), sizeof(uint32));
	GlbData.Append(reinterpret_cast<const uint8*>(&GLB_VERSION), sizeof(uint32));
	GlbData.Append(reinterpret_cast<const uint8*>(&TotalLength), sizeof(uint32));

	// Write JSON chunk header
	uint32 JsonChunkLength = JsonBytes.Num();
	GlbData.Append(reinterpret_cast<const uint8*>(&JsonChunkLength), sizeof(uint32));
	GlbData.Append(reinterpret_cast<const uint8*>(&GLB_CHUNK_TYPE_JSON), sizeof(uint32));

	// Write JSON chunk data
	GlbData.Append(JsonBytes);

	return GlbData;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmParserJsonChunkTest, "VrmToolchain.VrmParser.ReadJsonChunk", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmParserJsonChunkTest::RunTest(const FString& Parameters)
{
	// Create a minimal GLB with simple JSON
	FString TestJson = TEXT("{\"asset\":{\"version\":\"2.0\"}}");
	TArray<uint8> GlbData = CreateSyntheticGlb(TestJson);

	// Test reading JSON chunk from memory
	FString ExtractedJson;
	bool bSuccess = FVrmParser::ReadGlbJsonChunkFromMemory(GlbData.GetData(), GlbData.Num(), ExtractedJson);

	TestTrue(TEXT("JSON chunk extraction should succeed"), bSuccess);
	TestTrue(TEXT("Extracted JSON should contain asset version"), ExtractedJson.Contains(TEXT("asset")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmParserVrm0DetectionTest, "VrmToolchain.VrmParser.DetectVRM0", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmParserVrm0DetectionTest::RunTest(const FString& Parameters)
{
	// Create a minimal VRM0 GLB
	FString Vrm0Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRM": {
				"meta": {
					"title": "Test Model",
					"version": "1.0",
					"author": "Test Author"
				}
			}
		}
	})");

	TArray<uint8> GlbData = CreateSyntheticGlb(Vrm0Json);

	// Test VRM version detection
	FString ExtractedJson;
	FVrmParser::ReadGlbJsonChunkFromMemory(GlbData.GetData(), GlbData.Num(), ExtractedJson);

	// Parse JSON to manually verify
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ExtractedJson);
	FJsonSerializer::Deserialize(Reader, JsonObject);

	// Create a temp file to test file-based API
	FString TempFilePath = GetTestTempFilePath(TEXT("test_vrm0.glb"));
	FFileHelper::SaveArrayToFile(GlbData, *TempFilePath);

	EVrmVersion DetectedVersion = FVrmParser::DetectVrmVersion(TempFilePath);
	TestEqual(TEXT("VRM0 should be detected"), DetectedVersion, EVrmVersion::VRM0);

	// Clean up temp file
	IFileManager::Get().Delete(*TempFilePath);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmParserVrm1DetectionTest, "VrmToolchain.VrmParser.DetectVRM1", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmParserVrm1DetectionTest::RunTest(const FString& Parameters)
{
	// Create a minimal VRM1 GLB
	FString Vrm1Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRMC_vrm": {
				"specVersion": "1.0",
				"meta": {
					"name": "Test Model VRM1",
					"version": "2.0",
					"authors": ["Author1", "Author2"]
				}
			}
		}
	})");

	TArray<uint8> GlbData = CreateSyntheticGlb(Vrm1Json);

	// Create a temp file to test file-based API
	FString TempFilePath = GetTestTempFilePath(TEXT("test_vrm1.glb"));
	FFileHelper::SaveArrayToFile(GlbData, *TempFilePath);

	EVrmVersion DetectedVersion = FVrmParser::DetectVrmVersion(TempFilePath);
	TestEqual(TEXT("VRM1 should be detected"), DetectedVersion, EVrmVersion::VRM1);

	// Clean up temp file
	IFileManager::Get().Delete(*TempFilePath);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmParserMetadataVrm0Test, "VrmToolchain.VrmParser.ExtractMetadata.VRM0", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmParserMetadataVrm0Test::RunTest(const FString& Parameters)
{
	// Create a VRM0 GLB with metadata
	FString Vrm0Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRM": {
				"meta": {
					"title": "My VRM0 Model",
					"version": "1.5",
					"author": "John Doe",
					"contactInformation": "john@example.com",
					"licenseName": "CC0"
				}
			}
		}
	})");

	TArray<uint8> GlbData = CreateSyntheticGlb(Vrm0Json);

	// Create a temp file
	FString TempFilePath = GetTestTempFilePath(TEXT("test_vrm0_meta.glb"));
	FFileHelper::SaveArrayToFile(GlbData, *TempFilePath);

	// Extract metadata
	FVrmMetadata Metadata = FVrmParser::ExtractVrmMetadata(TempFilePath);

	TestEqual(TEXT("Version should be VRM0"), Metadata.Version, EVrmVersion::VRM0);
	TestEqual(TEXT("Name should match"), Metadata.Name, FString(TEXT("My VRM0 Model")));
	TestEqual(TEXT("Model version should match"), Metadata.ModelVersion, FString(TEXT("1.5")));
	TestEqual(TEXT("Should have one author"), Metadata.Authors.Num(), 1);
	if (Metadata.Authors.Num() > 0)
	{
		TestEqual(TEXT("Author should match"), Metadata.Authors[0], FString(TEXT("John Doe")));
	}
	TestEqual(TEXT("Copyright should contain contact info"), Metadata.Copyright, FString(TEXT("john@example.com")));
	TestEqual(TEXT("License should match"), Metadata.License, FString(TEXT("CC0")));

	// Clean up
	IFileManager::Get().Delete(*TempFilePath);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmParserMetadataVrm1Test, "VrmToolchain.VrmParser.ExtractMetadata.VRM1", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmParserMetadataVrm1Test::RunTest(const FString& Parameters)
{
	// Create a VRM1 GLB with metadata
	FString Vrm1Json = TEXT(R"({
		"asset": {"version": "2.0"},
		"extensions": {
			"VRMC_vrm": {
				"specVersion": "1.0",
				"meta": {
					"name": "My VRM1 Model",
					"version": "2.0",
					"authors": ["Jane Smith", "Bob Johnson"],
					"copyrightInformation": "Copyright 2024",
					"licenseUrl": "https://example.com/license"
				}
			}
		}
	})");

	TArray<uint8> GlbData = CreateSyntheticGlb(Vrm1Json);

	// Create a temp file
	FString TempFilePath = GetTestTempFilePath(TEXT("test_vrm1_meta.glb"));
	FFileHelper::SaveArrayToFile(GlbData, *TempFilePath);

	// Extract metadata
	FVrmMetadata Metadata = FVrmParser::ExtractVrmMetadata(TempFilePath);

	TestEqual(TEXT("Version should be VRM1"), Metadata.Version, EVrmVersion::VRM1);
	TestEqual(TEXT("Name should match"), Metadata.Name, FString(TEXT("My VRM1 Model")));
	TestEqual(TEXT("Model version should match"), Metadata.ModelVersion, FString(TEXT("2.0")));
	TestEqual(TEXT("Should have two authors"), Metadata.Authors.Num(), 2);
	if (Metadata.Authors.Num() >= 2)
	{
		TestEqual(TEXT("First author should match"), Metadata.Authors[0], FString(TEXT("Jane Smith")));
		TestEqual(TEXT("Second author should match"), Metadata.Authors[1], FString(TEXT("Bob Johnson")));
	}
	TestEqual(TEXT("Copyright should match"), Metadata.Copyright, FString(TEXT("Copyright 2024")));
	TestEqual(TEXT("License URL should match"), Metadata.License, FString(TEXT("https://example.com/license")));

	// Clean up
	IFileManager::Get().Delete(*TempFilePath);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmParserInvalidGlbTest, "VrmToolchain.VrmParser.InvalidGLB", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmParserInvalidGlbTest::RunTest(const FString& Parameters)
{
	// Test with invalid data
	TArray<uint8> InvalidData;
	InvalidData.Add(0x00);
	InvalidData.Add(0x01);
	InvalidData.Add(0x02);

	FString ExtractedJson;
	bool bSuccess = FVrmParser::ReadGlbJsonChunkFromMemory(InvalidData.GetData(), InvalidData.Num(), ExtractedJson);

	TestFalse(TEXT("Invalid GLB should fail to parse"), bSuccess);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmParserNoExtensionsTest, "VrmToolchain.VrmParser.NoExtensions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmParserNoExtensionsTest::RunTest(const FString& Parameters)
{
	// Create a GLB without VRM extensions
	FString PlainGltfJson = TEXT(R"({
		"asset": {"version": "2.0"},
		"scenes": [{"nodes": [0]}]
	})");

	TArray<uint8> GlbData = CreateSyntheticGlb(PlainGltfJson);

	// Create a temp file
	FString TempFilePath = GetTestTempFilePath(TEXT("test_plain.glb"));
	FFileHelper::SaveArrayToFile(GlbData, *TempFilePath);

	EVrmVersion DetectedVersion = FVrmParser::DetectVrmVersion(TempFilePath);
	TestEqual(TEXT("Unknown version should be returned for non-VRM GLB"), DetectedVersion, EVrmVersion::Unknown);

	// Clean up
	IFileManager::Get().Delete(*TempFilePath);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmParserNonNullTerminatedJsonTest, "VrmToolchain.VrmParser.NonNullTerminatedJson", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmParserNonNullTerminatedJsonTest::RunTest(const FString& Parameters)
{
	// Create a GLB with JSON chunk followed immediately by a BIN chunk (no null terminator)
	TArray<uint8> GlbData;

	// JSON content without null terminator
	FString JsonContent = TEXT(R"({"asset":{"version":"2.0"},"extensions":{"VRM":{"meta":{"title":"Test"}}}})");
	FTCHARToUTF8 JsonUtf8(*JsonContent);
	TArray<uint8> JsonBytes;
	JsonBytes.Append(reinterpret_cast<const uint8*>(JsonUtf8.Get()), JsonUtf8.Length());

	// Pad JSON chunk to 4-byte alignment
	while (JsonBytes.Num() % 4 != 0)
	{
		JsonBytes.Add(' ');
	}

	// Create BIN chunk data (dummy binary data)
	TArray<uint8> BinBytes;
	for (int32 i = 0; i < 16; ++i)
	{
		BinBytes.Add(static_cast<uint8>(i));
	}

	// GLB constants
	const uint32 GLB_MAGIC = 0x46546C67; // "glTF"
	const uint32 GLB_VERSION = 2;
	const uint32 GLB_CHUNK_TYPE_JSON = 0x4E4F534A; // "JSON"
	const uint32 GLB_CHUNK_TYPE_BIN = 0x004E4942;  // "BIN\0"

	// Calculate total file length
	const uint32 HeaderSize = 12;
	const uint32 ChunkHeaderSize = 8;
	const uint32 TotalLength = HeaderSize + ChunkHeaderSize + JsonBytes.Num() + ChunkHeaderSize + BinBytes.Num();

	// Write GLB header
	GlbData.Reserve(TotalLength);
	GlbData.Append(reinterpret_cast<const uint8*>(&GLB_MAGIC), sizeof(uint32));
	GlbData.Append(reinterpret_cast<const uint8*>(&GLB_VERSION), sizeof(uint32));
	GlbData.Append(reinterpret_cast<const uint8*>(&TotalLength), sizeof(uint32));

	// Write JSON chunk header and data
	uint32 JsonChunkLength = JsonBytes.Num();
	GlbData.Append(reinterpret_cast<const uint8*>(&JsonChunkLength), sizeof(uint32));
	GlbData.Append(reinterpret_cast<const uint8*>(&GLB_CHUNK_TYPE_JSON), sizeof(uint32));
	GlbData.Append(JsonBytes);

	// Write BIN chunk header and data (immediately following JSON)
	uint32 BinChunkLength = BinBytes.Num();
	GlbData.Append(reinterpret_cast<const uint8*>(&BinChunkLength), sizeof(uint32));
	GlbData.Append(reinterpret_cast<const uint8*>(&GLB_CHUNK_TYPE_BIN), sizeof(uint32));
	GlbData.Append(BinBytes);

	// Test reading JSON chunk from memory
	FString ExtractedJson;
	bool bSuccess = FVrmParser::ReadGlbJsonChunkFromMemory(GlbData.GetData(), GlbData.Num(), ExtractedJson);

	TestTrue(TEXT("JSON chunk extraction should succeed with BIN chunk following"), bSuccess);
	TestTrue(TEXT("Extracted JSON should contain asset version"), ExtractedJson.Contains(TEXT("asset")));
	TestTrue(TEXT("Extracted JSON should contain VRM extension"), ExtractedJson.Contains(TEXT("VRM")));
	TestFalse(TEXT("Extracted JSON should not contain binary data from BIN chunk"), ExtractedJson.Contains(TEXT("BIN")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
