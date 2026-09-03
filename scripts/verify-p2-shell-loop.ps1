[CmdletBinding()]
param(
    # verify-all.ps1 always forwards these two; the loop arm drives its own
    # emulator through the runner-slot context, so -MelonDS is accepted and the
    # slot wins, exactly as the menu probes do.
    [string]$MelonDS = '',
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateRange(1,65535)][int]$GdbPort = 4333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    # Accepted so verify-all's uniform forwarding finds it; this verifier locks
    # on the guest's own scene entries and never on wall-clock.
    [ValidateRange(0,3600)][int]$DelaySeconds = 5,
    [string]$Target = 'smash64ds-p2-shell-loop-hwtri',
    [string]$Build = 'build-p2-shell-loop',
    # THE PHASE-CLOSE GATE IS 20 (docs/p2/P2-1-vs-shell.md, exit criterion 1).
    # The ROM is compiled with that budget; a smaller value is poked into
    # gNdsMenuShellWalkBudget at the first stop and costs no build.
    # P2-1M amendment (owner, 2026-08-19): "twenty battle legs is excessive
    # ... 1 lap is fine for everything." One full lap exercises every scene
    # transition, the rematch START press, and the arena rewind exactly once;
    # the lap count stays a parameter for anyone who wants a soak.
    [ValidateRange(1,64)][int]$Loops = 1,
    [ValidateRange(60,7200)][int]$TimeoutSeconds = 3000,
    # The battle arena high-water is not exactly flat across laps and never was:
    # P2-1f measured 1,400 B of NON-MONOTONIC spread over three entries (the
    # minimum was the second), which is allocation order inside one scene and
    # not a leak -- a leak is monotonic. Deterministic menu scenes are held to
    # EXACT equality; match scenes carry this band. PlayersVS has its own band
    # below because its source-random CPU proof can change fighter hierarchy.
    # Monotonic growth fails regardless of either value.
    [ValidateRange(0,131072)][int]$BattleHighWaterSpreadMax = 8192,
    # PlayersVS is a menu, but its source behavior is no longer byte-flat once
    # more than the original Mario/Fox proof roster is admitted. The scripted
    # player-kind toggle exercises BattleShip's NA -> CPU path, whose
    # mnPlayersVSUpdatePlayerKind deliberately calls mnPlayersVSRandFighterKind
    # when that slot has no fighter. Different unlocked fighters have different
    # source DObj/MObj/XObj hierarchies, so two otherwise-identical CSS visits
    # can legitimately reach different arena peaks. Keep a narrow, explicit
    # band and the same monotonic-leak rejection used for match-owned content;
    # deterministic menus remain byte-flat below.
    [ValidateRange(0,131072)][int]$PlayersVSHighWaterSpreadMax = 8192,
    # docs/p2/P2-1-vs-shell.md work item 2: N loops leak nothing. 32 KiB of
    # arena still free at the worst scene exit of the run.
    [ValidateRange(0,1048576)][int]$MinArenaFreeFloor = 32768,
    [string]$Artifact = '',
    # Re-run every assertion below against a capture already on disk. This is
    # how the gate proves it can go RED without spending a run: doctor one
    # number in a banked artifact and the same assertions that passed must fail.
    [string]$AnalyzeOnly = ''
)

# P2-1g -- THE VS SHELL LOOP VERIFIER, and the Boundary profile's loop arm.
#
# WHAT IT PROVES. One scripted pass through the whole shell -- title, main
# menu, VS menu (rules moved both ways), character select (token grab, refused
# drop, drop, player-kind toggle, CPU level), stage select (moves, refused
# moves, both confirm paths), battle, results -- repeated $Loops times, closing
# every lap through a REAL START on the Results screen. Per scene KIND it then
# asserts deterministic-scene arena high-water is flat, source-variable scenes
# stay inside their explicit bands without monotonic leak shape, the free floor
# holds, no scene request was refused, no CPU abort was taken, the walk's input
# ring holds exactly one entry per scripted step, and the scene ring is the lap
# pattern and nothing else.
#
# WHY THE RESULTS START MATTERS ENOUGH TO BE THE ROW'S POINT. P2-1b's scene
# walk closed its laps with a substitute scene-manager hop out of Results. That
# hop never runs `ndsMNVSResultsSetLoadScene` -- the function P2-1f rewrote so a
# rematch keeps the fighters and the stage the player chose instead of having
# the mode-163 preset stamped back over them. Its shell arm had therefore never
# executed. The walk now holds START on Results in a four-frames-in-sixteen
# pulse through the real keypad latch, so the lap closes through
# `mnVSResultsCheckExit`'s own edge test and that body runs on every lap; the
# descriptor's survival is then read out of the struct the next match owns.
#
# WHY THE FAST-LOGIC ARM. The battle leg here is a bounded run, because this
# measures the scene BOUNDARY across many laps rather than gameplay. NO TICK
# FIGURE FROM THIS ARM IS A CADENCE OR PERFORMANCE FIGURE. Menu cadence comes
# from the shipping-configuration shell arm (`scripts/menus/probe-p2-shell.ps1`)
# and gameplay comes from Boundary's own mode-163 realtime arm, which this
# profile still runs beside this one.
#
# WHY A BREAKPOINT PER SCENE ENTRY. Every counter read here is cumulative, so
# the last stop is the whole run and the intermediate stops are its timeline. A
# per-frame stop would be one host round trip per presented frame, which this
# repository has already paid for once. `ndsSceneManagerEnter` also runs before
# the arena is re-initialised, so the ring it prints is complete for every entry
# that has already exited -- which is what makes a 16-slot ring cover an
# 87-entry run: with a stop at every entry, each entry's high-water is printed
# long before its slot is reused.

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'

# Scene kinds, from include/sc/scene.h. Spelled out so a ring dump reads as a
# flow rather than as numbers.
$kindName = @{
    1  = 'Title'; 7 = 'ModeSelect'; 9 = 'VSMode'; 16 = 'PlayersVS';
    21 = 'Maps'; 22 = 'VSBattle'; 24 = 'VSResults'; 27 = 'Startup'
}
# The lap, in scene-entry order, once the shell is looping.
$lapPattern = @(16, 21, 22, 24)
# THE THREE CLASSES OF SCENE, and the split is what makes "flat" mean something.
#
# Deterministic menu scenes draw the same content on every lap, so their
# high-water is held to EXACT equality. PlayersVS used to measure byte-flat on
# the small proof roster. With the production roster admitted, its
# source-faithful NA -> CPU toggle calls
# mnPlayersVSRandFighterKind and may construct a different fighter hierarchy on
# each visit. That one menu therefore gets its own narrow band + leak-shape test.
#
# VSBattle and VSResults cannot be, because their content is the MATCH: a lap
# that ties re-enters VSBattle for Sudden Death (`nds_scene_manager.c` names
# this: Sudden Death is not a scene kind, it is a second entry into this one
# with `scVSBattleStartSuddenDeath` swapped into func_start), and Results
# builds a different graph and plays a different winner BGM depending on who
# won. So they get a BAND plus a monotonic test -- a leak is monotonic; the
# measured series is not, and it returns to its own base value for nine
# consecutive laps after each excursion.
$variableKinds = @(16, 22, 24)
# nSCKindStartup declares the N64 overlay-derived arena rather than the
# taskman's, so it is outside the registry and outside the arena invariant by
# design (nds_scene_manager.c's own comment). It is the only expected
# unregistered entry in a run.
$expectedUnregistered = 1

$failures = New-Object System.Collections.Generic.List[string]
function Assert-Loop {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) { $script:failures.Add($Message) | Out-Null }
}

# --- Capture -------------------------------------------------------------

if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-shell-loop.txt')
}
$emulatorExitCode = $null
$emulatorAlive = $false

