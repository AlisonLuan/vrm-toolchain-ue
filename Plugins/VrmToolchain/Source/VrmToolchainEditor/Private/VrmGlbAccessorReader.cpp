#include "VrmGlbAccessorReader.h"
#include "VrmToolchain/VrmMetadata.h" // for FVrmParser::ReadGlbJsonChunk
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Math/UnrealMathUtility.h"

FVrmGlbAccessorReader::FDecodeResult FVrmGlbAccessorReader::LoadGlbFile(const FString& FilePath, FString& OutJsonString)
{
    FDecodeResult Result;
    
    // First try to read JSON chunk using existing utility
    if (!FVrmParser::ReadGlbJsonChunk(FilePath, OutJsonString))
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("Failed to read GLB JSON chunk from: %s"), *FilePath);
        return Result;
    }

    // Read the entire file for BIN chunk extraction
    TArray<uint8> FileData;
    if (!FFileHelper::LoadFileToArray(FileData, *FilePath))
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("Failed to read GLB file: %s"), *FilePath);
        return Result;
    }

    // Parse GLB header
    if (FileData.Num() < 12)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("GLB file too small for header");
        return Result;
    }

    // Check magic number (should be "glTF")
    uint32 Magic = *reinterpret_cast<const uint32*>(&FileData[0]);
    if (Magic != 0x46546C67) // "glTF" in little endian
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Invalid GLB magic number");
        return Result;
    }

    // Skip version (4 bytes) and total length (4 bytes), start reading chunks
    int64 Offset = 12;

    while (Offset < FileData.Num())
    {
        FGlbChunk Chunk;
        if (!ReadChunk(FileData, Offset, Chunk))
        {
            Result.bSuccess = false;
            Result.ErrorMessage = TEXT("Failed to read GLB chunk");
            return Result;
        }

        // Check chunk type
        if (Chunk.Type == 0x4E4942) // "BIN" in little endian
        {
            BinData = TArray<uint8>(Chunk.Data.GetData(), Chunk.Data.Num());
            break; // We only need the BIN chunk for now
        }
    }

    if (BinData.Num() == 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("No BIN chunk found in GLB file");
        return Result;
    }

    Result.bSuccess = true;
    return Result;
}

bool FVrmGlbAccessorReader::ReadChunk(const TArray<uint8>& Data, int64& Offset, FGlbChunk& OutChunk)
{
    if (Offset + 8 > Data.Num())
    {
        return false;
    }

    OutChunk.Length = *reinterpret_cast<const uint32*>(&Data[Offset]);
    Offset += 4;
    
    OutChunk.Type = *reinterpret_cast<const uint32*>(&Data[Offset]);
    Offset += 4;

    if (Offset + OutChunk.Length > Data.Num())
    {
        return false;
    }

    OutChunk.Data = TArrayView<const uint8>(&Data[Offset], OutChunk.Length);
    Offset += OutChunk.Length;

    return true;
}

