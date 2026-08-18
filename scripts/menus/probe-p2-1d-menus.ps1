[CmdletBinding()]
param(
    [string]$Build = 'build-p2-1d-menus',
    [string]$Target = 'smash64ds-p2-1d-menus-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 900,
    # Stops, not frames, and it must not exceed what the ROM produces (see the
    # trailing `continue` below). The breakpoint is ndsSceneManagerEnter, so
    # one stop is one scene entry. MEASURED, not predicted: a one-pass menus
    # build makes SEVEN -- Startup, Title, ModeSelect, VSMode, VSBattle,
    # VSBattle again for Sudden Death, VSResults -- and the three-lap walk
    # build makes THIRTEEN, because the walk stops injecting once its lap
    # budget is spent and the shell parks on the VS screen instead of opening
    # a fourth lap. Asking for sixteen cost one run to a timeout.
    [ValidateRange(2, 64)][int]$Hits = 7,
    [string]$Artifact = ''
)

# P2-1d evidence. Reads the menu shell's per-screen frame/cadence histograms,
# its transition and input rings, and the UI kit counters underneath them, out
# of a ROM built NDS_P2_MENU_SHELL=1.
#
# WHY THIS BREAKPOINT. Every figure here is cumulative, so the LAST line of a
# run is the whole window and the intermediate lines are the timeline. A stop
# per scene ENTRY is the cheapest site that produces that timeline: a per-frame
# stop would be one host round trip per presented frame, which this repository
# has already paid for once (memory: sample the ring in one stop, never 128
# per-frame stops). ndsSceneManagerEnter also runs before the arena is
# re-initialised, so the arena ring it prints is complete for every entry that
# has already exited.
#
# WHAT THE NUMBERS MEAN.
#   MSFRAMES   presented frames measured per screen. Every percentile below is
#              over THAT window; a percentile without its window is not a
#              number. The scene-load frame is excluded and reported by itself
#              as MSENTER.
#   MSWORK<s>  ARM9 work per presented frame on screen <s>, sixteen buckets of
#              35,012 ticks -- one sixteenth of the 560,190-tick 60 Hz VBlank
#              budget. Bucket b is [b*35012, (b+1)*35012), so the whole
#              distribution sitting in bucket 0 is "under 6.25% of budget".
#   MSVB<s>    presents whose VBlank interval was 1/2/3/4+. Index 0 is a clean
#              60 Hz present; the menus are 60 Hz screens, so anything else is
#              a slip.
#   MSTRANS    ((screen << 8) | next scene kind) per menu transition.
#   MSINPUT    ((screen << 16) | tap mask) per input the screens acted on. This
#              is what makes the walk an INPUT-driven proof: the taps and the
#              transitions they produced are both recorded, rather than a hop
#              counter asserting the flow happened.
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
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-1d-menus.txt')
}