if (-not [string]::IsNullOrWhiteSpace($AnalyzeOnly)) {
    if (-not (Test-Path -LiteralPath $AnalyzeOnly -PathType Leaf)) {
        throw "verify-p2-shell-loop: -AnalyzeOnly capture not found: $AnalyzeOnly"
    }
    $Artifact = (Resolve-Path -LiteralPath $AnalyzeOnly).Path
    Write-Output "Re-analysing banked capture: $Artifact"
    # A re-analysis cannot speak to liveness -- the emulator is long gone -- so
    # it says so rather than asserting a state it did not observe.
    $emulatorAlive = $true
} else {
    # -NoBuild must skip the make, not merely be forwarded: the lib helper only
    # honours SMASH64DS_VERIFY_NO_BUILD, which verify-all sets and a direct
    # invocation does not, so a hand-run -NoBuild used to relink anyway -- and
    # a relink beside another build is exactly the concurrent-make the rails
    # forbid (the asset generators write shared paths outside $(BUILD)).
    if (-not $NoBuild) {
        make -C $root TARGET=$Target BUILD=$Build
        if ($LASTEXITCODE -ne 0) {
            throw "verify-p2-shell-loop: build of $Target failed ($LASTEXITCODE)."
        }
    } else {
        Write-Output 'Skipping make: -NoBuild.'
    }

    $rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
    $elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
    foreach ($needed in @($rom, $elf)) {
        if (-not (Test-Path -LiteralPath $needed -PathType Leaf)) {
            throw "verify-p2-shell-loop: missing build output: $needed"
        }
    }

    # THE CONFIGURATION IS READ, NOT ASSUMED. A shell-less or walk-less ROM
    # would report every counter as a missing symbol and read as a broken shell
    # rather than as the wrong build; a ROM still carrying the P2-1b substitute
    # hop would close its laps WITHOUT the Results START and this verifier would
    # pass having proved the opposite of what it claims.
    $buildConfig = Join-Path (Resolve-Smash64DSBuildPath -Root $root -Build $Build) 'nds_build_config.h'
    if (-not (Test-Path -LiteralPath $buildConfig -PathType Leaf)) {
        throw "verify-p2-shell-loop: $Build has no nds_build_config.h; refusing stale evidence."
    }
    $configText = Get-Content -LiteralPath $buildConfig -Raw
    function Get-ConfigValue {
        param([string]$Flag)
        $m = [regex]::Match($configText, ('(?m)^#define\s+' + $Flag + '\s+(\d+)u?$'))
        if ($m.Success) { return [int]$m.Groups[1].Value }
        return -1
    }
    foreach ($flag in @('NDS_P2_UI_KIT', 'NDS_P2_MENU_SHELL', 'NDS_P2_MENU_WALK')) {
        $value = Get-ConfigValue -Flag $flag
        Write-Output ("build config: {0}={1}" -f $flag, $value)
        if ($value -le 0) {
            throw "verify-p2-shell-loop: $Build was built with $flag off; nothing to walk."
        }
    }
    $sceneWalk = Get-ConfigValue -Flag 'NDS_R2_SCENE_LOOP_WALK'
    Write-Output ("build config: NDS_R2_SCENE_LOOP_WALK={0}" -f $sceneWalk)
    if ($sceneWalk -ne 0) {
        throw ('verify-p2-shell-loop: NDS_R2_SCENE_LOOP_WALK=' + $sceneWalk +
            '. The substitute Results hop would close the laps instead of the ' +
            'scripted START, so ndsMNVSResultsSetLoadScene would never run ' +
            'and the rematch claim would be false. It must be 0 on this arm.')
    }
    $compiledBudget = Get-ConfigValue -Flag 'NDS_P2_MENU_WALK'
    Write-Output ("walk budget: compiled={0} requested={1}" -f $compiledBudget, $Loops)

    $required = @(
        'ndsSceneManagerEnter',
        'gNdsSceneManagerEnterCount', 'gNdsSceneManagerExitCount',
        'gNdsSceneManagerRejectCount', 'gNdsSceneManagerUnregisteredEnterCount',
        'gNdsSceneManagerArenaMismatchCount', 'gNdsSceneManagerRingKind',
        'gNdsSceneManagerRingArenaHigh', 'gNdsSceneManagerRingArenaFree',
        'gNdsSceneManagerArenaBase', 'gNdsSceneManagerArenaSize',
        'gNdsSceneManagerCurrKind', 'gNdsSceneManagerPrevKind',
        'gNdsMenuShellWalkSteps', 'gNdsMenuShellWalkLoops',
        'gNdsMenuShellWalkBudget', 'gNdsMenuShellWalkResultsPressCount',
        'gNdsMenuShellInputCount', 'gNdsMenuShellInputRing',
        'gNdsMenuShellTransitionCount', 'gNdsMenuShellTransitionRing',
        'gNdsMenuShellFrames', 'gNdsMenuShellEnterCount',
        'gNdsMenuShellExitCount', 'gNdsMenuShellDeniedCount',
        'gNdsMenuShellCssCommitCount', 'gNdsMenuShellCssStartCount',
        'gNdsMenuShellSssCommitCount', 'gNdsMenuShellSssConfirmCount',
        'gNdsMenuShellSssBackCount', 'gNdsMenuShellSssRandomFallbackCount',
        'gNdsVSResultsRematchCount', 'gNdsVSResultsPadMask',
        'gNdsSCVSBattleSuddenDeathPrepareCount',
        'gNdsAudioBgmWinMarioPlayCount', 'gNdsAudioBgmWinFoxPlayCount',
        'gNdsAudioBgmResidentBytes',
        'gNdsVSResultsInputSeenMask', 'gNdsVSResultsInputTapMask',
        'gNdsMatchConfig',
        'gNdsUiKitPackHash', 'gNdsUiKitPackHashMismatchCount',
        'gNdsUiKitPackReadFailCount',
        'gNdsUiKitEnterRejectCount',
        # P2-1 closeout: the shell now owns live CSS fighters before VSBattle.
        # Keep the source fighter-file/effect seam in the capture so an entry
        # crash distinguishes "file never became resident" from "resident file
        # built the wrong tree" without a second, bespoke emulator lap.
        'gFTDataFoxSpecial2', 'gFTDataFoxSpecial3',
        'gNdsFighterManagerStatusBufferMask', 'gNdsRelocHeapDeclineCount',
        'gNdsFtPoseBindFull',
        'gNdsEFDescResolveCount', 'gNdsEFDescDisabledCount',
        'gNdsEFDescUnknownFileCount', 'gNdsEFDescDeferRecoverCount',
        'gNdsEFDescDeferOverflowCount', 'dEFManagerFoxEntryArwingEffectDesc',
        # P2-1h. The frameless boot scene, and the backdrop surfaces' own
        # failure counters -- a lap that silently stopped drawing the collage
        # would otherwise close green.
        'gNdsMenuShellStartupCount',
        'gNdsUiKitSurfaceBlitCount', 'gNdsUiKitSurfaceHashMismatchCount',
        'gNdsUiKitSurfaceReadFailCount', 'gNdsUiKitSurfaceNoLayerCount',
        # P2-1i. The title's BG3 fire sheet, counted apart from the backdrops.
        'gNdsUiKitFireAtlasBlitCount',
        # P2-1j. The state-dependent surfaces -- the VS menu's four buttons and
        # the character select's four player panels. They are the reason the
        # backdrop assertion below is no longer "one blit per entry": these
        # count the rest of it exactly, so the invariant stays an equality.
        'gNdsMenuShellVsButtonBlitCount', 'gNdsMenuShellCssPanelBlitCount',
        # P2-1k (d). The title pop animation. Counted apart from the backdrops
        # for the same reason the fire sheet is: the blit equality below is an
        # equality, and these frames are neither a backdrop nor a state change.
        'gNdsUiKitTitleAnimArmCount', 'gNdsUiKitTitleAnimSettleCount',
        'gNdsUiKitTitleAnimLoadFailCount', 'gNdsUiKitTitleAnimPose',
        # P2-1L (3). The empty-pose guard's counter. It is listed here for the
        # reason the rest of this block exists: --gc-sections has dropped a
        # diagnostic global before, and a MISSING symbol must fail the run
        # loudly rather than let a gdb printf produce a number nobody can
        # attribute.
        'gNdsUiKitTitleAnimEmptyPoseCount'
    )
    $symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
    $missing = @($required | Where-Object { $symbols -notcontains $_ })
    if ($missing.Count -gt 0) {
        throw ("verify-p2-shell-loop: symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
    }
    # Calico's exception entry. Breaking here catches the FIRST abort with its
    # own banked registers rather than whatever the nested one degenerates into,
    # and -- because its command block does not continue -- it also ENDS the run
    # at the fault instead of letting the walk time out looking healthy.
    $hasExcptEntry = $symbols -contains '__excpt_entry'
    if (-not $hasExcptEntry) {
        Write-Output ('note: __excpt_entry is absent from this build; an abort ' +
            'will only be seen wherever the emulator finally stops.')
    }

    # FORWARD -MelonDS, do not hardcode ''. With a runner slot the slot still
    # wins (Resolve-MelonDSRunnerSlot ignores this argument, which is what the
    # param block above means by "the slot wins"), but with the default
    # RunnerSlot of -1 there is no slot to win: the context falls through to
    # Resolve-MelonDSPath -> Resolve-MelonDSRepoExecutablePath, whose $MelonDS
    # is [Parameter(Mandatory)] and rejects an empty string outright. A literal
    # '' therefore made `verify-all.ps1 -Profile Boundary` -- the invocation
    # AGENTS.md's Operating Model opens with, and the one with no -RunnerSlot --
    # die on this arm before the verifier ran, with a parameter-binding error
    # that reads nothing like a shell problem.
    # A direct invocation normally leaves -MelonDS empty and expects the same
    # repo-owned default executable every other verifier uses. The shared helper
    # intentionally validates an explicit path and rejects an empty mandatory
    # argument, so resolve that default here instead of making direct -NoBuild
    # runs require a redundant command-line argument.
    $selectedMelonDS = if ([string]::IsNullOrWhiteSpace($MelonDS)) {
        '.\\emulators\\melonds\\melonDS.exe'
    } else {
        $MelonDS
    }
    $context = Initialize-MelonDSVerifierContext `
        -Root $root -MelonDS $selectedMelonDS -RunnerSlot $RunnerSlot -NoBuild
    $melon_dir = Split-Path -Parent $context.MelonDSPath
    $log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
    $stdout = Join-Path $log_dir 'melonds.p2-shell-loop.stdout.log'
    $stderr = Join-Path $log_dir 'melonds.p2-shell-loop.stderr.log'
    $log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
        $env:SMASH64DS_VERIFY_TEMP_DIR
    } else {
        Join-Path $root 'artifacts\verifier-temp\default'
    }
    $config_state = $null
    $emulator = $null

    # Entries a run makes: four before the loop (Startup, Title, ModeSelect,
    # VS Mode), two extra on lap one (the stage select's own B back-out returns
    # to the character select), four a lap, and the character-select entry that
    # the last lap's Results START produces -- which is the stop that observes
    # the completed run. The cap is that plus slack, so a walk that stalls ends
    # by a stop count this script chose rather than by a timeout.
    $expectedStops = 4 + 2 + (4 * $Loops) + 1
    $maxStops = $expectedStops + 12

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

        $ringIdx = 0..15
        $ringK = ($ringIdx | ForEach-Object { "gNdsSceneManagerRingKind[$_]" }) -join ', '
        $ringH = ($ringIdx | ForEach-Object { "gNdsSceneManagerRingArenaHigh[$_]" }) -join ', '
        $ringF = ($ringIdx | ForEach-Object { "gNdsSceneManagerRingArenaFree[$_]" }) -join ', '
        $ringFmt = ($ringIdx | ForEach-Object { '%u' }) -join ','
        $inputRing = ($ringIdx | ForEach-Object { "gNdsMenuShellInputRing[$_]" }) -join ', '

        $commands = @(
            'set pagination off',
            'set confirm off',
            'set print elements 256',
            'set remotetimeout 20',
            ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
            'set $n = 0',
            'set $budget_set = 0'
        )
        if ($hasExcptEntry) {
            # NO `continue` IN THIS BLOCK. The run ends at the first abort, with
            # the banked abort registers still readable, instead of sliding
            # through zeroed RAM until the timeout -- which is what makes
            # "crashed" distinguishable from "slow" without a second attach.
            $commands += @(
                'break __excpt_entry',
                'commands',
                'silent',
                'printf "LOOPABORT n=%d pc=%08x lr=%08x cpsr=%08x\n", $n, $pc, $lr, $cpsr',
                # P2-4. An abort during a stage walk is nearly always a reloc
                # fixup that did not resolve: the pointer is left raw and the
                # first dereference aborts. Print the fixup ledger beside the
                # registers rather than needing a second attach for it. These
                # counters are flushed at their recording sites, so they read
                # true through the GDB stub rather than as stale cache lines.
                'printf "LOOPABORTFIX fail=%u firstasset=%x firstdep=%x lastasset=%x firstlr=%08x lastlr=%08x\n", gNdsRelocExternalFixupFailCount, gNdsRelocExternalFixupFailFirstAsset, gNdsRelocExternalFixupFailFirstDep, gNdsRelocExternalFixupFailLastAsset, gNdsRelocExternalFixupFailFirstLR, gNdsRelocExternalFixupFailLastLR',
                'printf "LOOPABORTSTAGE optinmask=%x optindep=%x optinok=%u optinfail=%u sizefallback=%u fbtoken=%x fbasset=%x\n", gNdsStageOptInAssetMask, gNdsStageOptInDependencyMask, gNdsStageOptInExternalFixupCount, gNdsStageOptInExternalFixupFailCount, gNdsRelocFileSizeFallbackCount, gNdsRelocFileSizeFallbackToken, gNdsRelocFileSizeFallbackAsset',
                'printf "LOOPABORTGD gd=%08x geom=%08x mapobjs=%08x\n", gMPCollisionGroundData, gMPCollisionGeometry, gMPCollisionMapObjs',
                'printf "LOOPABORTASSET openfail=%u shortread=%u formatfail=%u headerread=%u\n", gNdsRelocAssetOpenFailCount, gNdsRelocAssetShortReadCount, gNdsRelocAssetFormatFailCount, gNdsRelocAssetHeaderReadCount',
                'printf "LOOPABORTDEP unresolved=%u token=%x parent=%x\n", gNdsRelocUnresolvedDepCount, gNdsRelocUnresolvedDepToken, gNdsRelocUnresolvedDepParent',
                'end'
            )
        }
        $commands += @(
            'break ndsSceneManagerEnter',
            'commands',
            'silent',
            'set $n = $n + 1',
            # The budget poke lands at EVERY scene entry -- past bss init, long
            # before any lap can close -- so one linked ROM serves every lap
            # count. It is echoed once, so the artifact records which budget ran
            # without a line per stop.
            #
            # IT USED TO BE POKED ONCE, AT THE FIRST STOP, AND THAT IS NOT
            # DURABLE ON A WRITE-BACK-CACHED TARGET. The gdb stub writes main
            # RAM, not the ARM9 dcache. `gNdsMenuShellWalkBudget` shares its
            # 32-byte line with `gNdsMenuShellScreen`, which the CPU writes on
            # every screen transition, so the line is dirty with the CPU's own
            # copy of the pre-poke value; the next eviction writes all 32 bytes
            # back and silently restores it. Measured 2026-08-25 (row P2-3f5):
            # `LOOPBUDGET budget=1` at the first stop -- gdb reading RAM
            # straight after its own write -- and `LOOPDONE ... budget=20` at
            # the end of the same run, which is what the BUDGET assertion
            # caught. The previous run passed only because an unrelated layout
            # change had put a quiet neighbour in that line. Re-poking at every
            # stop makes the readback deterministic: the run ENDS at a
            # breakpoint stop, so the last poke and LOOPDONE's read are
            # separated by no guest instruction at all.
            #
            # RAIL: a one-shot gdb `set variable` on a guest global is only as
            # durable as its cache line is clean. Re-poke, or poke at the stop
            # you read at.
            ('set variable gNdsMenuShellWalkBudget = ' + $Loops),
            'if $budget_set == 0',
            'set $budget_set = 1',
            'printf "LOOPBUDGET budget=%u\n", gNdsMenuShellWalkBudget',
            'end',
            # `sd`, `winm`/`winf` and `resb` are the MATCH-SCENE ATTRIBUTION.
            # VSBattle and VSResults carry match attribution here. PlayersVS can
            # also vary now, but for a different source-owned reason: its
            # NA->CPU proof invokes mnPlayersVSRandFighterKind. `sd` steps on the
            # lap that tied (Sudden Death
            # is a second entry into VSBattle), and the winner counters plus
            # the resident BGM bytes say which Results graph and which winner
            # track that lap built.
            ('printf "LOOP %d kind=%u prev=%u enters=%u exits=%u rej=%u unreg=%u mism=%u ' +
             'walkloops=%u rematch=%u press=%u steps=%u input=%u trans=%u denied=%u ' +
             'sd=%u winm=%u winf=%u resb=%u dwell=%u\n", ' +
             '$n, gNdsSceneManagerCurrKind, gNdsSceneManagerPrevKind, ' +
             'gNdsSceneManagerEnterCount, gNdsSceneManagerExitCount, ' +
             'gNdsSceneManagerRejectCount, gNdsSceneManagerUnregisteredEnterCount, ' +
             'gNdsSceneManagerArenaMismatchCount, gNdsMenuShellWalkLoops, ' +
             'gNdsVSResultsRematchCount, gNdsMenuShellWalkResultsPressCount, ' +
             'gNdsMenuShellWalkSteps, gNdsMenuShellInputCount, ' +
             'gNdsMenuShellTransitionCount, gNdsMenuShellDeniedCount, ' +
             'gNdsSCVSBattleSuddenDeathPrepareCount, gNdsAudioBgmWinMarioPlayCount, ' +
             'gNdsAudioBgmWinFoxPlayCount, gNdsAudioBgmResidentBytes, ' +
             'gNdsMenuShellWalkDwellSteps'),
            ('printf "LOOPK %d ' + $ringFmt + '\n", $n, ' + $ringK),
            ('printf "LOOPH %d ' + $ringFmt + '\n", $n, ' + $ringH),
            ('printf "LOOPF %d ' + $ringFmt + '\n", $n, ' + $ringF),
            'set $done = 0',
            ('if gNdsVSResultsRematchCount >= ' + $Loops),
            'set $done = 1',
            'end',
            ('if $n >= ' + $maxStops),
            'set $done = 1',
            'end',
            'if $done == 0',
            'continue',
            'end',
            'end',
            # This `continue` STARTS the run; the breakpoint blocks above only
            # execute once the target is running. It RETURNS either at the stop
            # whose block declined to continue (the completed run, or the stop
            # cap) or at __excpt_entry (a CPU abort). Every exit reaches the
            # summary below, so the artifact is complete in all three cases.
            'continue',
            'printf "LOOPEND n=%d pc=%08x cpsr=%08x\n", $n, $pc, $cpsr',
            'info symbol $pc',
            ('printf "LOOPDONE enters=%u exits=%u rej=%u unreg=%u mism=%u walkloops=%u ' +
             'budget=%u rematch=%u press=%u steps=%u input=%u trans=%u denied=%u ' +
             'sd=%u winm=%u winf=%u resb=%u dwell=%u posefull=%u\n", ' +
             'gNdsSceneManagerEnterCount, gNdsSceneManagerExitCount, ' +
             'gNdsSceneManagerRejectCount, gNdsSceneManagerUnregisteredEnterCount, ' +
             'gNdsSceneManagerArenaMismatchCount, gNdsMenuShellWalkLoops, ' +
             'gNdsMenuShellWalkBudget, gNdsVSResultsRematchCount, ' +
             'gNdsMenuShellWalkResultsPressCount, gNdsMenuShellWalkSteps, ' +
             'gNdsMenuShellInputCount, gNdsMenuShellTransitionCount, ' +
             'gNdsMenuShellDeniedCount, gNdsSCVSBattleSuddenDeathPrepareCount, ' +
             'gNdsAudioBgmWinMarioPlayCount, gNdsAudioBgmWinFoxPlayCount, ' +
             'gNdsAudioBgmResidentBytes, gNdsMenuShellWalkDwellSteps, ' +
             'gNdsFtPoseBindFull'),
            # The Results input chain, split three ways so a refusal names its
            # own half (battleship_mnvsresults.c:376): the pad the port can
            # read, the hold the source pipeline saw, and the rising edge
            # mnVSResultsCheckExit samples.
            ('printf "LOOPINPUT pad=%04x seen=%04x tap=%04x poll=%u\n", ' +
             'gNdsVSResultsPadMask, gNdsVSResultsInputSeenMask, ' +
             'gNdsVSResultsInputTapMask, gNdsVSResultsInputPollCount'),
            # THE DESCRIPTOR AT THE END OF TWENTY REMATCHES. If the Results path
            # had re-run the mode-163 preset, every field here would read the
            # preset's value; the character select raises Fox's CPU level once a
            # visit (clamped at 9 by mnplayersvs.c:2027's own rule), so a level
            # above the preset's is the survival proof and a level equal to it
            # is the defect.
            ('printf "LOOPCFG s0=%u/%u/%u s1=%u/%u/%u s2=%u/%u/%u s3=%u/%u/%u ' +
             'gkind=%02x rule=%u time=%u stagesel=%u\n", ' +
             'gNdsMatchConfig.fighters[0].fkind, gNdsMatchConfig.fighters[0].pkind, ' +
             'gNdsMatchConfig.fighters[0].level, gNdsMatchConfig.fighters[1].fkind, ' +
             'gNdsMatchConfig.fighters[1].pkind, gNdsMatchConfig.fighters[1].level, ' +
             'gNdsMatchConfig.fighters[2].fkind, gNdsMatchConfig.fighters[2].pkind, ' +
             'gNdsMatchConfig.fighters[2].level, gNdsMatchConfig.fighters[3].fkind, ' +
             'gNdsMatchConfig.fighters[3].pkind, gNdsMatchConfig.fighters[3].level, ' +
             'gNdsMatchConfig.gkind, gNdsMatchConfig.game_rules, ' +
             'gNdsMatchConfig.time_limit, gNdsMatchConfig.is_stage_select'),
            ('printf "LOOPXFER s0=%u/%u/%u s1=%u/%u/%u pl=%u cp=%u gkind=%02x\n", ' +
             'gSCManagerTransferBattleState.players[0].fkind, ' +
             'gSCManagerTransferBattleState.players[0].pkind, ' +
             'gSCManagerTransferBattleState.players[0].level, ' +
             'gSCManagerTransferBattleState.players[1].fkind, ' +
             'gSCManagerTransferBattleState.players[1].pkind, ' +
             'gSCManagerTransferBattleState.players[1].level, ' +
             'gSCManagerTransferBattleState.pl_count, ' +
             'gSCManagerTransferBattleState.cp_count, ' +
             'gSCManagerSceneData.gkind'),
            # P2-1h deleted the splash screen and renumbered the rest down one:
            # there are FIVE screens now, title at 0. `startup` is the frameless
            # boot scene that replaced it -- 1 per run, whatever the lap count,
            # because it runs before the loop starts.
            ('printf "LOOPSCREENS startup=%u f0=%u f1=%u f2=%u f3=%u f4=%u ' +
             'e0=%u e1=%u e2=%u e3=%u e4=%u x0=%u x1=%u x2=%u x3=%u x4=%u\n", ' +
             'gNdsMenuShellStartupCount, ' +
             'gNdsMenuShellFrames[0], gNdsMenuShellFrames[1], gNdsMenuShellFrames[2], ' +
             'gNdsMenuShellFrames[3], gNdsMenuShellFrames[4], ' +
             'gNdsMenuShellEnterCount[0], gNdsMenuShellEnterCount[1], ' +
             'gNdsMenuShellEnterCount[2], gNdsMenuShellEnterCount[3], ' +
             'gNdsMenuShellEnterCount[4], ' +
             'gNdsMenuShellExitCount[0], gNdsMenuShellExitCount[1], ' +
             'gNdsMenuShellExitCount[2], gNdsMenuShellExitCount[3], ' +
             'gNdsMenuShellExitCount[4]'),
            ('printf "LOOPKIT hash=%08x mismatch=%u readfail=%u overflow=0 rej=%u ' +
             'csscommit=%u cssstart=%u ssscommit=%u sssconfirm=%u sssback=%u fallback=%u\n", ' +
             'gNdsUiKitPackHash, gNdsUiKitPackHashMismatchCount, ' +
             'gNdsUiKitPackReadFailCount, gNdsUiKitEnterRejectCount, ' +
             'gNdsMenuShellCssCommitCount, ' +
             'gNdsMenuShellCssStartCount, gNdsMenuShellSssCommitCount, ' +
             'gNdsMenuShellSssConfirmCount, gNdsMenuShellSssBackCount, ' +
             'gNdsMenuShellSssRandomFallbackCount'),
            # P2-1h. The backdrop art, over the WHOLE run: `blit` must climb by
            # three per lap (title + two collage screens) and every failure
            # counter must stay 0. A lap that stopped drawing the collage would
            # otherwise close green on scene bookkeeping alone.
            ('printf "LOOPSURF blit=%u mismatch=%u readfail=%u nolayer=%u ' +
             'fireblit=%u vsbtn=%u csspanel=%u sssplaque=%u\n", ' +
             'gNdsUiKitSurfaceBlitCount, gNdsUiKitSurfaceHashMismatchCount, ' +
             'gNdsUiKitSurfaceReadFailCount, gNdsUiKitSurfaceNoLayerCount, ' +
             'gNdsUiKitFireAtlasBlitCount, gNdsMenuShellVsButtonBlitCount, ' +
             'gNdsMenuShellCssPanelBlitCount, ' +
             'gNdsMenuShellSssPlaqueBlitCount'),
            # P2-1k (d). The title pop animation: one arm and one settle per
            # title entry, ending on mnTitleSetEndLayout's own snap. `frames`
            # and the tick pair are reported for the board, not asserted --
            # cadence is measured beside the profile, never inside it.
            ('printf "LOOPANIM arm=%u settle=%u fail=%u pose=%u frames=%u ' +
             'ticks=%u maxticks=%u maxpose=%u bytes=%u draw=%u erase=%u ' +
             'empty=%u\n", ' +
             'gNdsUiKitTitleAnimArmCount, gNdsUiKitTitleAnimSettleCount, ' +
             'gNdsUiKitTitleAnimLoadFailCount, gNdsUiKitTitleAnimPose, ' +
             'gNdsUiKitTitleAnimFrameCount, gNdsUiKitTitleAnimTicks, ' +
             'gNdsUiKitTitleAnimMaxTicks, gNdsUiKitTitleAnimMaxPose, ' +
             'gNdsUiKitTitleAnimBytes32, gNdsUiKitTitleAnimDrawTexels, ' +
             'gNdsUiKitTitleAnimEraseTexels, ' +
             'gNdsUiKitTitleAnimEmptyPoseCount'),
            ('printf "LOOPARENA base=%08x size=%u\n", ' +
             'gNdsSceneManagerArenaBase, gNdsSceneManagerArenaSize'),
            ('printf "LOOPEF fox2=%08x fox3=%08x fstat=%08x heapdecl=%u ' +
             'resolve=%u disabled=%u unknown=%u recover=%u deferovf=%u ' +
             'arwingproc=%08x arwingoff=%08x\n", ' +
             'gFTDataFoxSpecial2, gFTDataFoxSpecial3, ' +
             'gNdsFighterManagerStatusBufferMask, gNdsRelocHeapDeclineCount, ' +
             'gNdsEFDescResolveCount, gNdsEFDescDisabledCount, ' +
             'gNdsEFDescUnknownFileCount, gNdsEFDescDeferRecoverCount, ' +
             'gNdsEFDescDeferOverflowCount, ' +
             'dEFManagerFoxEntryArwingEffectDesc.proc_display, ' +
             'dEFManagerFoxEntryArwingEffectDesc.o_dobjsetup'),
            ('printf "LOOPINPUTRING ' + $ringFmt + '\n", ' + $inputRing),
            'detach',
            'quit'
        )

        Invoke-GdbMarkerScript `
            -Gdb $Gdb -Elf $elf -Root $root -Commands $commands `
            -ScriptName 'p2_shell_loop.gdb' `
            -TimeoutSeconds $TimeoutSeconds | Out-Null
    }
    finally {
        # From the capture file, never the helper's return value: a stalled walk
        # exits by timeout and the capture still holds every line taken first.
        $captured = Join-Path $log_temp 'p2_shell_loop.gdb.out'
        if (Test-Path -LiteralPath $captured) {
            New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
            Copy-Item -LiteralPath $captured -Destination $Artifact -Force
            Write-Output "loop capture: $Artifact"
        }
        # LIVENESS, READ BEFORE THE PROCESS IS KILLED. A Boundary run has
        # already died externally once and read as a verifier failure. "Stalled"
        # (emulator alive, stops stopped advancing) and "dead" (emulator gone)
        # are different defects with different owners, so the verdict names
        # which one it saw instead of reporting one timeout for both.
        if ($null -ne $emulator) {
            $emulator.Refresh()
            $emulatorAlive = -not $emulator.HasExited
            if (-not $emulatorAlive) { $emulatorExitCode = $emulator.ExitCode }
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
        }
        if ($null -ne $config_state) { Restore-MelonDSGdbConfig -State $config_state }
    }
}

