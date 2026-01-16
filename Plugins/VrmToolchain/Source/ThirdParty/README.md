# VRM SDK ThirdParty Integration

This directory contains the integration layer for the VRM SDK.

## Setup

There are two ways to provide the VRM SDK to the plugin:

### Option 1: Use VRM_SDK_ROOT Environment Variable

Set the `VRM_SDK_ROOT` environment variable to point to your local VRM SDK installation:

```bash
# Windows
set VRM_SDK_ROOT=C:\path\to\vrm-sdk

# Linux/Mac
export VRM_SDK_ROOT=/path/to/vrm-sdk
```

The VRM SDK directory should have the following structure:
```
vrm-sdk/
  include/          # Header files
  lib/
    Win64/          # Windows libraries
    Linux/          # Linux libraries
    Mac/            # macOS libraries
  bin/
    Win64/          # Windows DLLs
```

### Option 2: Download Repo5 CI Artifacts

Run the PowerShell script to download prebuilt artifacts:

```powershell
# Download specific version
.\DownloadVrmSdk.ps1 -ArtifactUrl "https://example.com/vrm-sdk-artifacts.zip"

# Force re-download even if VRM_SDK_ROOT is set
.\DownloadVrmSdk.ps1 -ArtifactUrl "https://example.com/vrm-sdk-artifacts.zip" -Force
```

## Directory Structure

After setup, the VrmSdk directory should contain:
- `include/` - VRM SDK header files
- `lib/` - Platform-specific libraries
- `bin/` - Platform-specific runtime binaries

## Build Integration

The `VrmToolchainRuntime.Build.cs` file automatically:
1. Checks for `VRM_SDK_ROOT` environment variable
2. Falls back to `Plugins/VrmToolchain/Source/ThirdParty/VrmSdk` if not set
3. Adds appropriate include paths and libraries based on the target platform
