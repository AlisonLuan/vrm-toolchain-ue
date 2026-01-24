# Polls PR #23 checks and merges when all required checks pass
$repo = 'AlisonLuan/vrm-toolchain-ue'
$pr = 23
$log = Join-Path $PSScriptRoot 'pr23-poll.log'
Function Log($msg) { $t = Get-Date -Format 'o'; Add-Content -Path $log -Value "[$t] $msg" }
$max = 120 # ~1 hour (120 * 30s)
for ($i=0; $i -lt $max; $i++) {
    Log "Attempt $($i+1)/$max: checking PR status"
    $prJson = gh api repos/$repo/pulls/$pr --jq '{mergeable_state: .mergeable_state, mergeable: .mergeable, head_sha: .head.sha}' 2>$null
    if (-not $prJson) { Log 'Failed to get PR info'; Start-Sleep -Seconds 30; continue }
    $prObj = $prJson | ConvertFrom-Json
    $head = $prObj.head_sha
    $runsJson = gh api repos/$repo/commits/$head/check-runs --jq '.check_runs | map({name:.name,status:.status,conclusion:.conclusion})' 2>$null
    if (-not $runsJson) { Log 'No check-runs yet'; Start-Sleep -Seconds 30; continue }
    $runs = $runsJson | ConvertFrom-Json
    $pending = @()
    foreach ($r in $runs) { if ($r.status -ne 'completed' -or $r.conclusion -ne 'success') { $pending += $r } }
    if ($pending.Count -eq 0) {
        Log 'All checks successful; attempting merge now'
        gh pr merge $pr --repo $repo --merge --delete-branch >> $log 2>&1
        Log 'Merge command executed; exiting'
        exit 0
    } else {
        Log ('Pending checks: ' + ($pending | ForEach-Object { $_.name + ':' + $_.status + '/' + $_.conclusion } | Out-String))
    }
    Start-Sleep -Seconds 30
}
Log 'Timeout waiting for checks to pass'
exit 2