# --- Assertions ----------------------------------------------------------

if (-not (Test-Path -LiteralPath $Artifact -PathType Leaf)) {
    throw "verify-p2-shell-loop: no capture was produced at $Artifact."
}
$lines = @(Get-Content -LiteralPath $Artifact)

Assert-Loop ($emulatorAlive) (
    'LIVENESS: melonDS exited on its own during the walk (exit ' +
    "$emulatorExitCode). The guest died; this is not a slow capture.")

$abort = @($lines | Where-Object { $_ -match '^LOOPABORT ' })
Assert-Loop ($abort.Count -eq 0) (
    'ABORT: the ARM9 took a CPU exception during the walk -- ' +
    ($abort -join ' | '))

$done = $lines | Where-Object { $_ -match '^LOOPDONE ' } | Select-Object -Last 1
Assert-Loop ($null -ne $done) 'LOOPDONE is absent: the run never reached its summary.'

$stops = @($lines | Where-Object { $_ -match '^LOOP \d+ ' })
Write-Output ("stops: {0}" -f $stops.Count)

if ($null -ne $done) {
    $d = @{}
    foreach ($m in [regex]::Matches($done, '(\w+)=(\d+)')) {
        $d[$m.Groups[1].Value] = [int64]$m.Groups[2].Value
    }
    Write-Output ("LOOPDONE: " + (($d.Keys | Sort-Object | ForEach-Object {
        "$_=$($d[$_])" }) -join ' '))

    Assert-Loop ($d['budget'] -eq $Loops) (
        "BUDGET: gNdsMenuShellWalkBudget=$($d['budget']), requested $Loops. " +
        'The poke did not land, so the lap count below is not the one asked for.')
    Assert-Loop ($d['walkloops'] -eq $Loops) (
        "LAPS: gNdsMenuShellWalkLoops=$($d['walkloops']), expected $Loops " +
        '(a lap is counted at the stage select''s own confirm).')
    # THE ROW'S CENTRAL CLAIM. Every lap left Results through the source's own
    # exit test, so ndsMNVSResultsSetLoadScene's shell body ran $Loops times.
    Assert-Loop ($d['rematch'] -eq $Loops) (
        "REMATCH: gNdsVSResultsRematchCount=$($d['rematch']), expected $Loops. " +
        'A lap closed without running ndsMNVSResultsSetLoadScene.')
    Assert-Loop ($d['press'] -ge $Loops) (
        "RESULTS START: gNdsMenuShellWalkResultsPressCount=$($d['press']), " +
        "expected at least $Loops rising edges.")
    Assert-Loop ($d['rej'] -eq 0) (
        "SCENE REJECT: gNdsSceneManagerRejectCount=$($d['rej']), expected 0.")
    Assert-Loop ($d['mism'] -eq 0) (
        "ARENA MISMATCH: gNdsSceneManagerArenaMismatchCount=$($d['mism']), expected 0.")
    Assert-Loop ($d['posefull'] -eq 0) (
        "FIGHTER POSE SLOTS: gNdsFtPoseBindFull=$($d['posefull']), expected 0. " +
        'A CSS destroy/rebuild exhausted the fixed pose-slot pool.')
    Assert-Loop ($d['unreg'] -eq $expectedUnregistered) (
        "UNREGISTERED ENTRIES: $($d['unreg']), expected $expectedUnregistered " +
        '(nSCKindStartup alone).')
    # One entry per scripted step. A held step injects for many frames and must
    # still post ONE ring entry, which is what makes the ring a record of what
    # the player pressed rather than of how long a direction was held.
    # A DWELL step (button 0) posts no ring entry by construction: it gives a
    # screen time without pressing anything. The character select's Luigi leg
    # has one, and that leg only compiles when the in-progress roster is built,
    # which is why this read exactly until the roster landed. Entries plus
    # dwells is still an EXACT account of the script, which is the point.
    Assert-Loop (($d['input'] + $d['dwell']) -eq $d['steps']) (
        "INPUT RING: gNdsMenuShellInputCount=$($d['input']) + dwell=$($d['dwell']) but " +
        "gNdsMenuShellWalkSteps=$($d['steps']); a step posted no entry, or a " +
        'hold posted more than one.')
    Assert-Loop (($d['enters'] - $d['exits']) -ge 0 -and ($d['enters'] - $d['exits']) -le 1) (
        "SCENE BRACKET: enters=$($d['enters']) exits=$($d['exits']); at most " +
        'one scene may be in flight at the final stop.')
}

