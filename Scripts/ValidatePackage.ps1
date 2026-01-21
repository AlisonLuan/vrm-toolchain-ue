#!/usr/bin/env pwsh
<#
.SYNOPSIS
Validates that a packaged VrmToolchain plugin meets the distribution contract:
- No forbidden binaries (*.exe, *.pdb)
- No forbidden directories (HostProject, Intermediate, Saved)
- Only expected top-level structure (Binaries, Resources, Source, .uplugin, README.md)

This script is designed to run post-packaging as a CI gate or manual verification step.

.PARAMETER PackagePath
Path to the packaged plugin (e.g., build/Package/VrmToolchain)
Default: build/Package/VrmToolchain (relative to current directory)

.EXAMPLE
.\Scripts\ValidatePackage.ps1 -PackagePath "build/Package/VrmToolchain"
.\Scripts\ValidatePackage.ps1  # Uses default path

.EXIT CODES
0   - All validations passed
1   - Forbidden binaries found
2   - Forbidden directories found
3   - Missing required files
4   - Invalid package path
#>

param(
  [string]$PackagePath = "build/Package/VrmToolchain"
)

$ErrorActionPreference = "Stop"

Write-Host "Validating package contract: $PackagePath" -ForegroundColor Cyan

# 1. Verify package path exists
if (-not (Test-Path $PackagePath)) {
    Write-Error "Package path not found: $PackagePath"
    exit 4
}

# 2. Check for forbidden binaries
Write-Host "Checking for forbidden binaries..." -ForegroundColor Gray
$exeFiles = @(Get-ChildItem $PackagePath -Recurse -Include "*.exe" -File -ErrorAction SilentlyContinue)
$pdbFiles = @(Get-ChildItem $PackagePath -Recurse -Include "*.pdb" -File -ErrorAction SilentlyContinue)

if ($exeFiles.Count -gt 0) {
    Write-Error "Package validation FAILED: Found .exe files ($($exeFiles.Count) file(s))"
    $exeFiles | ForEach-Object { Write-Error "  $_" }
    exit 1
}

if ($pdbFiles.Count -gt 0) {
    Write-Error "Package validation FAILED: Found .pdb files ($($pdbFiles.Count) file(s))"
    $pdbFiles | ForEach-Object { Write-Error "  $_" }
    exit 1
}

Write-Host "  ✓ No .exe files" -ForegroundColor Green
Write-Host "  ✓ No .pdb files" -ForegroundColor Green

# 3. Check for forbidden directories
Write-Host "Checking for forbidden directories..." -ForegroundColor Gray
$forbiddenDirs = @("HostProject", "Intermediate", "Saved")
$foundForbidden = $false

foreach ($dirName in $forbiddenDirs) {
    $path = Join-Path $PackagePath $dirName
    if (Test-Path $path) {
        Write-Error "Package validation FAILED: Found forbidden directory: $dirName"
        $foundForbidden = $true
    } else {
        Write-Host "  ✓ $dirName absent" -ForegroundColor Green
    }
}

if ($foundForbidden) {
    exit 2
}

# 4. Verify required files exist
Write-Host "Checking required files..." -ForegroundColor Gray
$requiredFiles = @("*.uplugin")
$missingRequired = $false

foreach ($pattern in $requiredFiles) {
    $matches = @(Get-ChildItem $PackagePath -Include $pattern -File -Depth 0 -ErrorAction SilentlyContinue)
    if ($matches.Count -eq 0) {
        Write-Error "Package validation FAILED: Missing required file pattern: $pattern"
        $missingRequired = $true
    } else {
        Write-Host "  ✓ $pattern found ($($matches.Count))" -ForegroundColor Green
    }
}

if ($missingRequired) {
    exit 3
}

# 5. Verify expected folder structure
Write-Host "Checking folder structure..." -ForegroundColor Gray
$requiredFolders = @("Binaries", "Resources", "Source")
$missingFolders = $false

foreach ($folderName in $requiredFolders) {
    $path = Join-Path $PackagePath $folderName
    if (Test-Path $path -PathType Container) {
        $itemCount = @(Get-ChildItem $path -Recurse -File | Measure-Object).Count
        Write-Host "  ✓ $folderName/ ($itemCount files)" -ForegroundColor Green
    } else {
        Write-Error "Package validation FAILED: Missing required folder: $folderName/"
        $missingFolders = $true
    }
}

if ($missingFolders) {
    exit 3
}

# 6. Summary
Write-Host "`nPackage contract validation PASSED" -ForegroundColor Green
$totalFiles = (Get-ChildItem $PackagePath -Recurse -File | Measure-Object).Count
Write-Host "Total files in package: $totalFiles" -ForegroundColor Gray

exit 0
