param(
    [switch]$Build,
    [switch]$NoBuild,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$DelaySeconds = 5,
    [int]$RunnerSlot = -1,
    [int]$GdbPort = 3333,
    [switch]$List,
    [switch]$SkipRegistryCheck
)
$ErrorActionPreference = 'Stop'
if ($Build -and $NoBuild) {
    throw 'Use either -Build or -NoBuild, not both.'
}

if ($List) {
    $steps = @(
        [PSCustomObject]@{ Step = 'GBI fixtures'; Script = 'check-gbi-decode-fixtures.ps1' }
    )
    if (-not $SkipRegistryCheck) {
        $steps += [PSCustomObject]@{ Step = 'Harness registry'; Script = 'check-harness-registry.ps1' }
    }
    $steps += [PSCustomObject]@{
        Step = 'Canonical realtime fast iteration'
        Script = 'verify-battle-playable-realtime-harness.ps1 -FastIteration'
    }
    $steps | Format-Table -AutoSize
    exit 0
}

$powerShellExe = (Get-Process -Id $PID).Path
function Invoke-DevFastStep {
    param(
        [string]$Script,
        [string[]]$Arguments = @()
    )
    & $powerShellExe `
        -NoProfile `
        -ExecutionPolicy Bypass `
        -File (Join-Path $PSScriptRoot $Script) `
        @Arguments
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Invoke-DevFastStep -Script 'check-gbi-decode-fixtures.ps1'
if (-not $SkipRegistryCheck) {
    Invoke-DevFastStep -Script 'check-harness-registry.ps1'
}

. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
$selectedGdbPort = if (($RunnerSlot -ge 0) -and
    -not $PSBoundParameters.ContainsKey('GdbPort')) {
    Get-MelonDSRunnerPort -RunnerSlot $RunnerSlot -Cpu ARM9
} else {
    $GdbPort
}
$realtimeArgs = @(
    '-MelonDS', $MelonDS,
    '-Gdb', $Gdb,
    '-GdbPort', "$selectedGdbPort",
    '-RunnerSlot', "$RunnerSlot",
    '-DelaySeconds', "$DelaySeconds",
    '-FastIteration'
)
if ($NoBuild) {
    $realtimeArgs += '-NoBuild'
} else {
    Write-Output 'Building the canonical battle ROM incrementally (no forced normal-ROM rebuild).'
}
Invoke-DevFastStep `
    -Script 'verify-battle-playable-realtime-harness.ps1' `
    -Arguments $realtimeArgs

Write-Output 'Fast canonical iteration passed.'
exit 0
