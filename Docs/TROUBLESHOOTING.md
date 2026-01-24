# VRM Toolchain Troubleshooting Guide

## Common Issues & Solutions

### 1. Plugin Fails to Load: Missing VRM SDK Files

**Symptoms:**
- Error: `Missing required SDK file: vrm_glb_parser.lib`
- Plugin fails to compile
- Linker errors referencing VRM libraries

**Root Cause:**
VRM SDK binaries are not staged at the expected location.

**Solution:**
```powershell
# Option A: Run the FetchVrmSdk script
.\Scripts\FetchVrmSdk.ps1

# Option B: Manual staging
# Ensure C:\VRM_SDK contains:
#   - lib/Win64/Release/vrm_glb_parser.lib
#   - lib/Win64/Release/vrm_normalizers.lib
#   - lib/Win64/Release/vrm_validate.lib
#   - lib/Win64/Debug/* (debug variants)
#   - include/vrm_*/*.h (headers)
#   - bin/Win64/vrm_validate.exe
```

**Verification:**
```powershell
Test-Path C:\VRM_SDK\lib\Win64\Release\vrm_glb_parser.lib
```
Should return `True`.

---

### 2. Plugin Fails to Load: Include Path Errors

**Symptoms:**
- Error: `Cannot find include file: VrmToolchain/VrmMetadata.h`
- Compilation fails with "file not found" for VRM header files
- Missing `Public/VrmToolchain/…` headers

**Root Cause:**
Public metadata headers were moved to the `Public/VrmToolchain/…` namespace to ensure proper include resolution when the plugin is consumed as a binary.

**Solution:**
- **In your code:** Use namespaced includes:
  ```cpp
  #include "VrmToolchain/VrmMetadata.h"        // Correct
  // Not: #include "VrmMetadata.h"
  ```

- **Rebuild the plugin** to ensure canonical public headers are properly placed:
  ```powershell
  .\Scripts\PackagePlugin.ps1 -UE "C:\Program Files\Epic Games\UE_5.7" `
      -RepoRoot "$PWD" -PluginName "VrmToolchain"
  ```

---

### 3. Tests Fail with Timeout: Fallback Editor Hangs

**Symptoms:**
- Error: `Fallback automation run timed out after 600 seconds`
- RunUAT tests passed but fallback editor test timeout is triggered
- Tests show `2 of 2 Tests Passed` but run still fails

**Root Cause:**
Script was unnecessarily triggering a fallback editor test after RunUAT succeeded. This has been fixed in recent versions.

**Solution:**
- **Verify you have the latest script:**
  ```powershell
  git pull origin main
  ```
- **Re-run tests:**
  ```powershell
  $UE_PATH = "C:\Program Files\Epic Games\UE_5.7"
  .\Scripts\RunAutomationTests.ps1 -EditorPath "$UE_PATH\Engine\Binaries\Win64\UnrealEditor.exe" `
      -ProjectPath ".\HostProject\HostProject.uproject" `
      -TestFilter "VrmToolchain" `
      -TimeoutSeconds 1200
  ```

---

### 4. VRM File Validation Fails

**Symptoms:**
- Error: `vrm_validate: File does not exist`
- Validation contract test fails
- VRM file import shows validation errors

**Root Cause:**
VRM SDK validation tool is missing or the test VRM file is not accessible.

**Solution:**

1. **Verify vrm_validate.exe is staged:**
   ```powershell
   Test-Path "Plugins\VrmToolchain\Source\ThirdParty\VrmSdk\bin\Win64\vrm_validate.exe"
   ```

2. **Check VRM file permissions:**
   ```powershell
   # Test VRM file should be readable
   Get-Item "Path\To\Your\Model.vrm" | Select-Object FullName, Length
   ```

