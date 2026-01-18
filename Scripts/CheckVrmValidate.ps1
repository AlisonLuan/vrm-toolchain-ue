[CmdletBinding()]
param(
  # Optional: if you want deeper checks locally, you can pass real VRM paths.
  [string]$Vrm0 = "",
  [string]$Vrm1 = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).ProviderPath
$Exe = Join-Path $RepoRoot 'Plugins\VrmToolchain\Source\ThirdParty\VrmSdk\bin\Win64\vrm_validate.exe'

if (-not (Test-Path $Exe)) {
  Write-Host "vrm_validate.exe not found. Editor validation via CLI is disabled. To enable, install the VRM SDK developer tools and set VRM_SDK_ROOT or place vrm_validate.exe under Source/ThirdParty/VrmSdk/bin/Win64 (developer only). Skipping check." -ForegroundColor Yellow
  exit 0
}

Write-Host "Found vrm_validate.exe at $Exe" -ForegroundColor Green

function Invoke-Validate {
  param(
    [Parameter(Mandatory=$true)][string[]]$Args,
    [Parameter(Mandatory=$true)][int]$ExpectedExitCode,
    [Parameter(Mandatory=$true)][string]$Label,
    [Parameter()][switch]$ExpectJson
  )

  Write-Host "`n--- $Label ---" -ForegroundColor Cyan
  Write-Host ("Args: " + ($Args -join ' '))

  # Prevent PowerShell from throwing on non-zero exit codes from the native process so we can capture rc and stdout/stderr.
  $prevEAP = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'
  $out = & $Exe @Args 2>&1
  $rc = $LASTEXITCODE
  $ErrorActionPreference = $prevEAP

  # Normalize output to an array so we can safely use .Count regardless of scalar vs array output
  $outLines = @($out)

  Write-Host "Exit code: $rc"
  if ($null -ne $out -and $outLines.Count -gt 0) {
    # Keep logs readable but still useful
    $preview = ($out | Out-String).Trim()
    if ($preview.Length -gt 2000) { $preview = $preview.Substring(0, 2000) + "`n...[truncated]..." }
    Write-Host "Stdout/Stderr (preview):`n$preview"
  }

  if ($rc -ne $ExpectedExitCode) {
    throw "${Label}: Expected exit code $ExpectedExitCode, got $rc."
  }

  if ($ExpectJson) {
    $text = ($out | Out-String).Trim()
    if ([string]::IsNullOrWhiteSpace($text)) {
      throw "${Label}: Expected JSON output, but stdout was empty."
    }

    # Tolerate tool output that prefixes human-readable stderr lines before JSON.
    # First try to locate well-known payload keys (strong heuristic) and extract JSON block from the nearest preceding '{'.
    $searchKeys = @('"files"','"schemaVersion"','"tool"')
    $startIdx = -1
    foreach ($k in $searchKeys) {
      $kidx = $text.IndexOf($k)
      if ($kidx -ge 0) {
        $braceIdx = $text.LastIndexOf('{', $kidx)
        if ($braceIdx -ge 0) { $startIdx = $braceIdx; break }
      }
    }

    if ($startIdx -lt 0) {
      # Fallback: first brace or bracket if heuristic failed
      $firstBrace = $text.IndexOf('{')
      $firstBracket = $text.IndexOf('[')
      if ($firstBrace -ge 0 -and ($firstBracket -lt 0 -or $firstBrace -lt $firstBracket)) { $startIdx = $firstBrace }
      elseif ($firstBracket -ge 0) { $startIdx = $firstBracket }
    }

    if ($startIdx -ge 0) { $jsonText = $text.Substring($startIdx) } else { $jsonText = $text }

    try {
      $json = $jsonText | ConvertFrom-Json
    } catch {
      throw "${Label}: Output was not valid JSON. Parse error: $($_.Exception.Message)"
    }

    if ($null -eq $json) {
      throw "${Label}: JSON parsed to null; expected object."
    }

    # Minimal sanity: contract harness already enforces full schema; CI just ensures JSON exists and is parseable.
    Write-Host "${Label}: JSON parse OK" -ForegroundColor Green
  }

  Write-Host "${Label}: PASS" -ForegroundColor Green
}

# 1) No files case: should be exit 3
# Prefer JSON mode if supported; if your CLI doesn't output JSON here, flip ExpectJson off.
Invoke-Validate -Args @('--json') -ExpectedExitCode 3 -Label 'D_no_files' -ExpectJson

# 2) Missing file case: should be exit 2
$missing = Join-Path $env:TEMP ("does_not_exist_" + [guid]::NewGuid().ToString("N") + ".vrm")
Invoke-Validate -Args @('--json','--strict','--fail-on-error', $missing) -ExpectedExitCode 2 -Label 'A_missing' -ExpectJson

# 3) Not a GLB case: should be exit 2
$tmpBad = Join-Path $env:TEMP ("not_a_glb_" + [guid]::NewGuid().ToString("N") + ".vrm")
Set-Content -Path $tmpBad -Value "not a glb" -Encoding Ascii
try {
  Invoke-Validate -Args @('--json','--strict','--fail-on-error', $tmpBad) -ExpectedExitCode 2 -Label 'B_not_a_glb' -ExpectJson
} finally {
  Remove-Item -Force $tmpBad -ErrorAction SilentlyContinue
}

# Optional: if you pass VRM paths (local only), run a strict OK check.
if ($Vrm0 -and $Vrm1 -and (Test-Path $Vrm0) -and (Test-Path $Vrm1)) {
  Invoke-Validate -Args @('--json','--strict','--fail-on-error', $Vrm0) -ExpectedExitCode 0 -Label 'OK_vrm0' -ExpectJson
  Invoke-Validate -Args @('--json','--strict','--fail-on-error', $Vrm1) -ExpectedExitCode 0 -Label 'OK_vrm1' -ExpectJson
} else {
  Write-Host "`nOK_vrm0/OK_vrm1 skipped (no local VRM assets provided)." -ForegroundColor Yellow
}

Write-Host "`nvrm_validate contract-smoke checks passed." -ForegroundColor Green
exit 0
