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

# 2) Strip ALL dev-only tools and debug symbols from the ENTIRE package
Write-Host "Sanitizing package: Removing all .exe and .pdb files..." -ForegroundColor Cyan

# Find every executable and symbol file in the output (recursive from root)
$allLeaks = Get-ChildItem $OutPkg -Recurse -Include "*.exe", "*.pdb" -File

foreach ($file in $allLeaks) {
    # Clear ReadOnly flag to prevent 'Access Denied' errors
    if ($file.Attributes -band [IO.FileAttributes]::ReadOnly) {
        $file.Attributes = $file.Attributes -bxor [IO.FileAttributes]::ReadOnly
    }
    Remove-Item -Force $file.FullName
    Write-Host "Stripped: $($file.Name)" -ForegroundColor Gray
}

# Optional: Cleanup empty folders left behind in ThirdParty
$PkgBinRoot = Join-Path $OutPkg "Source\ThirdParty\VrmSdk\bin"
if (Test-Path $PkgBinRoot) {
    if (-not (Get-ChildItem $PkgBinRoot -Recurse -File)) { 
        Remove-Item -Recurse -Force $PkgBinRoot 
    }
}

# 3) Final Leak Gate Check (Global)
$finalCheck = Get-ChildItem $OutPkg -Recurse -Include "*.exe", "*.pdb" -File
if ($finalCheck) {
    $finalCheck | Select-Object FullName | Format-Table -AutoSize
    throw "Packaging leak: Binaries or Symbols still exist in the output folder!"
}

Write-Host "OK: Package is fully sanitized (No .exe or .pdb found)." -ForegroundColor Green

# Add this to the end of PackagePlugin.ps1
Write-Host "Auditing build for forbidden binaries..."
$Forbidden = Get-ChildItem $PackagePath -Recurse -File | Where-Object { $_.Name -match '\.(exe|pdb)$' }
if ($Forbidden) {
    Write-Warning "Found forbidden files in package!"
    $Forbidden.FullName
} else {
    Write-Host "Audit passed: No .exe or .pdb files found." -ForegroundColor Green
}