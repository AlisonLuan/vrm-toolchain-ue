# VRM Toolchain for Unreal Engine

Unreal Engine 5.x plugin that consumes the vrm-sdk toolchain (Repo1–Repo5) via prebuilt SDK artifacts or a local install prefix.

## Overview

This repository contains:
- **VrmToolchain Plugin**: UE5 plugin with Runtime + Editor modules
- **ThirdParty Integration**: Flexible VRM SDK integration via `VRM_SDK_ROOT` or downloaded artifacts
- **Host Project**: Minimal test project for local smoke builds
- **CI/CD Workflows**: GitHub Actions for sanity checks and optional plugin builds

## Quick Start

1. **Clone the repository**
   ```bash
   git clone https://github.com/AlisonLuan/vrm-toolchain-ue.git
   cd vrm-toolchain-ue
   ```

2. **Setup VRM SDK** (choose one):
   - Set environment variable: `VRM_SDK_ROOT=/path/to/vrm-sdk`
   - Or run: `Plugins/VrmToolchain/Source/ThirdParty/DownloadVrmSdk.ps1 -ArtifactUrl <url>`

3. **Open HostProject**
   ```bash
   # Open HostProject/HostProject.uproject with Unreal Engine 5.x
   ```

## Documentation

- [Plugin README](Plugins/VrmToolchain/README.md) - Comprehensive plugin documentation
- [Implementation Details](IMPLEMENTATION.md) - Technical implementation overview
- [ThirdParty Setup](Plugins/VrmToolchain/Source/ThirdParty/README.md) - VRM SDK integration guide
- [Host Project](HostProject/README.md) - Test project documentation

## Features

- ✅ Runtime module with public API wrapper
- ✅ Editor module with importer and validator hooks
- ✅ ThirdParty VRM SDK integration
- ✅ PowerShell script for downloading Repo5 CI artifacts
- ✅ Support for `VRM_SDK_ROOT` environment variable
- ✅ Host project for local smoke builds
- ✅ GitHub Actions workflows for automated checks
- ✅ Cross-platform support (Windows, Linux, Mac)

## Repository Structure

```
vrm-toolchain-ue/
├── Plugins/VrmToolchain/          # Main plugin
│   ├── Source/
│   │   ├── VrmToolchainRuntime/   # Runtime module
│   │   ├── VrmToolchainEditor/    # Editor module
│   │   └── ThirdParty/            # VRM SDK integration
│   └── VrmToolchain.uplugin
├── HostProject/                   # Test project
│   └── HostProject.uproject
├── .github/workflows/             # CI/CD workflows
├── IMPLEMENTATION.md              # Implementation details
└── README.md                      # This file
```

## Building

### Using HostProject (Recommended)

Simply open `HostProject/HostProject.uproject` in Unreal Engine 5.x. The plugin will be compiled automatically.

### Using Unreal Automation Tool

```bash
# Build plugin package
RunUAT BuildPlugin -Plugin="Plugins/VrmToolchain/VrmToolchain.uplugin" -Package="Build/VrmToolchain"
```

See [Plugin README](Plugins/VrmToolchain/README.md) for detailed build instructions.

## CI/CD

- **Sanity Checks**: Runs on every push/PR
  - Validates plugin structure
  - Lints PowerShell scripts
  - Validates JSON files
  
- **Build Plugin** (Optional): Self-hosted workflow
  - Requires Unreal Engine on runner
  - Builds plugin package
  - Uploads artifacts

## License

See [LICENSE](LICENSE) file for details.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Ensure sanity checks pass
5. Submit a pull request
