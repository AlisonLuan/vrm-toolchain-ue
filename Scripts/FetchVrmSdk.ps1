[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function FailFetch([string]$Message) {
    Write-Host "FetchVrmSdk ERROR: $Message" -ForegroundColor Red
    throw $Message
}

function EnsureDirectory([string]$Path) {
    if (-not (Test-Path $Path)) {
        New-Item -ItemType Directory -Path $Path | Out-Null
    }
}

function StageTree([string]$Source, [string]$Destination, [string]$FriendlyName) {
    if (-not (Test-Path $Source)) {
        FailFetch "$FriendlyName not found at '$Source'."
    }

    if (Test-Path $Destination) {
        Remove-Item -Recurse -Force $Destination
    }

    EnsureDirectory (Split-Path $Destination -Parent)
    Copy-Item -Path $Source -Destination $Destination -Recurse -Force
}

function ResolveExtractedRoot([string]$ExtractRoot) {
    if (Test-Path (Join-Path $ExtractRoot 'include')) {
        return $ExtractRoot
    }

    foreach ($Child in Get-ChildItem -Path $ExtractRoot -Directory -ErrorAction SilentlyContinue) {
        if (Test-Path (Join-Path $Child.FullName 'include')) {
            return $Child.FullName
        }
    }

    FailFetch "Unable to locate VRM SDK include directory inside '$ExtractRoot'."
}

function GetMetadataValue($Metadata, [string]$Key) {
    if ($null -eq $Metadata) {
        return $null
    }

    foreach ($Property in $Metadata.PSObject.Properties) {
        if ($Property.Name.Equals($Key, 'OrdinalIgnoreCase')) {
            return $Property.Value
        }
    }

    return $null
}

function ValidateMetadata([string]$SdkRoot) {
    $MetadataFile = Join-Path $SdkRoot 'metadata.json'
    if (-not (Test-Path $MetadataFile)) {
        return
    }

    $Metadata = Get-Content -Raw $MetadataFile | ConvertFrom-Json
    $Rules = @{
        toolset = 'v143'
        arch    = 'Win64'
        crt     = '/MD'
    }

    $Violations = New-Object System.Collections.Generic.List[string]

    foreach ($Rule in $Rules.GetEnumerator()) {
        $Value = GetMetadataValue -Metadata $Metadata -Key $Rule.Key
        if ($null -ne $Value -and $Value -ne $Rule.Value) {
            $Violations.Add("Metadata '$($Rule.Key)' expects '$($Rule.Value)' but found '$Value'.")
        }
    }

    if ($Violations.Count -gt 0) {
        FailFetch ($Violations -join ' ')
    }
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).ProviderPath
$SdkDestination = Join-Path $RepoRoot 'Plugins/VrmToolchain/Source/ThirdParty/VrmSdk'
EnsureDirectory $SdkDestination

$Destination = @{
    include   = Join-Path $SdkDestination 'include'
    debugLibs = Join-Path $SdkDestination 'lib/Win64/Debug'
    releaseLibs = Join-Path $SdkDestination 'lib/Win64/Release'
    bin       = Join-Path $SdkDestination 'bin/Win64'
}

$SourceRoot = $null
if ($Env:VRM_SDK_ROOT) {
    $Expanded = Resolve-Path $Env:VRM_SDK_ROOT -ErrorAction SilentlyContinue
    if (-not $Expanded) {
        FailFetch "Environment variable VRM_SDK_ROOT is set to '$Env:VRM_SDK_ROOT' which does not exist."
    }

    $SourceRoot = $Expanded.ProviderPath
    Write-Host "Staging VRM SDK from VRM_SDK_ROOT=$SourceRoot"
} else {
    $VersionFile = Join-Path $PSScriptRoot 'VrmSdkVersion.json'
    if (-not (Test-Path $VersionFile)) {
        FailFetch "Version file not found at '$VersionFile'."
    }

    $Version = Get-Content -Raw $VersionFile | ConvertFrom-Json
    if (-not $Version.artifactUrl -or -not $Version.artifactName) {
        FailFetch "Version file must define both 'artifactUrl' and 'artifactName'."
    }

    $TempRoot = Join-Path $env:TEMP 'VrmSdkFetch'
    if (Test-Path $TempRoot) {
        Remove-Item $TempRoot -Recurse -Force
    }

    New-Item -ItemType Directory -Path $TempRoot | Out-Null
    $DownloadPath = Join-Path $TempRoot $Version.artifactName

    Write-Host "Downloading VRM SDK from $($Version.artifactUrl)"
    Invoke-WebRequest -Uri $Version.artifactUrl -OutFile $DownloadPath

    $ExtractRoot = Join-Path $TempRoot 'extracted'
    Expand-Archive -Path $DownloadPath -DestinationPath $ExtractRoot -Force

    $SourceRoot = ResolveExtractedRoot -ExtractRoot $ExtractRoot
    Write-Host "Extracted VRM SDK to $SourceRoot"
}

ValidateMetadata -SdkRoot $SourceRoot

StageTree -Source (Join-Path $SourceRoot 'include') -Destination $Destination.include -FriendlyName 'VRM SDK include directory'
StageTree -Source (Join-Path $SourceRoot 'lib/Win64/Debug') -Destination $Destination.debugLibs -FriendlyName 'VRM SDK Win64 Debug libraries'
StageTree -Source (Join-Path $SourceRoot 'lib/Win64/Release') -Destination $Destination.releaseLibs -FriendlyName 'VRM SDK Win64 Release libraries'

$BinSource = Join-Path $SourceRoot 'bin/Win64'
if (Test-Path $BinSource) {
    StageTree -Source $BinSource -Destination $Destination.bin -FriendlyName 'VRM SDK Win64 binaries'
}

Write-Host "VRM SDK staged at $SdkDestination"
