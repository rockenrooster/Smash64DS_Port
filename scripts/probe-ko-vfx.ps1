[CmdletBinding()]
param(
    [string]$Build = 'build-r2-bothcpu',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(60, 600)][int]$TimeoutSeconds = 480
)

# THE KO HALF OF THE PARTICLE/VFX CLUSTER: BUGS rows 3, 8 and 9.
#
# It exists as its own script because of one measured constraint. Two level-3
# CPUs do not KO each other inside the first thirty seconds of a match, so a
# KO probe has to survive a whole match -- and probe-vfx-contracts.ps1 cannot,
# because its frame-marker callback stops the core on EVERY presented frame.
# At 900 stops that run already spans most of its 380-second budget and still
# reported dead_calls=0, rebirth_calls=0, explode_calls=0: not a finding about
# the KO path, just a run that ended before the first KO.
#
# So this probe never breaks per frame. It arms a `tbreak` with an IGNORE COUNT
# from inside each maker's own callback, which is gdb's way of saying "stop once,
# N frames from now" without stopping in between. Between events the guest runs
# free, and a full one-minute match fits comfortably.
#
#   Rebirth (row 3) -- "Respawn floating platform isn't visible when
#                      respawning." The DS substitutes a RING
#                      (ndsEFManagerBuildRing, battleship_efmanager.c:483)
#                      attached to the fighter's nFTPartsJointTopN, where the
#                      source hangs a halo at status_vars.common.rebirth
#                      .halo_offset and LOWERS it over halo_lower_wait
#                      (ftcommonrebirth.c). 24 frames in, so the lower has
#                      started.
#   Star-KO (row 8) -- "correct VFX and SFX never play". Script 0x5C wants
#                      texture 24, which IS admitted with its single frame
#                      packed, so if nothing appears the atlas is not the
#                      reason.
#   KO burst (row 9) -- "it gets clipped or something so I can't see it fully."
#                      Six frames in, inside the animation rather than at its
#                      first frame, so a burst that dies partway through shows
#                      as a partial frame rather than as nothing at all.
#
# The captures are the deliverable; the counters only say whether each path was
# reached at all, which is the half a screenshot cannot prove on its own.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$artifact = Join-Path $root 'artifacts\verification\ko-vfx-probe.txt'
$capture_helper = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.ko-vfx-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.ko-vfx-probe.stderr.log'
$config_state = $null
$emulator = $null

$required = @(
    'efManagerDeadExplodeMakeEffect', 'efManagerSparkleWhiteDeadMakeEffect',
    'efManagerRebirthHaloMakeEffect', 'efManagerDamageNormalLightMakeEffect',
    'ndsBattlePlayableFrameCompleteMarker', 'mnVSResultsMakeConfetti',
    'gNdsParticleQuadMissCount', 'gNdsParticleQuadEmitCount',
    'gNdsVisualEffectCreateCount', 'gNdsVisualEffectDropCount',
    'gNdsVisualEffectKindMask', 'gMPCollisionGroundData',
    'ftCommonDeadUpStarProcUpdate'
)
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw "probe symbols absent from $elf : $($missing -join ', ')"
}

