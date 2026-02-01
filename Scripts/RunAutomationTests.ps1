param(
    [Parameter(Mandatory=$true)]
    [string]$EditorPath,

    [Parameter(Mandatory=$true)]
    [string]$ProjectPath,

    # Explicit test filter with VrmToolchain as default for CI gates
    [string]$TestFilter = "VrmToolchain",

    # UAT/AutomationTool test selector (default preserves historical behavior)
    [string]$UATTest = "EditorTest.EditorTestNode",

    # Additional args passed through to RunUAT/AutomationTool (e.g. -runtest="VrmToolchain")
    [string]$UATExtraArgs = "",

    [string]$LogDir = "$PSScriptRoot\AutomationLogs",

    [string]$ReportOutputPath = "$PSScriptRoot\AutomationReports",

    # Timeout in seconds (30 minutes default for headless automation)
    [int]$TimeoutSeconds = 1800
)

# Ensure paths
if (-not (Test-Path $EditorPath)) { throw "Editor executable not found: $EditorPath" }
if (-not (Test-Path $ProjectPath)) { throw "Project file not found: $ProjectPath" }

New-Item -Path $LogDir -ItemType Directory -Force | Out-Null
New-Item -Path $ReportOutputPath -ItemType Directory -Force | Out-Null
$timestamp = (Get-Date).ToString('yyyyMMdd_HHmmss')
$logPath = Join-Path $LogDir "Automation_$timestamp.log"

# Build ExecCmds: target specific tests and request JSON report output
# Compose ExecCmds carefully to avoid nested quoting issues
$execCmds = 'Automation RunTests'
# Only add UATTest node if running a specific test filter (not "All")
if (($TestFilter -and $TestFilter -ne 'All') -and $UATTest -and $UATTest -ne '') { 
    $execCmds += " $UATTest" 
}
if ($UATExtraArgs -and $UATExtraArgs -ne '') { 
    $execCmds += " $UATExtraArgs" 
} elseif ($TestFilter -and $TestFilter -ne 'All') { 
    $execCmds += " $TestFilter" 
} elseif ($TestFilter -eq 'All' -or -not $TestFilter) {
    $execCmds += " All"
}
$execCmds += " -ReportOutputPath=$ReportOutputPath; Quit"
$editorArgs = "`"$ProjectPath`" -unattended -nullrhi -ExecCmds=`"$execCmds`" -LOG=`"$logPath`""

# Prefer RunUAT / AutomationTool for headless, CI-friendly automation runs
# Compose a RunUAT call that runs the tests and emits a JSON report to $ReportOutputPath
# Derive UE root robustly from the editor path by walking up until we find an 'Engine' directory
$searchDir = Split-Path $EditorPath -Parent
$ueRoot = $null
while ($searchDir -and -not $ueRoot) {
    if (Test-Path (Join-Path $searchDir 'Engine')) { $ueRoot = $searchDir; break }
    $parent = Split-Path $searchDir -Parent
    if ($parent -eq $searchDir) { break }
    $searchDir = $parent
}
if (-not $ueRoot) { Write-Warning "Failed to detect UE root from EditorPath '$EditorPath'" }
$uat = if ($ueRoot) { Join-Path $ueRoot "Engine\Build\BatchFiles\RunUAT.bat" } else { $null }
$automationDll = if ($ueRoot) { Join-Path $ueRoot "Engine\Binaries\DotNET\AutomationTool\AutomationTool.dll" } else { $null }

