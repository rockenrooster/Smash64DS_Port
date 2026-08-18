[CmdletBinding()]
param(
    [string]$Build = 'build-p2-shell',
    [string]$Target = 'smash64ds-p2-shell-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 1800,
    # Stops, not frames, and it must not exceed what the ROM produces (the
    # trailing `continue` below otherwise waits for a stop that never comes and
    # the probe exits by timeout). The breakpoint is ndsSceneManagerEnter, so
    # one stop is one scene entry. A one-pass shipping-configuration run makes
    # ELEVEN -- Startup, Title, ModeSelect, VSMode, PlayersVS, Maps, PlayersVS
    # (the stage select's own B back-out), Maps, VSBattle, VSResults, and then
    # PlayersVS again because P2-1g's scripted START closes the lap through the
    # source's own Results exit. Nine was the P2-1f ceiling for a reason that no
    # longer holds: the run used to end at a Results screen nothing could leave,
    # so a tenth stop depended on how the match ENDED (Sudden Death or Results).
    # It still does, but only for stop 10's KIND -- and the eleventh stop is
    # what makes the realtime arm carry the rematch evidence too.
    [ValidateRange(2, 64)][int]$Hits = 11,
    [string]$Artifact = ''
)

# THE VS SHELL'S SHIPPING-CONFIGURATION PROBE, and the phase's cadence
# instrument. One scripted pass through all six screens and the REAL one-minute
# match at NDS_HARNESS_FAST_LOGIC=0, out of a ROM built NDS_P2_MENU_SHELL=1 --
# so every tick figure here is a shipping-cadence figure, unlike the loop
# verifier's fast-logic arm (scripts/verify-p2-shell-loop.ps1), which is the
# scene-boundary instrument and no cadence surface at all.
#
# IT WAS probe-p2-1f-sss.ps1 UNTIL P2-1g. Each row of P2-1 renamed its
# predecessor because the flow it walked stopped existing; the phase is now
# closed, so the name is the phase's. The last flow change is this row's own:
# Results no longer parks at the end of the pass, because the walk presses
# START on it, so the run continues back to the character select and the
# realtime arm carries the rematch evidence (MSREMATCH) as well as the
# fast-logic loop arm.
#
# WHY THIS BREAKPOINT. Every figure here is cumulative, so the LAST line of a
# run is the whole window and the intermediate lines are the timeline. A stop
# per scene ENTRY is the cheapest site that produces that timeline: a per-frame
# stop would be one host round trip per presented frame, which this repository
# has already paid for once. ndsSceneManagerEnter also runs before the arena is
# re-initialised, so the arena ring it prints is complete for every entry that
# has already exited.
#
# WHAT THE NEW NUMBERS MEAN.
#   CSSPOS    the cursor in the SOURCE's own 320x240 frame -- the frame every
#             hit test in mnplayersvs.c is written in -- and its status
#             (0 Pointer / 1 Grab / 2 Hover).
#   CSSACT    token grabs, drops, REFUSED drops (the token was over a locked
#             cell), recalls, player-kind toggles, CPU-level changes, accepted
#             and refused STARTs, and back-outs.
#   CSSCOMMIT what the screen wrote into the match descriptor, one word a slot,
#             ((fkind << 16) | (pkind << 8) | cpu level). pkind is
#             0 HUMAN / 1 CPU / 2 empty and fkind 0 is Mario, 1 Fox, 0x0b none.
#   CSSXFER   the same four slots read back OUT of gSCManagerTransferBattleState
#             -- the struct the battle actually consumes -- so the descriptor
#             end-to-end claim is a comparison and not an assertion.
#   CSSLIVE   and the same fields out of the LIVE battle state once the match
#             owns one, which is the P2-1a CPU_CONFIG pin read at a scene stop.
#   CSSCUE    cue requests this screen made, by the source's own FGM id. Paired
#             with MSMISS this separates "the seam never asked" from "the pack
#             has no sample" -- P2-1e-1 packed the four ids this screen asks
#             for (121/127/167/512), so MSMISS now reads ring=0 here.
#   MSBGM     ...unsupported is the same split on the BGM side: BGM 10
#             (BattleSelect) joined the assets at P2-1e-1, so unsupported
#             reads 0 and `track=10` engages at the CSS's own scene stop. The
#             stage select starts NO track (mnMapsFuncStart has no BGM call),
#             so `calls` must be FLAT across its scene stops -- that flatness
#             is the evidence for "the CSS's music plays through", not a
#             comment.
#
# WHAT P2-1f ADDS.
#   SSSPOS    the cursor's SLOT (0..9, mnMapsGetGroundKind's numbering) and the
#             ground kind it names -- 0xde is RANDOM, the source's own spelling.
#   SSSACT    cursor moves that changed the slot, direction presses the lock
#             table REFUSED, A/START confirms and B back-outs. `blocked` being
#             non-zero is the proof the locked cells are inert rather than
#             absent.
#   SSSCOMMIT the stage write. `n` counts mnMapsSaveSceneData-equivalents,
#             `gkind` is the ground it RESOLVED to and `slot` is what the
#             cursor NAMED -- the two differ on the random path (slot 0xde,
#             gkind 6) and agree on the direct one, which is what makes them a
#             control that can fail. `rand`/`fallback` say which arm of
#             mnMapsSaveSceneData's random pick resolved: with ONE unlocked
#             ground the source's no-repeat clause is unsatisfiable, so
#             `fallback` is the expected reading until P2-4 lands a second.
#   SSSCFG    the descriptor's own stage field and the scene data's, read back
#             independently, so "the loader consumed it" is a comparison.
#
# Nothing here writes guest memory.

