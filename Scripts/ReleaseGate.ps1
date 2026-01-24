#requires -Version 7.0
<#
.SYNOPSIS
Deterministic release gate: package, validate, zip, manifest, and prepare GitHub release.

.DESCRIPTION
ReleaseGate enforces a repeatable release discipline:
1. Verifies clean working tree (unless -AllowDirty)
2. Runs PackagePlugin.ps1 (includes contract validation)
3. Creates deterministic zip
4. Writes SHA256 manifest to RELEASES/<tag>.sha256
5. Commits manifest to git
6. Prints ready-to-use 'gh release create' command
7. Optionally publishes the release if -Publish is specified

This ensures every release is reproducible and audit-trailed.

.PARAMETER UE
Path to Unreal Engine installation (e.g., C:\Program Files\Epic Games\UE_5.7).
Required.

.PARAMETER Tag
Git tag for this release (e.g., green-ue5.7-2026-01-24).
Used for manifest filename and release identification.
Required.

.PARAMETER PluginName
Name of the plugin to package. Default: VrmToolchain.

.PARAMETER Title
GitHub release title. If omitted, defaults to "UE 5.7 Green Build (<tag>)".

.PARAMETER Notes
GitHub release notes/description. If omitted, auto-generates standard format.

.PARAMETER Publish
If specified, automatically publishes the GitHub release (runs 'gh release create').
Without this flag, only prints the command for manual review.

.PARAMETER AllowDirty
If specified, allows the script to run even with uncommitted changes.
Not recommended for production releases.

.EXAMPLE
# Generate package, zip, manifest, and print release command (review before publishing)
.\Scripts\ReleaseGate.ps1 -UE "C:\Program Files\Epic Games\UE_5.7" -Tag "green-ue5.7-2026-01-24"

.EXAMPLE
# Same, but also publish the GitHub release automatically
.\Scripts\ReleaseGate.ps1 -UE "C:\Program Files\Epic Games\UE_5.7" -Tag "green-ue5.7-2026-01-24" -Publish

.EXAMPLE
# Override title and notes
.\Scripts\ReleaseGate.ps1 -UE "C:\Program Files\Epic Games\UE_5.7" -Tag "green-ue5.7-2026-01-24" `
  -Title "Custom Release Title" `
  -Notes "Custom release notes here"

.NOTES
Requires:
- Git (git command available on PATH)
- PowerShell 7.0+ (for Compress-Archive)
- GitHub CLI (if -Publish is used)

The script always performs a clean build to ensure determinism. Any build artifacts
are removed before packaging.
#>

[CmdletBinding()]
param(
  [Parameter(Mandatory=$true, HelpMessage="Path to UE installation")]
  [string]$UE,

  [Parameter(Mandatory=$false, HelpMessage="Plugin name to package")]
  [string]$PluginName = "VrmToolchain",

  [Parameter(Mandatory=$true, HelpMessage="Git tag for this release")]
  [string]$Tag,

  [Parameter(Mandatory=$false, HelpMessage="GitHub release title")]
  [string]$Title = "",

  [Parameter(Mandatory=$false, HelpMessage="GitHub release notes")]
  [string]$Notes = "",

  [Parameter(Mandatory=$false, HelpMessage="Publish GitHub release automatically")]
  [switch]$Publish,

  [Parameter(Mandatory=$false, HelpMessage="Allow dirty working tree")]
  [switch]$AllowDirty
)

$ErrorActionPreference = "Stop"

function Fail([string]$Message) {
  Write-Error $Message
  exit 1
}

function Ensure-Tool([string]$Cmd, [string]$Hint) {
  if (-not (Get-Command $Cmd -ErrorAction SilentlyContinue)) {
    Fail "Missing required tool '$Cmd'. $Hint"
  }
}

function Get-RepoRoot {
  $root = (git rev-parse --show-toplevel 2>$null)
  if (-not $root) { Fail "Not in a git repository." }
  return $root.Trim()
}

