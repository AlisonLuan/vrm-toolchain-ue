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
Write-Host "Building plugin package..." -ForegroundColor Cyan
& $UAT BuildPlugin -Plugin="$Uplugin" -Package="$OutPkg" -TargetPlatforms=Win64 -Rocket
if ($LASTEXITCODE -ne 0) { throw "BuildPlugin failed (exit $LASTEXITCODE)" }

# 2) Strip forbidden binaries and temporary build artifacts
Write-Host "Stripping forbidden binaries and temporary build artifacts..." -ForegroundColor Cyan

# Scan and report forbidden files
$exeFiles = Get-ChildItem $OutPkg -Recurse -Include "*.exe" -File -ErrorAction SilentlyContinue
$pdbFiles = Get-ChildItem $OutPkg -Recurse -Include "*.pdb" -File -ErrorAction SilentlyContinue

if ($exeFiles -or $pdbFiles) {
    Write-Host "Found forbidden files (will be removed):" -ForegroundColor Yellow
    foreach ($file in @($exeFiles + $pdbFiles)) {
        Write-Host "  - $($file.Name)" -ForegroundColor Yellow
    }
}

# Remove the files
foreach ($file in @($exeFiles + $pdbFiles)) {
    Remove-Item -Force $file.FullName -ErrorAction Continue
}

if ($exeFiles -or $pdbFiles) {
    Write-Host "  Removed: $(($exeFiles.Count ?? 0) + ($pdbFiles.Count ?? 0)) file(s)" -ForegroundColor Green
}

# Delete temporary build folders (if they are not locked)
$tempDirs = @(
    (Join-Path $OutPkg "Intermediate"),
    (Join-Path $OutPkg "Saved")
)

foreach ($dir in $tempDirs) {
    if (Test-Path $dir) {
        try {
            Remove-Item -Recurse -Force $dir
            Write-Host "  Removed dir: $(Split-Path -Leaf $dir)" -ForegroundColor Gray
        } catch {
            Write-Host "  Warning: Could not remove $dir (may be in use): $($_.Exception.Message)" -ForegroundColor Yellow
        }
    }
}

# 3) Final gate: verify NO binaries or symbols remain
Write-Host "Running final validation gate..." -ForegroundColor Cyan
$finalLeaks = Get-ChildItem $OutPkg -Recurse -Include "*.exe", "*.pdb" -File -ErrorAction SilentlyContinue
if ($finalLeaks) {
    Write-Error "Packaging validation FAILED: forbidden binaries/symbols still exist in output:"
    $finalLeaks | ForEach-Object { Write-Error "  $($_.FullName)" }
    throw "Packaging validation failed. Cannot proceed."
}

Write-Host "Package validation passed. No forbidden binaries found." -ForegroundColor Green
Write-Host "Package output: $OutPkg" -ForegroundColor Green
