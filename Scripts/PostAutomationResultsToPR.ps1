param(
    [string]$ResultsFile = "build/TestResults/VrmToolchain_Results.json",
    [string]$GitHubTokenEnvVar = "GITHUB_TOKEN"
)

$eventName = $env:GITHUB_EVENT_NAME
if ($eventName -ne 'pull_request') {
    Write-Host "Not a pull_request event ($eventName). Skipping PR comment step."
    exit 0
}

$eventPath = $env:GITHUB_EVENT_PATH
if (-not $eventPath -or -not (Test-Path $eventPath)) {
    Write-Warning "GITHUB_EVENT_PATH not set or file missing; cannot determine PR number. Skipping."
    exit 0
}

$eventJson = Get-Content $eventPath -Raw | ConvertFrom-Json
if (-not $eventJson.pull_request -or -not $eventJson.pull_request.number) {
    Write-Warning "Pull request number not found in event payload. Skipping."
    exit 0
}
$prNumber = $eventJson.pull_request.number

if (-not (Test-Path $ResultsFile)) {
    Write-Warning "Results file not found: $ResultsFile. Skipping comment."
    exit 0
}

try {
    $report = Get-Content $ResultsFile -Raw | ConvertFrom-Json -ErrorAction Stop
} catch {
    Write-Error "Failed to parse JSON results file: $_"
    exit 1
}

# Extract failing tests. Common fields: FullTestPath, State or Status, Message
$failing = @()
if ($report.Results -ne $null) {
    foreach ($r in $report.Results) {
        $state = $null
        if ($r.Passed -ne $null) { 
            if ($r.Passed -eq $true) { $state = 'Passed' } 
            else { $state = 'Fail' } 
        }
        if (-not $state) { $state = $r.State -or $r.Status -or $r.Result -or '' }
        if ($state -match 'Fail|Failed|Error') {
            $entry = [pscustomobject]@{
                Test = ($r.FullTestPath -or $r.Test -or $r.Name -or "<unknown>")
                State = $state
                Message = ($r.Message -or $r.FailureMessage -or $r.Output -or '')
            }
            $failing += $entry
        }
    }
}

# Also check summary-level Fail count
$failedCount = 0
if ($report.Summary -ne $null -and $report.Summary.Failed -ne $null) {
    $failedCount = [int]$report.Summary.Failed
} elseif ($failing.Count -gt 0) {
    $failedCount = $failing.Count
}

# Prepare run URL for linking back to the workflow run
$runUrl = $null
if ($env:GITHUB_RUN_ID) {
    $repo = $env:GITHUB_REPOSITORY
    if ($env:GITHUB_SERVER_URL) { $runUrl = "$($env:GITHUB_SERVER_URL)/$repo/actions/runs/$($env:GITHUB_RUN_ID)" }
    else { $runUrl = "https://github.com/$repo/actions/runs/$($env:GITHUB_RUN_ID)" }
}

if ($failedCount -eq 0) {
    # Green pass summary
    $md = "## ✅ All Tests Passed: VrmToolchain`n`n"
    if ($runUrl) { $md += "[View full automation run]($runUrl)`n`n" }
    $md += "Deterministic results are available at `build/TestResults/VrmToolchain_Results.json` (attached as workflow artifact).`n"
} else {
    # Build markdown table (limit number of rows to avoid huge comments)
    $maxRows = 200
    $rows = $failing | Select-Object -First $maxRows

    $md = "## Automated Test Failures: VrmToolchain`n`n"
    $md += "**$failedCount** failing tests detected. Showing up to $($rows.Count) failures.`n`n"
    $md += "| Test | State | Message |`n"
    $md += "|---|---|---|`n"
    foreach ($row in $rows) {
        $test = ($row.Test -replace '\|','\|')
        $state = ($row.State -replace '\|','\|')
        $msg = ($row.Message -replace '\r?\n',' ' )
        if ($msg.Length -gt 400) { $msg = $msg.Substring(0,400) + '...' }
        $md += "| $test | $state | $msg |`n"
    }

    if ($failing.Count -gt $rows.Count) {
        $md += "`n(Only first $($rows.Count) failures shown.)`n"
    }

    if ($runUrl) { $md += "`n[View full automation run]($runUrl)`n" }
}

# Post comment to PR
$token = $env:$GitHubTokenEnvVar
if (-not $token) {
    Write-Error "GitHub token environment variable '$GitHubTokenEnvVar' not set. Cannot post comment."
    exit 1
}

$repo = $env:GITHUB_REPOSITORY
if (-not $repo) { Write-Warning "GITHUB_REPOSITORY not set; posting may fail." }

$marker = '<!-- vrm-toolchain-automation-results -->'
$md = "$marker`n`n$md"

$commentsUri = "https://api.github.com/repos/$repo/issues/$prNumber/comments"
$body = @{ body = $md } | ConvertTo-Json -Depth 6

Write-Host "Looking for existing bot comment on PR #$prNumber to update (marker: $marker)"
try {
    $existingComments = Invoke-RestMethod -Uri $commentsUri -Headers @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github.v3+json'; 'User-Agent'='vrm-toolchain-ci' } -Method Get
} catch {
    Write-Error "Failed to list PR comments: $_"
    exit 1
}

$existing = $null
foreach ($c in $existingComments) {
    $author = $null
    if ($c.user -ne $null) { $author = $c.user.login }
    $hasMarker = $false
    if ($c.body -ne $null -and $c.body -match [regex]::Escape($marker)) { $hasMarker = $true }
    if ($hasMarker -and ($author -eq 'github-actions[bot]' -or $author -eq 'github-actions' -or $author -eq $env:GITHUB_ACTOR)) {
        $existing = $c
        break
    }
}

if ($existing -ne $null) {
    Write-Host "Found existing comment (id: $($existing.id)), updating it."
    $patchUri = "https://api.github.com/repos/$repo/issues/comments/$($existing.id)"
    try {
        Invoke-RestMethod -Uri $patchUri -Headers @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github.v3+json'; 'User-Agent'='vrm-toolchain-ci' } -Method Patch -Body $body -ContentType 'application/json'
        Write-Host "Comment updated successfully."
    } catch {
        Write-Error "Failed to update comment: $_"
        exit 1
    }
} else {
    Write-Host "No existing bot comment found; posting a new comment to PR #$prNumber."
    $postUri = $commentsUri
    try {
        Invoke-RestMethod -Uri $postUri -Headers @{ Authorization = "Bearer $token"; Accept = 'application/vnd.github.v3+json'; 'User-Agent'='vrm-toolchain-ci' } -Method Post -Body $body -ContentType 'application/json'
        Write-Host "Comment posted successfully."
    } catch {
        Write-Error "Failed to post comment: $_"
        exit 1
    }
}
