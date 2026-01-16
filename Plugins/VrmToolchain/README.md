# VRM Toolchain Plugin

Unreal Engine 5.x plugin that consumes the vrm-sdk toolchain via prebuilt SDK artifacts or a local install prefix.

## Features

- **Runtime Module**: Core functionality for VRM file handling
- **Editor Module**: Import/export and validation tools for VRM files in the Unreal Editor
- **ThirdParty Integration**: Flexible integration with vrm-sdk via environment variable or downloaded artifacts
- **Host Project**: Minimal test project for local smoke builds
- **CI/CD**: GitHub Actions workflows for automated sanity checks and optional plugin builds

## Directory Structure

```
vrm-toolchain-ue/
├── Plugins/
│   └── VrmToolchain/
│       ├── VrmToolchain.uplugin          # Plugin descriptor
│       ├── Source/
│       │   ├── VrmToolchainRuntime/      # Runtime module
│       │   │   ├── Public/
│       │   │   │   ├── VrmToolchainRuntime.h
│       │   │   │   └── VrmSdkWrapper.h   # Public API wrapper
│       │   │   ├── Private/
│       │   │   └── VrmToolchainRuntime.Build.cs
│       │   ├── VrmToolchainEditor/       # Editor module
│       │   │   ├── Public/
│       │   │   │   ├── VrmToolchainEditor.h
│       │   │   │   ├── VrmImporter.h     # Import factory
│       │   │   │   └── VrmValidator.h    # Validation hooks
│       │   │   ├── Private/
│       │   │   └── VrmToolchainEditor.Build.cs
│       │   └── ThirdParty/               # VRM SDK integration
│       │       ├── DownloadVrmSdk.ps1    # Download script
│       │       ├── README.md
│       │       └── VrmSdk/               # SDK files (local or downloaded)
│       └── Resources/
├── HostProject/                          # Test host project
│   ├── HostProject.uproject
│   ├── Source/
│   └── Config/
└── .github/
    └── workflows/
        ├── sanity-checks.yml             # Automated sanity checks
        └── build-plugin.yml              # Optional self-hosted build
```

## Getting Started

### Prerequisites

- Unreal Engine 5.0 or later
- Visual Studio 2019/2022 (Windows) or Xcode (Mac) or appropriate build tools (Linux)
- PowerShell 5.0+ (for downloading VRM SDK artifacts)

### VRM SDK Setup

The plugin requires the VRM SDK to be available. There are two options:

#### Option 1: Use VRM_SDK_ROOT Environment Variable

Set the `VRM_SDK_ROOT` environment variable to point to your local VRM SDK installation:

**Windows (PowerShell):**
```powershell
$env:VRM_SDK_ROOT = "C:\path\to\vrm-sdk"
# Or set permanently:
[System.Environment]::SetEnvironmentVariable('VRM_SDK_ROOT', 'C:\path\to\vrm-sdk', 'User')
```

**Linux/Mac:**
```bash
export VRM_SDK_ROOT=/path/to/vrm-sdk
# Add to ~/.bashrc or ~/.zshrc for persistence
```

#### Option 2: Download Repo5 CI Artifacts

Run the PowerShell download script:

```powershell
cd Plugins/VrmToolchain/Source/ThirdParty
.\DownloadVrmSdk.ps1 -ArtifactUrl "https://example.com/vrm-sdk-artifacts.zip"
```

### Building the Plugin

#### Using the HostProject

1. Open `HostProject/HostProject.uproject` in Unreal Editor
2. The plugin will be automatically loaded and compiled
3. Check the Output Log for any compilation issues

#### Using Unreal Automation Tool (UAT)

Build the plugin package:

```bash
# Windows
"%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin ^
  -Plugin="%CD%\Plugins\VrmToolchain\VrmToolchain.uplugin" ^
  -Package="%CD%\Build\VrmToolchain" ^
  -CreateSubFolder ^
  -TargetPlatforms=Win64

# Linux/Mac
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildPlugin \
  -Plugin="$PWD/Plugins/VrmToolchain/VrmToolchain.uplugin" \
  -Package="$PWD/Build/VrmToolchain" \
  -CreateSubFolder \
  -TargetPlatforms=Linux
```

### Installing the Plugin

Copy the built plugin to your project:

```
YourProject/
└── Plugins/
    └── VrmToolchain/
```

Then enable it in your `.uproject` file or through the Plugins window in the Unreal Editor.

## Usage

### Importing VRM Files

1. In the Unreal Editor, use **File > Import**
2. Select a `.vrm` file
3. The VRM importer will validate and import the file

### API Usage

```cpp
#include "VrmSdkWrapper.h"

// Initialize the VRM SDK
FVrmSdkWrapper::Initialize();

// Check if SDK is available
if (FVrmSdkWrapper::IsAvailable())
{
    FString Version = FVrmSdkWrapper::GetVersion();
    UE_LOG(LogTemp, Log, TEXT("VRM SDK Version: %s"), *Version);
}

// Validate a VRM file
TArray<FString> Errors;
bool bValid = FVrmSdkWrapper::ValidateVrmFile(TEXT("path/to/file.vrm"), Errors);

// Shutdown when done
FVrmSdkWrapper::Shutdown();
```

### Validation API

```cpp
#include "VrmValidator.h"

TArray<FString> Errors, Warnings;
bool bValid = FVrmValidator::ValidateFile(FilePath, Errors, Warnings);

if (!bValid)
{
    for (const FString& Error : Errors)
    {
        UE_LOG(LogTemp, Error, TEXT("%s"), *Error);
    }
}
```

## CI/CD

### Sanity Checks Workflow

The `sanity-checks.yml` workflow runs automatically on push and pull requests:

- Validates plugin structure
- Checks module Build.cs files exist
- Lints PowerShell scripts
- Validates JSON descriptors

### Build Plugin Workflow (Self-Hosted)

The `build-plugin.yml` workflow is optional and requires self-hosted runners:

1. Set up a self-hosted runner with Unreal Engine installed
2. Set the `UE_ROOT` environment variable on the runner
3. Trigger the workflow manually or include `[build]` in commit messages

## Development

### Module Structure

#### VrmToolchainRuntime

The Runtime module provides:
- VRM SDK wrapper (`FVrmSdkWrapper`)
- Core functionality available at runtime
- Platform-independent VRM handling

#### VrmToolchainEditor

The Editor module provides:
- Import factory (`UVrmImporterFactory`)
- Validation tools (`FVrmValidator`)
- Editor-only functionality

### Adding New Features

1. Runtime features: Add to `VrmToolchainRuntime` module
2. Editor tools: Add to `VrmToolchainEditor` module
3. Update API wrapper as needed
4. Add tests to HostProject

## Troubleshooting

### Build Errors

**"vrm-sdk.lib not found"**
- Ensure VRM SDK is properly installed
- Check `VRM_SDK_ROOT` environment variable
- Run `DownloadVrmSdk.ps1` script

**"Module VrmToolchainRuntime could not be loaded"**
- Rebuild the project
- Check Output Log for specific errors
- Verify all dependencies are present

### Runtime Errors

**"VRM SDK is not initialized"**
- Call `FVrmSdkWrapper::Initialize()` before using the API
- Check that VRM SDK libraries are in the binary path

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run sanity checks locally
5. Submit a pull request

## License

See [LICENSE](LICENSE) file for details.

## Acknowledgments

- VRM SDK team for the core toolchain
- Unreal Engine community
