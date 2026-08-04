[CmdletBinding()]
param(
    [string]$Build = 'build-r2-bothcpu',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(60, 600)][int]$TimeoutSeconds = 480,
    # Empty preserves the original artifact names. Candidate evidence should
    # pass a unique label; labelled runs fail before launch rather than
    # overwrite an existing verification artifact or visibility capture.
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')]
    [string]$EvidenceLabel = ''
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
$legacy_evidence_prefix = '2026-08-01'
$evidence_prefix = if ([string]::IsNullOrWhiteSpace($EvidenceLabel)) {
    $legacy_evidence_prefix
} else {
    $EvidenceLabel
}
$artifact_name = if ([string]::IsNullOrWhiteSpace($EvidenceLabel)) {
    'ko-vfx-probe.txt'
} else {
    $EvidenceLabel + '_ko-vfx-probe.txt'
}
$artifact = Join-Path $root ('artifacts\verification\' + $artifact_name)
$capture_names = @(
    $evidence_prefix + '_pre-death-baseline-probe.png',
    $evidence_prefix + '_ko-burst-probe.png',
    $evidence_prefix + '_post-death-late-probe.png',
    $evidence_prefix + '_star-ko-probe.png',
    $evidence_prefix + '_rebirth-halo-probe.png'
)
if (-not [string]::IsNullOrWhiteSpace($EvidenceLabel)) {
    $candidate_outputs = @($artifact) + @($capture_names | ForEach-Object {
        Join-Path $root ('artifacts\visibility\' + $_)
    })
    $existing_outputs = @($candidate_outputs | Where-Object {
        Test-Path -LiteralPath $_
    })
    if ($existing_outputs.Count -ne 0) {
        throw ("EvidenceLabel '{0}' would overwrite: {1}" -f
            $EvidenceLabel, ($existing_outputs -join ', '))
    }
}
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
    'efManagerImpactWaveMakeEffect',
    'ndsBattlePlayableFrameCompleteMarker', 'mnVSResultsMakeConfetti',
    'gNdsParticleQuadMissCount', 'gNdsParticleQuadEmitCount',
    'gNdsVisualEffectCreateCount', 'gNdsVisualEffectDropCount',
    'gNdsVisualEffectKindMask', 'gMPCollisionGroundData',
    'ftCommonDeadUpStarProcUpdate', 'gGCCommonLinks',
    'dEFManagerDeadExplodeRotateD',
    'dEFManagerDeadExplodeEnvColorChildR',
    'dEFManagerDeadExplodeEnvColorChildG',
    'dEFManagerDeadExplodeEnvColorChildB',
    'dEFManagerDeadExplodeEnvColorSiblingR',
    'dEFManagerDeadExplodeEnvColorSiblingG',
    'dEFManagerDeadExplodeEnvColorSiblingB',
    'gNdsKOBurstAttemptCount', 'gNdsKOBurstCompleteCount',
    'gNdsKOBurstDropMask',
    'gNdsR2TexMemoHitCount', 'gNdsR2TexMemoMissCount',
    'gNdsR2TexMemoFillCount', 'gNdsR2TexMemoStaleCount',
    'gNdsR2TexMemoVerifyFail',
    'gNdsR2StagePrepareReuseCount', 'gNdsR2StagePrepareBuildCount',
    'gNdsRendererBattleStaticTextureViolationCount',
    # Read but never declared required until now, which is the counter-with-no-
    # writer trap one step earlier: an absent symbol makes gdb abandon the rest
    # of the command batch silently.
    'gNdsVisualEffectActiveCount', 'gNdsParticleStructsMax',
    'sLBParticleStructsAllocLinks',
    # When a capture happened, published rather than inferred from the HUD
    # drawn inside it.
    'gNdsFrameCounter', 'gNdsBattleTextHudTimeSeconds',
    'gNdsBattleTextHudP0Stock', 'gNdsBattleTextHudP0Damage',
    'gNdsBattleTextHudP1Stock', 'gNdsBattleTextHudP1Damage',
    'gNdsFighterBattlePlayableVictimSlot',
    'gNdsFighterBattlePlayableVictimStockStart',
    'gNdsFighterBattlePlayableVictimStockFinal',
    # BUGS row 6, the scene-wide post-death texture loss. The owner reports it
    # never recovers for the rest of the match, which rules out ordinary
    # eviction -- a cache that re-resolves every frame would refill on the next
    # one. A pinned span uploaded once at scene prepare and then torn down or
    # written through is the shape that loses everything at once and never
    # heals, so its teardown/violation/prepare-fail rows are sampled directly.
    'gNdsRendererBattleStaticTextureTeardownCount',
    'gNdsRendererBattleStaticTexturePrepareFailCount',
    'gNdsRendererBattleStaticTexturePrepareCount',
    'gNdsRendererBattleStaticTexturePreparedCount',
    'gNdsRendererBattleStaticTextureArmCount',
    'gNdsRendererBattleStaticTexturePinnedHitCount',
    'gNdsRendererBattleStaticTextureOwnerMask',
    'gNdsRendererBattleStaticTextureSeenMask',
    'gNdsRendererBattleStaticTextureAllocationSpanBytes',
    'gNdsRendererBattleStaticTextureFirstAddress',
    'gNdsRendererBattleStaticTextureEndAddress',
    'gNdsRendererSceneTextureVramResetCount',
    # Attempt-versus-stop. Rising bind/upload after the corruption means the
    # renderer is still trying and being refused; flat means something latched.
    'gNdsFighterDLAllDrawHardwareTextureBindCount',
    'gNdsFighterDLAllDrawHardwareTextureReadyCount',
    'gNdsFighterDLAllDrawHardwareTextureRejectCount',
    'gNdsFighterDLAllDrawHardwareTextureUploadCount',
    'gNdsEffectRendererTextureReadyCount',
    'gNdsEffectRendererTextureRejectCount',
    'gNdsRendererParticleAtlasFailCount',
    'gNdsRendererParticleAtlasPrepareCount',
    'gNdsRendererBattleTextureFenceCounts',
    'gNdsRendererBattleTextureFenceFirstFrame',
    'gNdsRendererBattleTextureFenceFirstClassPlus1',
    'gNdsFighterDisplayContractNoTextureCount'
)
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw "probe symbols absent from $elf : $($missing -join ', ')"
}