$ErrorActionPreference = 'Stop'
$scripts = Split-Path -Parent $PSScriptRoot
$root = Split-Path -Parent $scripts
. (Join-Path $scripts 'lib\melonds.ps1')
. (Join-Path $scripts 'lib\gdb-markers.ps1')
. (Join-Path $scripts 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-shell.txt')
}

$buildConfig = Join-Path (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
    throw "p2-shell probe: $Build has no nds_build_config.h; refusing stale evidence."
}
$configText = Get-Content -LiteralPath $buildConfig -Raw
foreach ($flag in @('NDS_P2_UI_KIT', 'NDS_P2_MENU_SHELL')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    if ((-not $m.Success) -or ($m.Groups[1].Value -eq '0')) {
        # A run against a shell-less ROM would report every counter as
        # "missing symbol" and read as a broken shell rather than a wrong build.
        throw "p2-shell probe: $Build was built with $flag off; nothing to read."
    }
    Write-Output ("build config: {0}={1}" -f $flag, $m.Groups[1].Value)
}
foreach ($flag in @('NDS_P2_MENU_WALK', 'NDS_R2_SCENE_LOOP_WALK',
                    'NDS_HARNESS_FAST_LOGIC', 'NDS_DEV_SCENE_HARNESS')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    $value = if ($m.Success) { $m.Groups[1].Value } else { 'absent' }
    Write-Output ("build config: {0}={1}" -f $flag, $value)
}

