[CmdletBinding()]
param(
    [string]$Build = 'build-r2-a5i3',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 400)][int]$TimeoutSeconds = 300
)

# ONE BATCHED PROBE FOR THE WHOLE PARTICLE/VFX CLUSTER, per BUG_FIXING_PROCESS
# v2: "plant one batched probe build that answers every open question in it",
# and rung 2 of the evidence ladder -- GDB on an EXISTING ROM, no build.
#
# It answers the contract rows that were blocking BUGS.md rows 1, 4 and 10 by
# reading the number that actually decides each one, rather than the number that
# is easy to reach:
#
# BATTLE PHASE ONLY. Results-reachable rows (the confetti) need their own short
# run at their own trigger; waiting through a match for them here is the exact
# anti-pattern the process names.
#
#   Whispy (row 1)  -- the emitter origin dust_xf->translate and the side
#                      lr_players chose, read at the line after the source
#                      assigns them. PREDICTED: exactly (-715, 100) or
#                      (-205, 100) from dGRPupupuWhispyDustEffectPositions,
#                      i.e. within 320 of the tree at -525 (ground.h:19). If it
#                      matches, the source placement is intact and the reported
#                      "too far away" is downstream -- the tree's own drawn
#                      position or the camera -- and NOT the effect.
#                      Also latches pc->size, whose script value is the contract.
#   Hit spark (row 4) -- the position efManagerDamageNormalLightMakeEffect is
#                      handed. PREDICTED: within a fighter's reach of a
#                      fighter, i.e. |x| < 2000; the reported symptom is sparks
#                      "across the stage", which would show as an x far from
#                      either fighter.
#   Star-KO (row 7)  -- whether efManagerSparkleWhiteDeadMakeEffect is reached
#                      at all under KO stress, and with what position.
#   Rebirth (row 3)  -- whether the halo maker returns non-NULL on the natural
#                      respawn path.
#
# Every value is latched in a `silent` breakpoint callback that continues
# immediately, so the guest is never held still and nothing here changes timing
# enough to move the behaviour being measured. One printf at the end, because a
# gdb batch abandons the rest of its commands on the first error and eighty
# separate reads would risk all of them on one bad symbol.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$artifact = Join-Path $root 'artifacts\verification\vfx-contract-probe.txt'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.vfx-contract-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.vfx-contract-probe.stderr.log'
$config_state = $null
$emulator = $null

