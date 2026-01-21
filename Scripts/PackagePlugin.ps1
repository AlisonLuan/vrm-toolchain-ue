param(
  [string]$UE = "C:\Program Files\Epic Games\UE_5.7",
  [string]$RepoRoot = "C:\Users\Aliso\Documents\GIT\vrm-toolchain-ue",
  [string]$PluginName = "VrmToolchain",
  [switch]$AllowBinaries
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

# 2) Fail-fast leak gate for dev-only tools and debug symbols in the package output
Write-Host "Scanning package for forbidden binaries (.exe, .pdb) under $OutPkg..." -ForegroundColor Cyan
# Find every executable and symbol file in the package output (recursive under $OutPkg only)
$allLeaks = Get-ChildItem $OutPkg -Recurse -Include "*.exe", "*.pdb" -File -ErrorAction SilentlyContinue
if ($allLeaks -and -not $AllowBinaries) {
    Write-Error "Packaging leak detected in $OutPkg: the following binaries/symbols were found:" 
    $allLeaks | ForEach-Object { Write-Error "  $($_.FullName)" }
    throw "Packaging leak: Binaries or Symbols found in package output. Rerun with -AllowBinaries if this is intentional (developer-only)."
}
elseif ($allLeaks -and $AllowBinaries) {
    Write-Warning "AllowBinaries specified: removing found binaries from package output (opt-in only)."
    foreach ($file in $allLeaks) {
        if ($file.Attributes -band [IO.FileAttributes]::ReadOnly) { $file.Attributes = $file.Attributes -bxor [IO.FileAttributes]::ReadOnly }
        Remove-Item -Force $file.FullName
        Write-Host "Stripped: $($file.FullName)" -ForegroundColor Gray
    }
}
else {
    Write-Host "No binaries found in package output." -ForegroundColor Green
}

# Optional: Cleanup empty folders left behind in ThirdParty
$PkgBinRoot = Join-Path $OutPkg "Source\ThirdParty\VrmSdk\bin"
if (Test-Path $PkgBinRoot) {
    if (-not (Get-ChildItem $PkgBinRoot -Recurse -File)) { 
        Remove-Item -Recurse -Force $PkgBinRoot 
    }
}

# Final verification: if anything remains, fail
$finalCheck = Get-ChildItem $OutPkg -Recurse -Include "*.exe", "*.pdb" -File -ErrorAction SilentlyContinue
if ($finalCheck) {
    $finalCheck | Select-Object FullName | Format-Table -AutoSize
    throw "Packaging leak: Binaries or Symbols still exist in the output folder!"
}