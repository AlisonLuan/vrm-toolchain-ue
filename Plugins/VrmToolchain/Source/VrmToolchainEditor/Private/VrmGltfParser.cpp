#include "VrmGltfParser.h"
#include "VrmToolchain/VrmMetadata.h" // for ReadGlbJsonChunk
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Containers/Set.h"
#include "Containers/Map.h"

static bool TryExtractSkin0Joints(const TSharedPtr<FJsonObject>& Root, TArray<int32>& OutJoints)
{
	OutJoints.Reset();

	const TArray<TSharedPtr<FJsonValue>>* Skins = nullptr;
	if (!Root->TryGetArrayField(TEXT("skins"), Skins) || !Skins || Skins->Num() == 0)
	{
		return false;
	}

	const TSharedPtr<FJsonObject> Skin0 = (*Skins)[0]->AsObject();
	if (!Skin0.IsValid())
	{
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* Joints = nullptr;
	if (!Skin0->TryGetArrayField(TEXT("joints"), Joints) || !Joints || Joints->Num() == 0)
	{
		return false;
	}

	for (const TSharedPtr<FJsonValue>& V : *Joints)
	{
		// glTF joints are integer indices but appear as JSON numbers
		OutJoints.Add((int32)V->AsNumber());
	}

	return OutJoints.Num() > 0;
}

static void AddAncestorsClosure(const TArray<int32>& ParentMap, const TArray<int32>& Joints, TSet<int32>& InOutKeep)
{
	for (int32 Joint : Joints)
	{
		int32 Cur = Joint;
		while (Cur != INDEX_NONE && !InOutKeep.Contains(Cur))
		{
			InOutKeep.Add(Cur);
			if (ParentMap.IsValidIndex(Cur))
			{
				Cur = ParentMap[Cur];
			}
			else
			{
				break;
			}
		}
	}
}

static bool OrderKeptNodesParentFirst(const TSet<int32>& KeepNodes, const TArray<int32>& ParentMap, TArray<int32>& OutOrdered)
{
	OutOrdered.Reset();
	OutOrdered.Reserve(KeepNodes.Num());

	TSet<int32> Remaining = KeepNodes;
	bool bProgress = true;

	while (Remaining.Num() > 0 && bProgress)
	{
		bProgress = false;

		for (auto It = Remaining.CreateIterator(); It; ++It)
		{
			const int32 NodeIdx = *It;
			const int32 ParentNode = ParentMap.IsValidIndex(NodeIdx) ? ParentMap[NodeIdx] : INDEX_NONE;

			// Parent is either not kept, or already emitted
			const bool bParentOk =
				(ParentNode == INDEX_NONE) ||
				(!KeepNodes.Contains(ParentNode)) ||
				(OutOrdered.Contains(ParentNode)); // small N, acceptable; can optimize with a TSet if needed

			if (bParentOk)
			{
				OutOrdered.Add(NodeIdx);
				It.RemoveCurrent();
				bProgress = true;
			}
		}
	}

	return Remaining.Num() == 0;
}

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
	if (!Root->TryGetArrayField(TEXT("nodes"), NodesArray) || !NodesArray)
	{
		OutError = TEXT("No 'nodes' array in GLTF JSON");
		return false;
	}

	// Build mapping from node index -> parent index by inspecting children
	TArray<int32> ParentMap;
	ParentMap.Init(INDEX_NONE, NodesArray->Num());

	for (int32 i = 0; i < NodesArray->Num(); ++i)
	{
		const TSharedPtr<FJsonObject> Obj = (*NodesArray)[i]->AsObject();
		if (!Obj.IsValid())
		{
			continue;
		}

		const TArray<TSharedPtr<FJsonValue>>* Children = nullptr;
		if (Obj->TryGetArrayField(TEXT("children"), Children) && Children)
		{
			for (const TSharedPtr<FJsonValue>& ChildVal : *Children)
			{
				const int32 ChildIndex = (int32)ChildVal->AsNumber();
				if (ChildIndex >= 0 && ChildIndex < ParentMap.Num())
				{
					ParentMap[ChildIndex] = i;
				}
			}
		}
	}

	// B1.2: If skins[0].joints exists, filter to joints + ancestors; otherwise keep all nodes.
	TSet<int32> KeepNodes;
	{
		TArray<int32> Joints;
		if (TryExtractSkin0Joints(Root, Joints))
		{
			AddAncestorsClosure(ParentMap, Joints, KeepNodes);
		}
		else
		{
			for (int32 i = 0; i < NodesArray->Num(); ++i)
			{
				KeepNodes.Add(i);
			}
		}
	}

	// Order kept nodes so parents appear before children and build NodeIndex->BoneIndex mapping
	TArray<int32> OrderedNodes;
	if (!OrderKeptNodesParentFirst(KeepNodes, ParentMap, OrderedNodes))
	{
		OutError = TEXT("Failed to order filtered nodes parent-first (cycle or invalid parent map).");
		return false;
	}

	TMap<int32, int32> NodeToBone;
	NodeToBone.Reserve(OrderedNodes.Num());
	for (int32 i = 0; i < OrderedNodes.Num(); ++i)
	{
		NodeToBone.Add(OrderedNodes[i], i);
	}

	// Build bones
	OutSkeleton.Bones.Reserve(OrderedNodes.Num());

	for (int32 OrderedIdx = 0; OrderedIdx < OrderedNodes.Num(); ++OrderedIdx)
	{
		const int32 NodeIdx = OrderedNodes[OrderedIdx];
		const TSharedPtr<FJsonObject> Obj = (*NodesArray)[NodeIdx]->AsObject();
		if (!Obj.IsValid())
		{
			continue;
		}

		FVrmGltfBone Bone;
		Bone.GltfNodeIndex = NodeIdx;

		// name
		FString NameStr;
		Obj->TryGetStringField(TEXT("name"), NameStr);
		Bone.Name = NameStr.IsEmpty() ? FName(*FString::Printf(TEXT("node_%d"), NodeIdx)) : FName(*NameStr);

		// remapped parent index (bone index)
		const int32 ParentNode = ParentMap.IsValidIndex(NodeIdx) ? ParentMap[NodeIdx] : INDEX_NONE;
		Bone.ParentIndex = (ParentNode != INDEX_NONE && NodeToBone.Contains(ParentNode)) ? NodeToBone[ParentNode] : INDEX_NONE;

		// transforms (keep your existing behavior)
		FTransform T = FTransform::Identity;
		const TArray<TSharedPtr<FJsonValue>>* Matrix = nullptr;
		if (Obj->TryGetArrayField(TEXT("matrix"), Matrix) && Matrix && Matrix->Num() == 16)
		{
			double M[16];
			for (int32 m = 0; m < 16; ++m) M[m] = (*Matrix)[m]->AsNumber();

			FVector Translation((float)M[12], (float)M[13], (float)M[14]);
			T.SetTranslation(Translation);
		}
		else
		{
			const TArray<TSharedPtr<FJsonValue>>* Trans = nullptr;
			if (Obj->TryGetArrayField(TEXT("translation"), Trans) && Trans && Trans->Num() == 3)
			{
				T.SetTranslation(FVector((float)(*Trans)[0]->AsNumber(), (float)(*Trans)[1]->AsNumber(), (float)(*Trans)[2]->AsNumber()));
			}
			const TArray<TSharedPtr<FJsonValue>>* Rot = nullptr;
			if (Obj->TryGetArrayField(TEXT("rotation"), Rot) && Rot && Rot->Num() == 4)
			{
				FQuat Q((float)(*Rot)[0]->AsNumber(), (float)(*Rot)[1]->AsNumber(), (float)(*Rot)[2]->AsNumber(), (float)(*Rot)[3]->AsNumber());
				T.SetRotation(Q);
			}
			const TArray<TSharedPtr<FJsonValue>>* Scale = nullptr;
			if (Obj->TryGetArrayField(TEXT("scale"), Scale) && Scale && Scale->Num() == 3)
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