if (Test-Path $uat) {
    # Only use RunUAT/Gauntlet if we have a specific test filter.
    # EditorTest.EditorTestNode requires -testname, so skip RunUAT for "All" tests.
    if ($TestFilter -and $TestFilter -ne 'All') {
        # Prefer Gauntlet's RunEditorTests to run targeted tests via the EditorTest node
        $runArgs = @()
        $runArgs += 'RunEditorTests'
        $runArgs += "-Test=`"$UATTest`""
        if ($UATExtraArgs) { $runArgs += $UATExtraArgs }
        $runArgs += "-testname=`"$TestFilter`""
        $runArgs += "-project=`"$ProjectPath`""
        $runArgs += "-reportoutputpath=`"$ReportOutputPath`""
        $runArgs += "-log=`"$logPath`""

        $ExecutedCommandLine = '& "' + $uat + '" ' + (($runArgs | ForEach-Object { if ($_ -match ' ') { '"' + $_ + '"' } else { $_ } }) -join ' ')
        Write-Host "Using RunUAT: $uat $($runArgs -join ' ')"
        & $uat @runArgs
        $exitCode = $LASTEXITCODE
    } else {
        # For "All" tests, skip RunUAT and go straight to fallback editor launch
        Write-Host "Running all tests; skipping RunUAT (Gauntlet requires -testname). Using fallback editor launch."
        $exitCode = -1  # Force fallback
    }
} elseif (Test-Path $automationDll) {
    # Only use AutomationTool.dll if we have a specific test filter
    if ($TestFilter -and $TestFilter -ne 'All') {
        $dotnet = (Get-Command dotnet -ErrorAction SilentlyContinue).Path
        if (-not $dotnet) { throw "dotnet not found to run AutomationTool.dll" }
        $dtArgs = @('RunEditorTests', "-Test=`"$UATTest`"")
        if ($UATExtraArgs) { $dtArgs += $UATExtraArgs }
        $dtArgs += "-testname=`"$TestFilter`""
        $dtArgs += "-project=`"$ProjectPath`""
        $dtArgs += "-reportoutputpath=`"$ReportOutputPath`""
        $dtArgs += "-log=`"$logPath`""

        $ExecutedCommandLine = '& dotnet "' + $automationDll + '" ' + (($dtArgs | ForEach-Object { if ($_ -match ' ') { '"' + $_ + '"' } else { $_ } }) -join ' ')
        Write-Host "Invoking AutomationTool.dll via dotnet: $automationDll $($dtArgs -join ' ')"
        & $dotnet "$automationDll" @dtArgs
        $exitCode = $LASTEXITCODE
    } else {
        # For "All" tests, skip AutomationTool and go straight to fallback
        Write-Host "Running all tests; skipping AutomationTool.dll. Using fallback editor launch."
        $exitCode = -1  # Force fallback
    }
} else {
    Write-Warning "RunUAT or AutomationTool not found under UE root '$ueRoot'. Falling back to launching UnrealEditor directly."

    # Fallback: launch UnrealEditor directly with ExecCmds
    Write-Host "Running automation tests: $editorArgs"

    # Prefer headless command-line editor if available (less interactive): UnrealEditor-Cmd.exe
    $editorCmdCandidate = Join-Path (Split-Path $EditorPath -Parent) 'UnrealEditor-Cmd.exe'
    $editorExeToUse = if (Test-Path $editorCmdCandidate) { $editorCmdCandidate } else { $EditorPath }
    Write-Host "Launching: $editorExeToUse"

    $ExecutedCommandLine = '& "' + $editorExeToUse + '" ' + $editorArgs
    $proc = Start-Process -FilePath $editorExeToUse -ArgumentList $editorArgs -NoNewWindow -PassThru
    try {
        $proc | Wait-Process -Timeout (New-TimeSpan -Seconds $TimeoutSeconds) -ErrorAction Stop
    } catch {
        # Gather diagnostics to help debug headless failures
        Write-Warning "Automation run timed out after $TimeoutSeconds seconds. Attempting to stop the process."
        try { if (-not $proc.HasExited) { $proc.Kill() } } catch {}
        Write-Host "--- Diagnostics: running UnrealEditor processes ---"
        Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue | Select-Object Id,Path,StartTime | ForEach-Object { Write-Host ("Id={0} Path={1} Start={2}" -f $_.Id, $_.Path, $_.StartTime) }
        Write-Host "--- Diagnostics: running UnrealEditor-Cmd processes ---"
        Get-Process -Name UnrealEditor-Cmd -ErrorAction SilentlyContinue | Select-Object Id,Path,StartTime | ForEach-Object { Write-Host ("Id={0} Path={1} Start={2}" -f $_.Id, $_.Path, $_.StartTime) }
        $projDir = Split-Path $ProjectPath -Parent
        Write-Host "--- Diagnostics: project Saved/Logs ---"
        Get-ChildItem -Path (Join-Path $projDir 'Saved\Logs') -File -ErrorAction SilentlyContinue | Select-Object -First 10 | ForEach-Object { Write-Host $_.FullName }
        Write-Host "--- Diagnostics: engine logs ---"
        Get-ChildItem -Path (Join-Path (Get-Item $Env:LOCALAPPDATA).FullName 'UnrealEngine\AutomationTool\Logs') -File -ErrorAction SilentlyContinue | Select-Object -First 10 | ForEach-Object { Write-Host $_.FullName }

        throw "Automation run timed out or failed to complete. See logs at $logPath"
    }

    try { $exitCode = $proc.ExitCode } catch { $exitCode = $null }
}