FVrmGlbAccessorReader::FDecodeResult FVrmGlbAccessorReader::DecodeAccessors(const FString& JsonString)
{
    FDecodeResult Result;

    // Parse JSON
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    
    if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Failed to parse GLB JSON");
        return Result;
    }

    // Get required arrays
    const TArray<TSharedPtr<FJsonValue>>* AccessorsArray = nullptr;
    if (!RootObject->TryGetArrayField(TEXT("accessors"), AccessorsArray) || !AccessorsArray)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("No accessors array in GLB JSON");
        return Result;
    }

    const TArray<TSharedPtr<FJsonValue>>* BufferViewsArray = nullptr;
    if (!RootObject->TryGetArrayField(TEXT("bufferViews"), BufferViewsArray) || !BufferViewsArray)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("No bufferViews array in GLB JSON");
        return Result;
    }

    // Get meshes to find the primitive we want
    const TArray<TSharedPtr<FJsonValue>>* MeshesArray = nullptr;
    if (!RootObject->TryGetArrayField(TEXT("meshes"), MeshesArray) || !MeshesArray || MeshesArray->Num() == 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("No meshes in GLB JSON");
        return Result;
    }

    // For now, take the first mesh and first primitive
    const TSharedPtr<FJsonObject> FirstMesh = (*MeshesArray)[0]->AsObject();
    if (!FirstMesh.IsValid())
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Invalid mesh object");
        return Result;
    }

    const TArray<TSharedPtr<FJsonValue>>* PrimitivesArray = nullptr;
    if (!FirstMesh->TryGetArrayField(TEXT("primitives"), PrimitivesArray) || !PrimitivesArray || PrimitivesArray->Num() == 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("No primitives in first mesh");
        return Result;
    }

    const TSharedPtr<FJsonObject> FirstPrimitive = (*PrimitivesArray)[0]->AsObject();
    if (!FirstPrimitive.IsValid())
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Invalid primitive object");
        return Result;
    }

    // Get attributes
    const TSharedPtr<FJsonObject>* AttributesObject = nullptr;
    if (!FirstPrimitive->TryGetObjectField(TEXT("attributes"), AttributesObject) || !AttributesObject)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("No attributes in primitive");
        return Result;
    }

    // Decode POSITION (required)
    int32 PositionAccessorIndex = INDEX_NONE;
    if ((*AttributesObject)->TryGetNumberField(TEXT("POSITION"), PositionAccessorIndex))
    {
        if (PositionAccessorIndex >= 0 && PositionAccessorIndex < AccessorsArray->Num())
        {
            const TSharedPtr<FJsonObject> AccessorObj = (*AccessorsArray)[PositionAccessorIndex]->AsObject();
            if (AccessorObj.IsValid())
            {
                FDecodeResult PosResult = DecodeAccessor(AccessorObj, *BufferViewsArray, Positions);
                if (!PosResult.bSuccess)
                {
                    Result.bSuccess = false;
                    Result.ErrorMessage = FString::Printf(TEXT("Failed to decode POSITION: %s"), *PosResult.ErrorMessage);
                    return Result;
                }
            }
        }
    }

    // Decode NORMAL (optional)
    int32 NormalAccessorIndex = INDEX_NONE;
    if ((*AttributesObject)->TryGetNumberField(TEXT("NORMAL"), NormalAccessorIndex))
    {
        if (NormalAccessorIndex >= 0 && NormalAccessorIndex < AccessorsArray->Num())
        {
            const TSharedPtr<FJsonObject> AccessorObj = (*AccessorsArray)[NormalAccessorIndex]->AsObject();
            if (AccessorObj.IsValid())
            {
                FDecodeResult NormalResult = DecodeAccessor(AccessorObj, *BufferViewsArray, Normals);
                if (!NormalResult.bSuccess)
                {
                    // Normals are optional, so we don't fail here
                    UE_LOG(LogTemp, Warning, TEXT("Failed to decode NORMAL: %s"), *NormalResult.ErrorMessage);
                }
            }
        }
    }

    // Decode TEXCOORD_0 (optional)
    int32 TexCoordAccessorIndex = INDEX_NONE;
    if ((*AttributesObject)->TryGetNumberField(TEXT("TEXCOORD_0"), TexCoordAccessorIndex))
    {
        if (TexCoordAccessorIndex >= 0 && TexCoordAccessorIndex < AccessorsArray->Num())
        {
            const TSharedPtr<FJsonObject> AccessorObj = (*AccessorsArray)[TexCoordAccessorIndex]->AsObject();
            if (AccessorObj.IsValid())
            {
                FDecodeResult TexCoordResult = DecodeAccessor(AccessorObj, *BufferViewsArray, TexCoords);
                if (!TexCoordResult.bSuccess)
                {
                    // TexCoords are optional, so we don't fail here
                    UE_LOG(LogTemp, Warning, TEXT("Failed to decode TEXCOORD_0: %s"), *TexCoordResult.ErrorMessage);
                }
            }
        }
    }

    // Decode WEIGHTS_0 (required for skinned mesh)
    int32 WeightsAccessorIndex = INDEX_NONE;
    if ((*AttributesObject)->TryGetNumberField(TEXT("WEIGHTS_0"), WeightsAccessorIndex))
    {
        if (WeightsAccessorIndex >= 0 && WeightsAccessorIndex < AccessorsArray->Num())
        {
            const TSharedPtr<FJsonObject> AccessorObj = (*AccessorsArray)[WeightsAccessorIndex]->AsObject();
            if (AccessorObj.IsValid())
            {
                FDecodeResult WeightsResult = DecodeAccessor(AccessorObj, *BufferViewsArray, Weights);
                if (!WeightsResult.bSuccess)
                {
                    Result.bSuccess = false;
                    Result.ErrorMessage = FString::Printf(TEXT("Failed to decode WEIGHTS_0: %s"), *WeightsResult.ErrorMessage);
                    return Result;
                }
            }
        }
    }

    // Decode JOINTS_0 (required for skinned mesh)
    int32 JointsAccessorIndex = INDEX_NONE;
    if ((*AttributesObject)->TryGetNumberField(TEXT("JOINTS_0"), JointsAccessorIndex))
    {
        if (JointsAccessorIndex >= 0 && JointsAccessorIndex < AccessorsArray->Num())
        {
            const TSharedPtr<FJsonObject> AccessorObj = (*AccessorsArray)[JointsAccessorIndex]->AsObject();
            if (AccessorObj.IsValid())
            {
                FDecodeResult JointsResult = DecodeAccessor(AccessorObj, *BufferViewsArray, Joints);
                if (!JointsResult.bSuccess)
                {
                    Result.bSuccess = false;
                    Result.ErrorMessage = FString::Printf(TEXT("Failed to decode JOINTS_0: %s"), *JointsResult.ErrorMessage);
                    return Result;
                }
            }
        }
    }

    // Decode indices (required)
    int32 IndicesAccessorIndex = INDEX_NONE;
    if (FirstPrimitive->TryGetNumberField(TEXT("indices"), IndicesAccessorIndex))
    {
        if (IndicesAccessorIndex >= 0 && IndicesAccessorIndex < AccessorsArray->Num())
        {
            const TSharedPtr<FJsonObject> AccessorObj = (*AccessorsArray)[IndicesAccessorIndex]->AsObject();
            if (AccessorObj.IsValid())
            {
                FDecodeResult IndicesResult = DecodeAccessor(AccessorObj, *BufferViewsArray, Indices);
                if (!IndicesResult.bSuccess)
                {
                    Result.bSuccess = false;
                    Result.ErrorMessage = FString::Printf(TEXT("Failed to decode indices: %s"), *IndicesResult.ErrorMessage);
                    return Result;
                }
            }
        }
    }

    // Validate that we have the minimum required data
    if (Positions.Num() == 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("No POSITION data found");
        return Result;
    }

    if (Weights.Num() == 0 || Joints.Num() == 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Skinned mesh requires WEIGHTS_0 and JOINTS_0 data");
        return Result;
    }

    if (Indices.Num() == 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("No indices data found");
        return Result;
    }

    // Validate array sizes match
    if (Weights.Num() != Positions.Num() || Joints.Num() != Positions.Num())
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("Array size mismatch: Positions=%d, Weights=%d, Joints=%d"), 
                                             Positions.Num(), Weights.Num(), Joints.Num());
        return Result;
    }

    Result.bSuccess = true;
    return Result;
}

