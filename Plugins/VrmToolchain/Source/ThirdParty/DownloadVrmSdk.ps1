# Download VRM SDK artifacts
# This script downloads prebuilt VRM SDK artifacts from Repo5 CI or uses a local VRM_SDK_ROOT
param(
    [string]$VrmSdkRoot = $env:VRM_SDK_ROOT,
    [string]$ArtifactUrl = "",
    [string]$Version = "latest",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

# Get the script directory
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ThirdPartyDir = Join-Path $ScriptDir ".."
$VrmSdkDir = Join-Path $ThirdPartyDir "VrmSdk"

Write-Host "VRM SDK Download Script" -ForegroundColor Cyan
Write-Host "======================" -ForegroundColor Cyan

# Check if VRM_SDK_ROOT is set
if ($VrmSdkRoot -and (Test-Path $VrmSdkRoot)) {
    Write-Host "VRM_SDK_ROOT is set to: $VrmSdkRoot" -ForegroundColor Green
    Write-Host "Using local VRM SDK installation." -ForegroundColor Green
    Write-Host "To use downloaded artifacts instead, unset VRM_SDK_ROOT or use -Force flag." -ForegroundColor Yellow
    
    if (-not $Force) {
        exit 0
    }
}

# Check if already downloaded
if ((Test-Path $VrmSdkDir) -and -not $Force) {
    Write-Host "VRM SDK already exists at: $VrmSdkDir" -ForegroundColor Green
    Write-Host "Use -Force to re-download." -ForegroundColor Yellow
    exit 0
}

# Create ThirdParty directory if it doesn't exist
if (-not (Test-Path $VrmSdkDir)) {
    Write-Host "Creating directory: $VrmSdkDir" -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $VrmSdkDir -Force | Out-Null
}

# Download artifacts
if ($ArtifactUrl) {
    Write-Host "Downloading VRM SDK from: $ArtifactUrl" -ForegroundColor Yellow
    
    $TempFile = Join-Path $env:TEMP "vrm-sdk.zip"
    
    try {
        Invoke-WebRequest -Uri $ArtifactUrl -OutFile $TempFile -UseBasicParsing
        Write-Host "Download complete." -ForegroundColor Green
        
        Write-Host "Extracting artifacts..." -ForegroundColor Yellow
        Expand-Archive -Path $TempFile -DestinationPath $VrmSdkDir -Force
        Write-Host "Extraction complete." -ForegroundColor Green
        
        Remove-Item $TempFile -Force
    }
    catch {
        Write-Host "Error downloading or extracting VRM SDK: $_" -ForegroundColor Red
        exit 1
    }
}
else {
    Write-Host "No artifact URL provided." -ForegroundColor Yellow
    Write-Host "Creating stub directory structure for VRM SDK..." -ForegroundColor Yellow
    
    # Create stub directory structure
    $IncludeDir = Join-Path $VrmSdkDir "include"
    $LibDir = Join-Path $VrmSdkDir "lib"
    $BinDir = Join-Path $VrmSdkDir "bin"
    
    New-Item -ItemType Directory -Path $IncludeDir -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $LibDir "Win64") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $LibDir "Linux") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $LibDir "Mac") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $BinDir "Win64") -Force | Out-Null
    
    # Create a README
    $ReadmePath = Join-Path $VrmSdkDir "README.txt"
    @"
VRM SDK ThirdParty Directory
============================

This directory should contain the VRM SDK artifacts.

Directory structure:
- include/          : Header files
- lib/Win64/        : Windows 64-bit libraries
- lib/Linux/        : Linux libraries  
- lib/Mac/          : macOS libraries
- bin/Win64/        : Windows 64-bit DLLs

To populate this directory:
1. Set VRM_SDK_ROOT environment variable to point to a local VRM SDK installation
2. Run this script with -ArtifactUrl parameter to download from CI
3. Manually copy VRM SDK files to this directory

For more information, see the plugin documentation.
"@ | Out-File -FilePath $ReadmePath -Encoding UTF8
    
    Write-Host "Stub directory structure created." -ForegroundColor Green
    Write-Host "Please populate the VRM SDK files or set VRM_SDK_ROOT." -ForegroundColor Yellow
}

Write-Host "VRM SDK setup complete." -ForegroundColor Green
