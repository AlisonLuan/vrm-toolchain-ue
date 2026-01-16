[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function FailFetch([string]$Message) {
  Write-Host "FetchVrmSdk ERROR: $Message" -ForegroundColor Red
  throw $Message
}

function EnsureDirectory([string]$Path) {
  New-Item -ItemType Directory -Force -Path $Path | Out-Null
}

function ClearDirectoryPreserveGitkeep([string]$Path) {
  EnsureDirectory $Path
  Get-ChildItem -Path $Path -Force -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -ne ".gitkeep" } |
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
}

function CopyTreeInto([string]$Source, [string]$Destination, [string]$FriendlyName) {
  if (-not (Test-Path $Source)) {
    FailFetch "$FriendlyName not found at '$Source'."
  }
  ClearDirectoryPreserveGitkeep $Destination
  # If source and destination resolve to the same path, skip copying to avoid self-overwrite errors.
  $srcResolved = Resolve-Path $Source -ErrorAction SilentlyContinue
  $destResolved = Resolve-Path $Destination -ErrorAction SilentlyContinue
  if ($srcResolved -and $destResolved -and ($srcResolved.ProviderPath -eq $destResolved.ProviderPath)) {
    Write-Host "Skipping copy for '$FriendlyName' because source and destination are the same: $($srcResolved.ProviderPath)"
    return
  }
  Copy-Item -Path (Join-Path $Source "*") -Destination $Destination -Recurse -Force
}

function GetToken() {
  if ($env:VRM_SDK_TOKEN) { return $env:VRM_SDK_TOKEN }
  if ($env:GH_TOKEN) { return $env:GH_TOKEN }
  if ($env:GITHUB_TOKEN) { return $env:GITHUB_TOKEN }
  return $null
}

function FindIncludeRoot([string]$Root) {
  $candidates = Get-ChildItem -Path $Root -Recurse -Directory -Filter "include" -ErrorAction SilentlyContinue
  foreach ($c in $candidates) {
    $p = $c.FullName
    foreach ($name in @("vrm_glb_parser","vrm_avatar_model","vrm_validate","vrm_normalizers","vrm-normalizers")) {
      if (Test-Path (Join-Path $p $name)) { return $p }
    }
  }
  return $null
}

function FindDirEndingWith([string]$Root, [string]$Suffix) {
  $dirs = Get-ChildItem -Path $Root -Recurse -Directory -ErrorAction SilentlyContinue
  foreach ($d in $dirs) {
    if ($d.FullName.EndsWith($Suffix)) { return $d.FullName }
  }
  return $null
}

function FindLibSource([string]$Root) {
  # Preferred layouts
  $p1 = Join-Path $Root "lib\Win64\Release"
  if (Test-Path $p1) { return $p1 }

  $p2 = Join-Path $Root "lib\Release"
  if (Test-Path $p2) { return $p2 }

  # Flat layout: lib\*.lib
  $p3 = Join-Path $Root "lib"
  if (Test-Path $p3) {
    $libs = @(Get-ChildItem -Path $p3 -File -Filter *.lib -ErrorAction SilentlyContinue)
    if ($libs.Count -gt 0) { return $p3 }
  }

  # Last resort: find any directory named 'lib' that contains .lib files
  $libDirs = @(Get-ChildItem -Path $Root -Recurse -Directory -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -eq "lib" })

  foreach ($d in $libDirs) {
    $libs = @(Get-ChildItem -Path $d.FullName -File -Filter *.lib -ErrorAction SilentlyContinue)
    if ($libs.Count -gt 0) { return $d.FullName }
  }

  return $null
}

function FindBinSource([string]$Root) {
  $p1 = Join-Path $Root "bin\Win64"
  if (Test-Path $p1) { return $p1 }

  $p2 = Join-Path $Root "bin"
  if (Test-Path $p2) {
    if (Test-Path (Join-Path $p2 "vrm_validate.exe")) { return $p2 }
  }

  # Last resort: locate vrm_validate.exe anywhere and use its parent folder
  $exe = Get-ChildItem -Path $Root -Recurse -File -Filter "vrm_validate.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($exe) { return (Split-Path $exe.FullName -Parent) }

  return $null
}

function ValidateMetadataIfPresent([string]$CandidateRoot) {
  $MetadataFile = Join-Path $CandidateRoot 'metadata.json'
  if (-not (Test-Path $MetadataFile)) { return }

  $Metadata = Get-Content -Raw $MetadataFile | ConvertFrom-Json
  $Rules = @{ toolset='v143'; arch='Win64'; crt='/MD' }
  foreach ($k in $Rules.Keys) {
    if ($Metadata.PSObject.Properties.Name -contains $k) {
      $v = $Metadata.$k
      if ($v -ne $Rules[$k]) {
        FailFetch "Metadata '$k' expects '$($Rules[$k])' but found '$v'."
      }
    }
  }
}

# ---- Paths ----
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).ProviderPath
$SdkDestination = Join-Path $RepoRoot 'Plugins/VrmToolchain/Source/ThirdParty/VrmSdk'
EnsureDirectory $SdkDestination

$DestInclude = Join-Path $SdkDestination 'include'
$DestDbgLib  = Join-Path $SdkDestination 'lib/Win64/Debug'
$DestRelLib  = Join-Path $SdkDestination 'lib/Win64/Release'
$DestBin     = Join-Path $SdkDestination 'bin/Win64'

# ---- Source selection ----
$SourceRoot = $null