$kit = $lines | Where-Object { $_ -match '^LOOPKIT ' } | Select-Object -Last 1
if ($null -ne $kit) {
    Write-Output $kit
    $k = @{}
    foreach ($m in [regex]::Matches($kit, '(\w+)=([0-9a-f]+)')) {
        $k[$m.Groups[1].Value] = $m.Groups[2].Value
    }
    foreach ($zero in @('mismatch', 'readfail', 'overflow', 'rej')) {
        Assert-Loop ([int64]$k[$zero] -eq 0) (
            "UI KIT: $zero=$($k[$zero]), expected 0.")
    }
    Assert-Loop ([int64]$k['ssscommit'] -ge $Loops) (
        "STAGE COMMITS: ssscommit=$($k['ssscommit']), expected at least $Loops.")
}

# P2-1h. The backdrop art, gated rather than merely printed: a run that quietly
# stopped drawing the collage, or drew a stale pack, is a RED here instead of a
# screenshot nobody takes on the twentieth lap.
#
# THE EXPECTED COUNT IS COUNTED, NOT GUESSED. Three screens carry a backdrop --
# the title and the two the source gives the collage to -- and each draws it
# ONCE per entry, so the expectation is the sum of those three screens' own
# entry counts. Writing it as `2 * Loops + 1` instead was wrong on its first
# run: this walk re-enters at the CHARACTER SELECT, not at VS Mode, so the
# three backdrop screens are entered once for the whole run whatever the lap
# count. Tying the assertion to the entry counters also makes it survive a
# future walk that does re-enter them.
$surf = $lines | Where-Object { $_ -match '^LOOPSURF ' } | Select-Object -Last 1
$screens = $lines | Where-Object { $_ -match '^LOOPSCREENS ' } | Select-Object -Last 1
if ($null -ne $surf) {
    $s = @{}
    foreach ($m in [regex]::Matches($surf, '(\w+)=(\d+)')) {
        $s[$m.Groups[1].Value] = $m.Groups[2].Value
    }
    foreach ($zero in @('mismatch', 'readfail', 'nolayer')) {
        Assert-Loop ([int64]$s[$zero] -eq 0) (
            "BACKDROP SURFACES: $zero=$($s[$zero]), expected 0.")
    }
    if ($null -ne $screens) {
        $e = @{}
        foreach ($m in [regex]::Matches($screens, '(\w+)=(\d+)')) {
            $e[$m.Groups[1].Value] = [int64]$m.Groups[2].Value
        }
        # e0 title, e1 mode select, e2 VS mode, and since P2-1i's finding (1)
        # e3 character select and e4 stage select too -- both now sit on the
        # source's own stone tile instead of a flat blue field, so ALL FIVE
        # screens are backdrop screens and the two looped ones make this an
        # assertion about 21 entries apiece rather than one.
        #
        # P2-1j SPLIT THE EQUALITY rather than loosening it. The VS menu's four
        # buttons and the character select's four player panels are surfaces
        # too now, and they are re-blitted on a cursor move or a player-kind
        # change -- so "one blit per entry" stopped being true, and a >= would
        # have thrown away the invariant. The two state counters below record
        # exactly those blits, so the total stays an EQUALITY: every surface
        # blit is either a backdrop (one per screen entry) or a state change
        # that named itself.
        # P2-1k adds the THIRD state source: the stage select's per-stage
        # plaque, re-blitted on a cursor move exactly as a VS button is on a
        # cursor move (mnMapsMakeNameAndEmblem re-makes the pair every time).
        # P2-1L (9) puts the PREVIEW PANEL on the same counter, because
        # mnMapsMakePreview re-makes it on exactly the same event
        # (mnmaps.c:1100) and both are spent through ndsMenuShellSssSyncSurfaces
        # one a frame -- so `sssplaque` is "the stage select's per-cursor
        # surfaces", up to two per cursor move rather than one.
        $state = [int64]$s['vsbtn'] + [int64]$s['csspanel'] +
                 [int64]$s['sssplaque']
        $expected = $e['e0'] + $e['e1'] + $e['e2'] + $e['e3'] + $e['e4'] + $state
        Assert-Loop ([int64]$s['blit'] -eq $expected) (
            "BACKDROP SURFACES: blit=$($s['blit']) against " +
            "e0+e1+e2+e3+e4=$($expected - $state) backdrop-screen entries " +
            "plus $state state blits (vsbtn=$($s['vsbtn']) " +
            "csspanel=$($s['csspanel']) sssplaque=$($s['sssplaque'])); every " +
            'entry of the title, the mode select, the VS menu, the character ' +
            'select and the stage select draws exactly one backdrop, and ' +
            'every other blit is a button, panel or plaque state change.')
        # The state blits must also be BOUNDED: four buttons and four panels
        # are written on every entry of their screen, and everything above that
        # is a scripted input. A run whose state blits scaled with FRAMES
        # instead of with actions would still satisfy the equality above.
        Assert-Loop ([int64]$s['vsbtn'] -ge (4 * $e['e2'])) (
            "VS BUTTONS: vsbtn=$($s['vsbtn']) is below the 4 per VS-menu " +
            "entry ($($e['e2']) entries) the screen writes on entry alone.")
        Assert-Loop ([int64]$s['csspanel'] -ge (4 * $e['e3'])) (
            "CSS PANELS: csspanel=$($s['csspanel']) is below the 4 per " +
            "character-select entry ($($e['e3']) entries) the screen writes " +
            'on entry alone.')
        # P2-1k/P2-1L (9). TWO surfaces per stage-select entry is the floor:
        # the screen draws exactly one name-and-emblem pair at a time
        # (mnMapsMakeNameAndEmblem ejects the previous GObj before making the
        # next, mnmaps.c:822) and exactly one preview panel beside it
        # (mnMapsMakePreview, mnmaps.c:1100), and the entry spends both. Every
        # cursor move that actually changes cell writes up to two more.
        Assert-Loop ([int64]$s['sssplaque'] -ge (2 * $e['e4'])) (
            "SSS PLAQUE+PREVIEW: sssplaque=$($s['sssplaque']) is below the 2 " +
            "per stage-select entry ($($e['e4']) entries) the screen writes " +
            'on entry alone.')
        # P2-1i. The title's BG3 fire sheet, asserted on its own rather than
        # folded into the backdrop count above -- one atlas blit per title
        # entry. A run that stopped blitting it would still draw a correct
        # backdrop and would otherwise close green with a black title field.
        if ($null -ne $s['fireblit']) {
            Assert-Loop ([int64]$s['fireblit'] -eq $e['e0']) (
                "TITLE FIRE ATLAS: fireblit=$($s['fireblit']) against " +
                "e0=$($e['e0']) title entries; every title entry blits the " +
                'thirty-state fire sheet into BG3 exactly once.')
        }
        # P2-1k (d). THE POP ANIMATION RAN, AND IT FINISHED. One arm per title
        # entry proves it was set up; one settle per entry proves it reached
        # `mnTitleSetEndLayout`'s tic-220 snap, which is the only state in which
        # the title is the layout every later screenshot and the owner's own
        # pass is taken against. An animation that armed and stalled mid-pose
        # would leave a half-scaled wordmark on screen and would otherwise close
        # green on scene bookkeeping alone.
        $anim = $lines | Where-Object { $_ -match '^LOOPANIM ' } |
            Select-Object -Last 1
        if ($null -ne $anim) {
            $a = @{}
            foreach ($m in [regex]::Matches($anim, '(\w+)=(\d+)')) {
                $a[$m.Groups[1].Value] = [int64]$m.Groups[2].Value
            }
            Assert-Loop ($a['fail'] -eq 0) (
                "TITLE ANIM: fail=$($a['fail']), expected 0 -- every title " +
                'entry must load the six baked rasters.')
            Assert-Loop ($a['arm'] -eq $e['e0']) (
                "TITLE ANIM: arm=$($a['arm']) against e0=$($e['e0']) title " +
                'entries; the pop animation arms exactly once per entry.')
            Assert-Loop ($a['settle'] -eq $e['e0']) (
                "TITLE ANIM: settle=$($a['settle']) against e0=$($e['e0']) " +
                'title entries; every armed animation must reach ' +
                'mnTitleSetEndLayout''s snap before the screen is left.')
            Assert-Loop ($a['pose'] -eq 51) (
                "TITLE ANIM: pose=$($a['pose']), expected 51 -- the snap the " +
                'source takes at tic 220.')
            # P2-1L (3). EVERY POSE PUTS SOMETHING ON THE SCREEN. The composer
            # ORs every word it stores, so a pose that stored nothing but zeros
            # counts here; pose 1 is excluded in the runtime because its five
            # table entries are legitimately all zero-width. This is the cheap
            # standing guard for the class of defect the row fixed -- a pose
            # that renders blank -- which arm/settle/pose bookkeeping alone
            # cannot see.
            Assert-Loop ($a['empty'] -eq 0) (
                "TITLE ANIM: empty=$($a['empty']), expected 0 -- every pose " +
                'after the first must store at least one non-transparent ' +
                'texel into the band.')
        }
        Assert-Loop ($e['startup'] -eq 1) (
            "BOOT SCENE: startup=$($e['startup']), expected exactly 1 -- the " +
            'frameless boot scene runs once and carries the menu audio load.')
    }
}