template<typename T>
FVrmGlbAccessorReader::FDecodeResult FVrmGlbAccessorReader::DecodeAccessor(
    const TSharedPtr<FJsonObject>& AccessorJson, 
    const TArray<TSharedPtr<FJsonValue>>& BufferViewsJson,
    TArray<T>& OutArray)
{
    FDecodeResult Result;

    // Get accessor properties
    int32 BufferViewIndex = INDEX_NONE;
    if (!AccessorJson->TryGetNumberField(TEXT("bufferView"), BufferViewIndex) || BufferViewIndex < 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Accessor missing bufferView");
        return Result;
    }

    int32 Count = 0;
    if (!AccessorJson->TryGetNumberField(TEXT("count"), Count) || Count <= 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Accessor missing or invalid count");
        return Result;
    }

    int32 ComponentType = 0;
    if (!AccessorJson->TryGetNumberField(TEXT("componentType"), ComponentType))
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Accessor missing componentType");
        return Result;
    }

    FString TypeString;
    if (!AccessorJson->TryGetStringField(TEXT("type"), TypeString))
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Accessor missing type");
        return Result;
    }

    // Get buffer view
    if (BufferViewIndex >= BufferViewsJson.Num())
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("BufferView index %d out of range"), BufferViewIndex);
        return Result;
    }

    const TSharedPtr<FJsonObject> BufferViewObj = BufferViewsJson[BufferViewIndex]->AsObject();
    if (!BufferViewObj.IsValid())
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Invalid bufferView object");
        return Result;
    }

    int32 BufferIndex = 0;
    if (!BufferViewObj->TryGetNumberField(TEXT("buffer"), BufferIndex) || BufferIndex != 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = TEXT("Only buffer index 0 (BIN chunk) is supported");
        return Result;
    }

    int32 ByteOffset = 0;
    BufferViewObj->TryGetNumberField(TEXT("byteOffset"), ByteOffset);

    int32 ByteStride = 0;
    BufferViewObj->TryGetNumberField(TEXT("byteStride"), ByteStride);

    int32 AccessorByteOffset = 0;
    AccessorJson->TryGetNumberField(TEXT("byteOffset"), AccessorByteOffset);

    // Calculate sizes
    int32 ComponentSize = GetComponentTypeSize(ComponentType);
    if (ComponentSize == 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("Unsupported componentType: %d"), ComponentType);
        return Result;
    }

    int32 ComponentCount = GetTypeComponentCount(TypeString);
    if (ComponentCount == 0)
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("Unsupported type: %s"), *TypeString);
        return Result;
    }

    int32 ElementSize = ComponentSize * ComponentCount;
    int32 Stride = (ByteStride > 0) ? ByteStride : ElementSize;
    int32 TotalOffset = ByteOffset + AccessorByteOffset;

    // Validate bounds
    int64 RequiredSize = (int64)TotalOffset + (int64)Count * Stride;
    if (RequiredSize > BinData.Num())
    {
        Result.bSuccess = false;
        Result.ErrorMessage = FString::Printf(TEXT("Accessor data exceeds BIN chunk size: %lld > %d"), RequiredSize, BinData.Num());
        return Result;
    }

    // Decode elements
    OutArray.Reserve(Count);
    for (int32 i = 0; i < Count; ++i)
    {
        int64 ElementOffset = TotalOffset + i * Stride;
        const uint8* ElementData = &BinData[ElementOffset];

        T Element;
        if (!DecodeElement(ComponentType, ComponentCount, ElementData, Element))
        {
            Result.bSuccess = false;
            Result.ErrorMessage = FString::Printf(TEXT("Failed to decode element %d"), i);
            return Result;
        }

        OutArray.Add(Element);
    }

    Result.bSuccess = true;
    return Result;
}

