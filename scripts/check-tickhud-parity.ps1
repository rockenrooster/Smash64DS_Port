<#
.SYNOPSIS
Assert the tick-HUD target builds the same program as the published ROM.

.DESCRIPTION
`smash64ds-battle-playable-tickhud-hwtri` exists to measure
`smash64ds-battle-playable-hwtri` -- same program, plus the Task 41 timers. Every
tick-HUD bucket, every device histogram and the whole Task 37 census are readings
of that ROM, so any flag that is on in the published target block and off in the
tick-HUD block silently turns those numbers into a different binary's.

That drifted once already: Task 37 shipped on 2026-07-22 by adding
NDS_TASK37_ITCM_LEAVES := 7 to the published block only, leaving the tick-HUD
build a release behind and its measurements describing code that no longer
matched what shipped.

This compares `make print-benchmark-flags` for both targets -- the Makefile's own
resolved values, not a text scrape of the target blocks, so conditionals and
overrides are evaluated exactly as a real build would evaluate them. Two
differences are legitimate and allowlisted below; anything else is drift.

Read-only: `print-benchmark-flags` is deliberately outside the build graph, so
this builds nothing and takes about a second.
#>
[CmdletBinding()]
param(
    [string]$Published = 'smash64ds-battle-playable-hwtri',
    [string]$TickHud = 'smash64ds-battle-playable-tickhud-hwtri'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

# The only two keys the two targets are allowed to disagree on.
$allowed = [ordered]@{
    'BENCH_MAKE_TARGET'   = 'the target name itself'
    'BENCH_MAKE_TICK_HUD' = 'the entire reason the tick-HUD target exists'
}

function Get-MakeFlagIdentity {
    param([Parameter(Mandatory = $true)][string]$Target)

    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    # An unused outer BUILD path keeps make from reading or repairing the real
    # build's dependency files while it parses, matching print-toolchain-paths.
    $makeExe = (Get-Command make.exe -CommandType Application).Source
    $lines = @(& $makeExe -C $root "TARGET=$Target" 'BUILD=build-flag-probe' `
        'print-benchmark-flags' 2>&1 | ForEach-Object { "$_" })
    if ($LASTEXITCODE -ne 0) {
        throw "make print-benchmark-flags failed for target '$Target'."
    }
    $map = [ordered]@{}
    foreach ($line in $lines) {
        if ($line -match '^(BENCH_MAKE_[A-Z0-9_]+)=(.*)$') {
            $map[$Matches[1]] = $Matches[2]
        }
    }
    if ($map.Count -lt 10) {
        throw ("make print-benchmark-flags returned only $($map.Count) keys " +
            "for target '$Target'; the probe is broken, not the parity.")
    }
    return $map
}

$publishedFlags = Get-MakeFlagIdentity -Target $Published
$tickHudFlags = Get-MakeFlagIdentity -Target $TickHud

$keys = @($publishedFlags.Keys) + @($tickHudFlags.Keys) |
    Sort-Object -Unique
$drift = [Collections.Generic.List[string]]::new()
foreach ($key in $keys) {
    $p = if ($publishedFlags.Contains($key)) { $publishedFlags[$key] } else { '<absent>' }
    $t = if ($tickHudFlags.Contains($key)) { $tickHudFlags[$key] } else { '<absent>' }
    if ($p -ceq $t) { continue }
    if ($allowed.Contains($key)) {
        Write-Verbose "$key differs as expected ($($allowed[$key])): '$p' vs '$t'"
        continue
    }
    $drift.Add(('  {0}' -f $key))
    $drift.Add(('      published {0} = {1}' -f $Published, $p))
    $drift.Add(('      tick HUD  {0} = {1}' -f $TickHud, $t))
}

if ($drift.Count -gt 0) {
    Write-Output 'Tick-HUD parity FAILED. These flags differ:'
    $drift | ForEach-Object { Write-Output $_ }
    throw ("The tick-HUD target no longer builds the published program. Add the " +
        "missing override(s) to its Makefile block, or -- if the difference is " +
        "deliberate -- allowlist it in this script with the reason. Do not " +
        "publish tick-HUD measurements until this passes.")
}

Write-Output ("Tick-HUD parity passed: $($keys.Count) make flags compared, " +
    "$($allowed.Count) allowlisted differences, 0 drift.")
exit 0