if ($exitCode -ne $null -and $exitCode -ne 0) {
    Write-Host "Automation runner exit code: $exitCode"
    # If RunUAT/AutomationTool failed, we'll process logs/reports below for diagnostics
} else {
    # Check if we ran RunUAT/AutomationTool (and if so, it succeeded since exit code is 0 or null)
    $hadUATRun = (($uat -ne $null) -and (Test-Path $uat)) -or (($automationDll -ne $null) -and (Test-Path $automationDll))
    if ($hadUATRun -and ($exitCode -eq 0 -or $exitCode -eq $null)) {
        # RunUAT or AutomationTool succeeded (exit code 0 or undefined) - we're done
        Write-Host "Automation tests completed successfully (exit code 0)."
        Write-Host "All automation tests passed."
        exit 0
    }
    
    # If we get here, RunUAT/AutomationTool failed (non-zero exit), so fall back to launching the editor directly
    Write-Warning "RunUAT/AutomationTool invocation failed with exit code $exitCode. Falling back to launching the editor directly."

    # Fallback launch (headless) - prefer UnrealEditor-Cmd.exe
    $editorCmdCandidate = Join-Path (Split-Path $EditorPath -Parent) 'UnrealEditor-Cmd.exe'
    $editorExeToUse = if (Test-Path $editorCmdCandidate) { $editorCmdCandidate } else { $EditorPath }
    Write-Host "Launching fallback editor: $editorExeToUse with args: $editorArgs"

    $ExecutedCommandLine = '& "' + $editorExeToUse + '" ' + $editorArgs
    $proc = Start-Process -FilePath $editorExeToUse -ArgumentList $editorArgs -NoNewWindow -PassThru
    try {
        $proc | Wait-Process -Timeout (New-TimeSpan -Seconds $TimeoutSeconds) -ErrorAction Stop
    } catch {
        Write-Warning "Fallback automation run timed out after $TimeoutSeconds seconds. Attempting to stop the process."
        try { if (-not $proc.HasExited) { $proc.Kill() } } catch {}
        Write-Host "--- Diagnostics: running UnrealEditor processes ---"
        Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue | Select-Object Id,Path,StartTime | ForEach-Object { Write-Host ("Id={0} Path={1} Start={2}" -f $_.Id, $_.Path, $_.StartTime) }
        Write-Host "--- Diagnostics: running UnrealEditor-Cmd processes ---"
        Get-Process -Name UnrealEditor-Cmd -ErrorAction SilentlyContinue | Select-Object Id,Path,StartTime | ForEach-Object { Write-Host ("Id={0} Path={1} Start={2}" -f $_.Id, $_.Path, $_.StartTime) }
        $projDir = Split-Path $ProjectPath -Parent
        Write-Host "--- Diagnostics: project Saved/Logs ---"
        Get-ChildItem -Path (Join-Path $projDir 'Saved\Logs') -File -ErrorAction SilentlyContinue | Select-Object -First 10 | ForEach-Object { Write-Host $_.FullName }
        Write-Host "--- Diagnostics: engine logs ---"
        Get-ChildItem -Path (Join-Path (Get-Item $Env:LOCALAPPDATA).FullName 'UnrealEngine\AutomationTool\Logs') -File -ErrorAction SilentlyContinue | Select-Object -First 10 | ForEach-Object { Write-Host $_.FullName }

        throw "Fallback automation run timed out or failed to complete. See logs at $logPath"
    }

    try { $exitCode = $proc.ExitCode } catch { $exitCode = $null }
}

# Continue to parse logs / reports as before
if (-not (Test-Path $logPath)) {
    Write-Host "Warning: expected log file was not created: $logPath" -ForegroundColor Yellow
} else {
    Write-Host "Parsing automation log: $logPath"
    $log = Get-Content $logPath -Raw
}

