[CmdletBinding()]
param(
    [string]$Build = 'build-c163-gate-bp1',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 2400,
    [ValidateRange(1, 4000)][int]$Hits = 2300,
    [string]$Artifact = ''
)

# WHAT OWNS THE RESIDENT PACK'S +12.4% BATTLE WALL TIME?
#
# BATTLEPACK_RESIDENT.md section 5 banked gNdsBattlePlayablePacingVBlanks
# 4,274 -> 4,805 with present-interval buckets [4]/[5] 5/12 -> 42/108, and named
# three candidates without separating them: the 18 streamed 16 KiB chunks, the
# 126 acquisitions the un-packed fighter now takes uncached, and cross-build
# placement.
#
# Two of the three are separable INSIDE one arm, with no build and no placement
# term at all. gNdsBattlePlayablePacingVBlanks is a guest counter updated once
# per presented frame, so differencing it at consecutive
# ndsBattlePlayableFrameCompleteMarker stops yields that frame's present
# interval; gNdsBattlePackLoadSteps and gNdsR2AnimCacheRejects say which frames
# streamed a chunk and which took an uncached load. Whatever excess is left over
# after those two are subtracted is the cross-build term, and it is the only one
# that needs a second binary to see.
#
# HALTING THE GUEST DOES NOT PERTURB THESE NUMBERS. Every counter read here is
# advanced by guest execution -- VBlank IRQs and cpuGetTiming -- and melonDS
# stops the machine at a breakpoint, so a stop costs host wall time and no guest
# ticks. That is also why this may run beside a build: guest timing is
# deterministic under host load.

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
        (Get-Date -Format 'yyyy-MM-dd') + "_battlepack-pacing-$Build.txt")
}

# The pack counters only exist on a NDS_R2_BATTLEPACK=1 arm; the control arm
# must still produce every other column, so they are resolved per build and
# printed as literal 0 when absent rather than failing the run.
$required = @(
    'ndsBattlePlayableFrameCompleteMarker',
    'scVSBattleStartBattle',
    'gNdsBattlePlayablePacingVBlanks',
    'gNdsBattlePlayablePacingPresentedFrames',
    'gNdsBattlePlayablePacingLogicFrames',
    'gNdsR2AnimCacheRejects',
    'gNdsR2AnimCacheHits',
    'gNdsBattlePackHits',
    'gNdsBattlePackMisses',
    'gNdsBattlePackLoadSteps'
)
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Battlepack pacing probe symbols absent from {0}: {1}" -f $elf,
        ($missing -join ', '))
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.battlepack-pacing.stdout.log'
$stderr = Join-Path $log_dir 'melonds.battlepack-pacing.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
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
        'set remotetimeout 30',
        # INCREMENTAL LOGGING, and it is not optional for this probe. A
        # whole-match per-frame run is long enough to be killed or to reach its
        # timeout, and gdb's stdout is redirected by the helper -- i.e. buffered
        # by the process and DISCARDED by a forced terminate. On 2026-08-15 that
        # threw away ~25 minutes of samples and left a 587-byte capture holding
        # only the lines written before the buffer filled. `set logging` writes
        # through, so a killed run still carries every frame it sampled.
        ("set logging file {0}" -f ($Artifact -replace '\\', '/')),
        'set logging overwrite on',
        'set logging enabled on',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $n = 0',
        ('printf "BPP build=' + $Build + '\n"'),
        'tbreak scVSBattleStartBattle',
        'continue',
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        'set $n = $n + 1',
        'printf "BPP %d vbl=%u pres=%u lgc=%u steps=%u rej=%u ach=%u ph=%u pm=%u\n", $n, gNdsBattlePlayablePacingVBlanks, gNdsBattlePlayablePacingPresentedFrames, gNdsBattlePlayablePacingLogicFrames, gNdsBattlePackLoadSteps, gNdsR2AnimCacheRejects, gNdsR2AnimCacheHits, gNdsBattlePackHits, gNdsBattlePackMisses',
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        'continue',
        'printf "BPPDONE n=%d vbl=%u pres=%u steps=%u rej=%u ach=%u ph=%u pm=%u p0=%u p1=%u\n", $n, gNdsBattlePlayablePacingVBlanks, gNdsBattlePlayablePacingPresentedFrames, gNdsBattlePackLoadSteps, gNdsR2AnimCacheRejects, gNdsR2AnimCacheHits, gNdsBattlePackHits, gNdsBattlePackMisses, gNdsBattleTextHudP0Damage, gNdsBattleTextHudP1Damage',
        'printf "BPPBUCKET 2=%u 3=%u 4=%u 5=%u max=%u viol=%u\n", gNdsBattlePlayablePacingPresentIntervalBucket[2], gNdsBattlePlayablePacingPresentIntervalBucket[3], gNdsBattlePlayablePacingPresentIntervalBucket[4], gNdsBattlePlayablePacingPresentIntervalBucket[5], gNdsBattlePlayablePacingPresentIntervalMax, gNdsBattlePlayablePacingCadenceViolationCount',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'battlepack_pacing_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # `set logging` above already wrote $Artifact through, so it is authoritative
    # even for a killed run. The helper's own .out is copied over it ONLY when it
    # is larger, i.e. when the run exited cleanly and flushed more than the log
    # holds.
    $captured = Join-Path $log_temp 'battlepack_pacing_probe.gdb.out'
    if ((Test-Path -LiteralPath $captured) -and
        ((-not (Test-Path -LiteralPath $Artifact)) -or
         ((Get-Item -LiteralPath $captured).Length -gt
          (Get-Item -LiteralPath $Artifact).Length))) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
    }
    if (Test-Path -LiteralPath $Artifact) {
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match 'BPPDONE|BPPBUCKET' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
