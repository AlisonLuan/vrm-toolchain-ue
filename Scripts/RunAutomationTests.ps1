param(
    [Parameter(Mandatory=$true)]
    [string]$EditorPath,

    [Parameter(Mandatory=$true)]
    [string]$ProjectPath,

    [string]$TestFilter = "All",

    # UAT/AutomationTool test selector (default preserves historical behavior)
    [string]$UATTest = "EditorTest.EditorTestNode",

    # Additional args passed through to RunUAT/AutomationTool (e.g. -runtest="VrmToolchain")
    [string]$UATExtraArgs = "",

    [string]$LogDir = "$PSScriptRoot\AutomationLogs",

    [string]$ReportOutputPath = "$PSScriptRoot\AutomationReports",

    [int]$TimeoutSeconds = 600
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
if ($UATTest -and $UATTest -ne '') { $execCmds += " $UATTest" }
if ($UATExtraArgs -and $UATExtraArgs -ne '') { $execCmds += " $UATExtraArgs" } elseif ($TestFilter -and $TestFilter -ne 'All') { $execCmds += " $TestFilter" }
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
    # Prefer Gauntlet's RunEditorTests to run in-editor Automation tests via the EditorTest node
    $runArgs = @()
    $runArgs += 'RunEditorTests'
    $runArgs += "-Test=`"$UATTest`""
    if ($UATExtraArgs) { $runArgs += $UATExtraArgs }
    if ($TestFilter -and $TestFilter -ne 'All') { $runArgs += "-testname=`"$TestFilter`"" }
    $runArgs += "-project=`"$ProjectPath`""
    $runArgs += "-reportoutputpath=`"$ReportOutputPath`""
    $runArgs += "-log=`"$logPath`""

    Write-Host "Using RunUAT: $uat $($runArgs -join ' ')"
    & $uat @runArgs
    $exitCode = $LASTEXITCODE
} elseif (Test-Path $automationDll) {
    # If RunUAT.bat isn't available, invoke AutomationTool.dll directly via dotnet with RunEditorTests
    $dotnet = (Get-Command dotnet -ErrorAction SilentlyContinue).Path
    if (-not $dotnet) { throw "dotnet not found to run AutomationTool.dll" }
    $dtArgs = @('RunEditorTests', "-Test=`"$UATTest`"")
    if ($UATExtraArgs) { $dtArgs += $UATExtraArgs }
    if ($TestFilter -and $TestFilter -ne 'All') { $dtArgs += "-testname=`"$TestFilter`"" }
    $dtArgs += "-project=`"$ProjectPath`""
    $dtArgs += "-reportoutputpath=`"$ReportOutputPath`""
    $dtArgs += "-log=`"$logPath`""

    Write-Host "Invoking AutomationTool.dll via dotnet: $automationDll $($dtArgs -join ' ')"
    & $dotnet "$automationDll" @dtArgs
    $exitCode = $LASTEXITCODE
} else {
    Write-Warning "RunUAT or AutomationTool not found under UE root '$ueRoot'. Falling back to launching UnrealEditor directly."

    # Fallback: launch UnrealEditor directly with ExecCmds
    Write-Host "Running automation tests: $editorArgs"

    # Prefer headless command-line editor if available (less interactive): UnrealEditor-Cmd.exe
    $editorCmdCandidate = Join-Path (Split-Path $EditorPath -Parent) 'UnrealEditor-Cmd.exe'
    $editorExeToUse = if (Test-Path $editorCmdCandidate) { $editorCmdCandidate } else { $EditorPath }
    Write-Host "Launching: $editorExeToUse"

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
}

# If we invoked RunUAT/AutomationTool but it failed to produce a usable report, fall back to launching the editor directly
$hadUATRun = (($uat -ne $null) -and (Test-Path $uat)) -or (($automationDll -ne $null) -and (Test-Path $automationDll))
$foundReports = (Get-ChildItem -Path $ReportOutputPath -Filter '*.json' -File -Recurse -ErrorAction SilentlyContinue)
if ($hadUATRun -and (($exitCode -ne $null -and $exitCode -ne 0) -or (-not $foundReports))) {
    Write-Warning "RunUAT/AutomationTool invocation failed or produced no reports. Falling back to launching the editor directly."

    # Fallback launch (headless) - prefer UnrealEditor-Cmd.exe
    $editorCmdCandidate = Join-Path (Split-Path $EditorPath -Parent) 'UnrealEditor-Cmd.exe'
    $editorExeToUse = if (Test-Path $editorCmdCandidate) { $editorCmdCandidate } else { $EditorPath }
    Write-Host "Launching fallback editor: $editorExeToUse with args: $editorArgs"

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
}


Write-Host "Parsing automation log: $logPath"
$log = Get-Content $logPath -Raw

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
        if (-not $passed -and $report.Passed -ne $null) {
            $passed = $report.Passed -as [int]
        }
        if (-not $failed -and $report.Failed -ne $null) {
            $failed = $report.Failed -as [int]
        }
        # Fallback: inspect test list
        if (($passed -eq $null -or $failed -eq $null) -and $report.Results -ne $null) {
            $passed = ($report.Results | Where-Object { $_.Status -match 'Success|Passed' }).Count
            $failed = ($report.Results | Where-Object { $_.Status -match 'Fail|Failed|Error' }).Count
        }
        if ($passed -ne $null -and $failed -ne $null) {
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
    foreach ($p in $patterns) {
        $m = [regex]::Match($text, $p)
        if ($m.Success -and $m.Groups.Count -ge 2) {
            return [int]$m.Groups[1].Value
        }
    }
    return $null
}

if (-not $passed -or -not $failed) {
    $passed = Get-Number $log @("Passed:\s*(\d+)", "(\d+) passed", "Passed\s*:\s*(\d+)")
    $failed = Get-Number $log @("Failed:\s*(\d+)", "(\d+) failed", "Failed\s*:\s*(\d+)")
}

if (-not $passed -and -not $failed) {
    # As a final fallback, scan for summary lines
    $summaryLine = ($log -split "\r?\n") | Where-Object { $_ -match "Automation Test" -or $_ -match "Summary" } | Select-Object -First 1
    Write-Host "Could not extract pass/fail counts automatically. Found summary fragment: $summaryLine"
    throw "Unable to determine automation test results from log or report. See $logPath and $ReportOutputPath"
}

$passed = $passed -as [int]
$failed = $failed -as [int]

Write-Host "Automation results: Passed=$passed, Failed=$failed"

# Emit locations for CI capture
Write-Host "::notice file=$logPath::Automation log generated"
if ($reportFile) { Write-Host "::notice file=$($reportFile.FullName)::Automation JSON report generated" }

if ($failed -gt 0) {
    Write-Host "One or more automation tests failed. Failing CI step."
    exit 1
}

Write-Host "All automation tests passed."
exit 0
