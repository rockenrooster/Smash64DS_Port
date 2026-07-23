param(
    [switch]$Build,
    [switch]$NoBuild,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$DelaySeconds = 5,
    [int]$RunnerSlot = -1,
    [int]$GdbPort = 4333,
    [switch]$List,
    [switch]$SkipRegistryCheck
)
$ErrorActionPreference = 'Stop'
if ($Build -and $NoBuild) {
    throw 'Use either -Build or -NoBuild, not both.'
}

if ($List) {
    $steps = @(
        [PSCustomObject]@{ Step = 'Toolchain path normalization'; Script = 'check-toolchain-path-normalization.ps1' },
        [PSCustomObject]@{ Step = 'MP floor crossing fixtures'; Script = 'check-mp-floor-crossing-fixtures.ps1' },
        [PSCustomObject]@{ Step = 'MP topology fixtures'; Script = 'check-mp-topology-fixtures.ps1' },
        [PSCustomObject]@{ Step = 'GBI fixtures'; Script = 'check-gbi-decode-fixtures.ps1' },
        [PSCustomObject]@{ Step = 'Fighter matrix angle index'; Script = 'check-fighter-matrix-angle-index.ps1' },
        [PSCustomObject]@{ Step = 'FT hit-status fixtures'; Script = 'check-ft-hitstatus-fixtures.ps1' },
        [PSCustomObject]@{ Step = 'Audio ID fixtures'; Script = 'check-audio-id-fixtures.ps1' },
        [PSCustomObject]@{ Step = 'Derived BGM assets'; Script = 'check-audio-bgm-derived-assets.ps1' },
        [PSCustomObject]@{ Step = 'melonDS policy'; Script = 'check-melonds-policy.ps1' },
        [PSCustomObject]@{ Step = 'One-minute verifier contract'; Script = 'check-one-minute-match-verifier.ps1' },
        [PSCustomObject]@{ Step = 'FGM phase pack'; Script = 'check-audio-fgm-phase-pack.ps1' },
        [PSCustomObject]@{ Step = 'Pupupu water AOT corpus'; Script = 'check-pupupu-water-aot.ps1' },
        [PSCustomObject]@{ Step = 'Renderer parity corpus'; Script = 'check-renderer-parity-corpus.ps1' },
        [PSCustomObject]@{ Step = 'Tick-HUD target parity'; Script = 'check-tickhud-parity.ps1' }
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

Invoke-DevFastStep -Script 'check-toolchain-path-normalization.ps1'
Invoke-DevFastStep -Script 'check-mp-floor-crossing-fixtures.ps1'
Invoke-DevFastStep -Script 'check-mp-topology-fixtures.ps1'
Invoke-DevFastStep -Script 'check-gbi-decode-fixtures.ps1'
Invoke-DevFastStep -Script 'check-fighter-matrix-angle-index.ps1'
Invoke-DevFastStep -Script 'check-ft-hitstatus-fixtures.ps1'
Invoke-DevFastStep -Script 'check-audio-id-fixtures.ps1'
Invoke-DevFastStep -Script 'check-audio-bgm-derived-assets.ps1'
Invoke-DevFastStep -Script 'check-melonds-policy.ps1'
Invoke-DevFastStep -Script 'check-one-minute-match-verifier.ps1'
Invoke-DevFastStep -Script 'check-audio-fgm-phase-pack.ps1'
Invoke-DevFastStep -Script 'check-pupupu-water-aot.ps1'
Invoke-DevFastStep -Script 'check-renderer-parity-corpus.ps1'
# The tick-HUD ROM must stay the published program plus timers, or every bucket
# it reports describes a binary that is not the one shipping.
Invoke-DevFastStep -Script 'check-tickhud-parity.ps1'
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
    # Both captured presentations are independently content/detail-gated. The
    # short live-input sample may still move/zoom the camera by a full source
    # tick, so its pairwise delta allowance is wider than the normal path.
    '-MaxScreenshotChangedFraction', '0.50',
    '-MaxScreenshotMeanChannelDelta', '45',
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