foreach ($tag in @('LOOPINPUT', 'LOOPCFG', 'LOOPXFER', 'LOOPSCREENS', 'LOOPSURF',
                   'LOOPANIM', 'LOOPARENA')) {
    $line = $lines | Where-Object { $_ -match ("^$tag ") } | Select-Object -Last 1
    if ($null -ne $line) { Write-Output $line }
}

# THE PER-KIND HIGH-WATER, RECONSTRUCTED OVER EVERY ENTRY OF THE RUN.
#
# The ring holds 16 slots and an $Loops=20 run makes 87 entries, so the ring
# alone covers only the last four laps. With a stop at EVERY entry it covers all
# of them: at a stop whose enters counter reads E, slots hold entries E-16..E-1,
# and entry i always sits at slot i%16. Keying by absolute entry index therefore
# reassembles the whole run out of overlapping 16-entry windows, and every entry
# is seen long before its slot is reused.
$entryKind = @{}
$entryHigh = @{}
$entryFree = @{}
for ($i = 0; $i -lt $stops.Count; $i++) {
    $stopNum = [int]([regex]::Match($stops[$i], '^LOOP (\d+) ').Groups[1].Value)
    $enters = [int]([regex]::Match($stops[$i], ' enters=(\d+)').Groups[1].Value)
    $kLine = $lines | Where-Object { $_ -match ("^LOOPK $stopNum ") } | Select-Object -First 1
    $hLine = $lines | Where-Object { $_ -match ("^LOOPH $stopNum ") } | Select-Object -First 1
    $fLine = $lines | Where-Object { $_ -match ("^LOOPF $stopNum ") } | Select-Object -First 1
    if (($null -eq $kLine) -or ($null -eq $hLine) -or ($null -eq $fLine)) { continue }
    $kv = @(($kLine -replace '^LOOPK \d+ ', '') -split ',')
    $hv = @(($hLine -replace '^LOOPH \d+ ', '') -split ',')
    $fv = @(($fLine -replace '^LOOPF \d+ ', '') -split ',')
    if (($kv.Count -ne 16) -or ($hv.Count -ne 16) -or ($fv.Count -ne 16)) { continue }
    for ($slot = 0; $slot -lt 16; $slot++) {
        # The largest entry index below `enters` that lands in this slot.
        $entry = $slot
        while (($entry + 16) -lt $enters) { $entry += 16 }
        if ($entry -ge $enters) { continue }
        # A slot is only meaningful once its scene has EXITED; an entry still in
        # flight reads high=0/free=0 because ndsSceneManagerEnter zeroes them.
        if ([int64]$hv[$slot] -eq 0) { continue }
        $entryKind[$entry] = [int]$kv[$slot]
        $entryHigh[$entry] = [int64]$hv[$slot]
        $entryFree[$entry] = [int64]$fv[$slot]
    }
}

