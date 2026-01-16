[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function FailValidation([string]$Message) {
    Write-Host "VerifyVrmSdkLayout ERROR: $Message" -ForegroundColor Red
    throw $Message
}

function AssertPath([string]$Path, [string]$Friendly) {
    if (-not (Test-Path $Path)) {
        FailValidation "$Friendly path '$Path' is missing."
    }
}

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).ProviderPath
$PluginFile = Join-Path $RepoRoot 'Plugins/VrmToolchain/VrmToolchain.uplugin'
AssertPath $PluginFile '.uplugin file'

$Plugin = Get-Content -Raw $PluginFile | ConvertFrom-Json
$HasRuntimeModule = $false
foreach ($Module in $Plugin.Modules) {
    if ($Module.Name -eq 'VrmToolchain') {
        $HasRuntimeModule = $true
        break
    }
}
if (-not $HasRuntimeModule) {
    FailValidation "VrmToolchain module is not registered inside '$PluginFile'."
}

$SdkRoot = Join-Path $RepoRoot 'Plugins/VrmToolchain/Source/ThirdParty/VrmSdk'
AssertPath $SdkRoot 'VRM SDK root'

$IncludeDir = Join-Path $SdkRoot 'include'
AssertPath $IncludeDir 'VRM SDK include directory'

$HeaderCount = (Get-ChildItem -Path $IncludeDir -Recurse -Filter '*.h' -File -ErrorAction SilentlyContinue).Count
if ($HeaderCount -eq 0) {
    FailValidation "No header files were found under '$IncludeDir'."
}

$DebugLibDir = Join-Path $SdkRoot 'lib/Win64/Debug'
$ReleaseLibDir = Join-Path $SdkRoot 'lib/Win64/Release'
AssertPath $DebugLibDir 'Debug library directory'
AssertPath $ReleaseLibDir 'Release library directory'

$ReleaseLibCount = (Get-ChildItem -Path $ReleaseLibDir -Filter '*.lib' -File -ErrorAction SilentlyContinue).Count
if ($ReleaseLibCount -eq 0) {
    FailValidation "No .lib files were discovered in '$ReleaseLibDir'."
}

$BinDir = Join-Path $SdkRoot 'bin/Win64'
if (Test-Path $BinDir) {
    $DllCount = (Get-ChildItem -Path $BinDir -Filter '*.dll' -File -ErrorAction SilentlyContinue).Count
    if ($DllCount -eq 0) {
        FailValidation "Expected DLLs inside '$BinDir' but none were found."
    }
}

Write-Host "VRM SDK layout and plugin registration verified successfully."
