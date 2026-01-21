# VrmToolchain Implementation Summary

This repository now provides a minimal Unreal Engine 5 plugin skeleton plus a lightweight host project so the plugin can be compiled with Visual Studio 2022 on Win64.

## Plugin Structure
- `Plugins/VrmToolchain/VrmToolchain.uplugin` defines two modules:
  - `VrmToolchain` (Runtime) exposes `FVrmSdkFacade`, a small wrapper API whose public headers only include `CoreMinimal` and hide SDK state behind a PIMPL (`FImpl`).
  - `VrmToolchainEditor` (Editor) registers a stub menu entry inside the Level Editor main menu using `ToolMenus` and logs when the command executes.
- Both modules live under `Plugins/VrmToolchain/Source/` and follow the Unreal module convention with one public header and one private source file each.

## Runtime Module
- `VrmToolchain.Build.cs` declares dependencies on Core, CoreUObject, and Engine so the runtime module compiles across UE5 targets.
- `FVrmSdkFacade` offers `Initialize`, `Shutdown`, `IsAvailable`, and `GetVersion`, while `FVrmToolchainModule` handles module startup/shutdown logging.

## Editor Module
- `VrmToolchainEditor.Build.cs` adds Slate, LevelEditor, and ToolMenus dependencies plus the runtime module.
- The module is wrapped in `WITH_EDITOR`; when loaded it extends `LevelEditor.MainMenu.Window` with a `VRM Toolchain` menu item that logs a message via `UE_LOG`.

## Host Project
- The `HostProject` folder contains a minimal UE5 project (`HostProject.uproject`) that enables the plugin, plus a simple game module, editor/game targets, and configuration files.
- Building `HostProjectEditor` from Visual Studio 2022 (Win64) compiles both the host module and the plugin.

## Build Notes
- Run Unreal's `GenerateProjectFiles.bat` with `HostProject/HostProject.uproject`, then open the resulting solution in VS2022 and build the `HostProjectEditor` target.
- The README files under `Plugins/VrmToolchain/` and `HostProject/` explain the local build steps and how to package the plugin via `RunUAT BuildPlugin`.

## Packaging leak gate policy
- `Scripts/PackagePlugin.ps1` is *fail-fast*: it scans only the package output directory (`$OutPkg`, typically `build\Package\<PluginName>`) for `*.exe` / `*.pdb` and fails the packaging step if any are found.
- The script accepts an explicit opt-in (`-AllowBinaries`) for local debug only; **CI/workflows must never pass `-AllowBinaries`**. When `-AllowBinaries` is provided, the script will remove binaries from the package output but this is strictly for local developer diagnostics.
- Developer tooling (for example `vrm_validate.exe`) is **never** staged by `*.Build.cs` rules. Editor code must locate developer tools at runtime via `VRM_SDK_ROOT`, PATH, or local developer-only locations — do not depend on build staging.

## Next Steps
1. Replace the stubbed `FVrmSdkFacade` implementation with real VRM SDK calls once the SDK is available.
2. Expand the editor menu stub into actual tooling (importers, validators) as requirements emerge.
3. Keep documentation synchronized as new capabilities are added.
4. Integrate `vrm-sdk` v1.0.3 and add a CI check that runs `vrm_validate` (basic JSON-capable smoke test) against the staged binaries to assert compatibility.
