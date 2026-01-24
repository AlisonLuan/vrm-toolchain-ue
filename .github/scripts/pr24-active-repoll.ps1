# Active re-run and poller for PR #24
$repo = 'AlisonLuan/vrm-toolchain-ue'
$pr = 24
$log = Join-Path $PSScriptRoot 'pr24-active-repoll.log'
Function Log($msg) { $t = Get-Date -Format 'o'; Add-Content -Path $log -Value "[$t] $msg" }
Log 'Starting active re-run and poller'
$max = 240 # ~2 hours
for ($i=0; $i -lt $max; $i++) {
    try {
        $prJson = gh api repos/$repo/pulls/$pr --jq '{head_sha: .head.sha, mergeable_state: .mergeable_state, mergeable: .mergeable, merged: .merged}' -q 2>$null
    } catch {
        Log "Failed to fetch PR info: $_"
        Start-Sleep -Seconds 30
        continue
    }
    $o = $prJson | ConvertFrom-Json
    $head = $o.head_sha
    Log "Attempt $($i+1): head=$head, mergeable=$($o.mergeable), state=$($o.mergeable_state), merged=$($o.merged)"

    # Get check runs
    $runsJson = gh api repos/$repo/commits/$head/check-runs --jq '.check_runs | map({id:.id,name:.name,status:.status,conclusion:.conclusion,details_url:.details_url})' 2>$null
    if (-not $runsJson) { Log 'No check-runs yet'; Start-Sleep -Seconds 30; continue }
    $runs = $runsJson | ConvertFrom-Json

    # Find build-plugin run
    $buildRun = $runs | Where-Object { $_.name -eq 'build-plugin' } | Select-Object -First 1
    $pending = @($runs | Where-Object { $_.status -ne 'completed' -or $_.conclusion -ne 'success' -and $_.status -ne 'completed' })
    $failed = @($runs | Where-Object { $_.status -eq 'completed' -and $_.conclusion -eq 'failure' })

    if ($failed.Count -gt 0) {
        Log "Checks failed: $($failed | ForEach-Object { $_.name + ':' + $_.conclusion } | Out-String)"
        # Try to rerun the failed build-plugin job if it exists
        if ($buildRun -and $buildRun.conclusion -eq 'failure') {
            Log "Attempting to rerun failed build-plugin run id=$($buildRun.id)"
            gh run rerun $buildRun.id --repo $repo 2>&1 | ForEach-Object { Log $_ }
            Start-Sleep -Seconds 10
            continue
        }
        # If other checks failed, exit and let user decide
        Log 'Non-build failure detected, exiting poll for manual inspection.'
        break
    }

    if ($pending.Count -eq 0) {
        Log 'All checks successful; attempting merge now'
        $mergeOut = gh pr merge $pr --repo $repo --merge --delete-branch 2>&1
        Log $mergeOut
        if ($LASTEXITCODE -eq 0) { Log 'Merge succeeded'; break } else { Log 'Merge failed, attempting update-branch and retry'; gh api repos/$repo/pulls/$pr/update-branch -X PUT 2>&1 | ForEach-Object { Log $_ }; Start-Sleep -Seconds 5 }
    } else {
        Log ('Pending checks: ' + ($pending | ForEach-Object { $_.name + ':' + $_.status + '/' + $_.conclusion } | Out-String))
        Start-Sleep -Seconds 30
    }
}
Log 'Active re-run poll ended.'
exit 0
