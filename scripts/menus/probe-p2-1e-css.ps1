[CmdletBinding()]
param(
    [string]$Build = 'build-p2-1e-css',
    [string]$Target = 'smash64ds-p2-1e-css-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 900,
    # Stops, not frames, and it must not exceed what the ROM produces (the
    # trailing `continue` below otherwise waits for a stop that never comes and
    # the probe exits by timeout). The breakpoint is ndsSceneManagerEnter, so
    # one stop is one scene entry. A one-pass CSS build makes at least SEVEN --
    # Startup, Title, ModeSelect, VSMode, PlayersVS, VSBattle, and then either
    # Sudden Death or VSResults depending on how the match ends. Seven is
    # therefore the largest count that does not depend on the match's OUTCOME,
    # and the outcome moves with the CPU level this screen changes.
    [ValidateRange(2, 64)][int]$Hits = 7,
    [string]$Artifact = ''
)

# P2-1e evidence. Reads the character select's own seam state -- cursor, token,
# player-kind and CPU-level actions, and the descriptor it commits -- on top of
# everything the P2-1d probe read, out of a ROM built NDS_P2_MENU_SHELL=1.
# It SUPERSEDES scripts/menus/probe-p2-1d-menus.ps1: the flow that probe walked
# no longer exists (VS START now leads to the character select, not to the
# battle), and every figure it printed is printed here for five screens instead
# of four.
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
#             has no sample"; three of the CSS's ids are deliberately unpacked.
#   MSBGM     ...unsupported is the same split on the BGM side: BGM 10
#             (BattleSelect) is not one of the five tracks the assets carry.
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
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-1e-css.txt')
}

