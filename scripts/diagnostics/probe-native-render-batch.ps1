[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$CasesFile,
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunId,
    [ValidateRange(1,12)][int]$MaxParallel = 4,
    [ValidateRange(0,31)][int]$FirstSlot = 0,
    [ValidateRange(30,3600)][int]$TimeoutSeconds = 240,
    [string]$Rom = 'builds/build-p2-shell/smash64ds-p2-shell-hwtri.nds',
    [string]$Elf = 'builds/build-p2-shell/smash64ds-p2-shell-hwtri.elf',
    [switch]$NoCapture
)
# Orchestration only. Every child uses an already-built, unchanged ROM/ELF.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$Rom = (Resolve-Path -LiteralPath $Rom).Path
$Elf = (Resolve-Path -LiteralPath $Elf).Path
$cases = @(Get-Content -LiteralPath $CasesFile -Raw | ConvertFrom-Json)
if ($cases.Count -eq 0) { throw 'At least one diagnostic case is required.' }
if (($FirstSlot + $MaxParallel) -gt 32) { throw 'Requested runner slots exceed slot31.' }
$names = @{}
foreach ($case in $cases) {
    if ([string]$case.name -notmatch '^[A-Za-z0-9_-]+$' -or $names.ContainsKey([string]$case.name)) {
        throw 'Case names must be unique letters/digits/underscores/hyphens.'
    }
    $names[[string]$case.name] = $true
    if ($null -eq $case.stage -or [int]$case.stage -lt 0 -or [int]$case.stage -gt 8) {
        throw "Invalid stage in case $($case.name)."
    }
    foreach ($key in $case.PSObject.Properties.Name) {
        if ($key -notin @('name','stage','presents')) { throw "Unknown case field: $key" }
    }
}
$output = Join-Path $root "builds/diagnostics/$RunId"
if (Test-Path -LiteralPath $output) { throw "Use a fresh RunId; output already exists: $output" }
$slots = [Collections.Generic.Queue[int]]::new()
foreach ($slot in $FirstSlot..($FirstSlot+$MaxParallel-1)) {
    if (-not (Test-Path -LiteralPath (Join-Path $root "emulators/melonds-runners/slot$slot/melonDS.exe"))) {
        throw "Missing slot$slot. Provision slots first with scripts/New-MelonDSRunnerSlots.ps1."
    }
    $slots.Enqueue($slot)
}
New-Item -ItemType Directory -Path $output | Out-Null
$watch = [Diagnostics.Stopwatch]::StartNew()
$running = [Collections.Generic.List[object]]::new()
$completed = [Collections.Generic.List[object]]::new()
$next = 0
$worker = Join-Path $PSScriptRoot 'probe-native-render-scene.ps1'
$pwsh = (Get-Command pwsh -ErrorAction Stop).Source
while ($next -lt $cases.Count -or $running.Count) {
    while ($next -lt $cases.Count -and $slots.Count) {
        $case = $cases[$next++]
        $slot = $slots.Dequeue()
        $name = "$RunId-$($case.name)"
        $presents = if ($null -ne $case.presents) { [int]$case.presents } else { 16 }
        $arguments = @('-NoProfile','-File',('"'+$worker+'"'),'-Name',$name,
            '-RunnerSlot',"$slot",'-StageKind',"$($case.stage)",'-Presents',"$presents",
            '-TimeoutSeconds',"$TimeoutSeconds",'-Rom',('"'+$Rom+'"'),'-Elf',('"'+$Elf+'"'),
            '-OutputDirectory',('"'+$output+'"'))
        if ($NoCapture) { $arguments += '-NoCapture' }
        $process = Start-Process -FilePath $pwsh -ArgumentList $arguments -WindowStyle Hidden `
            -WorkingDirectory $root -PassThru -RedirectStandardOutput (Join-Path $output "$name.out.txt") `
            -RedirectStandardError (Join-Path $output "$name.err.txt")
        $running.Add([PSCustomObject]@{name=$name;case=$case.name;slot=$slot;process=$process})
        Write-Output "Started $($case.name) on slot$slot (process $($process.Id))."
    }
    foreach ($run in @($running.ToArray())) {
        $run.process.Refresh()
        if (-not $run.process.HasExited) { continue }
        $run.process.WaitForExit()
        $path = Join-Path $output "$($run.name).json"
        $row = if (Test-Path -LiteralPath $path) {
            Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
        } else {
            [PSCustomObject]@{name=$run.name;transport='failed';native='unobserved';error='Worker produced no result JSON.'}
        }
        $row | Add-Member -NotePropertyName case -NotePropertyValue $run.case
        $row | Add-Member -NotePropertyName worker_exit_code -NotePropertyValue $run.process.ExitCode
        $completed.Add($row)
        $slots.Enqueue($run.slot)
        $running.Remove($run) | Out-Null
        Write-Output "Finished $($run.case): transport=$($row.transport), native=$($row.native)."
        $run.process.Dispose()
    }
    if ($running.Count) { Start-Sleep -Milliseconds 250 }
}
$summary = [ordered]@{
    run_id=$RunId; concurrency=$MaxParallel; wall_seconds=[Math]::Round($watch.Elapsed.TotalSeconds,3);
    case_count=$cases.Count; transport_failures=@($completed | Where-Object transport -ne 'ok').Count;
    native_failures=@($completed | Where-Object native -eq 'fail').Count;
    performance_acceptance=$false; cases=@($completed.ToArray())
}
$summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'summary.json') -Encoding utf8
Write-Output "Batch completed in $($summary.wall_seconds)s; transport failures=$($summary.transport_failures), native failures=$($summary.native_failures)."
Write-Output (Join-Path $output 'summary.json')
if ($summary.transport_failures) { exit 1 }
if ($summary.native_failures) { exit 2 }
exit 0
