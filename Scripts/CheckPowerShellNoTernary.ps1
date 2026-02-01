# Scripts/CheckPowerShellNoTernary.ps1
# Fails if any Scripts/*.ps1 contains a '? ... : ...' ternary-like pattern.
# Designed for Windows PowerShell 5.x compatibility.

[CmdletBinding()]
param(
    [string]$Root = ""
)

$ErrorActionPreference = "Stop"

# Canonical fallback for $PSScriptRoot (works in all invocation contexts)
if (-not $Root) {
    $ScriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Path }
    $RepoRoot  = Resolve-Path (Join-Path $ScriptDir "..") | Select-Object -ExpandProperty Path
    $Root      = Join-Path $RepoRoot "Scripts"
}

function Strip-StringsAndComments([string]$Text) {
    # Remove single-quoted strings, double-quoted strings, and line comments.
    # This is heuristic (not a full PS parser), but works well for the regression we saw.
    $t = $Text
    $t = [regex]::Replace($t, "'(?:''|[^'])*'", "''")          # single-quoted strings
    $t = [regex]::Replace($t, '"(?:`"|[^"])*"', '""')         # double-quoted strings (handles `")
    $t = [regex]::Replace($t, "(?m)#.*$", "")                 # line comments
    return $t
}

function Test-TernaryPattern {
    param([object[]]$CodeTokens)

    # Walk tokens looking for '?' operator followed by ':' operator
    # Exclude: $? (success var), ? command alias (Where-Object)
    for ($i = 0; $i -lt $CodeTokens.Count; $i++) {
        $t = $CodeTokens[$i]

        if ($t.Text -eq '?') {
            # Skip if '?' is preceded by '$' (covers $? success variable)
            $prev = if ($i -gt 0) { $CodeTokens[$i-1].Text } else { '' }
            if ($prev -eq '$') { continue }

            # Skip if '?' is a command (alias for Where-Object)
            if ($t.Kind -eq 'Command') { continue }

            # Look ahead within a small window for ':' operator (ternary shape)
            for ($j = $i + 1; $j -lt [Math]::Min($i + 50, $CodeTokens.Count); $j++) {
                if ($CodeTokens[$j].Text -eq ':') {
                    # Found suspicious pattern; return details
                    return @{
                        Found         = $true
                        QuestionToken = $t
                        ColonToken    = $CodeTokens[$j]
                    }
                }
                # Stop if we hit something that breaks the ternary shape
                # (like a statement terminator or control keyword)
                if ($CodeTokens[$j].Kind -in @('Pipe', 'Semicolon')) { break }
            }
        }
    }

    return @{ Found = $false }
}

$scriptDir = (Resolve-Path $Root).Path
$files = Get-ChildItem -LiteralPath $scriptDir -Filter "*.ps1" -File -Recurse

$offenders = @()

foreach ($f in $files) {
    $raw = Get-Content -LiteralPath $f.FullName -Raw

    # Use PowerShell tokenizer to extract code tokens (exclude strings/comments)
    $tokens = $null
    $errors = $null
    [System.Management.Automation.Language.Parser]::ParseInput($raw, [ref]$tokens, [ref]$errors) | Out-Null

    # Keep only code tokens; exclude StringLiteral, Comment, etc.
    $codeTokens = $tokens | Where-Object {
        $_.Kind -notin @(
            'StringLiteral',
            'StringExpandable',
            'HereStringLiteral',
            'HereStringExpandable',
            'Comment'
        )
    }

    # Test for ternary pattern
    $result = Test-TernaryPattern $codeTokens

    if ($result.Found) {
        # Extract line numbers from token positions
        $qLine = $result.QuestionToken.Extent.StartLineNumber
        $cLine = $result.ColonToken.Extent.StartLineNumber

        # Report the line(s) with the suspicious pattern
        $lines = @($qLine, $cLine) | Select-Object -Unique | Sort-Object

        # Emit GitHub annotation(s) for each violation line
        $relPath = $f.FullName
        if ($relPath.StartsWith($RepoRoot, [StringComparison]::OrdinalIgnoreCase)) {
            $relPath = $relPath.Substring($RepoRoot.Length).TrimStart('\','/')
        }
        $relPath = $relPath -replace '\\','/'

        foreach ($ln in $lines) {
            $msg = "PowerShell ternary '? :' is forbidden (PS 5.x incompatible). Use: if (cond) { a } else { b }"
            Write-Host "::error file=$relPath,line=$ln::$msg"
        }

        $offenders += [pscustomobject]@{
            File  = $f.FullName
            Lines = ($lines -join ",")
            Context = "Found '?' token followed by ':' token"
        }
    }
}

if ($offenders.Count -gt 0) {
    Write-Host ""
    Write-Host "PowerShell ternary-style syntax detected in Scripts/*.ps1" -ForegroundColor Red
    $offenders | ForEach-Object {
        Write-Host ("- {0} (lines: {1})" -f $_.File, $_.Lines) -ForegroundColor Red
    }
    Write-Host ""
    Write-Host "Use PowerShell 5.x-safe pattern instead:" -ForegroundColor Yellow
    Write-Host "  return if (cond) { value } else { default }" -ForegroundColor Yellow
    exit 1
}

Write-Host "OK: No ternary-style '? :' patterns found in Scripts/*.ps1" -ForegroundColor Green
exit 0