# ADDRESS-RESOLVED, NOT NAME-RESOLVED, AND THE MATCH MUST BE UNIQUE.
# On 2026-08-04 `break <name>` handed this probe
#   Breakpoint 1 at 0x0: efManagerDeadExplodeMakeEffect. (2 locations)
# for three of its five makers, and the run still produced counters -- so the
# transcript looked healthy while the stop site was whatever gdb picked. The
# cause is the NDS_WEAK twin: battle_playable_compat_stubs.c defines weak
# stand-ins for these makers, --gc-sections drops the unused body, and the
# DWARF entry survives it, so gdb offers a live address and a dead one. nm
# reports only what the linker actually kept -- there is exactly one T for
# each of these names -- which is why resolution belongs here and not in gdb.
# `W` is accepted deliberately: a surviving weak definition is still the code
# that runs. `Select-Object -First 1` is gone on purpose; it would have made a
# genuine duplicate silently pick one.
function Get-TextAddress([string]$Name) {
    # NOT $matches: that is a PowerShell automatic variable holding regex
    # results, and -match inside the filter overwrites it mid-pipeline.
    $found = @($nm_lines |
        Where-Object {
            $_ -match ('^([0-9a-fA-F]{8})\s+[TtWw]\s+' +
                [regex]::Escape($Name) + '$')
        })
    if ($found.Count -eq 0) {
        throw "$Name has no text symbol in $elf."
    }
    if ($found.Count -gt 1) {
        throw ("$Name has $($found.Count) text symbols in ${elf}: " +
            ($found -join ' | '))
    }
    $address = '0x' + ($found[0] -split '\s+')[0]
    if ($address -eq '0x00000000') {
        throw "$Name resolved to a null address in $elf."
    }
    return $address
}

