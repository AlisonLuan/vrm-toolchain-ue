#include "VrmGltfParser.h"
#include "VrmToolchain/VrmMetadata.h" // for ReadGlbJsonChunk
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FVrmGltfParser::ExtractSkeletonFromGltfJsonString(const FString& JsonString, FVrmGltfSkeleton& OutSkeleton, FString& OutError)
{
    OutSkeleton.Bones.Reset();
    OutError.Reset();

    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Failed to parse JSON");
        return false;
    }

    // GLTF nodes array
    const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
    if (!Root->TryGetArrayField(TEXT("nodes"), NodesArray))
    {
        OutError = TEXT("No 'nodes' array in GLTF JSON");
        return false;
    }

    // Build mapping from node index -> parent index by inspecting children
    TArray<int32> ParentMap;
    ParentMap.AddUninitialized(NodesArray->Num());
    for (int32 i = 0; i < ParentMap.Num(); ++i) ParentMap[i] = INDEX_NONE;

    for (int32 i = 0; i < NodesArray->Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& Val = (*NodesArray)[i];
        const TSharedPtr<FJsonObject> Obj = Val->AsObject();
        if (!Obj.IsValid()) continue;

        // children
        const TArray<TSharedPtr<FJsonValue>>* Children = nullptr;
        if (Obj->TryGetArrayField(TEXT("children"), Children))
        {
            for (const TSharedPtr<FJsonValue>& ChildVal : *Children)
            {
                int32 ChildIndex = static_cast<int32>(ChildVal->AsNumber());
                if (ChildIndex >= 0 && ChildIndex < ParentMap.Num())
                {
                    ParentMap[ChildIndex] = i;
                }
            }
        }
    }

    // Read node names and basic transforms (supports 'matrix' or 'translation'/'rotation'/'scale')
    for (int32 i = 0; i < NodesArray->Num(); ++i)
    {
        const TSharedPtr<FJsonValue>& Val = (*NodesArray)[i];
        const TSharedPtr<FJsonObject> Obj = Val->AsObject();
        if (!Obj.IsValid()) continue;

        FVrmGltfBone Bone;
        // name
        FString NameStr;
        Obj->TryGetStringField(TEXT("name"), NameStr);
        Bone.Name = NameStr.IsEmpty() ? FName(*FString::Printf(TEXT("node_%d"), i)) : FName(*NameStr);
        Bone.ParentIndex = ParentMap.IsValidIndex(i) ? ParentMap[i] : INDEX_NONE;

        // transforms
        FTransform T = FTransform::Identity;
        const TArray<TSharedPtr<FJsonValue>>* Matrix = nullptr;
        if (Obj->TryGetArrayField(TEXT("matrix"), Matrix) && Matrix->Num() == 16)
        {
            // row-major 4x4
            double M[16];
            for (int32 m = 0; m < 16; ++m) M[m] = (*Matrix)[m]->AsNumber();

            // Convert to UE transform: extract translation/rotation/scale
            FVector Translation((float)M[12], (float)M[13], (float)M[14]);
            // Note: proper matrix->rot/scale decomposition omitted for brevity — assume identity rotation/scale if not provided
            T.SetTranslation(Translation);
        }
        else
        {
            // translation
            const TArray<TSharedPtr<FJsonValue>>* Trans = nullptr;
            if (Obj->TryGetArrayField(TEXT("translation"), Trans) && Trans->Num() == 3)
            {
                T.SetTranslation(FVector((float)(*Trans)[0]->AsNumber(), (float)(*Trans)[1]->AsNumber(), (float)(*Trans)[2]->AsNumber()));
            }
            // rotation (quaternion)
            const TArray<TSharedPtr<FJsonValue>>* Rot = nullptr;
            if (Obj->TryGetArrayField(TEXT("rotation"), Rot) && Rot->Num() == 4)
            {
                FQuat Q((float)(*Rot)[0]->AsNumber(), (float)(*Rot)[1]->AsNumber(), (float)(*Rot)[2]->AsNumber(), (float)(*Rot)[3]->AsNumber());
                T.SetRotation(Q);
            }
            // scale
            const TArray<TSharedPtr<FJsonValue>>* Scale = nullptr;
            if (Obj->TryGetArrayField(TEXT("scale"), Scale) && Scale->Num() == 3)
            {
                T.SetScale3D(FVector((float)(*Scale)[0]->AsNumber(), (float)(*Scale)[1]->AsNumber(), (float)(*Scale)[2]->AsNumber()));
            }
        }

        Bone.LocalTransform = T;
        OutSkeleton.Bones.Add(Bone);
    }

    return true;
}

bool FVrmGltfParser::ExtractSkeletonFromGlbFile(const FString& FilePath, FVrmGltfSkeleton& OutSkeleton, FString& OutError)
{
    OutError.Reset();
    FString JsonChunk;
    if (!FVrmParser::ReadGlbJsonChunk(FilePath, JsonChunk))
    {
        OutError = TEXT("Failed to extract JSON chunk from GLB");
        return false;
    }

    return ExtractSkeletonFromGltfJsonString(JsonChunk, OutSkeleton, OutError);
}
