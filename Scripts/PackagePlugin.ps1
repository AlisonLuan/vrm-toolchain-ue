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

# 2) Strip dev-only tools from packaged output (remove all .exe under VrmSdk/bin)
$PkgBinRoot = Join-Path $OutPkg "Source\ThirdParty\VrmSdk\bin"
if (Test-Path $PkgBinRoot) {
  Get-ChildItem -Path $PkgBinRoot -Recurse -Filter "*.exe" -File -ErrorAction SilentlyContinue |
    ForEach-Object {
      # Clear ReadOnly if needed
      if ($_.Attributes -band [IO.FileAttributes]::ReadOnly) {
        $_.Attributes = $_.Attributes -bxor [IO.FileAttributes]::ReadOnly
      }
      Remove-Item -Force $_.FullName
    }
}

# Optional cleanup: remove empty bin folder after stripping
if (Test-Path $PkgBinRoot) {
  $remaining = Get-ChildItem $PkgBinRoot -Recurse -File -ErrorAction SilentlyContinue
  if (-not $remaining) { Remove-Item -Recurse -Force $PkgBinRoot }
}

# 3) Verify no leak: no .exe under Source/ThirdParty/VrmSdk
$leaks = Get-ChildItem $OutPkg -Recurse -Filter "*.exe" -File -ErrorAction SilentlyContinue |
  Where-Object { $_.FullName -like "*\Source\ThirdParty\VrmSdk\*" }

if ($leaks) {
  $leaks | Select-Object FullName | Format-Table -AutoSize
  throw "Packaging leak: SDK executables are present in the distribution output."
}

Write-Host "OK: Packaged plugin built and vrm_validate.exe stripped successfully." -ForegroundColor Green
Write-Host "Package folder: $OutPkg"
