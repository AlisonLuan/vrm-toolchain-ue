# VRM Toolchain Plugin

Lightweight Unreal Engine 5 plugin that bundles a runtime wrapper API and a guarded editor extension, forming the basis for future VRM SDK integrations.

## Structure
- `Plugins/VrmToolchain/VrmToolchain.uplugin` enables two modules:
  - `VrmToolchain` (Runtime): exposes a PIMPL-based facade (`FVrmSdkFacade`) so downstream code never needs third-party headers.
  - `VrmToolchainEditor` (Editor): adds a stub entry under the Level Editor main menu using `ToolMenus`, and it is compiled only when `WITH_EDITOR` is true.

**ThirdParty dependency:** This plugin consumes `vrm-sdk` (pinned to **v1.0.3**) — which itself pins `vrm-validate` (v1.0.1). Use `Scripts/FetchVrmSdk.ps1` to stage the SDK into `Plugins/VrmToolchain/Source/ThirdParty/VrmSdk`.
- Source layout:
```
Plugins/VrmToolchain/Source/
- VrmToolchain/
  - Public/VrmToolchain.h
  - Public/VrmToolchainWrapper.h
  - Private/VrmToolchain.cpp
  - VrmToolchain.Build.cs
- VrmToolchainEditor/
  - Public/VrmToolchainEditor.h
  - Private/VrmToolchainEditor.cpp
  - VrmToolchainEditor.Build.cs
```

## Distribution contract

To avoid accidental leakage of developer-only tooling and to make packaging deterministic, the plugin package follows this contract:

- The packaged plugin ships:
  - `Source/ThirdParty/VrmSdk/include/**`
  - `Source/ThirdParty/VrmSdk/lib/**`

- The packaged plugin does NOT ship:
  - `Source/ThirdParty/VrmSdk/bin/**` (developer tools & executables)
  - Any `*.exe` or `*.pdb` (these are stripped and leak-gated by `Scripts/PackagePlugin.ps1` and CI)

Developer tooling (for example, `vrm_validate.exe`) is kept in the repository for contributors, but CI packaging enforces that these binaries are removed from packaged artifacts.

**Engineering note:** Pinned inputs and freeze metadata are recorded at the repository root `Docs/Versions.md` (create a `freeze/YYYY-MM-DD` branch and tag when advancing the pinned inputs).

## Developer validation tool (optional)

The plugin supports an optional command-line validation tool (`vrm_validate.exe`) used by developers to validate VRM assets locally. This tool is **not** shipped with packaged plugin artifacts.

How to obtain the developer tools:
- Set `VRM_SDK_ROOT` to a local SDK staging directory that contains `include/` and `lib/` (preferred for CI-like testing).
- Or, install the developer tools from a separate developer ZIP distribution (do not add executables to the plugin zip).

If `vrm_validate.exe` is not present, the editor will disable CLI-based validation features and log a message guiding developers how to install the tools.

## Runtime API
`FVrmSdkFacade` exposes simple helpers:
```cpp
FVrmSdkFacade::Initialize();
FVrmSdkFacade::IsAvailable();
FVrmSdkFacade::GetVersion();
FVrmSdkFacade::Shutdown();
```
The implementation hides its state through an internal `FImpl` struct, so the public headers never include real third-party SDK headers.

## Editor Stub
The editor module registers a menu entry called "VRM Toolchain" under `LevelEditor.MainMenu.Window` and logs a notice when the stub command executes. The entire UI hook is wrapped in `#if WITH_EDITOR` so packaged games omit the editor-only code.

## Building Locally
1. Generate project files for `HostProject/HostProject.uproject` using
   `"<UE_ROOT>\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -project="HostProject/HostProject.uproject" -game -engine`.
2. Open the generated `HostProject.sln` in Visual Studio 2022, choose the `HostProjectEditor` Win64 target, and build; the plugin will compile with the host project.
3. To package the plugin without the host project, run
   `RunUAT.bat BuildPlugin -Plugin="Plugins/VrmToolchain/VrmToolchain.uplugin" -Package="Build/VrmToolchain" -TargetPlatforms=Win64 -CreateSubFolder`.

The host project ensures the plugin is built with VS2022/Win64 settings and can be opened directly in Unreal Editor to test the menu stub or runtime wrapper behavior.

## VRM Normalization Feature (Issue #4)

The plugin provides an editor-only action to normalize/repair source VRM files. This is an **optional**, **opt-in** feature that developers can use to produce cleaned copies of VRM assets.

### Features
- **Content Browser Action**: Right-click on an imported `USkeletalMesh` (that originated from a `.vrm` or `.glb` file) and select **VrmToolchain → Normalize Source VRM**.
- **Configurable Output**: Settings control where normalized files are written:
  - **Next to Source** (default): Writes `<SourceDir>/<Name>.normalized.vrm`
  - **Saved Directory**: Writes to `Saved/VrmToolchain/Normalized/<Name>.normalized.vrm`
- **Report Generation**: Creates a JSON report (`.report.json`) describing changes made during normalization.
- **Non-Fatal Failures**: If normalization fails, it logs an error without breaking the project or asset state.
- **No External Dependencies**: Uses in-process library code (vrm_normalizers.lib); does not require `vrm_validate.exe`.

### Configuration
Open **Editor Preferences → Plugins → VRM Normalization** to configure:
- **Output Location**: Choose between "Next to Source" or "Saved Directory"
- **Overwrite Without Prompt**: Allow overwriting existing normalized files
- **Normalized Suffix**: Customize the suffix appended to normalized filenames (default: `.normalized`)

### Implementation
- All normalization code is in the `VrmToolchainEditor` module (editor-only).
- The runtime module (`VrmToolchain`) is not modified.
- Uses `UAssetImportData` to resolve source file paths from imported skeletal meshes.
- Validates that source files have `.vrm` or `.glb` extensions.
- Generates deterministic output paths based on configurable settings.

### Testing
Deterministic unit tests verify:
- Output path generation is deterministic for the same inputs
- Custom suffixes and different output strategies work correctly
- Validation logic correctly accepts/rejects file extensions

### Usage Example
1. Import a VRM file using VRM4U (creates a `USkeletalMesh` asset)
2. Right-click the skeletal mesh in Content Browser
3. Select **VrmToolchain → Normalize Source VRM**
4. The normalized file and report are written to the configured location
5. A notification shows the output paths or any errors

### Implementation Notes
- Currently implements a stub that validates and copies the file (actual normalization via vrm_normalizers.lib will be integrated when SDK headers are fully available).
- The action is only visible for skeletal meshes with valid VRM/GLB source import data.
- All operations are non-blocking and log results to the Output Log.