# NOT $Pid: that is a PowerShell automatic read-only variable, and binding a
# parameter to it fails the whole script before the emulator ever launches.
function New-CaptureCommand([string]$Name, [int]$EmulatorProcessId) {
    return ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' +
        $capture_helper + '" -EmulatorProcessId ' + $EmulatorProcessId +
        ' -Output "artifacts/visibility/2026-08-01_' + $Name + '.png"')
}

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
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'set $explode_calls = 0',
        'set $star_calls = 0',
        'set $rebirth_calls = 0',
        'set $spark_calls = 0',
        'set $spark_absmax = 0.0',
        'set $star_x = 0.0',
        'set $star_y = 0.0',
        'set $cam_top = 0',
        'set $map_top = 0',
        'set $star_updates = 0',
        'set $star_phase = -1',
        'set $star_wait = -1',
        'set $star_vy = 0.0',
        'set $star_vymax = 0.0',
        'set $star_posy = 0.0',
        'set $shot_explode = 0',
        'set $shot_star = 0',
        'set $shot_rebirth = 0',

        # Each maker arms ONE delayed stop and disarms itself, so the whole
        # match costs three stops rather than one per frame. `ignore $bpnum N`
        # is the delay; the tbreak is temporary so it never fires twice.
        'break efManagerDeadExplodeMakeEffect',
        'commands',
        'silent',
        'set $explode_calls = $explode_calls + 1',
        'if $shot_explode == 0',
        'set $shot_explode = 1',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'ignore $bpnum 5',
        'commands',
        'silent',
        (New-CaptureCommand 'ko-burst-probe' $emulator.Id),
        'continue',
        'end',
        'end',
        'continue',
        'end',

        'break efManagerSparkleWhiteDeadMakeEffect',
        'commands',
        'silent',
        'set $star_calls = $star_calls + 1',
        'set $star_x = pos->x',
        'set $star_y = pos->y',
        # THE NUMBER THAT SPLITS ROW 8 IN TWO. ftcommondead.c:340 drives the
        # fighter to camera_bound_top * 0.6 over FTCOMMON_DEADUP_WAIT (180)
        # frames and ftcommondead.c:357 then spawns the sparkle at whatever
        # nFTPartsJointTopN reached. camera_bound_top is an s16, so the target
        # cannot exceed 19,660 whatever the stage says -- and the first
        # measured sparkle landed at y 79,222. Either this reads absurdly
        # large, which makes it an asset/endianness bug, or it reads normally
        # and the ascent is running for far more than 180 frames, which makes
        # it a rate bug. Nothing else can produce that number.
        'if gMPCollisionGroundData != 0',
        'set $cam_top = gMPCollisionGroundData->camera_bound_top',
        'set $map_top = gMPCollisionGroundData->map_bound_top',
        'end',
        'if $shot_star == 0',
        'set $shot_star = 1',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'ignore $bpnum 5',
        'commands',
        'silent',
        (New-CaptureCommand 'star-ko-probe' $emulator.Id),
        'continue',
        'end',
        'end',
        'continue',
        'end',

        'break efManagerRebirthHaloMakeEffect',
        'commands',
        'silent',
        'set $rebirth_calls = $rebirth_calls + 1',
        'if $shot_rebirth == 0',
        'set $shot_rebirth = 1',
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'ignore $bpnum 23',
        'commands',
        'silent',
        (New-CaptureCommand 'rebirth-halo-probe' $emulator.Id),
        'continue',
        'end',
        'end',
        'continue',
        'end',

        # ROW 8, THE DECISIVE PAIR. ftCommonDeadUpStarProcUpdate is the whole
        # star ascent: phase 0 sets vel_air.y to
        # (camera_bound_top * 0.6 - y) / FTCOMMON_DEADUP_WAIT and phase 1 spawns
        # the sparkle. With camera_bound_top measured at 4000 the target is
        # 2400, the per-frame step should be about 13, and the first measured
        # sparkle sat at y 79,222 -- which is 2400 x 33, i.e. exactly what
        # integrating the UNDIVIDED delta for 33 frames produces. Sampling the
        # velocity the phase actually stored, and the wait it actually counts,
        # separates "the divide is gone" from "the ascent runs too long".
        # Sampled every call rather than once: the phase changes between them.
        'break ftCommonDeadUpStarProcUpdate',
        'commands',
        'silent',
        'set $star_updates = $star_updates + 1',
        # $ftp, NOT $fp. gdb reserves $fp for the frame-pointer REGISTER, so
        # assigning to it fails with "Left operand of assignment is not an
        # lvalue" -- a message that reads like the cast is wrong rather than
        # the name. And the struct has to be rebuilt from the GObj because the
        # function's own `fp` local is "value has been optimized out" at entry
        # under -Os; ftGetStruct is just ((FTStruct *)gobj->user_data.p).
        'set $ftp = (FTStruct *)fighter_gobj->user_data.p',
        'set $star_phase = $ftp->motion_vars.flags.flag1',
        'set $star_wait = $ftp->status_vars.common.dead.wait',
        'set $star_vy = $ftp->physics.vel_air.y',
        'set $star_posy = ((DObj *)fighter_gobj->obj)->translate.vec.f.y',
        'if $star_vy > $star_vymax',
        'set $star_vymax = $star_vy',
        'end',
        'continue',
        'end',

        # BUGS row 4, the same run for free: where the hit sparks are handed.
        # "Stray VFX across the stage" would show as an |x| far outside a
        # fighter's reach.
        'break efManagerDamageNormalLightMakeEffect',
        'commands',
        'silent',
        'set $spark_calls = $spark_calls + 1',
        'if pos->x > $spark_absmax',
        'set $spark_absmax = pos->x',
        'end',
        'if -pos->x > $spark_absmax',
        'set $spark_absmax = -pos->x',
        'end',
        'continue',
        'end',

        # The run ends where the match does, not on a frame count.
        'tbreak mnVSResultsMakeConfetti',
        'continue',

        ('printf "KOVFX explode_calls=%d star_calls=%d rebirth_calls=%d ' +
            'star=%f,%f cam_top=%d map_top=%d ' +
            'star_updates=%d phase=%d wait=%d vy=%f vymax=%f posy=%f ' +
            'spark_calls=%d spark_absmax=%f ' +
            'visual_created=%u visual_dropped=%u visual_kindmask=%#x ' +
            'miss=%u emit=%u structs_max=%u\n", ' +
            '$explode_calls, $star_calls, $rebirth_calls, ' +
            '$star_x, $star_y, $cam_top, $map_top, ' +
            '$star_updates, $star_phase, $star_wait, $star_vy, $star_vymax, $star_posy, ' +
            '$spark_calls, $spark_absmax, ' +
            'gNdsVisualEffectCreateCount, gNdsVisualEffectDropCount, ' +
            'gNdsVisualEffectKindMask, ' +
            'gNdsParticleQuadMissCount, gNdsParticleQuadEmitCount, ' +
            'gNdsParticleStructsMax'),
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'ko_vfx_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifact) | Out-Null
    Set-Content -LiteralPath $artifact -Value $capture
    $capture | Select-String -Pattern 'KOVFX'
    Write-Output "probe capture: $artifact"
}
finally {
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