$byKind = @{}
foreach ($entry in ($entryKind.Keys | Sort-Object)) {
    $kind = $entryKind[$entry]
    if (-not $byKind.ContainsKey($kind)) { $byKind[$kind] = New-Object System.Collections.Generic.List[int64] }
    $byKind[$kind].Add($entryHigh[$entry]) | Out-Null
}

$freeFloor = $null
foreach ($entry in $entryFree.Keys) {
    $value = $entryFree[$entry]
    if (($null -eq $freeFloor) -or ($value -lt $freeFloor)) { $freeFloor = $value }
}

Write-Output ("entries reconstructed: {0}" -f $entryKind.Count)
foreach ($kind in ($byKind.Keys | Sort-Object)) {
    $values = @($byKind[$kind])
    $min = ($values | Measure-Object -Minimum).Minimum
    $max = ($values | Measure-Object -Maximum).Maximum
    $name = if ($kindName.ContainsKey($kind)) { $kindName[$kind] } else { "kind$kind" }
    Write-Output ("HIGHWATER {0,-10} kind={1,2} n={2,3} min={3} max={4} spread={5}" -f
        $name, $kind, $values.Count, $min, $max, ($max - $min))
    if ($variableKinds -contains $kind) {
        Write-Output ("HIGHWATER {0} series: {1}" -f $name, ($values -join ','))
        $spreadMax = if ($kind -eq 16) { $PlayersVSHighWaterSpreadMax } else { $BattleHighWaterSpreadMax }
        Assert-Loop (($max - $min) -le $spreadMax) (
            "HIGHWATER $name : spread $($max - $min) B over $($values.Count) " +
            "entries exceeds $spreadMax B.")
        # A LEAK IS MONOTONIC, and the band above cannot see a slow one inside
        # it. STRICTLY RISING ON EVERY LAP is the leak shape; allocation order
        # inside one scene is not monotone.
        #
        # THE TEST NEEDS A RUN TO BE MEANINGFUL, and saying so is the point of
        # the floor. On a three-lap smoke the arm rose 1481496 -> 1481632 ->
        # 1481912 and tripped this, which three points cannot distinguish from
        # the character select raising Fox's CPU level once a visit -- a
        # different CPU allocates differently, and that climb stops at the
        # source's own 1..9 clamp. Eight laps carries the level past the clamp,
        # so from there a still-rising series is the arena and not the CPU.
        $monotonic = $values.Count -ge 8
        if ($monotonic) {
            for ($i = 1; $i -lt $values.Count; $i++) {
                if ($values[$i] -le $values[$i - 1]) { $monotonic = $false; break }
            }
        }
        Assert-Loop (-not $monotonic) (
            "HIGHWATER $name : strictly rising across all $($values.Count) " +
            'entries, which is the shape of a leak rather than of allocation order.')
    } else {
        Assert-Loop ($min -eq $max) (
            "HIGHWATER $name : NOT FLAT across $($values.Count) entries " +
            "(min=$min max=$max spread=$($max - $min)).")
    }
}
Assert-Loop ($null -ne $freeFloor -and $freeFloor -ge $MinArenaFreeFloor) (
    "ARENA FREE FLOOR: $freeFloor B, below the $MinArenaFreeFloor B minimum.")