3. **Run validation manually:**
   ```powershell
   & "Plugins\VrmToolchain\Source\ThirdParty\VrmSdk\bin\Win64\vrm_validate.exe" `
       --json "Path\To\Your\Model.vrm"
   ```

---

### 5. Build Fails: "All Debug libs are <= Release size"

**Symptoms:**
- Error: `All Debug libs are <= Release size; this strongly suggests Release->Debug mirroring`
- Linker receives incorrectly mirrored Debug binaries
- Runtime behavior is unpredictable

**Root Cause:**
VRM SDK staging script detected that all Debug binaries are smaller than or equal to their Release counterparts, which is abnormal and suggests the SDK was incorrectly built or staged.

**Solution:**

1. **Verify SDK integrity:**
   ```powershell
   $sdkRoot = "C:\VRM_SDK"
   
   # Check file sizes
   Get-Item "$sdkRoot\lib\Win64\Debug\vrm_glb_parser.lib" | Select-Object Length
   Get-Item "$sdkRoot\lib\Win64\Release\vrm_glb_parser.lib" | Select-Object Length
   ```
   Debug should be **larger** than Release (debug symbols add size).

2. **Rebuild the VRM SDK** if sizes are inverted:
   - Consult your VRM SDK build documentation
   - Ensure Debug configuration includes symbols
   - Ensure Release configuration uses optimizations without symbols

3. **Verify build output:**
   ```powershell
   # If only a warning (not an error), the build may still succeed
   # Re-run with updated SDK
   ```

---

### 6. CI Lite Tests Pass, But Tier B Fails

**Symptoms:**
- CI Lite (No UE) gates pass
- Tier B (self-hosted) workflow fails
- Errors reference missing canonical header paths

**Root Cause:**
Plugin package structure doesn't match expected canonical layout. Tier B runs actual UE builds and validates the complete deliverable.

**Solution:**

1. **Run local package validation:**
   ```powershell
   .\Scripts\PackagePlugin.ps1 -UE "C:\Program Files\Epic Games\UE_5.7" `
       -RepoRoot "$PWD" -PluginName "VrmToolchain"
   ```

2. **Verify canonical headers exist:**
   ```powershell
   # Should exist in packaged output:
   Test-Path "build\Package\VrmToolchain\VrmToolchain\Public\VrmToolchain\VrmMetadata.h"
   ```

3. **Check CI Lite hardening assertions:**
   - Review `.github/workflows/ci-lite.yml`
   - Ensure all canonical header checks pass locally before pushing

---

### 7. "Plugin Not Found" When Added to Project

**Symptoms:**
- Editor fails to load plugin after adding to Plugins/ directory
- Error: `Plugin 'VrmToolchain' failed to load because module could not be found`
- .uplugin file is present but unrecognized

**Root Cause:**
Project files need to be regenerated after adding a plugin.

**Solution:**

1. **Delete generated project files:**
   ```powershell
   Remove-Item "Intermediate" -Recurse -Force
   Remove-Item "Binaries" -Recurse -Force
   Remove-Item "*.sln" -Force
   ```

2. **Regenerate Visual Studio project:**
   ```powershell
   $UE_ROOT = "C:\Program Files\Epic Games\UE_5.7"
   & "$UE_ROOT\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -VSVersion=2022
   ```

3. **Rebuild in Visual Studio:**
   ```powershell
   msbuild "YourProject.sln" /p:Configuration=Development /p:Platform=x64
   ```

4. **Reopen in Unreal Editor:**
   - Open the `.uproject` file

---

### 8. Automated Tests Report "No Gauntlet Events Fired"

**Symptoms:**
- Test output: `##### No Gauntlet Events were fired during this test!`
- Tests still report as passed
- No structured test metrics in JSON report

**Root Cause:**
This is a normal message when running contract/smoke tests that don't use Gauntlet's event system.

**Solution:**
This is **not an error**. If tests show:
```
## 2 of 2 Tests Passed in 03:07. (0 Failed, 0 Passed with Warnings)
BUILD SUCCESSFUL
AutomationTool exiting with ExitCode=0 (Success)
```
Then all is well. The lack of Gauntlet events is expected for contract validation tests.

---

## Diagnostic Checklist

Before opening an issue, verify:

- [ ] VRM SDK is staged: `Test-Path C:\VRM_SDK\lib\Win64\Release\vrm_glb_parser.lib`
- [ ] UE version is 5.7+: `$UE_ROOT\Engine\Binaries\Win64\UnrealEditor.exe -version`
- [ ] Latest plugin code: `git status` shows no local changes to build scripts
- [ ] Build directory is clean: `Remove-Item build -Recurse -Force`
- [ ] Visual Studio 2022 is installed and up-to-date
- [ ] No firewall/antivirus blocking build tools

---

## Still Stuck?

1. **Check CI logs:**
   - Visit [GitHub Actions](../../actions)
   - Find the failed run
   - Expand "Run automation tests" step
   - Look for error messages with context

2. **Reproduce locally:**
   ```powershell
   # Run the exact same automation command from CI
   $UE_PATH = "C:\Program Files\Epic Games\UE_5.7"
   .\Scripts\RunAutomationTests.ps1 `
       -EditorPath "$UE_PATH\Engine\Binaries\Win64\UnrealEditor.exe" `
       -ProjectPath ".\HostProject\HostProject.uproject" `
       -TestFilter "VrmToolchain" `
       -LogDir ".\build\AutomationLogs" `
       -ReportOutputPath ".\build\AutomationReports"
   ```

3. **Open an issue:**
   - Include: Build log, error message, OS, UE version
   - Attach: Log file from `build\AutomationLogs\`

---

**Last Updated:** January 24, 2026  
**Plugin Version:** Compatible with UE 5.7+