// Template specializations for different accessor types
template<>
bool FVrmGlbAccessorReader::DecodeElement<FVector3f>(int32 ComponentType, int32 ComponentCount, const uint8* Data, FVector3f& OutElement)
{
    if (ComponentCount != 3 || ComponentType != 5126) // FLOAT
    {
        return false;
    }

    // GLTF uses right-handed coordinates, UE uses left-handed
    // For positions, we need to convert Y/Z axes
    const float* FloatData = reinterpret_cast<const float*>(Data);
    OutElement.X = FloatData[0];
    OutElement.Y = FloatData[2]; // Swap Y and Z
    OutElement.Z = -FloatData[1]; // Negate Y (original Z)

    return true;
}

template<>
bool FVrmGlbAccessorReader::DecodeElement<FVector2f>(int32 ComponentType, int32 ComponentCount, const uint8* Data, FVector2f& OutElement)
{
    if (ComponentCount != 2 || ComponentType != 5126) // FLOAT
    {
        return false;
    }

    const float* FloatData = reinterpret_cast<const float*>(Data);
    OutElement.X = FloatData[0];
    OutElement.Y = FloatData[1];

    return true;
}

template<>
bool FVrmGlbAccessorReader::DecodeElement<FVector4f>(int32 ComponentType, int32 ComponentCount, const uint8* Data, FVector4f& OutElement)
{
    if (ComponentCount != 4 || ComponentType != 5126) // FLOAT
    {
        return false;
    }

    const float* FloatData = reinterpret_cast<const float*>(Data);
    OutElement.X = FloatData[0];
    OutElement.Y = FloatData[1];
    OutElement.Z = FloatData[2];
    OutElement.W = FloatData[3];

    return true;
}