Write-Output ("ARENA FREE FLOOR: {0} B (minimum required {1})" -f $freeFloor, $MinArenaFreeFloor)

# THE LAP PATTERN, out of the scene ring rather than out of the transition
# count. The last complete ring is four full laps; every one of them must be
# PlayersVS -> Maps -> VSBattle -> VSResults and nothing else, so a lap that
# quietly skipped the stage select or re-entered a screen would fail here even
# though its counters still summed correctly.
$lapEntries = @()
foreach ($entry in ($entryKind.Keys | Sort-Object)) {
    if ($entry -ge 6) { $lapEntries += $entryKind[$entry] }
}
# The lap is PlayersVS -> Maps -> VSBattle -> VSResults, with ONE EXCEPTION
# that is the game's own: a tied match runs Sudden Death, and Sudden Death is a
# SECOND ENTRY into VSBattle rather than a scene of its own
# (nds_scene_manager.c's table says so, and the run proves it -- the second
# entry's own high-water is the LOWEST of the twenty-one and
# gNdsSCVSBattleSuddenDeathPrepareCount steps at exactly that entry). So the
# battle leg is "one or more VSBattle entries", and everything else is exact:
# a lap that skipped the stage select, re-entered a menu, or reached Results
# without a battle still fails here.
$patternOk = $true
$phase = 0
$sdEntries = 0
foreach ($k in $lapEntries) {
    # if/elseif rather than `switch ($phase)`: the phase is assigned inside the
    # body, and a switch whose condition is mutated by its own clause is a trap
    # this file should not carry.
    if ($phase -eq 0) {
        if ($k -eq $lapPattern[0]) { $phase = 1 } else { $patternOk = $false }
    } elseif ($phase -eq 1) {
        if ($k -eq $lapPattern[1]) { $phase = 2 } else { $patternOk = $false }
    } elseif ($phase -eq 2) {
        if ($k -eq $lapPattern[2]) { $phase = 3 } else { $patternOk = $false }
    } else {
        if ($k -eq $lapPattern[2]) { $sdEntries++ }
        elseif ($k -eq $lapPattern[3]) { $phase = 0 }
        else { $patternOk = $false }
    }
    if (-not $patternOk) { break }
}
Assert-Loop $patternOk (
    'LAP PATTERN: the scene ring is not PlayersVS,Maps,VSBattle[,VSBattle...],' +
    'VSResults repeated from the first full lap -- ' + (($lapEntries |
        ForEach-Object {
            if ($kindName.ContainsKey($_)) { $kindName[$_] } else { $_ }
        }) -join ','))