# Locate latest JSON report
$reportFile = Get-ChildItem -Path $ReportOutputPath -Filter '*.json' -File -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($reportFile) {
    Write-Host "Found JSON report: $($reportFile.FullName)"
    $report = Get-Content $reportFile.FullName -Raw | ConvertFrom-Json -ErrorAction SilentlyContinue
    if ($report -ne $null) {
        # Quick validation: ensure report contains results or a summary
        $hasResults = ($report.Results -ne $null -and $report.Results.Count -gt 0)
        $hasSummary = ($report.Summary -ne $null)
        if (-not ($hasResults -or $hasSummary)) {
            Write-Error "JSON report appears empty or incomplete: $($reportFile.FullName)"
            throw "Empty or invalid JSON report"
        }

        # Common structures: report.Summary.Passed/Failed or report.Passed / report.Failed
        $passed = $null; $failed = $null
        if ($report.Summary -ne $null) {
            $passed = $report.Summary.Passed -as [int]
            $failed = $report.Summary.Failed -as [int]
        }
        if ($null -eq $passed -and $report.Passed -ne $null) {
            $passed = $report.Passed -as [int]
        }
        if ($null -eq $failed -and $report.Failed -ne $null) {
            $failed = $report.Failed -as [int]
        }
        # Fallback: inspect test list
        if (($null -eq $passed -or $null -eq $failed) -and $report.Results -ne $null) {
            $passed = ($report.Results | Where-Object { $_.Status -match 'Success|Passed' }).Count
            $failed = ($report.Results | Where-Object { $_.Status -match 'Fail|Failed|Error' }).Count
        }
        if ($null -ne $passed -and $null -ne $failed) {
            Write-Host "Parsed JSON report: Passed=$passed, Failed=$failed"
        } else {
            Write-Host "Unable to parse pass/fail counts from JSON report."
        }

        # Copy to deterministic location for CI tooling
        $deterministicRoot = Join-Path (Split-Path $ReportOutputPath -Parent) 'TestResults'
        New-Item -Path $deterministicRoot -ItemType Directory -Force | Out-Null
        $safeFilter = ($TestFilter -replace '[^A-Za-z0-9_]','_')
        $detFile = Join-Path $deterministicRoot ("${safeFilter}_Results.json")
        try {
            # Normalize JSON and write deterministic file
            $report | ConvertTo-Json -Depth 10 | Out-File -FilePath $detFile -Encoding UTF8 -Force
            Write-Host "Copied JSON report to deterministic path: $detFile"
        } catch {
            Write-Warning "Failed to write deterministic JSON report: $_"
        }

    } else {
        Write-Host "JSON report exists but failed to parse as JSON: $($reportFile.FullName)"
    }
} else {
    Write-Host "No JSON report found under $ReportOutputPath; falling back to log parsing."
}

# Backwards-compatible: regex scan of console log if we don't have counts yet
function Get-Number($text, $patterns) {
    if (-not $text) { return $null }
    foreach ($p in $patterns) {
        $m = [regex]::Match($text, $p)
        if ($m.Success -and $m.Groups.Count -ge 2) {
            return [int]$m.Groups[1].Value
        }
    }
    return $null
}

# Robust failure detection: explicitly scan logs for failure markers
function Detect-TestFailures($logText) {
    if (-not $logText) { return @() }
    
    $failurePatterns = @(
        'Automation Test \(.*\) Failed',
        'Test Failed:',
        'ERROR: Automation test.*failed',
        '\[FAILED\]',
        'FAILURE:.*Automation',
        'Assertion failed:',
        'Test\s+.*\s+failed'
    )
    
    $failures = @()
    foreach ($line in ($logText -split "\r?\n")) {
        foreach ($pattern in $failurePatterns) {
            if ($line -match $pattern) {
                $failures += $line.Trim()
                break
            }
        }
    }
    return $failures
}

# Helper: Extract up to 3 failed test names from JSON report (multiple shapes)
function Get-FailedTestNames($reportObj, $maxNames = 3) {
    if ($null -eq $reportObj) { return @() }
    
    $failedTests = @()
    try {
        # Try multiple likely report structures
        $testsList = $reportObj.Results.Tests  # Shape 1
        if ($null -eq $testsList) { $testsList = $reportObj.Tests }  # Shape 2
        if ($null -eq $testsList) { $testsList = $reportObj.Report.Tests }  # Shape 3
        if ($null -eq $testsList) { $testsList = $reportObj.Results }  # Shape 4 (Results as tests)
        
        if ($testsList -is [array]) {
            foreach ($test in $testsList) {
                # Check various status field names
                $isFailed = $false
                if ($null -ne $test.State -and $test.State -match 'fail') { $isFailed = $true }
                if ($null -ne $test.Result -and $test.Result -match 'fail') { $isFailed = $true }
                if ($null -ne $test.Status -and $test.Status -match 'fail') { $isFailed = $true }
                if ($null -ne $test.Succeeded -and $test.Succeeded -eq $false) { $isFailed = $true }
                if ($null -ne $test.bSucceeded -and $test.bSucceeded -eq $false) { $isFailed = $true }
                if ($null -ne $test.Errors -and ($test.Errors -is [array] -and $test.Errors.Count -gt 0)) { $isFailed = $true }
                
                if ($isFailed) {
                    # Prefer FullTestPath, fallback to TestName, then Name
                    $testName = $test.FullTestPath
                    if (-not $testName) { $testName = $test.TestName }
                    if (-not $testName) { $testName = $test.Name }
                    if (-not $testName) { $testName = $test.DisplayName }
                    if (-not $testName) { $testName = $test.Test }
                    
                    if ($testName) {
                        $failedTests += $testName
                        if ($failedTests.Count -ge $maxNames) { break }
                    }
                }
            }
        }
    } catch {
        # Silently fail; will use log fallback
    }
    
    return if ($failedTests -ne $null) { $failedTests } else { @() }
}