if ($Env:VRM_SDK_ROOT) {
  $Expanded = Resolve-Path $Env:VRM_SDK_ROOT -ErrorAction SilentlyContinue
  if (-not $Expanded) {
    FailFetch "VRM_SDK_ROOT is set to '$Env:VRM_SDK_ROOT' which does not exist. It must be a LOCAL install prefix path."
  }
  $SourceRoot = $Expanded.ProviderPath
  Write-Host "Staging VRM SDK from VRM_SDK_ROOT=$SourceRoot"
} else {
  $VersionFile = Join-Path $PSScriptRoot 'VrmSdkVersion.json'
  if (-not (Test-Path $VersionFile)) {
    FailFetch "Version file not found at '$VersionFile'."
  }
  $Version = Get-Content -Raw $VersionFile | ConvertFrom-Json
  foreach ($k in @('repoOwner','repoName','tag','assetName')) {
    if (-not $Version.$k) { FailFetch "Version file must define '$k'." }
  }

  $Token = GetToken
  if (-not $Token) {
    FailFetch "No token found. Set VRM_SDK_TOKEN (preferred) or GH_TOKEN to download private release assets."
  }

  $Owner = $Version.repoOwner
  $Repo  = $Version.repoName
  $Tag   = $Version.tag
  $AssetName = $Version.assetName

  $TempRoot = Join-Path $env:TEMP 'VrmSdkFetch'
  Remove-Item -Recurse -Force $TempRoot -ErrorAction SilentlyContinue
  EnsureDirectory $TempRoot

  $ZipPath = Join-Path $TempRoot $AssetName
  $ExtractRoot = Join-Path $TempRoot 'extracted'
  EnsureDirectory $ExtractRoot

  $HeadersJson = @{
    Authorization = "token $Token"
    "User-Agent"  = "VrmSdkFetchScript"
    Accept        = "application/vnd.github+json"
  }

  Write-Host "Fetching release metadata via GitHub API for $Owner/$Repo tag $Tag..."
  $ReleaseApi = "https://api.github.com/repos/$Owner/$Repo/releases/tags/$Tag"
  $Release = Invoke-RestMethod -Uri $ReleaseApi -Headers $HeadersJson

  $Asset = $Release.assets | Where-Object { $_.name -eq $AssetName } | Select-Object -First 1
  if (-not $Asset) {
    FailFetch "Asset '$AssetName' not found in release '$Tag'."
  }

  Write-Host "Downloading asset '$AssetName' via API..."
  $HeadersBin = @{
    Authorization = "token $Token"
    "User-Agent"  = "VrmSdkFetchScript"
    Accept        = "application/octet-stream"
  }
  Invoke-WebRequest -Uri $Asset.url -Headers $HeadersBin -OutFile $ZipPath

  Expand-Archive -Path $ZipPath -DestinationPath $ExtractRoot -Force
  $SourceRoot = $ExtractRoot
  Write-Host "Extracted VRM SDK to $ExtractRoot"
}

# ---- Locate include/lib/bin (lib/bin may be flat in single-config artifacts) ----
$IncludeRoot = FindIncludeRoot $SourceRoot
if (-not $IncludeRoot) {
  FailFetch "Unable to locate an include directory containing VRM headers inside '$SourceRoot'."
}

$RelLibRoot = FindLibSource $SourceRoot
if (-not $RelLibRoot) {
  FailFetch "No .lib files found anywhere under extracted SDK. The release asset likely does not include compiled libraries."
}

$BinRoot = FindBinSource $SourceRoot

# ---- Optional metadata validation ----
# Try validating metadata.json at parent of include (common pattern), otherwise do nothing.
$CandidateSdkRoot = Split-Path $IncludeRoot -Parent
ValidateMetadataIfPresent $CandidateSdkRoot

# ---- Stage ----
CopyTreeInto -Source $IncludeRoot -Destination $DestInclude -FriendlyName "VRM SDK include directory"
CopyTreeInto -Source $RelLibRoot -Destination $DestRelLib -FriendlyName "VRM SDK libraries (Release)"

# ---- Verify required Release libraries are present ----
$required = @("vrm_glb_parser.lib", "vrm_normalizers.lib", "vrm_validate.lib")
$missing = @()
foreach ($r in $required) {
  if (-not (Test-Path (Join-Path $DestRelLib $r))) { $missing += $r }
}
if ($missing.Count -gt 0) {
  FailFetch "Missing required libraries in staged Release folder: $($missing -join ', ')"
}

# Debug libs: if none exist, optionally mirror Release into Debug as a fallback
ClearDirectoryPreserveGitkeep $DestDbgLib
$DbgCandidates = @( (Join-Path $SourceRoot "lib\Win64\Debug"), (Join-Path $SourceRoot "lib\Debug") ) | Where-Object { Test-Path $_ }
$DbgCandidates = @($DbgCandidates)

if ($DbgCandidates.Count -gt 0) {
  CopyTreeInto -Source $DbgCandidates[0] -Destination $DestDbgLib -FriendlyName "VRM SDK libraries (Debug)"
} else {
  # Fallback: mirror Release libs into Debug to keep UE Debug/DebugGame from failing immediately
  Copy-Item -Path (Join-Path $DestRelLib "*") -Destination $DestDbgLib -Recurse -Force
  Write-Host "Note: Debug libs not present in artifact; mirrored Release libs into Debug folder as fallback."
}

if ($BinRoot) {
  CopyTreeInto -Source $BinRoot -Destination $DestBin -FriendlyName "VRM SDK binaries"
}

# ---- Include alias fix: vrm-normalizers -> vrm_normalizers (if needed) ----
$Hyphen = Join-Path $DestInclude "vrm-normalizers"
$Underscore = Join-Path $DestInclude "vrm_normalizers"
if ((Test-Path $Hyphen) -and -not (Test-Path $Underscore)) {
  Copy-Item -Recurse -Force $Hyphen $Underscore
}

Write-Host "VRM SDK staged at $SdkDestination"