$required = @(
    'ndsSceneManagerEnter',
    'gNdsMenuShellScreen', 'gNdsMenuShellEnterCount', 'gNdsMenuShellExitCount',
    'gNdsMenuShellFrames', 'gNdsMenuShellWorkHist', 'gNdsMenuShellWorkMax',
    'gNdsMenuShellVBlankHist', 'gNdsMenuShellVBlankMax',
    'gNdsMenuShellWorkMaxFrame', 'gNdsMenuShellWorkMaxCues',
    'gNdsMenuShellEnterTicks', 'gNdsMenuShellTransitionRing',
    'gNdsMenuShellTransitionCount', 'gNdsMenuShellInputRing',
    'gNdsMenuShellInputCount', 'gNdsMenuShellDeniedCount',
    'gNdsMenuShellCommitCount', 'gNdsMenuShellCommitRule',
    'gNdsMenuShellCommitTime', 'gNdsMenuShellCommitStocks',
    'gNdsMenuShellWalkSteps', 'gNdsMenuShellWalkLoops',
    # P2-1e.
    'gNdsMenuShellCssCursorX', 'gNdsMenuShellCssCursorY',
    'gNdsMenuShellCssCursorStatus', 'gNdsMenuShellCssGrabCount',
    'gNdsMenuShellCssDropCount', 'gNdsMenuShellCssDropRefuseCount',
    'gNdsMenuShellCssRecallCount', 'gNdsMenuShellCssKindToggleCount',
    'gNdsMenuShellCssLevelChangeCount', 'gNdsMenuShellCssStartCount',
    'gNdsMenuShellCssStartDeniedCount', 'gNdsMenuShellCssBackCount',
    'gNdsMenuShellCssCommitCount', 'gNdsMenuShellCssCommitSlot',
    'gNdsMenuShellCssCueCount', 'gNdsMenuShellCssCueLastId',
    'gNdsMenuShellCssAnnounceCount',
    # P2-1f.
    'gNdsMenuShellSssCursorSlot', 'gNdsMenuShellSssCursorGkind',
    'gNdsMenuShellSssMoveCount', 'gNdsMenuShellSssBlockedCount',
    'gNdsMenuShellSssConfirmCount', 'gNdsMenuShellSssBackCount',
    'gNdsMenuShellSssCommitCount', 'gNdsMenuShellSssCommitGkind',
    'gNdsMenuShellSssCommitSlotGkind', 'gNdsMenuShellSssRandomCount',
    'gNdsMenuShellSssRandomFallbackCount', 'gNdsMenuShellSssCueCount',
    'gNdsMenuShellSssCueLastId',
    'gNdsMatchConfig',
    # P2-1g rematch seam.
    'gNdsVSResultsRematchCount', 'gNdsMenuShellWalkResultsPressCount',
    'gNdsVSResultsPadMask', 'gNdsVSResultsInputSeenMask',
    'gNdsVSResultsInputTapMask', 'gNdsVSResultsInputPollCount',
    'gNdsUiKitEnterCount', 'gNdsUiKitEnterRejectCount', 'gNdsUiKitExitCount',
    'gNdsUiKitPackOpenCount', 'gNdsUiKitPackBytesLoaded', 'gNdsUiKitPackHash',
    'gNdsUiKitPackHashMismatchCount', 'gNdsUiKitPackReadFailCount',
    'gNdsUiKitTextComposeCount', 'gNdsUiKitTextOverflowCount',
    'gNdsUiKitCommitCount', 'gNdsUiKitVisibleObjectCount',
    'gNdsUiKitSfxRequestCount', 'gNdsUiKitSfxLastId',
    'gNdsSceneManagerEnterCount', 'gNdsSceneManagerExitCount',
    'gNdsSceneManagerRejectCount', 'gNdsSceneManagerUnregisteredEnterCount',
    'gNdsSceneManagerArenaMismatchCount', 'gNdsSceneManagerRingKind',
    'gNdsSceneManagerRingArenaHigh', 'gNdsSceneManagerRingArenaFree',
    'gNdsSceneManagerArenaBase', 'gNdsSceneManagerArenaSize',
    'gNdsAudioBgmPlaying', 'gNdsAudioBgmTrackID', 'gNdsAudioBgmPlayCalls',
    'gNdsAudioBgmIsLooping', 'gNdsAudioBgmStreamBytes',
    'gNdsAudioBgmModeSelectPlayCount', 'gNdsAudioBgmPupupuPlayCount',
    'gNdsAudioBgmWinMarioPlayCount', 'gNdsAudioBgmWinFoxPlayCount',
    'gNdsAudioBgmResultsPlayCount', 'gNdsAudioBgmUnsupportedTrackCount',
    # P2-1g cadence attribution.
    'gNdsAudioBgmRefillCount', 'gNdsAudioBgmStreamedBytes',
    'gNdsAudioBgmStreamBytesPerSecond', 'gNdsAudioBgmElapsedFrames',
    'gNdsAudioBgmLoopCount', 'gNdsAudioFgmPlayCalls',
    'gNdsAudioFgmPlayFailCount', 'gNdsAudioFgmLoaded'
)
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("p2-shell probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
# The FGM miss ring is the SFX seam's negative half: the shell asks with the
# source's own cue ids and this ring says which of them the pack carries.
# Before P2-1e-1, three of the character select's own ids were NOT packed
# (121 MarioDash, 127 SamusDash, 167 PlayerSlotWhoosh) plus 512 FreeForAll,
# which is what row P2-1e-1's scope was derived from (MSMISS ring=4). P2-1e-1
# packed all four, so an EMPTY ring (ring=0) is now the expected reading.
$hasMissRing = ($symbols -contains 'gNdsAudioFgmMissRingIDs') -and
    ($symbols -contains 'gNdsAudioFgmMissRingCounts') -and
    ($symbols -contains 'gNdsAudioFgmMissRingCount')
if (-not $hasMissRing) {
    Write-Output ('note: gNdsAudioFgmMissRing* are absent from this build; ' +
        'the MSMISS line will read `absent`, not 0.')
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.p2-shell.stdout.log'
$stderr = Join-Path $log_dir 'melonds.p2-shell.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

function New-MenuScreenPrintf {
    param([int]$Screen)
    $s = $Screen
    @(
        ('printf "MSWORK' + $s + ' %d %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n", $n' +
         (0..15 | ForEach-Object { ", gNdsMenuShellWorkHist[$s][$_]" }) -join ''),
        ('printf "MSVB' + $s + ' %d %u %u %u %u max=%u\n", $n, ' +
         "gNdsMenuShellVBlankHist[$s][0], gNdsMenuShellVBlankHist[$s][1], " +
         "gNdsMenuShellVBlankHist[$s][2], gNdsMenuShellVBlankHist[$s][3], " +
         "gNdsMenuShellVBlankMax[$s]")
    )
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
        'set print elements 128',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'set $n = 0',
        'break ndsSceneManagerEnter',
        'commands',
        'silent',
        'set $n = $n + 1',
        'printf "MSSCENE %d curr=%u prev=%u enters=%u exits=%u rej=%u unreg=%u mism=%u\n", $n, gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsSceneManagerEnterCount, gNdsSceneManagerExitCount, gNdsSceneManagerRejectCount, gNdsSceneManagerUnregisteredEnterCount, gNdsSceneManagerArenaMismatchCount',
        'printf "MSSHELL %d screen=%u splash=%u/%u title=%u/%u mode=%u/%u vs=%u/%u css=%u/%u sss=%u/%u\n", $n, gNdsMenuShellScreen, gNdsMenuShellEnterCount[0], gNdsMenuShellExitCount[0], gNdsMenuShellEnterCount[1], gNdsMenuShellExitCount[1], gNdsMenuShellEnterCount[2], gNdsMenuShellExitCount[2], gNdsMenuShellEnterCount[3], gNdsMenuShellExitCount[3], gNdsMenuShellEnterCount[4], gNdsMenuShellExitCount[4], gNdsMenuShellEnterCount[5], gNdsMenuShellExitCount[5]',
        'printf "MSFRAMES %d f0=%u f1=%u f2=%u f3=%u f4=%u f5=%u\n", $n, gNdsMenuShellFrames[0], gNdsMenuShellFrames[1], gNdsMenuShellFrames[2], gNdsMenuShellFrames[3], gNdsMenuShellFrames[4], gNdsMenuShellFrames[5]',
        'printf "MSMAX %d w0=%u w1=%u w2=%u w3=%u w4=%u w5=%u\n", $n, gNdsMenuShellWorkMax[0], gNdsMenuShellWorkMax[1], gNdsMenuShellWorkMax[2], gNdsMenuShellWorkMax[3], gNdsMenuShellWorkMax[4], gNdsMenuShellWorkMax[5]',
        # P2-1g. THE WORST FRAME'S LABEL, per screen: which presented frame it
        # was and how many FGM play calls that same frame made. This is what
        # turns MSMAX from a number into an attribution -- a max whose `c` is
        # 0 is not an audio frame, whatever anyone suspected.
        'printf "MSMAXAT %d f0=%u/c%u f1=%u/c%u f2=%u/c%u f3=%u/c%u f4=%u/c%u f5=%u/c%u
", $n, gNdsMenuShellWorkMaxFrame[0], gNdsMenuShellWorkMaxCues[0], gNdsMenuShellWorkMaxFrame[1], gNdsMenuShellWorkMaxCues[1], gNdsMenuShellWorkMaxFrame[2], gNdsMenuShellWorkMaxCues[2], gNdsMenuShellWorkMaxFrame[3], gNdsMenuShellWorkMaxCues[3], gNdsMenuShellWorkMaxFrame[4], gNdsMenuShellWorkMaxCues[4], gNdsMenuShellWorkMaxFrame[5], gNdsMenuShellWorkMaxCues[5]',
        'printf "MSENTER %d e0=%u e1=%u e2=%u e3=%u e4=%u e5=%u\n", $n, gNdsMenuShellEnterTicks[0], gNdsMenuShellEnterTicks[1], gNdsMenuShellEnterTicks[2], gNdsMenuShellEnterTicks[3], gNdsMenuShellEnterTicks[4], gNdsMenuShellEnterTicks[5]'
    ) + (New-MenuScreenPrintf -Screen 0) + (New-MenuScreenPrintf -Screen 1) +
        (New-MenuScreenPrintf -Screen 2) + (New-MenuScreenPrintf -Screen 3) +
        (New-MenuScreenPrintf -Screen 4) + (New-MenuScreenPrintf -Screen 5) + @(
        'printf "MSFLOW %d trans=%u input=%u denied=%u commit=%u rule=%u time=%u stocks=%u walk=%u loops=%u\n", $n, gNdsMenuShellTransitionCount, gNdsMenuShellInputCount, gNdsMenuShellDeniedCount, gNdsMenuShellCommitCount, gNdsMenuShellCommitRule, gNdsMenuShellCommitTime, gNdsMenuShellCommitStocks, gNdsMenuShellWalkSteps, gNdsMenuShellWalkLoops',
        'printf "MSTRANS %d %04x %04x %04x %04x %04x %04x %04x %04x\n", $n, gNdsMenuShellTransitionRing[0], gNdsMenuShellTransitionRing[1], gNdsMenuShellTransitionRing[2], gNdsMenuShellTransitionRing[3], gNdsMenuShellTransitionRing[4], gNdsMenuShellTransitionRing[5], gNdsMenuShellTransitionRing[6], gNdsMenuShellTransitionRing[7]',
        # All SIXTEEN, not the first eight P2-1d printed: the character select
        # alone makes fifteen inputs, so half a ring would show half a screen.
        'printf "MSINPUT %d %06x %06x %06x %06x %06x %06x %06x %06x %06x %06x %06x %06x %06x %06x %06x %06x\n", $n, gNdsMenuShellInputRing[0], gNdsMenuShellInputRing[1], gNdsMenuShellInputRing[2], gNdsMenuShellInputRing[3], gNdsMenuShellInputRing[4], gNdsMenuShellInputRing[5], gNdsMenuShellInputRing[6], gNdsMenuShellInputRing[7], gNdsMenuShellInputRing[8], gNdsMenuShellInputRing[9], gNdsMenuShellInputRing[10], gNdsMenuShellInputRing[11], gNdsMenuShellInputRing[12], gNdsMenuShellInputRing[13], gNdsMenuShellInputRing[14], gNdsMenuShellInputRing[15]',
        'printf "CSSPOS %d x=%d y=%d status=%u\n", $n, gNdsMenuShellCssCursorX, gNdsMenuShellCssCursorY, gNdsMenuShellCssCursorStatus',
        'printf "CSSACT %d grab=%u drop=%u refuse=%u recall=%u kind=%u level=%u start=%u denied=%u back=%u\n", $n, gNdsMenuShellCssGrabCount, gNdsMenuShellCssDropCount, gNdsMenuShellCssDropRefuseCount, gNdsMenuShellCssRecallCount, gNdsMenuShellCssKindToggleCount, gNdsMenuShellCssLevelChangeCount, gNdsMenuShellCssStartCount, gNdsMenuShellCssStartDeniedCount, gNdsMenuShellCssBackCount',
        'printf "CSSCOMMIT %d n=%u s0=%06x s1=%06x s2=%06x s3=%06x\n", $n, gNdsMenuShellCssCommitCount, gNdsMenuShellCssCommitSlot[0], gNdsMenuShellCssCommitSlot[1], gNdsMenuShellCssCommitSlot[2], gNdsMenuShellCssCommitSlot[3]',
        'printf "CSSCFG %d s0=%u/%u/%u s1=%u/%u/%u s2=%u/%u/%u s3=%u/%u/%u\n", $n, gNdsMatchConfig.fighters[0].fkind, gNdsMatchConfig.fighters[0].pkind, gNdsMatchConfig.fighters[0].level, gNdsMatchConfig.fighters[1].fkind, gNdsMatchConfig.fighters[1].pkind, gNdsMatchConfig.fighters[1].level, gNdsMatchConfig.fighters[2].fkind, gNdsMatchConfig.fighters[2].pkind, gNdsMatchConfig.fighters[2].level, gNdsMatchConfig.fighters[3].fkind, gNdsMatchConfig.fighters[3].pkind, gNdsMatchConfig.fighters[3].level',
        'printf "CSSXFER %d s0=%u/%u/%u s1=%u/%u/%u s2=%u/%u/%u s3=%u/%u/%u pl=%u cp=%u\n", $n, gSCManagerTransferBattleState.players[0].fkind, gSCManagerTransferBattleState.players[0].pkind, gSCManagerTransferBattleState.players[0].level, gSCManagerTransferBattleState.players[1].fkind, gSCManagerTransferBattleState.players[1].pkind, gSCManagerTransferBattleState.players[1].level, gSCManagerTransferBattleState.players[2].fkind, gSCManagerTransferBattleState.players[2].pkind, gSCManagerTransferBattleState.players[2].level, gSCManagerTransferBattleState.players[3].fkind, gSCManagerTransferBattleState.players[3].pkind, gSCManagerTransferBattleState.players[3].level, gSCManagerTransferBattleState.pl_count, gSCManagerTransferBattleState.cp_count',
        'if gSCManagerBattleState != 0',
        # `gkind` here is the END of the stage chain and the read that can
        # FAIL: scvsbattle.c:500 copies gSCManagerSceneData.gkind into the live
        # battle state at scene start, so this is the ground the match is
        # actually running on, read out of the struct the match owns rather
        # than out of the descriptor that asked for it.
        'printf "CSSLIVE %d s0=%u/%u/%u s1=%u/%u/%u pl=%u cp=%u time=%u gkind=%02x\n", $n, gSCManagerBattleState->players[0].fkind, gSCManagerBattleState->players[0].pkind, gSCManagerBattleState->players[0].level, gSCManagerBattleState->players[1].fkind, gSCManagerBattleState->players[1].pkind, gSCManagerBattleState->players[1].level, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count, gSCManagerBattleState->time_limit, gSCManagerBattleState->gkind',
        'else',
        'printf "CSSLIVE %d none\n", $n',
        'end',
        'printf "CSSCUE %d cues=%u lastid=%u announce=%u\n", $n, gNdsMenuShellCssCueCount, gNdsMenuShellCssCueLastId, gNdsMenuShellCssAnnounceCount',
        'printf "SSSPOS %d slot=%u gkind=%02x enters=%u\n", $n, gNdsMenuShellSssCursorSlot, gNdsMenuShellSssCursorGkind, gNdsMenuShellEnterCount[5]',
        'printf "SSSACT %d move=%u blocked=%u confirm=%u back=%u cues=%u lastid=%u\n", $n, gNdsMenuShellSssMoveCount, gNdsMenuShellSssBlockedCount, gNdsMenuShellSssConfirmCount, gNdsMenuShellSssBackCount, gNdsMenuShellSssCueCount, gNdsMenuShellSssCueLastId',
        'printf "SSSCOMMIT %d n=%u gkind=%02x slot=%02x rand=%u fallback=%u\n", $n, gNdsMenuShellSssCommitCount, gNdsMenuShellSssCommitGkind, gNdsMenuShellSssCommitSlotGkind, gNdsMenuShellSssRandomCount, gNdsMenuShellSssRandomFallbackCount',
        # THE STAGE, END TO END, three independent reads: the descriptor the
        # stage select writes, the scene data ndsMatchConfigApply installs from
        # it, and the field mnMapsInitVars restores the cursor from on the next
        # visit. A write that never fired shows as a descriptor that still
        # carries the preset while `maps` stays at its seed.
        'printf "SSSCFG %d cfg=%02x scene=%02x default=%02x maps=%02x stagesel=%u\n", $n, gNdsMatchConfig.gkind, gSCManagerSceneData.gkind, dSCManagerDefaultSceneData.gkind, gSCManagerSceneData.maps_vsmode_gkind, gNdsMatchConfig.is_stage_select',
        'printf "MSKIT %d enters=%u rej=%u exits=%u opens=%u bytes=%u hash=%08x mismatch=%u readfail=%u\n", $n, gNdsUiKitEnterCount, gNdsUiKitEnterRejectCount, gNdsUiKitExitCount, gNdsUiKitPackOpenCount, gNdsUiKitPackBytesLoaded, gNdsUiKitPackHash, gNdsUiKitPackHashMismatchCount, gNdsUiKitPackReadFailCount',
        'printf "MSDRAW %d compose=%u overflow=%u commit=%u visible=%u\n", $n, gNdsUiKitTextComposeCount, gNdsUiKitTextOverflowCount, gNdsUiKitCommitCount, gNdsUiKitVisibleObjectCount',
        'printf "MSSFX %d move=%u confirm=%u back=%u value=%u start=%u lastid=%u\n", $n, gNdsUiKitSfxRequestCount[0], gNdsUiKitSfxRequestCount[1], gNdsUiKitSfxRequestCount[2], gNdsUiKitSfxRequestCount[3], gNdsUiKitSfxRequestCount[4], gNdsUiKitSfxLastId',
        $(if ($hasMissRing) {
            'printf "MSMISS %d ring=%u id0=%u c0=%u id1=%u c1=%u id2=%u c2=%u id3=%u c3=%u\n", $n, gNdsAudioFgmMissRingCount, gNdsAudioFgmMissRingIDs[0], gNdsAudioFgmMissRingCounts[0], gNdsAudioFgmMissRingIDs[1], gNdsAudioFgmMissRingCounts[1], gNdsAudioFgmMissRingIDs[2], gNdsAudioFgmMissRingCounts[2], gNdsAudioFgmMissRingIDs[3], gNdsAudioFgmMissRingCounts[3]'
        } else {
            'printf "MSMISS %d ring=absent\n", $n'
        }),
        # P2-1g CADENCE ATTRIBUTION. The one-frame outlier P2-1e/1f left
        # unattributed on the row screens is an AUDIO suspicion on record
        # (P2-1d-1's splash FGM-pack load and mode-select BGM stream), and
        # these are the counters that can refute it: a screen that carries
        # the outlier while its refill count is FLAT across its own scene
        # stops did not pay for a stream refill, and a screen with no track
        # at all cannot have paid for one. gNdsAudioBgmRefillTicksMax would
        # settle it directly and is NOT in this build -- it lives behind
        # NDS_RENDERER_PROFILE_LEVEL >= 1, which the shipping shell arm sets
        # to 0 -- so the split here is by PRESENCE, not by price.
        'printf "MSAUDIOWORK %d refills=%u streamed=%u bps=%u elapsed=%u loops=%u fgmcalls=%u fgmfail=%u fgmloaded=%u
", $n, gNdsAudioBgmRefillCount, gNdsAudioBgmStreamedBytes, gNdsAudioBgmStreamBytesPerSecond, gNdsAudioBgmElapsedFrames, gNdsAudioBgmLoopCount, gNdsAudioFgmPlayCalls, gNdsAudioFgmPlayFailCount, gNdsAudioFgmLoaded',
        'printf "MSBGM %d playing=%u track=%u calls=%u looping=%u streambytes=%u modesel=%u pupupu=%u winmario=%u winfox=%u results=%u unsupported=%u\n", $n, gNdsAudioBgmPlaying, gNdsAudioBgmTrackID, gNdsAudioBgmPlayCalls, gNdsAudioBgmIsLooping, gNdsAudioBgmStreamBytes, gNdsAudioBgmModeSelectPlayCount, gNdsAudioBgmPupupuPlayCount, gNdsAudioBgmWinMarioPlayCount, gNdsAudioBgmWinFoxPlayCount, gNdsAudioBgmResultsPlayCount, gNdsAudioBgmUnsupportedTrackCount',
        # The arena BASE and SIZE, which P2-1d's probe did not print and which a
        # cross-build high-water comparison is uninterpretable without: the
        # shared arena starts after the binary's bss, so a build that grew moves
        # the base and re-aligns every allocation behind it.
        'printf "MSARENABASE %d base=%08x size=%u mism=%u\n", $n, gNdsSceneManagerArenaBase, gNdsSceneManagerArenaSize, gNdsSceneManagerArenaMismatchCount',
        # The scene walk's own budget, so a run that stops producing scene
        # entries is read rather than guessed at: `hops` reaching 0 is the
        # walk deliberately parking, and any other reason would leave it
        # non-zero. Absent (and printed as such) on a non-walk build.
        # P2-1g. The rematch seam, on the REALTIME arm -- the one arm where
        # Results genuinely waits for a human. `rematch` counts
        # ndsMNVSResultsSetLoadScene bodies run, `press` counts the rising
        # edges the walk synthesised, and pad/seen/tap split a refusal into
        # its three possible halves (battleship_mnvsresults.c:376): the pad the
        # port can read, the hold the SOURCE pipeline saw, and the edge
        # mnVSResultsCheckExit samples. `pad` reading 0 while `seen`/`tap` carry
        # 0x1000 is correct and not a defect -- ndsControllerLiveButtons reads
        # the RAW keypad, which a synthesised press deliberately never touches.
        'printf "MSREMATCH %d rematch=%u press=%u pad=%04x seen=%04x tap=%04x poll=%u\n", $n, gNdsVSResultsRematchCount, gNdsMenuShellWalkResultsPressCount, gNdsVSResultsPadMask, gNdsVSResultsInputSeenMask, gNdsVSResultsInputTapMask, gNdsVSResultsInputPollCount',
        $(if ($symbols -contains 'gNdsSceneWalkHopsRemaining') {
            'printf "MSWALK %d hops=%u loops=%u\n", $n, gNdsSceneWalkHopsRemaining, gNdsSceneWalkLoopsCompleted'
        } else {
            'printf "MSWALK %d hops=absent\n", $n'
        }),
        # ALL SIXTEEN ring slots. NDS_SCENE_MANAGER_RING is 16 and P2-1d's probe
        # printed eight, which on a three-lap walk shows the FIRST lap and calls
        # it the run -- the flat-high-water claim needs the laps that follow.
        'printf "MSARENA %d k=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", $n, gNdsSceneManagerRingKind[0], gNdsSceneManagerRingKind[1], gNdsSceneManagerRingKind[2], gNdsSceneManagerRingKind[3], gNdsSceneManagerRingKind[4], gNdsSceneManagerRingKind[5], gNdsSceneManagerRingKind[6], gNdsSceneManagerRingKind[7], gNdsSceneManagerRingKind[8], gNdsSceneManagerRingKind[9], gNdsSceneManagerRingKind[10], gNdsSceneManagerRingKind[11], gNdsSceneManagerRingKind[12], gNdsSceneManagerRingKind[13], gNdsSceneManagerRingKind[14], gNdsSceneManagerRingKind[15]',
        'printf "MSHIGH %d %u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", $n, gNdsSceneManagerRingArenaHigh[0], gNdsSceneManagerRingArenaHigh[1], gNdsSceneManagerRingArenaHigh[2], gNdsSceneManagerRingArenaHigh[3], gNdsSceneManagerRingArenaHigh[4], gNdsSceneManagerRingArenaHigh[5], gNdsSceneManagerRingArenaHigh[6], gNdsSceneManagerRingArenaHigh[7], gNdsSceneManagerRingArenaHigh[8], gNdsSceneManagerRingArenaHigh[9], gNdsSceneManagerRingArenaHigh[10], gNdsSceneManagerRingArenaHigh[11], gNdsSceneManagerRingArenaHigh[12], gNdsSceneManagerRingArenaHigh[13], gNdsSceneManagerRingArenaHigh[14], gNdsSceneManagerRingArenaHigh[15]',
        'printf "MSFREE %d %u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n", $n, gNdsSceneManagerRingArenaFree[0], gNdsSceneManagerRingArenaFree[1], gNdsSceneManagerRingArenaFree[2], gNdsSceneManagerRingArenaFree[3], gNdsSceneManagerRingArenaFree[4], gNdsSceneManagerRingArenaFree[5], gNdsSceneManagerRingArenaFree[6], gNdsSceneManagerRingArenaFree[7], gNdsSceneManagerRingArenaFree[8], gNdsSceneManagerRingArenaFree[9], gNdsSceneManagerRingArenaFree[10], gNdsSceneManagerRingArenaFree[11], gNdsSceneManagerRingArenaFree[12], gNdsSceneManagerRingArenaFree[13], gNdsSceneManagerRingArenaFree[14], gNdsSceneManagerRingArenaFree[15]',
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        # This `continue` STARTS the run -- the breakpoint commands above only
        # execute once the target is running -- and it RETURNS at the Hits-th
        # stop, because that stop is the one whose command block does not
        # continue. So the run ends on its own and the summary below is
        # reached, PROVIDED `Hits` is a count the run actually produces.
        'continue',
        'printf "MSSTOP n=%d pc=%08x cpsr=%08x\n", $n, $pc, $cpsr',
        'info symbol $pc',
        'printf "ABORT lr=%08x spsr=%08x\n", $lr, $cpsr',
        'print gNdsMenuShellFrames',
        'print gNdsMenuShellTransitionRing',
        'print gNdsMenuShellInputRing',
        'print gNdsUiKitSfxRequestCount',
        'print gNdsMenuShellCssCommitSlot',
        'print gNdsMenuShellCssCommitCount',
        'print gNdsMenuShellCssStartCount',
        'print gNdsMatchConfig.fighters',
        'print gNdsMenuShellSssCommitCount',
        'print gNdsMenuShellSssCommitGkind',
        'print gNdsMenuShellSssCommitSlotGkind',
        'print gNdsMenuShellSssBackCount',
        'print gNdsMatchConfig.gkind',
        'print gSCManagerSceneData.gkind',
        'print gSCManagerSceneData.maps_vsmode_gkind',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'p2_shell_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the capture file, never the helper's return value: this probe's last
    # command is an unbounded `continue`, so it exits by timeout by design and
    # the capture still holds every line taken before that.
    $captured = Join-Path $log_temp 'p2_shell_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