# Extract pass/fail counts from log using regex (if not already from JSON)
if ($null -eq $passed -or $null -eq $failed) {
    if ($log) {
        $passed = Get-Number $log @("Passed:\s*(\d+)", "(\d+) passed", "Passed\s*:\s*(\d+)")
        $failed = Get-Number $log @("Failed:\s*(\d+)", "(\d+) failed", "Failed\s*:\s*(\d+)")
    }
}

# Strong failure detection: explicitly scan log for test failures if not caught above
$explicitFailures = @()
if ($log) {
    $explicitFailures = Detect-TestFailures $log
}

# If still no counts found, error out
if ($null -eq $passed -and $null -eq $failed) {
    # As a final fallback, scan for summary lines
    if ($log) {
        $summaryLine = ($log -split "\r?\n") | Where-Object { $_ -match "Automation Test" -or $_ -match "Summary" } | Select-Object -First 1
        Write-Host "Could not extract pass/fail counts automatically. Found summary fragment: $summaryLine"
    }
    throw "Unable to determine automation test results from log or report. See $logPath and $ReportOutputPath"
}

$passed = $passed -as [int]
$failed = $failed -as [int]

# Extract failed test names for actionable CI output
$failedTestNames = @()
if ($report -ne $null) {
    $failedTestNames = Get-FailedTestNames $report 3
}
if ($failedTestNames.Count -eq 0 -and $explicitFailures.Count -gt 0) {
    $failedTestNames = @($explicitFailures | Select-Object -First 3 | ForEach-Object { $_.Trim() })
}

# Final validation: if explicit failures detected, ensure failed >= 1
if ($explicitFailures.Count -gt 0) {
    Write-Host "Detected explicit test failures in log:" -ForegroundColor Red
    $explicitFailures | ForEach-Object { Write-Host "  $_" -ForegroundColor Red }
    $failed = [math]::Max([int]$failed, 1)
}

# Emit comprehensive summary
Write-Host ""
Write-Host "========== Automation Test Summary =========="
Write-Host "Test Filter: $TestFilter"
Write-Host "Passed: $passed"
Write-Host "Failed: $failed"
Write-Host "Total: $($passed + $failed)"
Write-Host "=========================================="
Write-Host ""

# Emit locations for CI capture
Write-Host "::notice file=$logPath::Automation log generated"
if ($reportFile) { Write-Host "::notice file=$($reportFile.FullName)::Automation JSON report generated" }

if ($failed -gt 0) {
    # Actionable CI output: failed test names and rerun command
    if ($failedTestNames.Count -gt 0) {
        Write-Host ""
        Write-Host "========== Failing Tests (first $($failedTestNames.Count)) =========="
        $failedTestNames | ForEach-Object { Write-Host "- $_" }
        Write-Host "=========================================="
        Write-Host ""
    }
    
    # Print rerun command if available
    if ($ExecutedCommandLine) {
        Write-Host "========== Automation Test Rerun =========="
        Write-Host $ExecutedCommandLine
        Write-Host "=========================================="
        Write-Host ""
        Write-Host "::notice::Rerun locally: $ExecutedCommandLine"
    }
    
    # GitHub error annotations (with normalized paths for runner compatibility)
    $logFull = if (Test-Path $logPath) { (Resolve-Path $logPath).Path } else { $logPath }
    if ($failedTestNames.Count -gt 0) {
        $failedList = $failedTestNames -join ', '
        Write-Host "::error file=$logFull::Automation failed. Failing tests: $failedList"
    } else {
        Write-Host "::error file=$logFull::Automation failed (Failed=$failed)."
    }
    if ($reportFile) {
        $reportFull = $reportFile.FullName
        if (Test-Path $reportFull) { $reportFull = (Resolve-Path $reportFull).Path }
        Write-Host "::error file=$reportFull::See JSON report for details"
    }
    
    Write-Error "One or more automation tests failed (Failed=$failed). Failing CI step."
    exit 1
}

Write-Host "All automation tests passed (Passed=$passed, Failed=0)." -ForegroundColor Green
exit 0
