// Copyright (c) 2024-2026 VRM Consortium; Copyright (c) 2025 Aliso Softworks LLC. All Rights Reserved.
#include "VrmAssetNaming.h"
#include "Misc/Paths.h"

FString FVrmAssetNaming::MakeVrmSourceAssetName(const FString& BaseName)
{
	// Sanitize first to ensure valid package/asset names
	FString Sanitized = SanitizeBaseName(BaseName);
	return FString::Printf(TEXT("%s%s"), *Sanitized, VrmSourceSuffix);
}

FString FVrmAssetNaming::MakeVrmSourcePackagePath(const FString& FolderPath, const FString& BaseName)
{
	FString AssetName = MakeVrmSourceAssetName(BaseName);
	
	// Ensure folder path doesn't end with slash
	FString Folder = FolderPath;
	if (Folder.EndsWith(TEXT("/")))
	{
		Folder = Folder.LeftChop(1);
	}
	
	return FString::Printf(TEXT("%s/%s"), *Folder, *AssetName);
}

FString FVrmAssetNaming::StripVrmSourceSuffix(const FString& Name)
{
	if (Name.EndsWith(VrmSourceSuffix))
	{
		return Name.LeftChop(FCString::Strlen(VrmSourceSuffix));
	}
	return Name;
}

bool FVrmAssetNaming::IsVrmSourceAssetName(const FString& Name)
{
	return Name.EndsWith(VrmSourceSuffix);
}

FString FVrmAssetNaming::SanitizeBaseName(const FString& BaseName)
{
	FString Result = BaseName;
	
	// Replace invalid package name characters with underscores
	// Unreal packages cannot contain: . (dots), spaces, and various special chars
	Result = Result.Replace(TEXT("."), TEXT("_"));
	Result = Result.Replace(TEXT(" "), TEXT("_"));
	Result = Result.Replace(TEXT("-"), TEXT("_"));
	Result = Result.Replace(TEXT("("), TEXT("_"));
	Result = Result.Replace(TEXT(")"), TEXT("_"));
	Result = Result.Replace(TEXT("["), TEXT("_"));
	Result = Result.Replace(TEXT("]"), TEXT("_"));
	Result = Result.Replace(TEXT("{"), TEXT("_"));
	Result = Result.Replace(TEXT("}"), TEXT("_"));
	Result = Result.Replace(TEXT(":"), TEXT("_"));
	Result = Result.Replace(TEXT(";"), TEXT("_"));
	Result = Result.Replace(TEXT(","), TEXT("_"));
	Result = Result.Replace(TEXT("'"), TEXT("_"));
	Result = Result.Replace(TEXT("\""), TEXT("_"));
	Result = Result.Replace(TEXT("\\"), TEXT("_"));
	Result = Result.Replace(TEXT("/"), TEXT("_"));
	Result = Result.Replace(TEXT("|"), TEXT("_"));
	Result = Result.Replace(TEXT("?"), TEXT("_"));
	Result = Result.Replace(TEXT("*"), TEXT("_"));
	Result = Result.Replace(TEXT("<"), TEXT("_"));
	Result = Result.Replace(TEXT(">"), TEXT("_"));
	
	// Remove consecutive underscores
	while (Result.Contains(TEXT("__")))
	{
		Result = Result.Replace(TEXT("__"), TEXT("_"));
	}
	
	// Trim leading/trailing underscores
	Result.TrimStartAndEndInline();
	Result = Result.TrimStartAndEnd().TrimChar('_');
	
	return Result;
}