Write-Output (("LAP PATTERN: {0} looped entries from entry 6, {1}; " +
    "{2} Sudden Death re-entries") -f
    $lapEntries.Count, $(if ($patternOk) { 'exact' } else { 'MISMATCH' }),
    $sdEntries)
# The scene ring says a lap re-entered the battle; this says the GAME agrees it
# was Sudden Death. Two independent counters for one claim -- without the
# second, an unexplained battle re-entry and a tie look identical.
$sdLine = $lines | Where-Object { $_ -match '^LOOPDONE ' } | Select-Object -Last 1
if (($null -ne $sdLine) -and ($sdLine -match ' sd=(\d+)')) {
    $sdCounted = [int]$Matches[1]
    Assert-Loop ($sdCounted -eq $sdEntries) (
        "SUDDEN DEATH: the scene ring shows $sdEntries battle re-entries but " +
        "gNdsSCVSBattleSuddenDeathPrepareCount=$sdCounted; a battle scene was " +
        're-entered for a reason the game does not call Sudden Death.')
    Write-Output "SUDDEN DEATH: ring=$sdEntries prepared=$sdCounted"
}

if ($failures.Count -gt 0) {
    Write-Output ''
    Write-Output "P2 shell loop verifier FAILED ($($failures.Count) assertion(s)):"
    foreach ($f in $failures) { Write-Output "  - $f" }
    exit 1
}
Write-Output ''
Write-Output (("P2 shell loop verifier passed: {0} laps, {1} scene entries, " +
    "deterministic-menu high-waters flat, variable-content high-waters bounded, " +
    "free floor {2} B, zero faults.") -f
    $Loops, $entryKind.Count, $freeFloor)
exit 0