# Every symbol this batch names, checked before the batch runs. A gdb command
# file abandons everything after its first error, so one absent symbol would
# silently cost the whole probe -- that has already happened once on this
# project and the rule is recorded in soak-freeze-watch.ps1.
$required = @(
    'grPupupuWhispyDustMakeEffect', 'gGRCommonStruct',
    'efManagerDamageNormalLightMakeEffect',
    'efManagerSparkleWhiteDeadMakeEffect', 'efManagerRebirthHaloMakeEffect',
    'ndsBattlePlayableFrameCompleteMarker', 'gNdsParticleQuadMissCount',
    'gLBParticleTransformsUsedNum', 'gNdsParticleTransformsMax',
    'gNdsBattlePlayablePacingPresentedFrames'
)
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw "probe symbols absent from $elf : $($missing -join ', ')"
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
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        # latches
        'set $whispy_calls = 0',
        'set $whispy_xf_reads = 0',
        'set $dust_frames = 0',
        'set $frame_stops = 0',
        'set $leaf_frames = 0',
        'set $leaf_x = 0.0',
        'set $whispy_lr = -1',
        'set $whispy_x = 0.0',
        'set $whispy_y = 0.0',
        'set $whispy_size = 0.0',
        'set $spark_calls = 0',
        'set $spark_x = 0.0',
        'set $spark_y = 0.0',
        'set $spark_absmax = 0.0',
        'set $dead_calls = 0',
        'set $dead_x = 0.0',
        'set $dead_y = 0.0',
        'set $rebirth_calls = 0',

        # grpupupu.c:507 is the line after `gGRCommonStruct.pupupu.dust_xf = xf`,
        # so the emitter transform is reachable through the GLOBAL. Reading the
        # function's own `xf` and `pc` locals at :502 was tried first and gdb
        # answered "value has been optimized out" -- this tree builds -Os, so a
        # local that is dead after its last store is not addressable. Globals
        # always are. The site is hit once per blow, not once per quad; an
        # earlier version of this probe broke on ndsRendererSubmitParticleQuad
        # and stopped the core ~95,000 times a match, which timed out at 300 s.
        'break grpupupu.c:507',
        'commands',
        'silent',
        'set $whispy_calls = $whispy_calls + 1',
        'set $whispy_lr = gGRCommonStruct.pupupu.lr_players',
        'if gGRCommonStruct.pupupu.dust_xf != 0',
        'set $whispy_xf_reads = $whispy_xf_reads + 1',
        'set $whispy_x = gGRCommonStruct.pupupu.dust_xf->translate.x',
        'set $whispy_y = gGRCommonStruct.pupupu.dust_xf->translate.y',
        'set $whispy_size = gGRCommonStruct.pupupu.dust_xf->rotate.y',
        'end',
        'continue',
        'end',

        'break efManagerDamageNormalLightMakeEffect',
        'commands',
        'silent',
        'set $spark_calls = $spark_calls + 1',
        'set $spark_x = pos->x',
        'set $spark_y = pos->y',
        'if pos->x > $spark_absmax',
        'set $spark_absmax = pos->x',
        'end',
        'if -pos->x > $spark_absmax',
        'set $spark_absmax = -pos->x',
        'end',
        'continue',
        'end',

        'break efManagerSparkleWhiteDeadMakeEffect',
        'commands',
        'silent',
        'set $dead_calls = $dead_calls + 1',
        'set $dead_x = pos->x',
        'set $dead_y = pos->y',
        'continue',
        'end',

        'break efManagerRebirthHaloMakeEffect',
        'commands',
        'silent',
        'set $rebirth_calls = $rebirth_calls + 1',
        'continue',
        'end',

        # THE UNAMBIGUOUS READ. gdb resolves grpupupu.c:507 to THREE locations
        # because -Os inlines the maker, so a latch taken there can be sampled
        # in a copy that has not yet stored the global -- which is how the first
        # run of this probe reported xf_reads=0 and nearly bought a wrong
        # conclusion. The frame marker is one location and the transform is a
        # global, so counting frames where it is non-NULL cannot be confounded.
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        'set $frame_stops = $frame_stops + 1',
        'if gGRCommonStruct.pupupu.dust_xf != 0',
        'set $dust_frames = $dust_frames + 1',
        'set $whispy_x = gGRCommonStruct.pupupu.dust_xf->translate.x',
        'set $whispy_y = gGRCommonStruct.pupupu.dust_xf->translate.y',
        'set $whispy_size = gGRCommonStruct.pupupu.dust_xf->rotate.y',
        'end',
        'if gGRCommonStruct.pupupu.leaves_xf != 0',
        'set $leaf_frames = $leaf_frames + 1',
        'set $leaf_x = gGRCommonStruct.pupupu.leaves_xf->translate.x',
        'end',
        # The callback stops itself rather than a separate tbreak at the same
        # address: two breakpoints on one PC, one of them auto-continuing, means
        # the continue resumes straight past the other's stop and the batch
        # never regains control. That cost one 260-second timeout.
        'if $frame_stops < 450',
        'continue',
        'end',
        'end',

        # Run only until the Whispy dust has spawned twice. The blow is
        # reachable in seconds and the process doc forbids waiting through a
        # match for a trigger that is not at the end of one. Results-only rows
        # (the confetti) need their own short run at their own trigger.
        'continue',

        ('printf "VFXCONTRACT whispy_calls=%d xf_reads=%d lr=%d whispy_x=%f whispy_y=%f whispy_rotY=%f ' +
            'spark_calls=%d spark_x=%f spark_y=%f spark_absmax=%f ' +
            'dead_calls=%d dead_x=%f dead_y=%f rebirth_calls=%d ' +
            'dust_frames=%d leaf_frames=%d leaf_x=%f ' +
            'transforms_used=%u transforms_max=%u structs_max=%u ' +
            'miss=%u emit=%u\n", ' +
            '$whispy_calls, $whispy_xf_reads, $whispy_lr, ' +
            '$whispy_x, $whispy_y, $whispy_size, ' +
            '$spark_calls, $spark_x, $spark_y, $spark_absmax, ' +
            '$dead_calls, $dead_x, $dead_y, $rebirth_calls, ' +
            '$dust_frames, $leaf_frames, $leaf_x, ' +
            'gLBParticleTransformsUsedNum, gNdsParticleTransformsMax, ' +
            'gNdsParticleStructsMax, ' +
            'gNdsParticleQuadMissCount, gNdsParticleQuadEmitCount'),
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'vfx_contract_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifact) | Out-Null
    Set-Content -LiteralPath $artifact -Value $capture
    $capture | Select-String -Pattern 'VFXCONTRACT'
    Write-Output "probe capture: $artifact"
}
finally {
    if ($emulator -ne $null) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($config_state -ne $null) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
