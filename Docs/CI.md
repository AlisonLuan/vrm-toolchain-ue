# CI Gates and Script Standards

## PowerShell scripts (Windows runners)

### Compatibility
All scripts under `Scripts/*.ps1` must run on **Windows PowerShell 5.x**.

### Forbidden patterns
- Ternary-style expressions using `?` and `:` (break on PS 5.x)

### Approved idioms
Use `if/else` expressions, e.g.:

```powershell
return if ($x -ne $null) { $x } else { @() }
```

### CI enforcement

CI runs `Scripts/CheckPowerShellNoTernary.ps1` to prevent PS-7-only syntax from landing.

## Commandlets

### VrmToolchainRecomputeImportReports

Bulk recompute VRM meta import reports from stored feature flags.

**Invocation:**
```bash
<UE-Editor-Cmd> <Project.uproject> -run=VrmToolchainRecomputeImportReports [options]
```

**Options:**
- `-Root=/Game/MyPath` - Override default `/Game` root path for asset enumeration
- `-Save` - Save changed packages after recompute (default: mark dirty only)
- `-FailOnChanges` - Exit with nonzero code if any assets changed (useful for CI validation)
- `-FailOnFailed` - Exit with nonzero code if any assets failed to load (default: true)
- `-NoFailOnFailed` - Override FailOnFailed to allow failures without error exit

**Example (CI validation):**
```bash
# Verify all meta assets have up-to-date import reports
UnrealEditor-Cmd.exe MyProject.uproject -run=VrmToolchainRecomputeImportReports -FailOnChanges
```

**Example (bulk update):**
```bash
# Recompute and save all meta assets in /Game/VRM
UnrealEditor-Cmd.exe MyProject.uproject -run=VrmToolchainRecomputeImportReports -Root=/Game/VRM -Save
```

**Use cases:**
- CI validation: ensure import reports are deterministic and up-to-date
- Bulk update after upgrading report format or detection logic
- Regenerate reports for assets imported with older plugin versions
