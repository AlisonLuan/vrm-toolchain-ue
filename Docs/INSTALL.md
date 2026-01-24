# VRM Toolchain Installation Guide

## Overview

The **VRM Toolchain** is an Unreal Engine 5.7+ plugin that provides comprehensive VRM (Virtual Reality Model) format support, including:
- VRM model import and validation
- VRM metadata handling
- Skeleton/humanoid retargeting
- VRM SDK integration with native C++ bindings

## Prerequisites

### System Requirements
- **Unreal Engine** 5.7 or later (Windows)
- **Visual Studio 2022** or compatible build tools
- **VRM SDK**: Must be staged at `C:\VRM_SDK` (Windows) before plugin build
  - Includes libraries: `vrm_glb_parser`, `vrm_normalizers`, `vrm_validate`
  - Validate placement: `C:\VRM_SDK\lib\Win64\Release\vrm_*.lib` (and Debug variants)

### Environment Setup
If not already available, obtain the VRM SDK from your organization's private repository or build system.

## Installation Steps

### Option A: Use Pre-Built Plugin (Recommended for End Users)

1. **Download the plugin package**
   - Get `VrmToolchain-ue5.7-*.zip` from [Releases](../../releases)
   - Extract to your project's `Plugins/` directory

2. **Verify plugin structure**
   ```
   <ProjectRoot>/
   └── Plugins/
       └── VrmToolchain/
           ├── Binaries/
           ├── Resources/
           ├── Source/
           └── VrmToolchain.uplugin
   ```

3. **Regenerate project files**
   ```
   # Windows
   <UE_ROOT>/Engine/Build/BatchFiles/Build.bat <ProjectName>Editor Win64 Development
   ```

4. **Open project in Unreal Editor**
   - The plugin will automatically compile and load on startup

### Option B: Build from Source (For Developers)

1. **Clone the repository**
   ```bash
   git clone https://github.com/AlisonLuan/vrm-toolchain-ue.git
   cd vrm-toolchain-ue
   ```

2. **Stage the VRM SDK**
   ```powershell
   # Windows PowerShell
   .\Scripts\FetchVrmSdk.ps1
   ```
   This will:
   - Check for pre-staged SDK at `C:\VRM_SDK`
   - Copy SDK files to `Plugins/VrmToolchain/Source/ThirdParty/VrmSdk`
   - Validate required binaries exist

3. **Generate Visual Studio project**
   ```powershell
   # Windows
   & "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -VSVersion=2022
   ```

4. **Build the plugin**
   ```powershell
   # Using Unreal Automation Tools
   $UE_PATH = "C:\Program Files\Epic Games\UE_5.7"
   & "$UE_PATH\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin `
       -Plugin=".\Plugins\VrmToolchain\VrmToolchain.uplugin" `
       -Package=".\build\Package\VrmToolchain" `
       -TargetPlatforms=Win64
   ```

5. **Open the HostProject**
   ```
   Open: HostProject/HostProject.uproject
   ```
   The plugin will compile and load on startup.

## Validation

### Quick Test: Verify Plugin Loads
1. Open Unreal Editor with the project
2. Go to **Window → Plugins**
3. Search for "VRM"
4. Confirm **VrmToolchain** shows as enabled

### Run Automated Tests
```powershell
# From project root
$UE_PATH = "C:\Program Files\Epic Games\UE_5.7"
& "$UE_PATH\Engine\Build\BatchFiles\RunUAT.bat" RunEditorTests `
    -Test="EditorTest.EditorTestNode" `
    -testname="VrmToolchain" `
    -project=".\HostProject\HostProject.uproject" `
    -reportoutputpath=".\build\AutomationReports"
```

Expected output: **`2 of 2 Tests Passed`**

## Troubleshooting

For common issues, see [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

## Project Structure

```
vrm-toolchain-ue/
├── Plugins/VrmToolchain/              # Main plugin directory
│   ├── Source/
│   │   ├── VrmToolchain/              # Runtime module
│   │   ├── VrmToolchainEditor/        # Editor module
│   │   └── ThirdParty/VrmSdk/         # VRM SDK integration
│   ├── VrmToolchain.uplugin           # Plugin descriptor
│   └── Binaries/                      # Compiled DLLs (post-build)
├── HostProject/                       # Example Unreal project
├── Scripts/                           # Build and utility scripts
│   ├── PackagePlugin.ps1              # Plugin packaging utility
│   ├── FetchVrmSdk.ps1                # VRM SDK staging script
│   └── RunAutomationTests.ps1         # Test runner
└── Docs/                              # Documentation
```

## Next Steps

- Read the [README](../README.md) for feature overview
- Review example integration in `HostProject/`
- Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for known issues

## Support

For issues or questions:
1. Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
2. Review CI logs in [GitHub Actions](../../actions)
3. Open an issue on [GitHub Issues](../../issues)

---

**Last Updated:** January 24, 2026  
**Tested On:** Unreal Engine 5.7.1, Windows 11
