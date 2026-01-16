[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function FailPrepare([string]$Message) {
    Write-Host "PrepareShimForCi ERROR: $Message" -ForegroundColor Red
    throw $Message
}

function EnsureDirectory([string]$Path) {
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).ProviderPath
$SdkRoot = Join-Path $RepoRoot 'Plugins/VrmToolchain/Source/ThirdParty/VrmSdk'
if (-not (Test-Path $SdkRoot)) {
    FailPrepare "Staged VRM SDK directory '$SdkRoot' does not exist."
}

$IncludeRoot = Join-Path $SdkRoot 'include'
if (-not (Test-Path $IncludeRoot)) {
    FailPrepare "Include directory '$IncludeRoot' is missing."
}

$Header = Get-ChildItem -Path $IncludeRoot -Recurse -Filter '*.h' -File -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $Header) {
    FailPrepare "Unable to locate any header files under '$IncludeRoot'."
}

$RelativeHeader = $Header.FullName.Substring($IncludeRoot.Length + 1) -replace '\\', '/'
$ShimDir = Join-Path $RepoRoot 'Shim'
EnsureDirectory $ShimDir

$ShimSource = Join-Path $ShimDir 'shim.cpp'
$ShimContents = @"
#include <$RelativeHeader>

int main()
{
    return 0;
}
"@
Set-Content -Path $ShimSource -Value $ShimContents -Encoding UTF8

$ReleaseLibDir = Join-Path $SdkRoot 'lib/Win64/Release'
if (-not (Test-Path $ReleaseLibDir)) {
    FailPrepare "Release libraries path '$ReleaseLibDir' is missing."
}

$LibFiles = Get-ChildItem -Path $ReleaseLibDir -Filter '*.lib' -File -ErrorAction SilentlyContinue
if ($LibFiles.Count -eq 0) {
    FailPrepare "No .lib files were discovered inside '$ReleaseLibDir'."
}

$LibRsp = Join-Path $ShimDir 'libs.rsp'
$LibNames = $LibFiles | ForEach-Object { $_.Name }
Set-Content -Path $LibRsp -Value ($LibNames -join "`n") -Encoding UTF8

if (-not $Env:GITHUB_ENV) {
    FailPrepare 'GITHUB_ENV environment variable is required to emit shim metadata.'
}

Add-Content -Path $Env:GITHUB_ENV -Value "SHIM_INCLUDE=$IncludeRoot"
Add-Content -Path $Env:GITHUB_ENV -Value "SHIM_LIB_DIR=$ReleaseLibDir"
Add-Content -Path $Env:GITHUB_ENV -Value "SHIM_LIB_RSP=$LibRsp"

Write-Host "Prepared shim using header '$RelativeHeader' with $($LibNames.Count) libraries."