$buildConfig = Join-Path (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
    throw "p2-1d probe: $Build has no nds_build_config.h; refusing stale evidence."
}
$configText = Get-Content -LiteralPath $buildConfig -Raw
foreach ($flag in @('NDS_P2_UI_KIT', 'NDS_P2_MENU_SHELL')) {
    $m = [regex]::Match($configText, ('(?m)^#define\s+' + $flag + '\s+(\d+)u?$'))
    if ((-not $m.Success) -or ($m.Groups[1].Value -eq '0')) {
        # A run against a shell-less ROM would report every counter as
        # "missing symbol" and read as a broken shell rather than a wrong build.
        throw "p2-1d probe: $Build was built with $flag off; nothing to read."
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
    'gNdsUiKitEnterCount', 'gNdsUiKitEnterRejectCount', 'gNdsUiKitExitCount',
    'gNdsUiKitPackOpenCount', 'gNdsUiKitPackBytesLoaded', 'gNdsUiKitPackHash',
    'gNdsUiKitPackHashMismatchCount', 'gNdsUiKitPackReadFailCount',
    'gNdsUiKitTextComposeCount', 'gNdsUiKitTextOverflowCount',
    'gNdsUiKitCommitCount', 'gNdsUiKitVisibleObjectCount',
    'gNdsUiKitSfxRequestCount', 'gNdsUiKitSfxLastId',
    'gNdsSceneManagerEnterCount', 'gNdsSceneManagerExitCount',
    'gNdsSceneManagerRejectCount', 'gNdsSceneManagerUnregisteredEnterCount',
    'gNdsSceneManagerArenaMismatchCount', 'gNdsSceneManagerRingKind',
    'gNdsSceneManagerRingArenaHigh', 'gNdsSceneManagerRingArenaFree'
)
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("p2-1d probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
# The FGM miss ring is the SFX seam's negative half: the shell asks with the
# source's own cue ids and this ring says which of them the pack carries.
# FGM 157 (TitlePressStart) is knowingly absent from the pack, so a non-empty
# ring naming 157 is the EXPECTED reading, not a failure.
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
$stdout = Join-Path $log_dir 'melonds.p2-1d-menus.stdout.log'
$stderr = Join-Path $log_dir 'melonds.p2-1d-menus.stderr.log'
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
        'printf "MSSHELL %d screen=%u splash=%u/%u title=%u/%u mode=%u/%u vs=%u/%u\n", $n, gNdsMenuShellScreen, gNdsMenuShellEnterCount[0], gNdsMenuShellExitCount[0], gNdsMenuShellEnterCount[1], gNdsMenuShellExitCount[1], gNdsMenuShellEnterCount[2], gNdsMenuShellExitCount[2], gNdsMenuShellEnterCount[3], gNdsMenuShellExitCount[3]',
        'printf "MSFRAMES %d f0=%u f1=%u f2=%u f3=%u\n", $n, gNdsMenuShellFrames[0], gNdsMenuShellFrames[1], gNdsMenuShellFrames[2], gNdsMenuShellFrames[3]',
        'printf "MSMAX %d w0=%u w1=%u w2=%u w3=%u\n", $n, gNdsMenuShellWorkMax[0], gNdsMenuShellWorkMax[1], gNdsMenuShellWorkMax[2], gNdsMenuShellWorkMax[3]',
        'printf "MSENTER %d e0=%u e1=%u e2=%u e3=%u\n", $n, gNdsMenuShellEnterTicks[0], gNdsMenuShellEnterTicks[1], gNdsMenuShellEnterTicks[2], gNdsMenuShellEnterTicks[3]'
    ) + (New-MenuScreenPrintf -Screen 0) + (New-MenuScreenPrintf -Screen 1) +
        (New-MenuScreenPrintf -Screen 2) + (New-MenuScreenPrintf -Screen 3) + @(
        'printf "MSFLOW %d trans=%u input=%u denied=%u commit=%u rule=%u time=%u stocks=%u walk=%u loops=%u\n", $n, gNdsMenuShellTransitionCount, gNdsMenuShellInputCount, gNdsMenuShellDeniedCount, gNdsMenuShellCommitCount, gNdsMenuShellCommitRule, gNdsMenuShellCommitTime, gNdsMenuShellCommitStocks, gNdsMenuShellWalkSteps, gNdsMenuShellWalkLoops',
        'printf "MSTRANS %d %04x %04x %04x %04x %04x %04x %04x %04x\n", $n, gNdsMenuShellTransitionRing[0], gNdsMenuShellTransitionRing[1], gNdsMenuShellTransitionRing[2], gNdsMenuShellTransitionRing[3], gNdsMenuShellTransitionRing[4], gNdsMenuShellTransitionRing[5], gNdsMenuShellTransitionRing[6], gNdsMenuShellTransitionRing[7]',
        'printf "MSINPUT %d %06x %06x %06x %06x %06x %06x %06x %06x\n", $n, gNdsMenuShellInputRing[0], gNdsMenuShellInputRing[1], gNdsMenuShellInputRing[2], gNdsMenuShellInputRing[3], gNdsMenuShellInputRing[4], gNdsMenuShellInputRing[5], gNdsMenuShellInputRing[6], gNdsMenuShellInputRing[7]',
        'printf "MSKIT %d enters=%u rej=%u exits=%u opens=%u bytes=%u hash=%08x mismatch=%u readfail=%u\n", $n, gNdsUiKitEnterCount, gNdsUiKitEnterRejectCount, gNdsUiKitExitCount, gNdsUiKitPackOpenCount, gNdsUiKitPackBytesLoaded, gNdsUiKitPackHash, gNdsUiKitPackHashMismatchCount, gNdsUiKitPackReadFailCount',
        'printf "MSDRAW %d compose=%u overflow=%u commit=%u visible=%u\n", $n, gNdsUiKitTextComposeCount, gNdsUiKitTextOverflowCount, gNdsUiKitCommitCount, gNdsUiKitVisibleObjectCount',
        'printf "MSSFX %d move=%u confirm=%u back=%u value=%u start=%u lastid=%u\n", $n, gNdsUiKitSfxRequestCount[0], gNdsUiKitSfxRequestCount[1], gNdsUiKitSfxRequestCount[2], gNdsUiKitSfxRequestCount[3], gNdsUiKitSfxRequestCount[4], gNdsUiKitSfxLastId',
        $(if ($hasMissRing) {
            'printf "MSMISS %d ring=%u id0=%u c0=%u id1=%u c1=%u\n", $n, gNdsAudioFgmMissRingCount, gNdsAudioFgmMissRingIDs[0], gNdsAudioFgmMissRingCounts[0], gNdsAudioFgmMissRingIDs[1], gNdsAudioFgmMissRingCounts[1]'
        } else {
            'printf "MSMISS %d ring=absent\n", $n'
        }),
        'printf "MSARENA %d k=%u,%u,%u,%u,%u,%u,%u,%u h=%u,%u,%u,%u,%u,%u,%u,%u\n", $n, gNdsSceneManagerRingKind[0], gNdsSceneManagerRingKind[1], gNdsSceneManagerRingKind[2], gNdsSceneManagerRingKind[3], gNdsSceneManagerRingKind[4], gNdsSceneManagerRingKind[5], gNdsSceneManagerRingKind[6], gNdsSceneManagerRingKind[7], gNdsSceneManagerRingArenaHigh[0], gNdsSceneManagerRingArenaHigh[1], gNdsSceneManagerRingArenaHigh[2], gNdsSceneManagerRingArenaHigh[3], gNdsSceneManagerRingArenaHigh[4], gNdsSceneManagerRingArenaHigh[5], gNdsSceneManagerRingArenaHigh[6], gNdsSceneManagerRingArenaHigh[7]',
        'printf "MSFREE %d %u,%u,%u,%u,%u,%u,%u,%u\n", $n, gNdsSceneManagerRingArenaFree[0], gNdsSceneManagerRingArenaFree[1], gNdsSceneManagerRingArenaFree[2], gNdsSceneManagerRingArenaFree[3], gNdsSceneManagerRingArenaFree[4], gNdsSceneManagerRingArenaFree[5], gNdsSceneManagerRingArenaFree[6], gNdsSceneManagerRingArenaFree[7]',
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        # This `continue` STARTS the run -- the breakpoint commands above only
        # execute once the target is running -- and it RETURNS at the Hits-th
        # stop, because that stop is the one whose command block does not
        # continue. So the run ends on its own and the summary below is
        # reached, PROVIDED `Hits` is a count the run actually produces:
        # ask for more scene entries than the ROM makes and this waits for one
        # that never comes and the probe exits by timeout instead. The entry
        # counts are stated on the -Hits parameter.
        'continue',
        'printf "MSSTOP n=%d pc=%08x cpsr=%08x\n", $n, $pc, $cpsr',
        'info symbol $pc',
        'print gNdsMenuShellFrames',
        'print gNdsMenuShellTransitionRing',
        'print gNdsMenuShellInputRing',
        'print gNdsUiKitSfxRequestCount',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'p2_1d_menus_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the capture file, never the helper's return value: this probe's last
    # command is an unbounded `continue`, so it exits by timeout by design and
    # the capture still holds every line taken before that.
    $captured = Join-Path $log_temp 'p2_1d_menus_probe.gdb.out'
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