template<>
bool FVrmGlbAccessorReader::DecodeElement<FIntVector4>(int32 ComponentType, int32 ComponentCount, const uint8* Data, FIntVector4& OutElement)
{
    if (ComponentCount != 4)
    {
        return false;
    }

    if (ComponentType == 5121) // UNSIGNED_BYTE
    {
        const uint8* ByteData = Data;
        OutElement.X = ByteData[0];
        OutElement.Y = ByteData[1];
        OutElement.Z = ByteData[2];
        OutElement.W = ByteData[3];
    }
    else if (ComponentType == 5123) // UNSIGNED_SHORT
    {
        const uint16* ShortData = reinterpret_cast<const uint16*>(Data);
        OutElement.X = ShortData[0];
        OutElement.Y = ShortData[1];
        OutElement.Z = ShortData[2];
        OutElement.W = ShortData[3];
    }
    else
    {
        return false; // Unsupported component type for joints
    }

    return true;
}

template<>
bool FVrmGlbAccessorReader::DecodeElement<uint32>(int32 ComponentType, int32 ComponentCount, const uint8* Data, uint32& OutElement)
{
    if (ComponentCount != 1)
    {
        return false;
    }

    if (ComponentType == 5123) // UNSIGNED_SHORT
    {
        const uint16* ShortData = reinterpret_cast<const uint16*>(Data);
        OutElement = *ShortData;
    }
    else if (ComponentType == 5125) // UNSIGNED_INT
    {
        const uint32* IntData = reinterpret_cast<const uint32*>(Data);
        OutElement = *IntData;
    }
    else
    {
        return false; // Unsupported component type for indices
    }

    return true;
}

int32 FVrmGlbAccessorReader::GetComponentTypeSize(int32 ComponentType)
{
    switch (ComponentType)
    {
    case 5120: // BYTE
    case 5121: // UNSIGNED_BYTE
        return 1;
    case 5122: // SHORT
    case 5123: // UNSIGNED_SHORT
        return 2;
    case 5125: // UNSIGNED_INT
    case 5126: // FLOAT
        return 4;
    default:
        return 0;
    }
}

int32 FVrmGlbAccessorReader::GetTypeComponentCount(const FString& TypeString)
{
    if (TypeString == TEXT("SCALAR")) return 1;
    if (TypeString == TEXT("VEC2")) return 2;
    if (TypeString == TEXT("VEC3")) return 3;
    if (TypeString == TEXT("VEC4")) return 4;
    return 0;
}