$buildConfig = Join-Path (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
    throw "p2-1e probe: $Build has no nds_build_config.h; refusing stale evidence."
}
$configText = Get-Content -LiteralPath $buildConfig -Raw
foreach ($flag in @('NDS_P2_UI_KIT', 'NDS_P2_MENU_SHELL')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    if ((-not $m.Success) -or ($m.Groups[1].Value -eq '0')) {
        # A run against a shell-less ROM would report every counter as
        # "missing symbol" and read as a broken shell rather than a wrong build.
        throw "p2-1e probe: $Build was built with $flag off; nothing to read."
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
    'gNdsMatchConfig',
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
    'gNdsAudioBgmResultsPlayCount', 'gNdsAudioBgmUnsupportedTrackCount'
)
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("p2-1e probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
# The FGM miss ring is the SFX seam's negative half: the shell asks with the
# source's own cue ids and this ring says which of them the pack carries. Three
# of the character select's are deliberately NOT packed (121 MarioDash, 127
# SamusDash, 167 PlayerSlotWhoosh) plus 512 FreeForAll, so a NON-EMPTY ring is
# the expected reading here and is what row P2-1e-1's scope is derived from.
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
$stdout = Join-Path $log_dir 'melonds.p2-1e-css.stdout.log'
$stderr = Join-Path $log_dir 'melonds.p2-1e-css.stderr.log'
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
        'printf "MSSHELL %d screen=%u splash=%u/%u title=%u/%u mode=%u/%u vs=%u/%u css=%u/%u\n", $n, gNdsMenuShellScreen, gNdsMenuShellEnterCount[0], gNdsMenuShellExitCount[0], gNdsMenuShellEnterCount[1], gNdsMenuShellExitCount[1], gNdsMenuShellEnterCount[2], gNdsMenuShellExitCount[2], gNdsMenuShellEnterCount[3], gNdsMenuShellExitCount[3], gNdsMenuShellEnterCount[4], gNdsMenuShellExitCount[4]',
        'printf "MSFRAMES %d f0=%u f1=%u f2=%u f3=%u f4=%u\n", $n, gNdsMenuShellFrames[0], gNdsMenuShellFrames[1], gNdsMenuShellFrames[2], gNdsMenuShellFrames[3], gNdsMenuShellFrames[4]',
        'printf "MSMAX %d w0=%u w1=%u w2=%u w3=%u w4=%u\n", $n, gNdsMenuShellWorkMax[0], gNdsMenuShellWorkMax[1], gNdsMenuShellWorkMax[2], gNdsMenuShellWorkMax[3], gNdsMenuShellWorkMax[4]',
        'printf "MSENTER %d e0=%u e1=%u e2=%u e3=%u e4=%u\n", $n, gNdsMenuShellEnterTicks[0], gNdsMenuShellEnterTicks[1], gNdsMenuShellEnterTicks[2], gNdsMenuShellEnterTicks[3], gNdsMenuShellEnterTicks[4]'
    ) + (New-MenuScreenPrintf -Screen 0) + (New-MenuScreenPrintf -Screen 1) +
        (New-MenuScreenPrintf -Screen 2) + (New-MenuScreenPrintf -Screen 3) +
        (New-MenuScreenPrintf -Screen 4) + @(
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
        'printf "CSSLIVE %d s0=%u/%u/%u s1=%u/%u/%u pl=%u cp=%u time=%u\n", $n, gSCManagerBattleState->players[0].fkind, gSCManagerBattleState->players[0].pkind, gSCManagerBattleState->players[0].level, gSCManagerBattleState->players[1].fkind, gSCManagerBattleState->players[1].pkind, gSCManagerBattleState->players[1].level, gSCManagerBattleState->pl_count, gSCManagerBattleState->cp_count, gSCManagerBattleState->time_limit',
        'else',
        'printf "CSSLIVE %d none\n", $n',
        'end',
        'printf "CSSCUE %d cues=%u lastid=%u announce=%u\n", $n, gNdsMenuShellCssCueCount, gNdsMenuShellCssCueLastId, gNdsMenuShellCssAnnounceCount',
        'printf "MSKIT %d enters=%u rej=%u exits=%u opens=%u bytes=%u hash=%08x mismatch=%u readfail=%u\n", $n, gNdsUiKitEnterCount, gNdsUiKitEnterRejectCount, gNdsUiKitExitCount, gNdsUiKitPackOpenCount, gNdsUiKitPackBytesLoaded, gNdsUiKitPackHash, gNdsUiKitPackHashMismatchCount, gNdsUiKitPackReadFailCount',
        'printf "MSDRAW %d compose=%u overflow=%u commit=%u visible=%u\n", $n, gNdsUiKitTextComposeCount, gNdsUiKitTextOverflowCount, gNdsUiKitCommitCount, gNdsUiKitVisibleObjectCount',
        'printf "MSSFX %d move=%u confirm=%u back=%u value=%u start=%u lastid=%u\n", $n, gNdsUiKitSfxRequestCount[0], gNdsUiKitSfxRequestCount[1], gNdsUiKitSfxRequestCount[2], gNdsUiKitSfxRequestCount[3], gNdsUiKitSfxRequestCount[4], gNdsUiKitSfxLastId',
        $(if ($hasMissRing) {
            'printf "MSMISS %d ring=%u id0=%u c0=%u id1=%u c1=%u id2=%u c2=%u id3=%u c3=%u\n", $n, gNdsAudioFgmMissRingCount, gNdsAudioFgmMissRingIDs[0], gNdsAudioFgmMissRingCounts[0], gNdsAudioFgmMissRingIDs[1], gNdsAudioFgmMissRingCounts[1], gNdsAudioFgmMissRingIDs[2], gNdsAudioFgmMissRingCounts[2], gNdsAudioFgmMissRingIDs[3], gNdsAudioFgmMissRingCounts[3]'
        } else {
            'printf "MSMISS %d ring=absent\n", $n'
        }),
        'printf "MSBGM %d playing=%u track=%u calls=%u looping=%u streambytes=%u modesel=%u pupupu=%u winmario=%u winfox=%u results=%u unsupported=%u\n", $n, gNdsAudioBgmPlaying, gNdsAudioBgmTrackID, gNdsAudioBgmPlayCalls, gNdsAudioBgmIsLooping, gNdsAudioBgmStreamBytes, gNdsAudioBgmModeSelectPlayCount, gNdsAudioBgmPupupuPlayCount, gNdsAudioBgmWinMarioPlayCount, gNdsAudioBgmWinFoxPlayCount, gNdsAudioBgmResultsPlayCount, gNdsAudioBgmUnsupportedTrackCount',
        # The arena BASE and SIZE, which P2-1d's probe did not print and which a
        # cross-build high-water comparison is uninterpretable without: the
        # shared arena starts after the binary's bss, so a build that grew moves
        # the base and re-aligns every allocation behind it.
        'printf "MSARENABASE %d base=%08x size=%u mism=%u\n", $n, gNdsSceneManagerArenaBase, gNdsSceneManagerArenaSize, gNdsSceneManagerArenaMismatchCount',
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
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'p2_1e_css_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the capture file, never the helper's return value: this probe's last
    # command is an unbounded `continue`, so it exits by timeout by design and
    # the capture still holds every line taken before that.
    $captured = Join-Path $log_temp 'p2_1e_css_probe.gdb.out'
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
