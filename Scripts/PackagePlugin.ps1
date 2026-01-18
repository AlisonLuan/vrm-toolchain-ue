param(
  [string]$UE = "C:\Program Files\Epic Games\UE_5.7",
  [string]$RepoRoot = "C:\Users\Aliso\Documents\GIT\vrm-toolchain-ue",
  [string]$PluginName = "VrmToolchain"
)

$ErrorActionPreference = "Stop"

$UAT     = Join-Path $UE "Engine\Build\BatchFiles\RunUAT.bat"
$Uplugin = Join-Path $RepoRoot "Plugins\$PluginName\$PluginName.uplugin"
$OutPkg  = Join-Path $RepoRoot "build\Package\$PluginName"

if (-not (Test-Path $UAT))     { throw "RunUAT not found: $UAT" }
if (-not (Test-Path $Uplugin)) { throw ".uplugin not found: $Uplugin" }

# Clean package output
Remove-Item -Recurse -Force $OutPkg -ErrorAction SilentlyContinue | Out-Null

# 1) Build package
& $UAT BuildPlugin -Plugin="$Uplugin" -Package="$OutPkg" -TargetPlatforms=Win64 -Rocket
if ($LASTEXITCODE -ne 0) { throw "BuildPlugin failed (exit $LASTEXITCODE)" }

# 2) Strip dev-only tool from packaged output (remove ALL copies)
$devExeName = "vrm_validate.exe"

$found = Get-ChildItem $OutPkg -Recurse -Filter $devExeName -ErrorAction SilentlyContinue
if ($found) {
  $found | ForEach-Object {
    # Clear ReadOnly if needed
    if ($_.Attributes -band [IO.FileAttributes]::ReadOnly) {
      $_.Attributes = $_.Attributes -bxor [IO.FileAttributes]::ReadOnly
    }
    Remove-Item -Force $_.FullName
  }
}

# Optional: also strip PDB if you ever ship it
$foundPdb = Get-ChildItem $OutPkg -Recurse -Filter "vrm_validate.pdb" -ErrorAction SilentlyContinue
if ($foundPdb) {
  $foundPdb | ForEach-Object { Remove-Item -Force $_.FullName }
}

# 3) Verify no leak
$leaks = Get-ChildItem $OutPkg -Recurse -Filter $devExeName -ErrorAction SilentlyContinue
if ($leaks) {
  $leaks | Select-Object FullName | Format-Table -AutoSize
  throw "Packaging leak: $devExeName is present in the distribution output."
}

Write-Host "OK: Packaged plugin built and vrm_validate.exe stripped successfully." -ForegroundColor Green
Write-Host "Package folder: $OutPkg"
