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
