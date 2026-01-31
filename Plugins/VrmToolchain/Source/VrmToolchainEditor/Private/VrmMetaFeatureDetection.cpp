#include "VrmMetaFeatureDetection.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

namespace VrmMetaDetection
{
    FVrmMetaFeatures ParseMetaFeaturesFromJson(const FString& JsonStr)
    {
        FVrmMetaFeatures Result;

        // Attempt to deserialize JSON
        TSharedPtr<FJsonObject> RootObj;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
        if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
        {
            // Parse failed; return conservative defaults (all Unknown/false)
            return Result;
        }

        // Safely get extensions object (may be missing or non-object in malformed JSON)
        const TSharedPtr<FJsonObject>* ExtObjPtr = nullptr;
        RootObj->TryGetObjectField(TEXT("extensions"), ExtObjPtr);
        TSharedPtr<FJsonObject> ExtObj = ExtObjPtr ? *ExtObjPtr : nullptr;

        // Detect VRM version based on extension presence
        EVrmVersion MetaVer = EVrmVersion::Unknown;
        if (ExtObj.IsValid())
        {
            if (ExtObj->HasField(TEXT("VRM")))
            {
                MetaVer = EVrmVersion::VRM0;
            }
            else if (ExtObj->HasField(TEXT("VRMC_vrm")))
            {
                MetaVer = EVrmVersion::VRM1;
            }
        }

        Result.SpecVersion = MetaVer;

        // Initialize feature flags
        bool bHasHumanoid = false;
        bool bHasSpring = false;
        bool bHasBlendOrExpr = false;
        bool bHasThumb = false;

        // VRM0 feature checks
        if (MetaVer == EVrmVersion::VRM0 && ExtObj.IsValid())
        {
            const TSharedPtr<FJsonObject>* VrmPtr = nullptr;
            ExtObj->TryGetObjectField(TEXT("VRM"), VrmPtr);
            TSharedPtr<FJsonObject> Vrm = VrmPtr ? *VrmPtr : nullptr;
            
            if (Vrm.IsValid())
            {
                bHasHumanoid = Vrm->HasField(TEXT("humanoid"));
            }
            bHasSpring = RootObj->HasField(TEXT("secondaryAnimation"));
            bHasBlendOrExpr = RootObj->HasField(TEXT("blendShapeMaster"));
            bHasThumb = RootObj->HasField(TEXT("thumbnail"));
        }
        // VRM1 feature checks
        else if (MetaVer == EVrmVersion::VRM1 && ExtObj.IsValid())
        {
            const TSharedPtr<FJsonObject>* Vrm1Ptr = nullptr;
            ExtObj->TryGetObjectField(TEXT("VRMC_vrm"), Vrm1Ptr);
            TSharedPtr<FJsonObject> Vrm1 = Vrm1Ptr ? *Vrm1Ptr : nullptr;
            
            if (Vrm1.IsValid())
            {
                bHasHumanoid = Vrm1->HasField(TEXT("humanoid"));
                bHasBlendOrExpr = Vrm1->HasField(TEXT("expressions")) || Vrm1->HasField(TEXT("blendShapeMaster"));
                bHasThumb = Vrm1->HasField(TEXT("thumbnail"));
            }
            bHasSpring = ExtObj->HasField(TEXT("VRMC_springBone")) || (Vrm1.IsValid() && Vrm1->HasField(TEXT("springBone")));
        }

        Result.bHasHumanoid = bHasHumanoid;
        Result.bHasSpringBones = bHasSpring;
        Result.bHasBlendShapesOrExpressions = bHasBlendOrExpr;
        Result.bHasThumbnail = bHasThumb;

        return Result;
    }
}