$impact_maker_address = Get-TextAddress 'efManagerImpactWaveMakeEffect'
# Every stop site in this probe is an nm address. Breaking at the raw entry
# also fixes the argument reads: gdb never skipped a prologue for these
# functions anyway (it placed breakpoint 3 at 0x2095ca8, byte-identical to
# nm's entry), so a named parameter here was already being read before its
# DWARF location was live. At the entry the AAPCS guarantees r0-r3 hold the
# arguments, so $r0 is the only reading that is true by construction.
$explode_maker_address = Get-TextAddress 'efManagerDeadExplodeMakeEffect'
$sparkle_maker_address = Get-TextAddress 'efManagerSparkleWhiteDeadMakeEffect'
$rebirth_maker_address = Get-TextAddress 'efManagerRebirthHaloMakeEffect'
$damage_maker_address = Get-TextAddress 'efManagerDamageNormalLightMakeEffect'
$star_update_address = Get-TextAddress 'ftCommonDeadUpStarProcUpdate'
$frame_marker_address = Get-TextAddress 'ndsBattlePlayableFrameCompleteMarker'
$results_confetti_address = Get-TextAddress 'mnVSResultsMakeConfetti'
$impact_display_symbols = @(
    'efManagerImpactWaveProcDisplay',
    'dEFManagerImpactWavePrimColorR',
    'dEFManagerImpactWavePrimColorG',
    'dEFManagerImpactWavePrimColorB',
    'dEFManagerImpactWaveEnvColorR',
    'dEFManagerImpactWaveEnvColorG',
    'dEFManagerImpactWaveEnvColorB'
)
$missing_impact_display_symbols = @($impact_display_symbols |
    Where-Object { $symbols -notcontains $_ })
$has_impact_display_evidence = ($missing_impact_display_symbols.Count -eq 0)
$impact_display_address = if ($has_impact_display_evidence) {
    Get-TextAddress 'efManagerImpactWaveProcDisplay'
} else {
    $null
}
$has_m3_postarm_counter =
    ($symbols -contains 'gNdsRendererM3PostArmFailureCount')
$m3_before_command = if ($has_m3_postarm_counter) {
    'set $m3_postarm_before = gNdsRendererM3PostArmFailureCount'
} else {
    'set $m3_postarm_before = -1'
}
$m3_after_command = if ($has_m3_postarm_counter) {
    'set $m3_postarm_after = gNdsRendererM3PostArmFailureCount'
} else {
    'set $m3_postarm_after = -1'
}

# NOT $Pid: that is a PowerShell automatic read-only variable, and binding a
# parameter to it fails the whole script before the emulator ever launches.
function New-CaptureCommand(
    [string]$Name,
    [int]$EmulatorProcessId,
    [string]$Prefix
) {
    # THE PREFIX IS MANDATORY, AND THAT IS THE WHOLE FIX. All three call sites
    # passed only two arguments, so PowerShell bound $Prefix to '' without a
    # word of complaint and every run -- labelled or not -- wrote
    # artifacts/visibility/_ko-burst-probe.png. The overwrite guard above was
    # meanwhile checking <label>_ko-burst-probe.png, a name nothing ever wrote,
    # so a labelled run silently clobbered the previous labelled run's evidence
    # while the guard reported the path as free. Throwing here makes the broken
    # call inexpressible instead of merely documented.
    if ([string]::IsNullOrWhiteSpace($Prefix)) {
        throw "New-CaptureCommand '$Name' called without an evidence prefix."
    }
    return ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' +
        $capture_helper + '" -EmulatorProcessId ' + $EmulatorProcessId +
        ' -Output "artifacts/visibility/' + $Prefix + '_' + $Name + '.png"')
}

