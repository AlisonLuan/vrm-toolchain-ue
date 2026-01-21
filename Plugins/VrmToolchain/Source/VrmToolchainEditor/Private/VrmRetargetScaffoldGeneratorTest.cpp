// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "VrmRetargetScaffoldGenerator.h"
#include "Misc/AutomationTest.h"

/**
 * Test that bone chain inference is deterministic and finds expected chains
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmRetargetChainInferenceTest, "VrmToolchain.Retarget.ChainInference",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmRetargetChainInferenceTest::RunTest(const FString& Parameters)
{
	// Create a mock skeleton bone list similar to VRM/humanoid skeletons
	TArray<FName> TestBoneNames = {
		TEXT("Root"),
		TEXT("Hips"),
		TEXT("Spine"),
		TEXT("Chest"),
		TEXT("UpperChest"),
		TEXT("Neck"),
		TEXT("Head"),
		TEXT("LeftShoulder"),
		TEXT("LeftUpperArm"),
		TEXT("LeftLowerArm"),
		TEXT("LeftHand"),
		TEXT("RightShoulder"),
		TEXT("RightUpperArm"),
		TEXT("RightLowerArm"),
		TEXT("RightHand"),
		TEXT("LeftUpperLeg"),
		TEXT("LeftLowerLeg"),
		TEXT("LeftFoot"),
		TEXT("RightUpperLeg"),
		TEXT("RightLowerLeg"),
		TEXT("RightFoot")
	};

	// Test finding root bone
	FName RootBone = FVrmRetargetScaffoldGenerator::FindRootBone(TestBoneNames);
	TestEqual(TEXT("Root bone found"), RootBone, FName(TEXT("Hips")));

	// Test finding spine chain
	FName SpineStart, SpineEnd;
	bool bHasSpine = FVrmRetargetScaffoldGenerator::FindSpineChain(TestBoneNames, SpineStart, SpineEnd);
	TestTrue(TEXT("Spine chain found"), bHasSpine);
	TestEqual(TEXT("Spine start is Hips"), SpineStart, FName(TEXT("Hips")));
	TestTrue(TEXT("Spine end is head/neck/chest"), 
		SpineEnd == FName(TEXT("Head")) || 
		SpineEnd == FName(TEXT("Neck")) || 
		SpineEnd == FName(TEXT("Chest")) ||
		SpineEnd == FName(TEXT("UpperChest")));

	// Test finding left arm chain
	FName LeftArmStart, LeftArmEnd;
	bool bHasLeftArm = FVrmRetargetScaffoldGenerator::FindArmChain(TestBoneNames, true, LeftArmStart, LeftArmEnd);
	TestTrue(TEXT("Left arm chain found"), bHasLeftArm);
	TestTrue(TEXT("Left arm start is shoulder or upperarm"),
		LeftArmStart == FName(TEXT("LeftShoulder")) ||
		LeftArmStart == FName(TEXT("LeftUpperArm")));
	TestEqual(TEXT("Left arm end is hand"), LeftArmEnd, FName(TEXT("LeftHand")));

	// Test finding right arm chain
	FName RightArmStart, RightArmEnd;
	bool bHasRightArm = FVrmRetargetScaffoldGenerator::FindArmChain(TestBoneNames, false, RightArmStart, RightArmEnd);
	TestTrue(TEXT("Right arm chain found"), bHasRightArm);
	TestTrue(TEXT("Right arm start is shoulder or upperarm"),
		RightArmStart == FName(TEXT("RightShoulder")) ||
		RightArmStart == FName(TEXT("RightUpperArm")));
	TestEqual(TEXT("Right arm end is hand"), RightArmEnd, FName(TEXT("RightHand")));

	// Test finding left leg chain
	FName LeftLegStart, LeftLegEnd;
	bool bHasLeftLeg = FVrmRetargetScaffoldGenerator::FindLegChain(TestBoneNames, true, LeftLegStart, LeftLegEnd);
	TestTrue(TEXT("Left leg chain found"), bHasLeftLeg);
	TestEqual(TEXT("Left leg start is upper leg"), LeftLegStart, FName(TEXT("LeftUpperLeg")));
	TestEqual(TEXT("Left leg end is foot"), LeftLegEnd, FName(TEXT("LeftFoot")));

	// Test finding right leg chain
	FName RightLegStart, RightLegEnd;
	bool bHasRightLeg = FVrmRetargetScaffoldGenerator::FindLegChain(TestBoneNames, false, RightLegStart, RightLegEnd);
	TestTrue(TEXT("Right leg chain found"), bHasRightLeg);
	TestEqual(TEXT("Right leg start is upper leg"), RightLegStart, FName(TEXT("RightUpperLeg")));
	TestEqual(TEXT("Right leg end is foot"), RightLegEnd, FName(TEXT("RightFoot")));

	// Test finding head bone
	FName HeadBone = FVrmRetargetScaffoldGenerator::FindHeadBone(TestBoneNames);
	TestTrue(TEXT("Head bone found"), HeadBone == FName(TEXT("Head")) || HeadBone == FName(TEXT("Neck")));

	// Test case insensitivity
	TArray<FName> CaseVariantBones = {
		TEXT("HIPS"),
		TEXT("lefthand"),
		TEXT("RiGhTfOoT")
	};

	FName CaseInsensitiveRoot = FVrmRetargetScaffoldGenerator::FindRootBone(CaseVariantBones);
	TestEqual(TEXT("Case insensitive root found"), CaseInsensitiveRoot, FName(TEXT("HIPS")));

	TArray<FString> LeftHandSearch = { TEXT("hand") };
	FName LeftHandFound = FVrmRetargetScaffoldGenerator::FindBoneByName(CaseVariantBones, LeftHandSearch);
	TestEqual(TEXT("Case insensitive left hand found"), LeftHandFound, FName(TEXT("lefthand")));

	return true;
}

/**
 * Test that chain inference is deterministic (produces same results on repeated runs)
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVrmRetargetChainDeterministicTest, "VrmToolchain.Retarget.ChainDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVrmRetargetChainDeterministicTest::RunTest(const FString& Parameters)
{
	TArray<FName> TestBones = {
		TEXT("Hips"), TEXT("Spine"), TEXT("Head"),
		TEXT("LeftShoulder"), TEXT("LeftHand"),
		TEXT("RightShoulder"), TEXT("RightHand"),
		TEXT("LeftUpperLeg"), TEXT("LeftFoot"),
		TEXT("RightUpperLeg"), TEXT("RightFoot")
	};

	// Run inference multiple times
	TArray<FName> Results1, Results2, Results3;
	
	for (int32 Run = 0; Run < 3; ++Run)
	{
		TArray<FName>* CurrentResults = (Run == 0) ? &Results1 : (Run == 1) ? &Results2 : &Results3;
		
		// Spine
		FName Start, End;
		if (FVrmRetargetScaffoldGenerator::FindSpineChain(TestBones, Start, End))
		{
			CurrentResults->Add(Start);
			CurrentResults->Add(End);
		}
		
		// Left arm
		if (FVrmRetargetScaffoldGenerator::FindArmChain(TestBones, true, Start, End))
		{
			CurrentResults->Add(Start);
			CurrentResults->Add(End);
		}
		
		// Right arm
		if (FVrmRetargetScaffoldGenerator::FindArmChain(TestBones, false, Start, End))
		{
			CurrentResults->Add(Start);
			CurrentResults->Add(End);
		}
		
		// Left leg
		if (FVrmRetargetScaffoldGenerator::FindLegChain(TestBones, true, Start, End))
		{
			CurrentResults->Add(Start);
			CurrentResults->Add(End);
		}
		
		// Right leg
		if (FVrmRetargetScaffoldGenerator::FindLegChain(TestBones, false, Start, End))
		{
			CurrentResults->Add(Start);
			CurrentResults->Add(End);
		}
	}

	// Verify all three runs produced identical results
	TestEqual(TEXT("Run 1 and Run 2 match"), Results1, Results2);
	TestEqual(TEXT("Run 2 and Run 3 match"), Results2, Results3);
	TestEqual(TEXT("Run 1 and Run 3 match"), Results1, Results3);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
