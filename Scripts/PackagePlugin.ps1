param(
  [string]$UE = "C:\Program Files\Epic Games\UE_5.7",
  [string]$RepoRoot = "C:\Users\Aliso\Documents\GIT\vrm-toolchain-ue",
  [string]$PluginName = "VrmToolchain"
)

$ErrorActionPreference = "Stop"

$UAT      = Join-Path $UE "Engine\Build\BatchFiles\RunUAT.bat"
$Uplugin  = Join-Path $RepoRoot "Plugins\$PluginName\$PluginName.uplugin"
$RawPkg   = Join-Path $RepoRoot "build\PackageRaw\$PluginName"
$OutPkg   = Join-Path $RepoRoot "build\Package\$PluginName"

if (-not (Test-Path $UAT))     { throw "RunUAT not found: $UAT" }
if (-not (Test-Path $Uplugin)) { throw ".uplugin not found: $Uplugin" }

# Clean raw and dist outputs
Remove-Item -Recurse -Force $RawPkg -ErrorAction SilentlyContinue | Out-Null
Remove-Item -Recurse -Force $OutPkg -ErrorAction SilentlyContinue | Out-Null

# 1) Build plugin to raw staging directory
Write-Host "Building plugin package to raw staging..." -ForegroundColor Cyan
& $UAT BuildPlugin -Plugin="$Uplugin" -Package="$RawPkg" -TargetPlatforms=Win64 -Rocket
if ($LASTEXITCODE -ne 0) { throw "BuildPlugin failed (exit $LASTEXITCODE)" }

# 2) Copy allowed folders from raw → dist using robocopy
Write-Host "Filtering plugin output to clean dist..." -ForegroundColor Cyan
New-Item -ItemType Directory -Path $OutPkg -Force | Out-Null

# Allowed folders to copy (everything else in Intermediate, Saved, HostProject, etc. is trash)
$allowedFolders = @("Binaries", "Resources", "Source")

foreach ($folder in $allowedFolders) {
    $src = Join-Path $RawPkg $folder
    $dst = Join-Path $OutPkg $folder
    
    if (Test-Path $src) {
        Write-Host "  Copying $folder..." -ForegroundColor Gray
        robocopy "$src" "$dst" /E /NP /NFL /NDL | Out-Null
        if ($LASTEXITCODE -gt 7) {
            throw "robocopy failed for $folder (exit $LASTEXITCODE)"
        }
    }
}

# Copy uplugin manifest (required)
$upluginFile = Join-Path $RawPkg "$PluginName.uplugin"
if (Test-Path $upluginFile) {
    Write-Host "  Copying $PluginName.uplugin..." -ForegroundColor Gray
    Copy-Item $upluginFile -Destination (Join-Path $OutPkg "$PluginName.uplugin") -Force
}

# Copy optional README if exists
$readmeFile = Join-Path $RawPkg "README.md"
if (Test-Path $readmeFile) {
    Write-Host "  Copying README.md..." -ForegroundColor Gray
    Copy-Item $readmeFile -Destination (Join-Path $OutPkg "README.md") -Force
}

# 3) Strip forbidden binaries from dist folder
Write-Host "Stripping forbidden binaries from dist folder..." -ForegroundColor Cyan

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
    $exeCount = if ($exeFiles.Count) { $exeFiles.Count } else { 0 }
    $pdbCount = if ($pdbFiles.Count) { $pdbFiles.Count } else { 0 }
    $totalRemoved = $exeCount + $pdbCount
    Write-Host "  Removed: $totalRemoved file(s)" -ForegroundColor Green
}

# 4) Cleanup raw staging (optional, can keep for inspection)
Write-Host "Cleaning up raw staging folder..." -ForegroundColor Cyan
Remove-Item -Recurse -Force $RawPkg -ErrorAction SilentlyContinue | Out-Null
Write-Host "✓ Raw staging cleaned" -ForegroundColor Green

# 5) Validate final dist folder using the contract validation script
Write-Host "Running package contract validation..." -ForegroundColor Cyan
$ValidateScript = Join-Path (Split-Path $PSScriptRoot) "Scripts\ValidatePackage.ps1"

# Run validator in a separate pwsh process to ensure exit codes are captured reliably
$pwshCmd = (Get-Command pwsh -ErrorAction SilentlyContinue)
$valExit = $null
try {
    $outLog = Join-Path $env:TEMP ("validate_$([Guid]::NewGuid().ToString()).out")
    $errLog = Join-Path $env:TEMP ("validate_$([Guid]::NewGuid().ToString()).err")
    if ($pwshCmd) {
        $procArgs = @('-NoProfile', '-NonInteractive', '-File', "`"$ValidateScript`"", '-PackagePath', "`"$OutPkg`"")
        $proc = Start-Process -FilePath $pwshCmd.Path -ArgumentList $procArgs -RedirectStandardOutput $outLog -RedirectStandardError $errLog -NoNewWindow -Wait -PassThru
        $valExit = $proc.ExitCode
    } else {
        # Fall back to Windows PowerShell if pwsh is not available
        $pw = (Get-Command powershell -ErrorAction SilentlyContinue)
        if (-not $pw) { throw "No PowerShell interpreter available to run validator" }
        $procArgs = @('-NoProfile', '-NonInteractive', '-File', "`"$ValidateScript`"", '-PackagePath', "`"$OutPkg`"")
        $proc = Start-Process -FilePath $pw.Path -ArgumentList $procArgs -RedirectStandardOutput $outLog -RedirectStandardError $errLog -NoNewWindow -Wait -PassThru
        $valExit = $proc.ExitCode
    }
    # If proc.ExitCode is not set (strange edge), attempt to parse logs for failures
    if (-not ($valExit -is [int])) {
        $output = ""
        if (Test-Path $outLog) { $output += Get-Content $outLog -Raw -ErrorAction SilentlyContinue }
        if (Test-Path $errLog) { $output += "`n" + (Get-Content $errLog -Raw -ErrorAction SilentlyContinue) }
        if ($output -match 'Package validation FAILED') {
            Write-Error "Validator log indicates failure. See logs: $outLog, $errLog"
            $valExit = 1
        } else {
            Write-Warning "Validator did not return numeric exit code but no failure text detected. Proceeding with caution. See logs: $outLog, $errLog"
            $valExit = 0
        }
    }
} catch {
    Write-Error "Failed to run validator subprocess: $_"
    $valExit = 1
}

if (-not ($valExit -is [int])) {
    Write-Error "Package contract validator did not return a numeric exit code. See validator output above."
    $valExit = 1
    throw "Package contract validation failed: invalid exit code"
}

if ($valExit -ne 0) {
    throw "Package contract validation failed: exit code $valExit"
}

Write-Host "`n✓ Package output: $OutPkg" -ForegroundColor Green