# EVERY CAPTURE STATES ITS OWN TIME AND ITS OWN TEXTURE STATE.
# The 2026-08-04 run wrote three PNGs whose only timestamp was the HUD drawn
# inside the picture, and two were then read as "match start" from that HUD
# alone -- an inference a capture cannot support, because the KO path can fire
# frames before the HUD reflects it. Frame counter, clock, stock and damage are
# code-published globals, so a capture's position in the match becomes a number
# in the artifact rather than a reading of the artifact.
#
# The texture rows answer the one question a screenshot cannot: after the
# corruption appears, is the renderer STILL ATTEMPTING to bind and upload and
# being refused (bind/upload keep climbing, or reject climbs), or has it
# STOPPED ATTEMPTING (every counter flat)? Those two point at different owners
# -- an allocator/VRAM bound versus a latched validation path -- and no amount
# of looking at the corrupted frame distinguishes them.
function New-StateCommand([string]$Name) {
    return ('printf "KOSHOT tag=' + $Name + ' frame=%u clock=%u ' +
        'p0stock=%u p0dmg=%u p1stock=%u p1dmg=%u ' +
        'explode=%d star=%d rebirth=%d death_seen=%d ' +
        'staticviol=%u staticteardown=%u staticprepfail=%u staticprep=%u ' +
        'staticprepared=%u staticarm=%u staticpin=%u staticowner=%#x ' +
        'staticseen=%#x staticspan=%u staticfirst=%#x staticend=%#x ' +
        'vramreset=%u fbind=%u fready=%u freject=%u fupload=%u ' +
        'eready=%u ereject=%u atlasfail=%u atlasprep=%u ' +
        'memohit=%u memomiss=%u memofill=%u memostale=%u memoverify=%u ' +
        'fence=%u fencefirstframe=%u fenceclass=%u notex=%u\n", ' +
        'gNdsFrameCounter, gNdsBattleTextHudTimeSeconds, ' +
        'gNdsBattleTextHudP0Stock, gNdsBattleTextHudP0Damage, ' +
        'gNdsBattleTextHudP1Stock, gNdsBattleTextHudP1Damage, ' +
        '$explode_calls, $star_calls, $rebirth_calls, $death_seen, ' +
        'gNdsRendererBattleStaticTextureViolationCount, ' +
        'gNdsRendererBattleStaticTextureTeardownCount, ' +
        'gNdsRendererBattleStaticTexturePrepareFailCount, ' +
        'gNdsRendererBattleStaticTexturePrepareCount, ' +
        'gNdsRendererBattleStaticTexturePreparedCount, ' +
        'gNdsRendererBattleStaticTextureArmCount, ' +
        'gNdsRendererBattleStaticTexturePinnedHitCount, ' +
        'gNdsRendererBattleStaticTextureOwnerMask, ' +
        'gNdsRendererBattleStaticTextureSeenMask, ' +
        'gNdsRendererBattleStaticTextureAllocationSpanBytes, ' +
        'gNdsRendererBattleStaticTextureFirstAddress, ' +
        'gNdsRendererBattleStaticTextureEndAddress, ' +
        'gNdsRendererSceneTextureVramResetCount, ' +
        'gNdsFighterDLAllDrawHardwareTextureBindCount, ' +
        'gNdsFighterDLAllDrawHardwareTextureReadyCount, ' +
        'gNdsFighterDLAllDrawHardwareTextureRejectCount, ' +
        'gNdsFighterDLAllDrawHardwareTextureUploadCount, ' +
        'gNdsEffectRendererTextureReadyCount, ' +
        'gNdsEffectRendererTextureRejectCount, ' +
        'gNdsRendererParticleAtlasFailCount, ' +
        'gNdsRendererParticleAtlasPrepareCount, ' +
        'gNdsR2TexMemoHitCount, gNdsR2TexMemoMissCount, ' +
        'gNdsR2TexMemoFillCount, gNdsR2TexMemoStaleCount, ' +
        'gNdsR2TexMemoVerifyFail, ' +
        'gNdsRendererBattleTextureFenceCounts, ' +
        'gNdsRendererBattleTextureFenceFirstFrame, ' +
        'gNdsRendererBattleTextureFenceFirstClassPlus1, ' +
        'gNdsFighterDisplayContractNoTextureCount')
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
        'set $forced_damage = 0',
        'set $vis_before = -1',
        'set $vis_dropped = -1',
        'set $vis_mask = 0',
        'set $vis_active = -1',
        'set $vis_after = -1',
        'set $slot2_count = -1',
        'set $slot2_size = -1.0',
        'set $slot0_count = -1',
        'set $slot0_noxf = -1',
        'set $slot0_noxf_absmax = -1.0',
        'set $shot_explode = 0',
        'set $shot_star = 0',
        'set $shot_rebirth = 0',
        'set $shot_late = 0',
        # Set ONLY by the two death makers. Nothing in the opening spawn can
        # raise it, which is the whole point -- see the rebirth gate below.
        'set $death_seen = 0',
        'set $explode_x = 0.0',
        'set $explode_y = 0.0',
        'set $explode_z = 0.0',
        'set $explode_kind = -1',

        # Each maker arms ONE delayed stop and disarms itself, so the whole
        # match costs three stops rather than one per frame. `ignore $bpnum N`
        # is the delay; the tbreak is temporary so it never fires twice.
        ('break *' + $explode_maker_address),
        'commands',
        'silent',
        'set $explode_calls = $explode_calls + 1',
        'set $death_seen = 1',
        # ROW 2 FOR FREE, and these are the exact quantities that row asks for:
        # "it doesn't seem to play at the players death location off screen,
        # check x,y,z coords". At the raw entry AAPCS puts the Vec3f* in r0 and
        # the kind in r2, so this reads the argument the caller actually passed
        # rather than a local whose DWARF location is not live yet.
        'set $explode_x = ((Vec3f *)$r0)->x',
        'set $explode_y = ((Vec3f *)$r0)->y',
        'set $explode_z = ((Vec3f *)$r0)->z',
        'set $explode_kind = $r2',
        'if $shot_explode == 0',
        'set $shot_explode = 1',
        ('tbreak *' + $frame_marker_address),
        'ignore $bpnum 5',
        'commands',
        'silent',
        # ROW 9, CONFIRMED AT RUNTIME RATHER THAN INFERRED. The burst's particle
        # half lands in alloc-link slot 2 -- LBPARTICLE_MASK_GENLINK(1) is 16 and
        # lbparticle.c:324 files under bank_id >> 3 -- and all eight of its
        # scripts (efcommon 42-45 and 60-63, named by dEFManagerDeadExplodeGenID)
        # carry size 100.0 in the HEADER with no size opcode in bytecode. That is
        # exactly the set the unswapped header word turned into a 5.7e-41
        # denormal, so before that fix the burst's particles were submitted,
        # counted, and sub-pixel, leaving only the DObj/matanim half on screen.
        # Reading the live size here says whether they now carry 100.
        # ROW 4, AND IT IS A DIFFERENT QUESTION FROM THE ONE I ANSWERED FIRST.
        # spark_absmax measures the position HANDED TO the maker, which came
        # back clean at 1,344 -- but ndsParticleTransformForDraw falls back to
        # raw pc->pos when pc->xf is NULL, and for a script-made particle that
        # is the SCRIPT-LOCAL origin, i.e. the middle of the stage. An effect
        # spawned correctly at a fighter and drawn at world zero is exactly
        # "stray VFX played across the stage when attacks are landed", and no
        # measurement of the maker's argument can see it. Slot 0 is where the
        # ungenlinked combat effects live.
        'set $slot0_count = 0',
        'set $slot0_noxf = 0',
        'set $slot0_noxf_absmax = 0.0',
        'set $p = sLBParticleStructsAllocLinks[0]',
        'while $p != 0',
        'set $slot0_count = $slot0_count + 1',
        'if $p->xf == 0',
        'set $slot0_noxf = $slot0_noxf + 1',
        'if $p->pos.x > $slot0_noxf_absmax',
        'set $slot0_noxf_absmax = $p->pos.x',
        'end',
        'if -$p->pos.x > $slot0_noxf_absmax',
        'set $slot0_noxf_absmax = -$p->pos.x',
        'end',
        'end',
        'set $p = $p->next',
        'end',
        'set $slot2_count = 0',
        'set $slot2_size = 0.0',
        'set $p = sLBParticleStructsAllocLinks[2]',
        'while $p != 0',
        'set $slot2_count = $slot2_count + 1',
        'if $p->size > $slot2_size',
        'set $slot2_size = $p->size',
        'end',
        'set $p = $p->next',
        'end',
        (New-StateCommand 'ko-burst'),
        (New-CaptureCommand 'ko-burst-probe' $emulator.Id $evidence_prefix),
        'continue',
        'end',
        'end',
        # BUGS ROW 6, THE LATE ARM. The owner reports that the post-death
        # texture loss never recovers for the rest of the match. A cache that
        # re-resolves every frame would refill on the next one, so "still
        # broken three seconds later" is the measurement that separates
        # eviction pressure from a state the renderer cannot leave. Armed as a
        # sibling of the burst arm rather than nested inside it: this file
        # already nests one commands block inside another, and a third level is
        # not worth discovering the limits of mid-measurement.
        'if $shot_late == 0',
        'set $shot_late = 1',
        ('tbreak *' + $frame_marker_address),
        'ignore $bpnum 180',
        'commands',
        'silent',
        (New-StateCommand 'post-death-late'),
        (New-CaptureCommand 'post-death-late-probe' $emulator.Id $evidence_prefix),
        'continue',
        'end',
        'end',
        'continue',
        'end',

        ('break *' + $sparkle_maker_address),
        'commands',
        'silent',
        'set $star_calls = $star_calls + 1',
        'set $death_seen = 1',
        'set $star_x = ((Vec3f *)$r0)->x',
        'set $star_y = ((Vec3f *)$r0)->y',
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
        ('tbreak *' + $frame_marker_address),
        'ignore $bpnum 5',
        'commands',
        'silent',
        (New-StateCommand 'star-ko'),
        (New-CaptureCommand 'star-ko-probe' $emulator.Id $evidence_prefix),
        'continue',
        'end',
        'end',
        'continue',
        'end',

        ('break *' + $rebirth_maker_address),
        'commands',
        'silent',
        'set $rebirth_calls = $rebirth_calls + 1',
        # READ THE VISUAL COUNTERS HERE, NOT AT THE END. The first run of this
        # reported visual_created=0 against rebirth_calls=2 and that looked like
        # the substitute never running -- but the printf happens at
        # mnVSResultsMakeConfetti, which is INSIDE Results, and the scene change
        # has already cleared them by then. structs_max read 0 in the same line
        # for the same reason. This is the non-sticky-counter trap that
        # sGCCommonsMaxNum is on record for; sampling one frame after the maker
        # is the only reading that means anything.
        'set $vis_before = gNdsVisualEffectCreateCount',
        'set $vis_dropped = gNdsVisualEffectDropCount',
        'set $vis_mask = gNdsVisualEffectKindMask',
        # GATED ON A DEATH HAVING ACTUALLY HAPPENED, WHICH THE MAKER ALONE DOES
        # NOT PROVE. Both fighters begin the match standing on revival
        # platforms, so this maker fires during the OPENING SPAWN: the
        # 2026-08-04 run read rebirth_calls=2 and armed its capture on the
        # first of them, landing at match start with both fighters at 0% and
        # full stock. That is a picture of the start of the match filed as
        # evidence about respawning. $death_seen is raised only by the explode
        # and star-KO callbacks, so this now waits for a real death.
        'if $shot_rebirth == 0 && $death_seen == 1',
        'set $shot_rebirth = 1',
        ('tbreak *' + $frame_marker_address),
        'ignore $bpnum 23',
        'commands',
        'silent',
        # AFTER the halo exists, not at the maker's entry. Reading
        # gNdsVisualEffectActiveCount in the maker's own callback reports the
        # count BEFORE this halo is constructed, which is how a lifetime change
        # from 8 to 390 frames produced a bit-identical run and briefly looked
        # like it had done nothing.
        'set $vis_active = gNdsVisualEffectActiveCount',
        'set $vis_after = gNdsVisualEffectCreateCount',
        (New-StateCommand 'rebirth-halo'),
        (New-CaptureCommand 'rebirth-halo-probe' $emulator.Id $evidence_prefix),
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
        ('break *' + $star_update_address),
        'commands',
        'silent',
        'set $star_updates = $star_updates + 1',
        # $ftp, NOT $fp. gdb reserves $fp for the frame-pointer REGISTER, so
        # assigning to it fails with "Left operand of assignment is not an
        # lvalue" -- a message that reads like the cast is wrong rather than
        # the name. And the struct has to be rebuilt from the GObj because the
        # function's own `fp` local is "value has been optimized out" at entry
        # under -Os; ftGetStruct is just ((FTStruct *)gobj->user_data.p).
        # And $r0, NOT `fighter_gobj`, for the same reason one step further on.
        # The stop is at the raw entry, so the parameter's DWARF location is
        # not live yet -- which is exactly what the note above describes for
        # the function's own `fp`. AAPCS puts the first argument in r0 there
        # unconditionally, so this is the reading that cannot be optimized out.
        'set $ftp = (FTStruct *)((GObj *)$r0)->user_data.p',
        'set $star_phase = $ftp->motion_vars.flags.flag1',
        'set $star_wait = $ftp->status_vars.common.dead.wait',
        'set $star_vy = $ftp->physics.vel_air.y',
        'set $star_posy = ((DObj *)((GObj *)$r0)->obj)->translate.vec.f.y',
        'if $star_vy > $star_vymax',
        'set $star_vymax = $star_vy',
        'end',
        'continue',
        'end',

        # BUGS row 4, the same run for free: where the hit sparks are handed.
        # "Stray VFX across the stage" would show as an |x| far outside a
        # fighter's reach.
        ('break *' + $damage_maker_address),
        'commands',
        'silent',
        'set $spark_calls = $spark_calls + 1',
        'if ((Vec3f *)$r0)->x > $spark_absmax',
        'set $spark_absmax = ((Vec3f *)$r0)->x',
        'end',
        'if -((Vec3f *)$r0)->x > $spark_absmax',
        'set $spark_absmax = -((Vec3f *)$r0)->x',
        'end',
        'continue',
        'end',

        # EVERY STOP SITE, PRINTED, BEFORE THE MATCH IS ALLOWED TO RUN. This is
        # the tripwire for the failure this probe just had: three breakpoints
        # reported `at 0x0 ... (2 locations)` and the run continued to produce
        # plausible counters. The addresses above come from nm so they can no
        # longer be ambiguous, but the transcript is what proves it, and the
        # PowerShell check after the run fails the whole probe on `at 0x0` or a
        # multi-location entry rather than leaving it to be noticed.
        'info breakpoints',

        # FORCE AN EARLY KO. Two level-3 CPUs starting from 0% reach their
        # first KO near the end of the minute, and the one this probe caught
        # landed ON the GAME SET frame -- so the match ended before anything
        # respawned and rebirth_calls and explode_calls both stayed 0. Those
        # are BUGS rows 3 and 9 and neither is observable without an EARLY
        # death. Damage is the honest lever: it changes when the KO happens and
        # nothing about what the KO path then does, which is the same argument
        # as forcing Whispy's wind countdown.
        ('tbreak *' + $frame_marker_address),
        'ignore $bpnum 240',
        'continue',
        # THE NEGATIVE CONTROL FOR ROW 6, AND IT COSTS ONE STOP. Same match,
        # same scene, same camera, one frame before the death is forced. If the
        # post-death captures show missing textures and this one does not, the
        # comparison is inside a single run on a single ROM -- no cross-build
        # floor to argue about and no second fight to align. If this frame is
        # ALREADY broken, the death is not the trigger and row 6's premise
        # moves.
        (New-StateCommand 'pre-death-baseline'),
        (New-CaptureCommand 'pre-death-baseline-probe' $emulator.Id $evidence_prefix),
        # Writing fp->damage was tried first and the run came back BIT-IDENTICAL:
        # fighter.h:3309's `damage` is the port-only extension field, not what
        # ftCommonDeadCheckBounds reads. The bound test is on POSITION
        # (ftcommondead.c:629, `pos->x < map_bound_left`), so moving the fighter
        # past the side blast zone is both simpler and unambiguous -- and it
        # picks the SIDE KO deliberately, because that is the one that runs
        # efManagerDeadExplodeMakeEffect. The upward bound at :635 gives the
        # star KO, which is already covered.
        'set $g = gGCCommonLinks[3]',
        'if $g != 0',
        'set ((DObj *)$g->obj)->translate.vec.f.x = gMPCollisionGroundData->map_bound_left - 500',
        'set $forced_damage = 1',
        'end',

        # The run ends where the match does, not on a frame count.
        ('tbreak *' + $results_confetti_address),
        'continue',
        (New-StateCommand 'results'),

        ('printf "KOVFX explode_calls=%d star_calls=%d rebirth_calls=%d ' +
            'star=%f,%f cam_top=%d map_top=%d ' +
            'star_updates=%d phase=%d wait=%d vy=%f vymax=%f posy=%f forced_damage=%d ' +
            'death_seen=%d explode_pos=%f,%f,%f explode_kind=%d ' +
            'victim_slot=%d victim_stock=%d->%d ' +
            'spark_calls=%d spark_absmax=%f ' +
            'burst_slot2_count=%d burst_slot2_size=%f ' +
            'slot0_count=%d slot0_noxf=%d slot0_noxf_absmax=%f ' +
            'at_rebirth_created=%d dropped=%d mask=%#x active_after=%d created_after=%d ' +
            'at_results_created=%u dropped=%u kindmask=%#x ' +
            'miss=%u emit=%u structs_max=%u\n", ' +
            '$explode_calls, $star_calls, $rebirth_calls, ' +
            '$star_x, $star_y, $cam_top, $map_top, ' +
            '$star_updates, $star_phase, $star_wait, $star_vy, $star_vymax, $star_posy, $forced_damage, ' +
            '$death_seen, $explode_x, $explode_y, $explode_z, $explode_kind, ' +
            'gNdsFighterBattlePlayableVictimSlot, ' +
            'gNdsFighterBattlePlayableVictimStockStart, ' +
            'gNdsFighterBattlePlayableVictimStockFinal, ' +
            '$spark_calls, $spark_absmax, ' +
            '$slot2_count, $slot2_size, ' +
            '$slot0_count, $slot0_noxf, $slot0_noxf_absmax, ' +
            '$vis_before, $vis_dropped, $vis_mask, $vis_active, $vis_after, ' +
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

    # FAIL LOUDLY ON A BAD RESOLVE -- AFTER THE ARTIFACT IS ON DISK, because a
    # probe that trips its own integrity check is precisely the run whose
    # transcript someone needs to read. On 2026-08-04 three breakpoints
    # resolved to 0x0 with "(2 locations)" and the run reported counters
    # anyway; nothing in the pipeline treated that as a failure, so it was
    # found by eye a session later.
    $capture_text = ($capture | Out-String)
    $bad_resolves = @([regex]::Matches($capture_text,
        '(?m)^.*reakpoint \d+ at 0x0\b.*$') |
        ForEach-Object { $_.Value.Trim() })
    $multi_locations = @([regex]::Matches($capture_text,
        '(?m)^.*\(\d+ locations\).*$') |
        ForEach-Object { $_.Value.Trim() })
    if (($bad_resolves.Count + $multi_locations.Count) -ne 0) {
        throw ("breakpoint resolution failed; artifact kept at $artifact -- " +
            (@($bad_resolves + $multi_locations) -join ' / '))
    }

    $capture | Select-String -Pattern 'KOVFX|KOSHOT'
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
