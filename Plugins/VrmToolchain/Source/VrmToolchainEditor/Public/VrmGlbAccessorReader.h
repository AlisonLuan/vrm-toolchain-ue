#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Math/Vector.h"
#include "Math/IntVector.h"

/**
 * Reads and decodes GLB accessor data from binary chunks.
 * Handles buffer views, accessors, and typed array decoding.
 */
class FVrmGlbAccessorReader
{
public:
    /** Result of accessor decoding operations */
    struct FDecodeResult
    {
        bool bSuccess = false;
        FString ErrorMessage;
    };

    /** Decoded vertex positions */
    TArray<FVector3f> Positions;
    
    /** Decoded vertex normals (optional) */
    TArray<FVector3f> Normals;
    
    /** Decoded texture coordinates (optional) */
    TArray<FVector2f> TexCoords;
    
    /** Decoded skin weights */
    TArray<FVector4f> Weights;
    
    /** Decoded joint indices */
    TArray<FIntVector4> Joints;
    
    /** Decoded triangle indices */
    TArray<uint32> Indices;

    /**
     * Load and parse GLB file, extracting JSON and BIN chunks
     * @param FilePath Path to the GLB file
     * @param OutJsonString Parsed JSON content
     * @return Success/failure result
     */
    FDecodeResult LoadGlbFile(const FString& FilePath, FString& OutJsonString);

    /**
     * Decode accessors from parsed JSON and BIN data
     * @param JsonString The GLB JSON content
     * @return Success/failure result
     */
    FDecodeResult DecodeAccessors(const FString& JsonString);

private:
    /** Raw BIN chunk data */
    TArray<uint8> BinData;

    /** GLB chunk header structure */
    struct FGlbChunk
    {
        uint32 Length;
        uint32 Type;
        TArrayView<const uint8> Data;
    };

    /**
     * Read a chunk from GLB data
     * @param Data Raw GLB data
     * @param Offset Current read offset
     * @param OutChunk Parsed chunk data
     * @return True if chunk was read successfully
     */
    bool ReadChunk(const TArray<uint8>& Data, int64& Offset, FGlbChunk& OutChunk);

    /**
     * Decode a typed accessor into an array
     * @param AccessorJson Accessor JSON object
     * @param BufferViewsJson Buffer views array
     * @param OutArray Output array to populate
     * @return Success/failure result
     */
    template<typename T>
    FDecodeResult DecodeAccessor(const TSharedPtr<FJsonObject>& AccessorJson, 
                                const TArray<TSharedPtr<FJsonValue>>& BufferViewsJson,
                                TArray<T>& OutArray);

    /**
     * Decode a single element from binary data
     * Template specializations handle different GLTF types
     */
    template<typename T>
    static bool DecodeElement(int32 ComponentType, int32 ComponentCount, const uint8* Data, T& OutElement);

    /**
     * Get component type size in bytes
     * @param ComponentType GLTF component type enum
     * @return Size in bytes, or 0 if invalid
     */
    static int32 GetComponentTypeSize(int32 ComponentType);

    /**
     * Get number of components per element
     * @param TypeString GLTF type string ("SCALAR", "VEC2", "VEC3", "VEC4")
     * @return Number of components, or 0 if invalid
     */
    static int32 GetTypeComponentCount(const FString& TypeString);
};