# Validate dependencies
Ensure-Tool git "Install Git and ensure it is on PATH."
Ensure-Tool Compress-Archive "Use PowerShell 7+."
if ($Publish) { Ensure-Tool gh "Install GitHub CLI and run 'gh auth login'." }

$RepoRoot = Get-RepoRoot
Set-Location $RepoRoot

if (-not (Test-Path $UE)) { Fail "UE path not found: $UE" }

# Guard: clean working tree (unless explicitly allowed)
if (-not $AllowDirty) {
  $st = (git status --porcelain)
  if ($st) {
    Write-Host "Working tree status:"
    Write-Host $st
    Fail "Working tree is not clean. Commit/stash changes or pass -AllowDirty."
  }
}

# Hard clean build outputs to ensure determinism
Write-Host "=== Cleaning build outputs for determinism ==="
Remove-Item -Recurse -Force (Join-Path $RepoRoot "build") -ErrorAction SilentlyContinue | Out-Null
Write-Host "Cleaned."

Write-Host ""
Write-Host "=== Running deterministic packaging ==="
& (Join-Path $RepoRoot "Scripts\PackagePlugin.ps1") -UE $UE -RepoRoot $RepoRoot -PluginName $PluginName
if ($LASTEXITCODE -ne 0) { Fail "PackagePlugin.ps1 failed." }

$Dist = Join-Path $RepoRoot "build\Package\$PluginName"
if (-not (Test-Path $Dist)) { Fail "Dist package not found: $Dist" }

$Zip = Join-Path $RepoRoot "build\$PluginName-UE5.7.zip"
Remove-Item $Zip -Force -ErrorAction SilentlyContinue | Out-Null

Write-Host ""
Write-Host "=== Creating deterministic zip ==="
Compress-Archive -Path (Join-Path $Dist '*') -DestinationPath $Zip -CompressionLevel Optimal
Write-Host "Created: $Zip"

$Hash = (Get-FileHash $Zip -Algorithm SHA256).Hash
Write-Host "SHA256:  $Hash"

# Write manifest
$RelDir = Join-Path $RepoRoot "RELEASES"
New-Item -ItemType Directory -Force -Path $RelDir | Out-Null

$Manifest = Join-Path $RelDir "$Tag.sha256"
"$Hash  $(Split-Path $Zip -Leaf)" | Set-Content -Encoding ASCII $Manifest
Write-Host "Manifest: $Manifest"

# Commit manifest
Write-Host ""
Write-Host "=== Committing manifest ==="
git add $Manifest | Out-Null
$commitMsg = "docs: add SHA256 manifest for $Tag"
git commit -m $commitMsg | Out-Null
Write-Host "Committed: $commitMsg"

Write-Host ""
Write-Host "=== Pushing main ==="
git push | Out-Null
Write-Host "Pushed."

# Prepare release metadata
if (-not $Title) { $Title = "UE 5.7 Green Build ($Tag)" }
if (-not $Notes) {
  $Notes = @"
Tier B CI green (build + tests + deterministic packaging + contract validation).

Binary asset: $(Split-Path $Zip -Leaf)
SHA256: $Hash
"@ -replace "`r`n", "`n"
}

# Print release command
$cmd = @(
  "gh release create `"$Tag`" `"$Zip`"",
  "--title `"$Title`"",
  "--notes `"$Notes`""
) -join " "

Write-Host ""
Write-Host "================================"
Write-Host "GitHub Release command:"
Write-Host "================================"
Write-Host $cmd
Write-Host ""

if ($Publish) {
  Write-Host "=== Publishing GitHub Release ==="
  & gh release create $Tag $Zip --title $Title --notes $Notes
  if ($LASTEXITCODE -ne 0) { Fail "gh release create failed." }
  Write-Host "Published: https://github.com/AlisonLuan/vrm-toolchain-ue/releases/tag/$Tag"
} else {
  Write-Host "Release command is printed above."
  Write-Host "Review it, then run:"
  Write-Host "  $cmd"
  Write-Host "Or re-run this script with -Publish to publish automatically."
}

Write-Host ""
Write-Host "✓ Release gate complete."
