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

# 2) Strip dev-only tool from packaged output
$PkgExe = Join-Path $OutPkg "Source\ThirdParty\VrmSdk\bin\Win64\vrm_validate.exe"
if (Test-Path $PkgExe) { Remove-Item -Force $PkgExe }

# Optional cleanup of empty bin folder
$PkgBin = Split-Path $PkgExe -Parent
if (Test-Path $PkgBin) {
  $files = Get-ChildItem $PkgBin -Recurse -File -ErrorAction SilentlyContinue
  if (-not $files) { Remove-Item -Recurse -Force $PkgBin }
}

# 3) Verify no leak
$leaks = Get-ChildItem $OutPkg -Recurse -Filter "vrm_validate.exe" -ErrorAction SilentlyContinue
if ($leaks) {
  $leaks | Select-Object FullName | Format-Table -AutoSize
  throw "Packaging leak: vrm_validate.exe is present in the distribution output."
}

Write-Host "OK: Packaged plugin built and vrm_validate.exe stripped successfully." -ForegroundColor Green
Write-Host "Package folder: $OutPkg"
