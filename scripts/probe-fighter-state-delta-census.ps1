[CmdletBinding()]
param(
    [string]$Build = 'build-c157-deltacensus',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 600,
    # Frames to let the match settle before the first snapshot. The counters
    # accumulate from boot and the load frames are not representative of a
    # steady-state draw, so the window has to start after them.
    [ValidateRange(1, 100000)][int]$WarmFrames = 200,
    [ValidateRange(1, 100000)][int]$WindowFrames = 500,
    [string]$Artifact = ''
)

# R2-03 E26 SIZING. docs/P1_EXECUTION_BOARD.md names this census as the thing
# that sizes the state-span bake and records it as blocked on ITCM. It is
# unblocked now (see NDS_R2_CENSUS_EVICTED_CODE in nds_renderer.c) and this is
# the run that reads it.
#
# THE QUESTION: of the ~500 state-delta applications a frame that
# ndsRendererNativeApplyStateSpan replays over a STATIC 70-entry delta table and
# 196-entry sequence, how many are re-applying something already applied this
# frame, and how many write operands identical to the previous application of
# the same effect? That fraction -- not the phase total -- is what a bake can
# actually elide, which is the distinction E19 established when it proved the
# spans cannot be priced by deleting them (deleting them takes the emit too).
#
# READ COUNTS FROM THIS ARM, NOT TICKS. The arm evicts ndsRendererScanList from
# ITCM to make room for the census, so instruction fetch is not representative.
# Counts are placement-invariant; gNdsR2Span*Ticks are not. The macro's comment
# in nds_renderer.c carries the same warning.
#
# TWO SNAPSHOTS, DIFFERENCED. A single read at the end would include the load
# frames and every boot-time draw, which is how a "per frame" figure ends up
# describing a frame that never happens during play.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\performance\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_fighter-state-delta-census\census.txt')
}

$required = @(
    'gNdsR2SpanCalls', 'gNdsR2SpanDeltasApplied', 'gNdsR2SpanDeltaRepeats',
    'gNdsR2SpanIdenticalOperands', 'gNdsR2SpanIdenticalGeometry',
    'gNdsR2DeltaEffectCounts', 'gNdsR2ExecEpochCalls',
    'ndsBattlePlayableFrameCompleteMarker'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("State-delta census symbols absent from {0}: {1}. Build with " +
           "NDS_TASK91_DRAW_PHASE_CENSUS=1." -f $elf, ($missing -join ', '))
}

# The census only exists when the flag is on, and a build directory name proves
# nothing -- read the generated config, the same rule probe-flame-quad-miss.ps1
# encodes for NDS_R2_BOTH_CPU after a both-CPU-NAMED build turned out not to be.
$configHeader = Join-Path $root "builds\$Build\nds_build_config.h"
if (Test-Path -LiteralPath $configHeader -PathType Leaf) {
    $raw = Get-Content -LiteralPath $configHeader -Raw
    $census = [regex]::Match($raw, '(?m)^#define\s+NDS_TASK91_DRAW_PHASE_CENSUS\s+(\d+)')
    $lean = [regex]::Match($raw, '(?m)^#define\s+NDS_R2_SPAN_LEAN_TIMING\s+(\d+)')
    if ($census.Success -and ([int]$census.Groups[1].Value -eq 0)) {
        throw 'This ROM has NDS_TASK91_DRAW_PHASE_CENSUS=0; the census never runs.'
    }
    if ($lean.Success -and ([int]$lean.Groups[1].Value -ne 0)) {
        throw ('NDS_R2_SPAN_LEAN_TIMING=1 compiles the per-delta census block ' +
               'OUT (see NDS_R2_DELTA_CENSUS). Build the lean arm for ticks, ' +
               'not for these counts.')
    }
} else {
    Write-Warning "No nds_build_config.h under builds\$Build; cannot confirm the arm."
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.state-delta-census.stdout.log'
$stderr = Join-Path $log_dir 'melonds.state-delta-census.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$emulator = $null

# One printf per snapshot rather than one per counter: the capture is parsed by
# prefix and a partial line is indistinguishable from a counter that read zero.
$snapshot = {
    param([string]$Tag)
    @(
        ('printf "' + $Tag + ' frame=%d calls=%u applied=%u repeats=%u ident=%u identgeo=%u matinval=%u epochs=%u\n", $n, gNdsR2SpanCalls, gNdsR2SpanDeltasApplied, gNdsR2SpanDeltaRepeats, gNdsR2SpanIdenticalOperands, gNdsR2SpanIdenticalGeometry, gNdsR2SpanMaterialInvalidations, gNdsR2ExecEpochCalls'),
        ('printf "' + $Tag + 'EFF %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", gNdsR2DeltaEffectCounts[0], gNdsR2DeltaEffectCounts[1], gNdsR2DeltaEffectCounts[2], gNdsR2DeltaEffectCounts[3], gNdsR2DeltaEffectCounts[4], gNdsR2DeltaEffectCounts[5], gNdsR2DeltaEffectCounts[6], gNdsR2DeltaEffectCounts[7], gNdsR2DeltaEffectCounts[8], gNdsR2DeltaEffectCounts[9], gNdsR2DeltaEffectCounts[10], gNdsR2DeltaEffectCounts[11], gNdsR2DeltaEffectCounts[12], gNdsR2DeltaEffectCounts[13], gNdsR2DeltaEffectCounts[14], gNdsR2DeltaEffectCounts[15]'),
        ('printf "' + $Tag + 'SPAN before_tk=%u before_d=%u after_tk=%u after_d=%u\n", gNdsR2SpanBeforeTicks, gNdsR2SpanBeforeDeltas, gNdsR2SpanAfterTicks, gNdsR2SpanAfterDeltas')
    )
}

try {
    New-Item -ItemType Directory -Force -Path $log_dir | Out-Null
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $n = 0',
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands 1',
        'silent',
        'set $n = $n + 1',
        ('if $n < ' + $WarmFrames),
        'continue',
        'end',
        'end',
        'continue'
    ) + (& $snapshot 'CENSUSA') + @(
        ('set $target = $n + ' + $WindowFrames),
        'commands 1',
        'silent',
        'set $n = $n + 1',
        'if $n < $target',
        'continue',
        'end',
        'end',
        'continue'
    ) + (& $snapshot 'CENSUSB') + @(
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'state_delta_census.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    $captured = Join-Path $log_temp 'state_delta_census.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^CENSUS' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "census capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
}
