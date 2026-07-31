param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4627,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-tickhud',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [switch]$NoBuild,
    # Tics left on the match clock once the countdown is live. See "WHY THE CLOCK
    # IS SHORTENED" below. 120 tics is two seconds of match, which is far too
    # short for either fighter to score, which is exactly the point.
    [ValidateRange(30, 3600)][int]$RemainTics = 120,
    [string]$Screenshot = '',
    # Seconds to watch the free-running Sudden Death match after gdb detaches.
    # 0 skips the watch. 60 is deliberately short: the reported symptom appears
    # immediately on entry, and this lane is meant to be cheap enough to re-run.
    [ValidateRange(0, 600)][int]$WatchSeconds = 60,
    # Keep gdb attached past the Sudden Death entry, let the core run, then
    # interrupt it and report where it stopped. Mutually exclusive with the
    # picture watch by construction -- one stub session per emulation run.
    # Clear gNdsR2MaterialWalkBoundEnabled at battle start, restoring the
    # pre-guard unbounded material write walk in the SAME binary. The only way
    # to attribute the Sudden Death freeze to the guard rather than to the code
    # placement that adding the guard caused -- two builds cannot separate those.
    [switch]$DisableWalkBound,
    # Equalise score/falls at the source's own tie check. NOT the default: the
    # natural 0-0 tie is the honest reproduction. Needed only when heavy
    # instrumentation slows the build enough to change the match outcome.
    [switch]$ForceTie,
    # Break on the default camera proc once Sudden Death is live, to settle
    # whether the convergence that eases target_dist runs there at all.
    [switch]$ProbeCamera,
    # Capture BOTH entries at the same camera state: N calls of the default
    # camera proc after each scene starts. The camera eases from its creation
    # value of 10000 to about 3937 within ~120 calls, so a shot taken at the
    # same call count on both sides is taken at the same converged distance --
    # which is the only way to tell a genuine rendering difference from "the
    # same stage, further away". Six mechanisms measured clean while I compared
    # unmatched frames; this is the comparison that should have come first.
    [switch]$MatchedCapture,
    # 60 is already past convergence and is the practical setting: the easing
    # removes 7.5% of the remaining gap per call, so 0.925^60 leaves under 1% of
    # the 10000->3937 gap, about 3990. Higher values cost real time because gdb
    # stops once per IGNORED hit -- 240 blew the 180 s cap on a Task 103 build,
    # and so did 120.
    [ValidateRange(30, 3600)][int]$MatchedCameraCalls = 60,
    # Build with NDS_R2_SECOND_ENTRY_DIAG=1 and read the second-entry
    # instruments: the per-caller allocation ledger and the MObj chain probe.
    # REQUIRED for any of the SD-LEDGER-* or SD-CHAIN-* output -- without it
    # those symbols do not exist and the reads are stripped from the script
    # rather than left to abort it. Costs a full rebuild and a slower ROM, which
    # is why it is not the default.
    [switch]$SecondEntryDiag,
    [switch]$DiagnoseHang,
    [ValidateRange(5, 300)][int]$HangSettleSeconds = 45,
    # Owner, 2026-07-30: "420 seconds is way too long, should be 180 secs max."
    # The lane reaches Sudden Death in well under two minutes, so anything past
    # 180 is a run that has already failed and is only burning the operator's
    # time.
    [ValidateRange(60, 180)][int]$TimeoutSeconds = 180,
    [string]$JsonOut = ''
)

# Enter Sudden Death deterministically, in one emulation run.
#
# WHY THIS EXISTS. The owner reports Sudden Death has its own issues: a 2.5-minute
# soak reached `ndsBaseSCVSBattleStartSuddenDeath` and then sat in the renderer's
# native stage display commit. That was never reproduced, because nothing could
# reach Sudden Death on purpose -- the both-CPU soak plays a real match and a real
# match ends decisively, so the tie the scene needs simply never happened.
#
# WHY THERE IS NO `sudden_death_playable` HARNESS MODE. The board asked for one and
# it is the wrong shape. Sudden Death is not a scene kind: `scVSBattleStartScene`
# (decomp scvsbattle.c:513) runs the match to completion in a BLOCKING
# `scManagerFuncUpdate`, then asks `scVSBattleSetScoreCheckSuddenDeath()`, and only
# if that returns TRUE does it call `scManagerFuncUpdate` a SECOND time with
# `func_start` swapped to `scVSBattleStartSuddenDeath`. A mode that "boots into
# Sudden Death" would therefore have to fork that control-flow function to skip the
# first match -- and then the thing being reproduced is the fork, not the bug.
# `results_playable` is not a precedent either: Results reads plain data out of the
# transfer state, so seeding it is sound; Sudden Death is entered from live match
# state by the source's own decision.
#
# HOW THE TIE IS PRODUCED -- WITHOUT TOUCHING A SINGLE SCORE. The decision is
# `tko = score - falls` per player, and any two-way tie for the top `tko` sends the
# match to Sudden Death (decomp scvsbattle.c:228-306). A match in which NOBODY
# scores is therefore a tie at 0-0: the most ordinary tie the game has. So this
# harness seeds nothing. It runs the canonical mode-163 match with the Fox CPU
# LIVE, drives no input, and lets the source declare its own tie. The printed
# SD-SCORES line is the proof that is what happened; if either number is non-zero
# the tie was not the one intended and the run says so.
#
# WHY THE CLOCK IS SHORTENED. `time_remain` is a plain tic countdown decremented by
# `ifCommonTimerFuncRun`, so writing it once mid-match is the whole of the
# shortening -- no source change, no rule change, and the time-up path that follows
# is the source's own. The alternative is emulating a full game minute, which the
# soak measured at roughly 136 seconds of wall clock BEFORE the breakpoint
# overhead. Two seconds of match is also what keeps the tie honest: neither
# fighter has time to land a KO, so 0-0 is not luck.
#
# WHY NOT PAUSE THE FOX CPU INSTEAD. `gNdsBattlePlayableFoxCpuEnabled = 0` looks
# like the obvious way to guarantee nobody scores, and it CANNOT work here. It is a
# fast-iteration switch, not just a Fox pause: `battleship_ifcommon.c:95-134` makes
# it hold `sIFCommonTimerIsStarted = FALSE` every frame and swap the tic source for
# a frozen stamp. With Fox paused the match clock never advances, so time-up never
# arrives and there is no tie to check. Leave the Fox CPU on.
#
# ONE SESSION. melonDS's GDB stub serves exactly one session per emulation run
# (measured 2026-07-29; a second connect is accepted and then produces nothing), so
# every stage below is one gdb invocation with sequenced breakpoints, and each
# stage prints a marker. A run that dies early names the stage it died in.

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')
. (Join-Path $PSScriptRoot 'lib\melonds-screenshot.ps1')

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS $MelonDS -RunnerSlot $RunnerSlot `
    -GdbPort $GdbPort -GdbPortExplicit:$PSBoundParameters.ContainsKey('GdbPort') `
    -NoBuild:$NoBuild
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'

if (-not $NoBuild) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    # NDS_R2_BOTH_CPU=0 explicitly. The tie this harness wants is the one the
    # canonical Boundary configuration produces -- Mario human and idle versus the
    # level-3 Fox CPU. A both-CPU build fights, scores, and ends decisively, which
    # is precisely the run that already failed to reach Sudden Death.
    # The diag flag MUST be part of the build line, not left to the build
    # directory's previous contents. This harness rebuilds on every run, so a
    # hand-built NDS_R2_SECOND_ENTRY_DIAG=1 ELF is silently overwritten by the
    # next invocation -- which is exactly what happened on 2026-07-31: the
    # allocation ledger vanished from build-sd-stg mid-investigation and two
    # runs died at the first stage.
    $makeArgs = @("TARGET=$Target", "BUILD=$Build", 'NDS_R2_BOTH_CPU=0')
    if ($SecondEntryDiag) { $makeArgs += 'NDS_R2_SECOND_ENTRY_DIAG=1' }
    make -C $root @makeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
# Trust the generated header, not the build directory's name. soak-freeze-watch
# soaked a both-CPU-named directory that was not both-CPU for a day.
$configHeader = Join-Path $root "$Build\nds_build_config.h"
if (Test-Path -LiteralPath $configHeader -PathType Leaf) {
    $seen = [regex]::Match(
        (Get-Content -LiteralPath $configHeader -Raw),
        '(?m)^#define\s+NDS_R2_BOTH_CPU\s+(\d+)')
    if ($seen.Success -and ([int]$seen.Groups[1].Value -ne 0)) {
        throw ('This ROM is NDS_R2_BOTH_CPU=1. Two level-3 CPUs fight to a ' +
               'decisive result, so the 0-0 tie this harness depends on cannot ' +
               'occur. Rebuild without -NoBuild.')
    }
    # Same rule for the diag flag, and for the same reason: the header is the
    # only thing that knows what this ELF actually exports. Asking for the
    # instruments and silently getting a build without them is how a run prints
    # nothing and looks like an emulator problem.
    $diagSeen = [regex]::Match(
        (Get-Content -LiteralPath $configHeader -Raw),
        '(?m)^#define\s+NDS_R2_SECOND_ENTRY_DIAG\s+(\d+)')
    $diagOn = ($diagSeen.Success -and ([int]$diagSeen.Groups[1].Value -ne 0))
    if ($SecondEntryDiag -and (-not $diagOn)) {
        throw ('-SecondEntryDiag was requested but ' + $Build +
               ' is built with NDS_R2_SECOND_ENTRY_DIAG=0, so the allocation ' +
               'ledger and chain probe do not exist. Re-run without -NoBuild.')
    }
    if ((-not $SecondEntryDiag) -and $diagOn) {
        Write-Host ('  note: ' + $Build + ' carries the second-entry diag; ' +
                    'pass -SecondEntryDiag to read it.')
    }
}
foreach ($path in @($rom, $elf)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required input is missing: $path"
    }
}

$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$logDir = Join-Path $root 'artifacts\verification\sudden-death'
[void](New-Item -ItemType Directory -Force -Path $logDir)
$stamp = (Get-Date).ToString('yyyy-MM-dd_HHmmss')
if (-not $Screenshot) {
    $Screenshot = Join-Path $logDir "$stamp-sudden-death-entry.png"
}

# Where the two Task 36 command-stream dumps land. Defined before the gdb
# script is written because the dump commands embed the paths.
$t36DumpA = Join-Path $logDir "$stamp-t36-words-match1.bin"
$t36DumpB = Join-Path $logDir "$stamp-t36-words-suddendeath.bin"

$configState = $null
$emulator = $null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    # WindowStyle: visible-by-design -- the Sudden Death picture is half the
    # evidence here (the reported symptom is a stall in the stage display commit,
    # which is a thing you look at). A hidden melonDS has no MainWindowHandle and
    # the capture below would write nothing.
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'sudden-death.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'sudden-death.melonds.err') `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null
    $emulator.Refresh()
    $window = $emulator.MainWindowHandle

    Write-Host ("sudden-death lane: {0} [{1}], {2} tics left on the clock" -f `
        [System.IO.Path]::GetFileName($rom), $Build, $RemainTics)

    $script = Join-Path $temp 'sudden-death.gdb'
    $stdout = Join-Path $temp 'sudden-death.gdb.out'
    $stderr = Join-Path $temp 'sudden-death.gdb.err'
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    # One printf per value. A single bad expression aborts the whole printf
    # statement, and losing four proofs to one typo has happened here before.
    $gdbLines = @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",

        # Stage 1: the match scene is up.
        'tbreak scVSBattleStartBattle',
        'continue',
        'printf "SD-STAGE=battle-start\n"',
        # THE COUNTER THAT SEEDS frame_draw_last. objman.c:1892 initialises it to
        # `dSYTaskmanFrameCount - 1`, and the measured second-entry value 0xFF is
        # that expression with the counter at 0, against 0x00 for a counter at 1.
        # Read at each setup entry, this says whether the two scene entries build
        # their stage GObjs at a different point in the taskman frame cycle --
        # which is the difference the arithmetic predicts, and predicting is not
        # measuring.
        'printf "SD-M1-FRAMECOUNT=%u\n", dSYTaskmanFrameCount',
        'printf "SD-M1-UPDATECOUNT=%u\n", dSYTaskmanUpdateCount',
        # ITEM 3 baseline. scVSBattleStartBattle is the exact structural analogue
        # of scVSBattleStartSuddenDeath, and this tbreak lands on entry, so match
        # 1's setup has not allocated yet. Paired with the `timer-live` dump below
        # this measures match 1's setup the same way the ENTRY/RUN pair measures
        # Sudden Death's -- with the same instrument, so the two are comparable.
        # Without it the match-1 side has to be back-computed from the Sudden
        # Death numbers, which assumes the ledger saw every allocation and so
        # cannot be used to test that assumption.
        # Ordered by value, because a gdb command file ABORTS the whole remaining
        # script on the first command that errors and drops to a bare prompt with
        # no diagnostic -- the same silent failure `dump binary memory` had here.
        # TOTAL alone decides the question, so it goes first and the bulk array
        # read goes last, where losing it costs only detail.
        'printf "SD-HEAP-USED-M1BASE=%u\n", (unsigned)((char *)gSYTaskmanGeneralHeap.ptr - (char *)gSYTaskmanGeneralHeap.start)',
        'printf "SD-LEDGER-M1BASE-TOTAL=%u\n", gNdsAllocLedgerTotalBytes',
        'printf "SD-LEDGER-M1BASE-USED=%u\n", gNdsAllocLedgerUsed',
        'printf "SD-LEDGER-M1BASE-OVERFLOW=%u\n", gNdsAllocLedgerOverflow',
        'printf "SD-LEDGER-M1BASE-BEGIN\n"',
        'print gNdsAllocLedger',
        'printf "SD-LEDGER-M1BASE-END\n"'
    ) + $(if ($MatchedCapture) { @(
        # Arm A: match 1, N camera-proc calls in. Same clock as arm B below, so
        # both shots are taken at the same converged camera distance.
        'break gmCameraDefaultFuncCamera',
        "ignore `$bpnum $MatchedCameraCalls",
        'continue',
        'printf "SD-MATCH1-DIST=%f\n", gGMCameraStruct.target_dist',
        # STG partition, arm A. Needs NDS_TASK103_STAGE_RUN_PHASE=1; the four
        # phases already exist behind that flag and sum to the STG bucket, so
        # diffing arm A against arm B says WHICH phase doubles rather than that
        # the bucket did.
        'printf "SD-M1-STG=%u\n", gNdsTickHudStageTicks',
        'printf "SD-M1-PREPARE=%u\n", gNdsTask103PrepareTicks',
        'printf "SD-M1-PREPARE-N=%u\n", gNdsTask103PrepareCount',
        'printf "SD-M1-TRAVERSAL=%u\n", gNdsTask103TraversalTicks',
        'printf "SD-M1-TRAVERSAL-N=%u\n", gNdsTask103TraversalCount',
        'printf "SD-M1-DISPLAY=%u\n", gNdsTask103DisplayTicks',
        'printf "SD-M1-DISPLAY-N=%u\n", gNdsTask103DisplayCount',
        'printf "SD-M1-FINISH=%u\n", gNdsTask103FinishTicks',
        'printf "SD-M1-FINISH-N=%u\n", gNdsTask103FinishCount',
        # Task 36 replay owner. It resets when topology_generation or
        # topology_stamp differ (nds_renderer.c:4429), and topology_generation
        # comes from owner_generation, which now advances correctly on a second
        # entry. If it resets and the stage still never re-traverses, the
        # re-capture does not route through the traversal callback -- which is a
        # different fix from "the generation never moved".
        'printf "SD-M1-T36-STATE=%u\n", sNdsRendererTask36ReplayOwner.state',
        'printf "SD-M1-T36-GEN=%u\n", sNdsRendererTask36ReplayOwner.topology_generation',
        'printf "SD-M1-T36-STAMP=%#x\n", sNdsRendererTask36ReplayOwner.topology_stamp',
        # THE PAYLOAD, not the identity. Every identifying field matched across
        # the two entries while the pixels differ, so the answer has to be in
        # what was captured. word_count is the size of the recorded command
        # stream, captured_segment_mask says WHICH of the eight segments got
        # captured, and capture_fault is non-zero if a capture aborted.
        'printf "SD-M1-T36-WORDS=%u\n", sNdsRendererTask36ReplayOwner.word_count',
        'printf "SD-M1-T36-SEGMASK=%#x\n", sNdsRendererTask36ReplayOwner.captured_segment_mask',
        'printf "SD-M1-T36-FAULT=%u\n", sNdsRendererTask36ReplayOwner.capture_fault',
        # Content, not just size. `dump binary memory` was tried first and
        # silently ended the gdb script -- do not retry it here without checking
        # the generated file, which the harness deletes. Sampling fixed indices
        # across the stream is robust, needs no paths, and is decisive in the
        # direction that matters: any differing sample proves the streams differ.
        'printf "SD-M1-W0=%#x\n", sNdsRendererTask36ReplayOwner.words[0]',
        'printf "SD-M1-W1=%#x\n", sNdsRendererTask36ReplayOwner.words[1]',
        'printf "SD-M1-W100=%#x\n", sNdsRendererTask36ReplayOwner.words[100]',
        'printf "SD-M1-W500=%#x\n", sNdsRendererTask36ReplayOwner.words[500]',
        'printf "SD-M1-W1000=%#x\n", sNdsRendererTask36ReplayOwner.words[1000]',
        'printf "SD-M1-W2000=%#x\n", sNdsRendererTask36ReplayOwner.words[2000]',
        'printf "SD-M1-W3000=%#x\n", sNdsRendererTask36ReplayOwner.words[3000]',
        'printf "SD-M1-W3915=%#x\n", sNdsRendererTask36ReplayOwner.words[3915]',
        # THE FIVE UNREPLAYED SEGMENTS, BY NAME. captured_segment_mask 0xa1 is
        # bits 0,5,7, and ndsRendererAdapterNativeStageSegmentGObj maps those to
        # gGRCommonLayerGObjs[0]/[2]/[3]. The unreplayed remainder -- 1,2,3,6 --
        # is pupupu.map_gobj[0..3], the Dream Land map objects, plus
        # gGRCommonLayerGObjs[1] at 4. That is precisely the geometry that looks
        # wrong in the matched pair, so print the GObj pointers on both sides: a
        # changed pointer set is a rebuilt stage, an unchanged one is reuse.
        'printf "SD-M1-MAP0=%#x\n", gGRCommonStruct.pupupu.map_gobj[0]',
        'printf "SD-M1-MAP1=%#x\n", gGRCommonStruct.pupupu.map_gobj[1]',
        'printf "SD-M1-MAP2=%#x\n", gGRCommonStruct.pupupu.map_gobj[2]',
        'printf "SD-M1-MAP3=%#x\n", gGRCommonStruct.pupupu.map_gobj[3]',
        'printf "SD-M1-LAYER1=%#x\n", gGRCommonLayerGObjs[1]',
        'printf "SD-M1-LAYER0=%#x\n", gGRCommonLayerGObjs[0]',
        'printf "SD-STAGE=shot-match1\n"',
        # Drop it so the drive to Sudden Death is not stopped 60 times a second.
        'delete $bpnum'
    ) } else { @() }) + $(if ($DisableWalkBound) { @(
        'set variable gNdsR2MaterialWalkBoundEnabled = 0',
        'printf "SD-WALK-BOUND-ENABLED=%u\n", gNdsR2MaterialWalkBoundEnabled'
    ) } else { @(
        'printf "SD-WALK-BOUND-ENABLED=%u\n", gNdsR2MaterialWalkBoundEnabled'
    ) }) + @(

        # Stage 2: past GO, with the countdown live. The condition matters --
        # `ifCommonTimerFuncRun` is a per-frame GObj proc that also runs BEFORE the
        # timer starts, and `ifCommonBattleUpdateInterfaceAll` resets the stamp and
        # the scheduler tic count at the GO transition (decomp ifcommon.c:3167).
        # Writing `time_remain` on an earlier hit would be overwritten by that.
        'tbreak ifCommonTimerFuncRun if sIFCommonTimerIsStarted != 0',
        'continue',
        'printf "SD-STAGE=timer-live\n"',
        # THE FIVE UNREPLAYED SEGMENTS, entry one. captured_segment_mask 0xa1
        # replays segments 0/5/7; 1/2/3/6 are drawn LIVE every frame from these
        # GObjs, and R2-07's stage corruption has been narrowed to them --
        # the direct path IS engaged (Build 2 / Reuse 2240 / Elide 11200), so
        # the defect is in the DATA this walk reaches.
        # POINTERS ONLY here. Pointer VALUES prove nothing on a rewound bump
        # allocator (same allocation order gives the same addresses), but the
        # null/non-null PATTERN does, and a printf of a pointer cannot fault.
        # Dereferences are confined to the `running` stage, which is last, so a
        # bad one cannot abort the command file before anything else is read.
        'printf "SD-M1-MAPGOBJ=%#x,%#x,%#x,%#x\n", gGRCommonStruct.pupupu.map_gobj[0], gGRCommonStruct.pupupu.map_gobj[1], gGRCommonStruct.pupupu.map_gobj[2], gGRCommonStruct.pupupu.map_gobj[3]',
        'printf "SD-M1-LAYER1=%#x\n", gGRCommonLayerGObjs[1]',
        # CONTENT, entry one. `x` rather than a typed dereference on purpose: it
        # needs no debug info for the GObj type and cannot fail on an address
        # already proven non-null by the printfs above, so it will not abort the
        # command file. Raw words are exactly what the comparison needs -- the
        # addresses are known to match across entries (bump allocator), so only
        # the CONTENT can distinguish them.
        'printf "SD-M1-CONTENT-BEGIN\n"',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[0]',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[1]',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[2]',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[3]',
        'x/40wx gGRCommonLayerGObjs[1]',
        'printf "SD-M1-CONTENT-END\n"',
        # MATCHED STOP. The block above is read at ifCommonTimerFuncRun while the
        # entry-two block is read at scVSBattleFuncUpdate, and GObj+0x0E is
        # `frame_draw_last` -- a field that legitimately varies WITHIN a frame.
        # Comparing the two unmatched stops would attribute a sampling-point
        # difference to the scene entry, which is the confound that has already
        # cost this bug three withdrawn causes. Re-read at the SAME function the
        # entry-two block uses, so the only remaining difference is the entry.
        'tbreak battleship_scvsbattle.c:scVSBattleFuncUpdate',
        'continue',
        'printf "SD-M1B-CONTENT-BEGIN\n"',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[0]',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[1]',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[2]',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[3]',
        'x/40wx gGRCommonLayerGObjs[1]',
        'printf "SD-M1B-CONTENT-END\n"',
        'printf "SD-REMAIN-BEFORE=%u\n", gSCManagerBattleState->time_remain',
        "set variable gSCManagerBattleState->time_remain = $RemainTics",
        'printf "SD-REMAIN-AFTER=%u\n", gSCManagerBattleState->time_remain',

        # Stage 3: the match is over and the source is deciding. These four numbers
        # ARE the proof that the tie is genuine: all zero means neither fighter
        # scored and neither fell, which is the ordinary 0-0 timeout tie.
        'tbreak scVSBattleSetScoreCheckSuddenDeath',
        'continue',
        'printf "SD-STAGE=tie-check\n"',
        'printf "SD-P0-SCORE=%d\n", gSCManagerBattleState->players[0].score',
        'printf "SD-P0-FALLS=%d\n", gSCManagerBattleState->players[0].falls',
        'printf "SD-P1-SCORE=%d\n", gSCManagerBattleState->players[1].score',
        'printf "SD-P1-FALLS=%d\n", gSCManagerBattleState->players[1].falls',
        'printf "SD-RULES=%u\n", gSCManagerBattleState->game_rules',
        'printf "SD-IS-RESET=%u\n", gSCManagerSceneData.is_reset'
    ) + $(if ($ForceTie) { @(
        # LAST RESORT, and only for instrumented A/B runs. The natural 0-0 tie is
        # the honest reproduction and is what the default does -- but it depends
        # on neither fighter scoring in the shortened match, and heavy
        # instrumentation breaks that: a NDS_R2_SECOND_ENTRY_DIAG build runs the
        # chain validator ~14,000 times per run, and the board records that input
        # sampling PHASE depends on execution speed, so a slow build diverges and
        # the CPU lands KOs it does not land otherwise (measured 2026-07-30: Fox
        # 2-0 where the same seed gives 0-0 uninstrumented).
        # Equalising the inputs to the source's own tie test is not faking Sudden
        # Death -- the scene setup and everything after it runs unmodified -- but
        # it IS a seeded tie, so any run using this must say so.
        'set variable gSCManagerBattleState->players[0].score = 0',
        'set variable gSCManagerBattleState->players[0].falls = 0',
        'set variable gSCManagerBattleState->players[1].score = 0',
        'set variable gSCManagerBattleState->players[1].falls = 0',
        'printf "SD-TIE-FORCED=1\n"'
    ) } else { @() }) + @(

        # Stage 4: entry. Breaking on the PORT wrapper, not the decomp one, is
        # deliberate -- the wrapper calls `ndsBaseSCVSBattleStartSuddenDeath()` and
        # only then increments the counter, so "entered" and "completed setup" are
        # separately observable and the stall localizes itself between them.
        'tbreak scVSBattleStartSuddenDeath',
        'continue',
        'printf "SD-STAGE=entered\n"',
        # Same counter at the second entry's setup. Compare against SD-M1-*.
        'printf "SD-SD-FRAMECOUNT=%u\n", dSYTaskmanFrameCount',
        'printf "SD-SD-UPDATECOUNT=%u\n", dSYTaskmanUpdateCount',
        'printf "SD-PREPARE-COUNT-AT-ENTRY=%u\n", gNdsSCVSBattleSuddenDeathPrepareCount',
        'printf "SD-ADAPTER-COUNT=%u\n", gNdsSCVSBattleLifecycleArenaAdapterCount',
        # HEAP, MEASURED ON BOTH SIDES OF THE SETUP. Owner: past Sudden Death
        # freezes "were because of heap". The mechanism is known and lethal --
        # syMallocSet (decomp sys/malloc.c:30) is `while (TRUE);` on overflow, so
        # the allocator HANGS rather than returning NULL. And Sudden Death re-runs
        # the entire battle setup (scVSBattleSetupFiles, ftManagerAllocFighter,
        # wpManagerAllocWeapons, efManagerInitEffects, every camera) against the
        # SAME arena that match one is already holding. These two readings bracket
        # exactly that second pass.
        # CAMERA AT THE END OF MATCH 1. Five texture/binding hypotheses came back
        # clean while the picture stayed different, and the Sudden Death frame is
        # shot from much further out -- so capture the camera on BOTH sides of the
        # boundary and let the numbers say whether the view actually changed.
        'printf "SD-CAM1-STATUS=%d\n", gGMCameraStruct.status_curr',
        'printf "SD-CAM1-STATUS-DEF=%d\n", gGMCameraStruct.status_default',
        'printf "SD-CAM1-DIST=%f\n", gGMCameraStruct.target_dist',
        'printf "SD-CAM1-FOVY=%f\n", gGMCameraStruct.fovy',
        'printf "SD-CAM1-PZOOM-DIST=%f\n", gGMCameraStruct.pzoom_dist',
        'printf "SD-CAM1-PFOLLOW-DIST=%f\n", gGMCameraStruct.pfollow_dist',
        'printf "SD-CAM1-VPW=%d\n", gGMCameraStruct.viewport_width',
        'printf "SD-CAM1-VPH=%d\n", gGMCameraStruct.viewport_height',
        'printf "SD-HEAP-USED-ENTRY=%u\n", (unsigned)((char *)gSYTaskmanGeneralHeap.ptr - (char *)gSYTaskmanGeneralHeap.start)',
        'printf "SD-HEAP-FREE-ENTRY=%u\n", (unsigned)((char *)gSYTaskmanGeneralHeap.end - (char *)gSYTaskmanGeneralHeap.ptr)',
        'printf "SD-ARENA-FAIL-ENTRY=%u\n", gNdsTaskmanArenaAllocFailCount',
        # ITEM 3: the allocation ledger, snapshotted BEFORE Sudden Death's setup
        # runs. Paired with the identical dump at the `running` stage, the
        # per-caller delta between them IS the Sudden Death setup, which is what
        # has to explain the ~119 KB it takes over match 1. One `print` emits the
        # whole table.
        'printf "SD-LEDGER-ENTRY-USED=%u\n", gNdsAllocLedgerUsed',
        'printf "SD-LEDGER-ENTRY-TOTAL=%u\n", gNdsAllocLedgerTotalBytes',
        'printf "SD-LEDGER-ENTRY-OVERFLOW=%u\n", gNdsAllocLedgerOverflow',
        'printf "SD-LEDGER-ENTRY-BEGIN\n"',
        'print gNdsAllocLedger',
        'printf "SD-LEDGER-ENTRY-END\n"',

        # Stage 5: did setup finish, and does the Sudden Death match then run? If
        # the reported stall is real, one of these two never prints and the last
        # marker on stdout says which half owns it.
        'tbreak battleship_scvsbattle.c:scVSBattleFuncUpdate',
        'continue',
        'printf "SD-STAGE=running\n"',
        # The same five segments on the SECOND entry. Compare the null/non-null
        # pattern against SD-M1-* above, not the addresses.
        'printf "SD-SD-MAPGOBJ=%#x,%#x,%#x,%#x\n", gGRCommonStruct.pupupu.map_gobj[0], gGRCommonStruct.pupupu.map_gobj[1], gGRCommonStruct.pupupu.map_gobj[2], gGRCommonStruct.pupupu.map_gobj[3]',
        'printf "SD-SD-LAYER1=%#x\n", gGRCommonLayerGObjs[1]',
        # Same words, entry two. Diff these blocks against SD-M1-CONTENT-*.
        'printf "SD-SD-CONTENT-BEGIN\n"',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[0]',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[1]',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[2]',
        'x/40wx gGRCommonStruct.pupupu.map_gobj[3]',
        'x/40wx gGRCommonLayerGObjs[1]',
        'printf "SD-SD-CONTENT-END\n"',
        'printf "SD-PREPARE-COUNT=%u\n", gNdsSCVSBattleSuddenDeathPrepareCount',
        # The other side of the bracket. If the second setup pass ate most of the
        # remaining arena, this is where it shows.
        'printf "SD-HEAP-USED-RUN=%u\n", (unsigned)((char *)gSYTaskmanGeneralHeap.ptr - (char *)gSYTaskmanGeneralHeap.start)',
        'printf "SD-HEAP-FREE-RUN=%u\n", (unsigned)((char *)gSYTaskmanGeneralHeap.end - (char *)gSYTaskmanGeneralHeap.ptr)',
        'printf "SD-ARENA-FAIL-RUN=%u\n", gNdsTaskmanArenaAllocFailCount',
        # ITEM 3, other side of the bracket: after Sudden Death's setup pass.
        'printf "SD-LEDGER-RUN-USED=%u\n", gNdsAllocLedgerUsed',
        'printf "SD-LEDGER-RUN-TOTAL=%u\n", gNdsAllocLedgerTotalBytes',
        'printf "SD-LEDGER-RUN-OVERFLOW=%u\n", gNdsAllocLedgerOverflow',
        'printf "SD-LEDGER-RUN-BEGIN\n"',
        'print gNdsAllocLedger',
        'printf "SD-LEDGER-RUN-END\n"',
        # The heap-generation contract, proven engaged rather than assumed. A
        # second scene entry MUST have bumped the generation and MUST have made
        # the cache notice; a zero mismatch count here means the contract never
        # fired and the run proves nothing about it.
        'printf "SD-HEAP-GEN=%u\n", gNdsTaskmanHeapGeneration',
        'printf "SD-CACHE-GEN-MISMATCH=%u\n", gNdsR2AnimCacheArenaGenerationMismatches',
        'printf "SD-CACHE-RANGE-FAULT=%u\n", gNdsR2AnimCacheArenaRangeFaults',
        'printf "SD-CACHE-INVALIDATIONS=%u\n", gNdsR2AnimCacheArenaInvalidations',
        'printf "SD-CACHE-RESERVES=%u\n", gNdsR2AnimCacheArenaReserveCount',
        # Second-entry chain validator. Only present in a
        # NDS_R2_SECOND_ENTRY_DIAG=1 build, so these are attempted separately
        # from the counters above -- one bad expression aborts a whole printf,
        # and on an ordinary ROM every one of these is an unknown symbol.
        # TEXTURE STATE ACROSS THE SECOND ENTRY. The owner reports Sudden Death
        # "assigns the wrong textures to each character/element/object", and the
        # same-run pair confirms it: match 1 renders Dream Land correctly, the
        # Sudden Death frame 45 s later has the tree trunk wearing foliage and
        # the side platforms wearing an ornate frame. Prepare early-returns while
        # sNdsRendererBattleStaticTexturePrepared is set, and nothing clears it
        # between the two matches -- Discard runs only after the whole scene
        # (battleship_scvsbattle.c:301, after the blocking base start). A
        # PrepareCount of 1 here means Sudden Death reused match 1's cache.
        'printf "SD-TEX-PREPARE-COUNT=%u\n", gNdsRendererBattleStaticTexturePrepareCount',
        'printf "SD-TEX-PREPARED-COUNT=%u\n", gNdsRendererBattleStaticTexturePreparedCount',
        'printf "SD-TEX-VIOLATIONS=%u\n", gNdsRendererBattleStaticTextureViolationCount',
        'printf "SD-TEX-TEARDOWNS=%u\n", gNdsRendererBattleStaticTextureTeardownCount',
        'printf "SD-TEX-FAILS=%u\n", gNdsRendererBattleStaticTexturePrepareFailCount',
        'printf "SD-OAM-PREPARE-COUNT=%u\n", gNdsIFCommonNativeOamPrepareCount',
        'printf "SD-OAM-CLOUD-COUNT=%u\n", gNdsIFCommonNativeOamPrepareCloudTextureCount',
        'printf "SD-OAM-FAILS=%u\n", gNdsIFCommonNativeOamPrepareFailCount',
        # Scene-cache re-entry eviction, split by cause. GEN non-zero means the
        # heap-generation contract caught the re-entry that the cursor test was
        # missing; RANGE non-zero would mean a file left the live region without
        # a rewind.
        'printf "SD-RELOC-EVICT=%u\n", gNdsRelocSceneReentryEvictCount',
        'printf "SD-RELOC-EVICT-GEN=%u\n", gNdsRelocSceneReentryGenerationEvictCount',
        'printf "SD-RELOC-EVICT-RANGE=%u\n", gNdsRelocSceneReentryRangeEvictCount',
        # ...and the same camera fields once Sudden Death is running. A large
        # target_dist here against match 1's, or a status that never leaves the
        # default, is the wide untracked view the capture shows.
        'printf "SD-CAM2-STATUS=%d\n", gGMCameraStruct.status_curr',
        'printf "SD-CAM2-STATUS-DEF=%d\n", gGMCameraStruct.status_default',
        'printf "SD-CAM2-DIST=%f\n", gGMCameraStruct.target_dist',
        'printf "SD-CAM2-FOVY=%f\n", gGMCameraStruct.fovy',
        'printf "SD-CAM2-PZOOM-DIST=%f\n", gGMCameraStruct.pzoom_dist',
        'printf "SD-CAM2-PFOLLOW-DIST=%f\n", gGMCameraStruct.pfollow_dist',
        'printf "SD-CAM2-VPW=%d\n", gGMCameraStruct.viewport_width',
        'printf "SD-CAM2-VPH=%d\n", gGMCameraStruct.viewport_height',
        'printf "SD-STAGE-STEADY-ADMIT=%u\n", gNdsR2StageSteadyAdmitCount',
        'printf "SD-STAGE-REBUILD=%u\n", gNdsR2StageTopologyRebuildCount',
        'printf "SD-STAGE-MATREJECT=%u\n", gNdsR2StageMaterialRejectCount',
        'printf "SD-STAGE-MATREJECT-IDX=%d\n", (int)gNdsR2StageMaterialRejectIndex',
        'printf "SD-STAGE-MATREJECT-BINDING=%u\n", gNdsR2StageMaterialRejectBinding',
        'printf "SD-STAGE-MATREJECT-DOBJ=%#x\n", gNdsR2StageMaterialRejectDObj',
        'printf "SD-STAGE-MATREJECT-MOBJ=%#x\n", gNdsR2StageMaterialRejectMObj',
        'printf "SD-STAGE-MATREJECT-WANT=%#x\n", gNdsR2StageMaterialRejectFlagsWant',
        'printf "SD-STAGE-MATREJECT-GOT=%#x\n", gNdsR2StageMaterialRejectFlagsGot',
        'printf "SD-STAGE-MATREJECT-GEN=%u\n", gNdsR2StageMaterialRejectHeapGen',
        'printf "SD-WALK-BOUND-HITS=%u\n", gNdsR2MaterialWalkBoundHits',
        'printf "SD-CHAIN-PROBES=%u\n", gNdsR2ChainProbeCount',
        'printf "SD-CHAIN-INVALID=%u\n", gNdsR2ChainProbeInvalidCount',
        'printf "SD-CHAIN-FIRSTBAD-PASS=%u\n", gNdsR2ChainProbeFirstBadPass',
        'printf "SD-CHAIN-FIRSTBAD-STATUS=%u\n", gNdsR2ChainProbeFirstBad.status',
        'printf "SD-CHAIN-FIRSTBAD-NODES=%u\n", gNdsR2ChainProbeFirstBad.nodes',
        'printf "SD-CHAIN-FIRSTBAD-ADDR=%#x\n", gNdsR2ChainProbeFirstBad.first_bad',
        'printf "SD-CHAIN-FIRSTBAD-DOBJ=%#x\n", gNdsR2ChainProbeFirstBad.dobj',
        'printf "SD-CHAIN-FIRSTBAD-GEN=%u\n", gNdsR2ChainProbeFirstBad.generation',
        'printf "SD-CHAIN-P1-STATUS=%u\n", gNdsR2ChainProbePass1.status',
        'printf "SD-CHAIN-P1-NODES=%u\n", gNdsR2ChainProbePass1.nodes',
        'printf "SD-CHAIN-P2-STATUS=%u\n", gNdsR2ChainProbePass2.status',
        'printf "SD-CHAIN-P2-NODES=%u\n", gNdsR2ChainProbePass2.nodes',
        'printf "SD-DONE=1\n"'
    ) + $(if ($MatchedCapture) { @(
        # Arm B: Sudden Death, the SAME N camera-proc calls in.
        'break gmCameraDefaultFuncCamera',
        "ignore `$bpnum $MatchedCameraCalls",
        'continue',
        'printf "SD-SD-DIST=%f\n", gGMCameraStruct.target_dist',
        # STG partition, arm B. Cumulative counters, so subtract arm A's values
        # offline: the difference is what Sudden Death's stage draw spent.
        'printf "SD-SD-STG=%u\n", gNdsTickHudStageTicks',
        'printf "SD-SD-PREPARE=%u\n", gNdsTask103PrepareTicks',
        'printf "SD-SD-PREPARE-N=%u\n", gNdsTask103PrepareCount',
        'printf "SD-SD-TRAVERSAL=%u\n", gNdsTask103TraversalTicks',
        'printf "SD-SD-TRAVERSAL-N=%u\n", gNdsTask103TraversalCount',
        'printf "SD-SD-DISPLAY=%u\n", gNdsTask103DisplayTicks',
        'printf "SD-SD-DISPLAY-N=%u\n", gNdsTask103DisplayCount',
        'printf "SD-SD-FINISH=%u\n", gNdsTask103FinishTicks',
        'printf "SD-SD-FINISH-N=%u\n", gNdsTask103FinishCount',
        'printf "SD-SD-T36-STATE=%u\n", sNdsRendererTask36ReplayOwner.state',
        'printf "SD-SD-T36-GEN=%u\n", sNdsRendererTask36ReplayOwner.topology_generation',
        'printf "SD-SD-T36-STAMP=%#x\n", sNdsRendererTask36ReplayOwner.topology_stamp',
        'printf "SD-SD-T36-WORDS=%u\n", sNdsRendererTask36ReplayOwner.word_count',
        'printf "SD-SD-T36-SEGMASK=%#x\n", sNdsRendererTask36ReplayOwner.captured_segment_mask',
        'printf "SD-SD-T36-FAULT=%u\n", sNdsRendererTask36ReplayOwner.capture_fault',
        'printf "SD-SD-W0=%#x\n", sNdsRendererTask36ReplayOwner.words[0]',
        'printf "SD-SD-W1=%#x\n", sNdsRendererTask36ReplayOwner.words[1]',
        'printf "SD-SD-W100=%#x\n", sNdsRendererTask36ReplayOwner.words[100]',
        'printf "SD-SD-W500=%#x\n", sNdsRendererTask36ReplayOwner.words[500]',
        'printf "SD-SD-W1000=%#x\n", sNdsRendererTask36ReplayOwner.words[1000]',
        'printf "SD-SD-W2000=%#x\n", sNdsRendererTask36ReplayOwner.words[2000]',
        'printf "SD-SD-W3000=%#x\n", sNdsRendererTask36ReplayOwner.words[3000]',
        'printf "SD-SD-W3915=%#x\n", sNdsRendererTask36ReplayOwner.words[3915]',
        'printf "SD-SD-MAP0=%#x\n", gGRCommonStruct.pupupu.map_gobj[0]',
        'printf "SD-SD-MAP1=%#x\n", gGRCommonStruct.pupupu.map_gobj[1]',
        'printf "SD-SD-MAP2=%#x\n", gGRCommonStruct.pupupu.map_gobj[2]',
        'printf "SD-SD-MAP3=%#x\n", gGRCommonStruct.pupupu.map_gobj[3]',
        'printf "SD-SD-LAYER1=%#x\n", gGRCommonLayerGObjs[1]',
        'printf "SD-SD-LAYER0=%#x\n", gGRCommonLayerGObjs[0]',
        'printf "SD-STAGE=shot-sd\n"',
        'printf "SD-MATCHED-DONE=1\n"',
        'detach',
        'quit'
    ) } elseif ($ProbeCamera) { @(
        # Does the default camera proc -- the only thing that eases target_dist
        # toward the fighters (decomp gmcamera.c:624 -> :638) -- run at all once
        # Sudden Death is live? target_dist sits on its creation value of exactly
        # 10000.0 there against match 1's converged 3937.42, and the port's
        # gmCameraMakeBattleCamera is a compat shim that installs NO proc, so the
        # question is which path drives it in match 1 and whether that path
        # survives the second entry. A breakpoint answers it without guessing.
        'break gmCameraDefaultFuncCamera',
        'continue',
        'printf "SD-CAM-PROC-RAN=1\n"',
        # Entry value, i.e. BEFORE this call converges anything. Reading only
        # this was the flaw in the first version of the probe: at a
        # function-entry breakpoint target_dist is necessarily the pre-call
        # value, so "still 10000" proved nothing.
        'printf "SD-CAM-PROC-DIST-FIRST=%f\n", gGMCameraStruct.target_dist',
        # 120 more calls -- two seconds of proc at 60 Hz. The easing moves 7.5%
        # of the remaining gap per call, so a converging camera is within a
        # fraction of a unit of its target long before this; anything still
        # sitting on 10000.0 here is not converging at all.
        'ignore $bpnum 120',
        'continue',
        'printf "SD-CAM-PROC-DIST-AFTER120=%f\n", gGMCameraStruct.target_dist',
        'printf "SD-CAM-PROC-DONE=1\n"',
        'detach',
        'quit'
    ) } elseif ($DiagnoseHang) { @(
        # Spend the session on the hang instead of the watch. melonDS's stub
        # serves ONE session per emulation run, so these two uses are mutually
        # exclusive: -DiagnoseHang keeps the core attached and probes it; the
        # default detaches and watches the picture instead.
        #
        # SYNCHRONOUS PROBES, NOT `interrupt`. The first version of this block
        # used `continue &` + `interrupt`, and gdb ran every command after the
        # interrupt BEFORE the async stop was processed -- so the stop landed
        # (naming a real function) but the PC, backtrace and registers were all
        # swallowed. Ordinary breakpoints cannot desynchronise like that.
        #
        # The decisive question is not "where is the PC" but "does the guest
        # present a frame at all". One PC sample cannot tell a stuck loop from a
        # hot function sampled by chance; a frame counter that does not move
        # can.
        'printf "SD-DIAG=begin\n"',
        'tbreak ndsPlatformEndFrame',
        'continue',
        'printf "SD-PRESENT-1=%u\n", gNdsFrameCounter',
        'tbreak ndsPlatformEndFrame',
        'continue',
        'printf "SD-PRESENT-2=%u\n", gNdsFrameCounter',
        # CATCH THE GIVE-UP DIRECTLY. BattleShip does not fail an overflow, it
        # STOPS: eleven sites across sys/taskman.c and sys/malloc.c end in
        # `while (TRUE);`, which is why every one of these bugs presents as a
        # frozen picture rather than a crash. Every one of the eleven is
        # immediately preceded by a syDebugPrintf naming the overflow, and the
        # port links a real (empty) syDebugPrintf at boot_stubs.c:91 -- so ONE
        # breakpoint there catches whichever assert fired, and its format string
        # says which. Far better than guessing a site: this cannot miss one.
        # DOES THE MATCH ACTUALLY START? ifCommonSuddenDeathMakeInterface leaves
        # game_status at nSCBattleGameStatusWait (0), and only
        # ifCommonSuddenDeathThread flips it to Go (1): it sleeps 90 tics, then
        # calls ifCommonAnnounceGoSetStatus. That thread is a coroutine on this
        # port, so if it never resumes, Sudden Death sits in Wait forever --
        # fighters idle, camera never tracking, timer never started (the timer
        # only runs while status == Go, decomp ifcommon.c:3167). Walking the
        # present counter is only meaningful now that frames advance again.
        'printf "SD-STATUS-1=%u\n", gSCManagerBattleState->game_status',
        'break ndsPlatformEndFrame',
        # `$bpnum`, never a literal: breakpoint numbers depend on how many drive
        # stages ran first, and an earlier `ignore 4` silently targeted a deleted
        # temporary.
        'ignore $bpnum 240',
        'continue',
        'printf "SD-PRESENT-3=%u\n", gNdsFrameCounter',
        'printf "SD-STATUS-2=%u\n", gSCManagerBattleState->game_status',
        'printf "SD-TIME-REMAIN=%u\n", gSCManagerBattleState->time_remain',
        'printf "SD-DIAG=end\n"',
        'quit'
    ) } else { @(
        # Hand the core back before quitting. Killing gdb at a breakpoint leaves
        # the emulator HALTED, and a halted emulator's picture never changes --
        # which the watch below would report as a freeze. The bug this harness
        # hunts is a frozen picture, so the one thing it must never do is
        # manufacture one.
        'detach',
        'quit'
    ) })
    # A gdb command file ABORTS on the first command that errors, silently,
    # leaving a bare `(gdb) ` prompt and no diagnostic -- so ONE printf naming a
    # symbol this build does not export destroys every proof after it. That is
    # measured, not theoretical: on 2026-07-31 the ledger reads survived in the
    # script after the build stopped defining them, and two consecutive runs
    # reached `battle-start` and printed nothing further.
    #
    # Emitting the reads only when their defining flag is on makes the two
    # impossible to separate. Keep this filter keyed to the SYMBOLS, not the
    # markers, so a new diag read cannot be added without being covered.
    if (-not $SecondEntryDiag) {
        $gdbLines = $gdbLines | Where-Object {
            # Symbols first. The bare SD-LEDGER-*-BEGIN/END markers name no
            # symbol, so they would survive and frame an empty block that reads
            # like a zero result rather than an absent one.
            $_ -notmatch 'gNdsAllocLedger|gNdsR2ChainProbe|gNdsR2Stage(SteadyAdmit|TopologyRebuild|MaterialReject)|SD-LEDGER-|SD-CHAIN-'
        }
    }
    [System.IO.File]::WriteAllLines($script, $gdbLines)
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $text = ''
    $lastStage = ''
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        if (Test-Path -LiteralPath $stdout) {
            $text = Get-Content -LiteralPath $stdout -Raw
            if ($null -eq $text) { $text = '' }
            $stages = [regex]::Matches($text, 'SD-STAGE=(\S+)')
            if ($stages.Count -gt 0) {
                $stage = $stages[$stages.Count - 1].Groups[1].Value
                if ($stage -ne $lastStage) {
                    $lastStage = $stage
                    Write-Host "  reached: $stage"
                    # Shoot the screen the moment the core halts inside the Sudden
                    # Death entry, while the picture still belongs to that stop.
                    if (($stage -eq 'entered') -and
                        ($window -ne [IntPtr]::Zero)) {
                        [void](Save-MelonDSWindowCapture -WindowHandle $window `
                            -Path $Screenshot -PreferPrintWindow)
                        Write-Host "  captured -> $Screenshot"
                    }
                    # The matched pair. Both shots are taken with the core
                    # halted at the same camera-proc call count, so the camera
                    # distance is the same in each and any surviving difference
                    # is the scene itself.
                    if (($stage -eq 'shot-match1') -and
                        ($window -ne [IntPtr]::Zero)) {
                        $script:MatchedShotA = Join-Path $logDir `
                            "$stamp-matched-match1.png"
                        [void](Save-MelonDSWindowCapture -WindowHandle $window `
                            -Path $script:MatchedShotA -PreferPrintWindow)
                        Write-Host "  matched arm A -> $script:MatchedShotA"
                    }
                    if (($stage -eq 'shot-sd') -and
                        ($window -ne [IntPtr]::Zero)) {
                        $script:MatchedShotB = Join-Path $logDir `
                            "$stamp-matched-suddendeath.png"
                        [void](Save-MelonDSWindowCapture -WindowHandle $window `
                            -Path $script:MatchedShotB -PreferPrintWindow)
                        Write-Host "  matched arm B -> $script:MatchedShotB"
                    }
                }
            }
            # In diagnose mode the run is not over at SD-DONE -- that is where
            # the interesting half starts.
            $finished = $(if ($DiagnoseHang) { 'SD-DIAG=end' }
                          elseif ($MatchedCapture) { 'SD-MATCHED-DONE=1' }
                          elseif ($ProbeCamera) { 'SD-CAM-PROC-DONE=1' }
                          else { 'SD-DONE=1' })
            if ($text -match $finished) { break }
        }
        if ($gdbProcess.HasExited) { break }
    }
    # Give the detach a moment to land before deciding whether to kill.
    if (-not $gdbProcess.WaitForExit(5000)) {
        Stop-Process -Id $gdbProcess.Id -Force
        Write-Warning ('gdb did not detach cleanly; the core may still be ' +
                       'halted, so the watch below cannot be trusted.')
    }

    function Get-SdValue {
        param([string]$Text, [string]$Key)
        $m = [regex]::Match($Text, "$Key=(-?\d+)")
        if ($m.Success) { return [int]$m.Groups[1].Value }
        return $null
    }

    $entered = ($text -match 'SD-STAGE=entered')
    $running = ($text -match 'SD-STAGE=running')
    $prepare = Get-SdValue -Text $text -Key 'SD-PREPARE-COUNT'
    $report = [ordered]@{
        timestamp   = $stamp
        rom         = $rom
        build       = $Build
        remainTics  = $RemainTics
        lastStage   = $lastStage
        entered     = $entered
        running     = $running
        p0Score     = Get-SdValue -Text $text -Key 'SD-P0-SCORE'
        p0Falls     = Get-SdValue -Text $text -Key 'SD-P0-FALLS'
        p1Score     = Get-SdValue -Text $text -Key 'SD-P1-SCORE'
        p1Falls     = Get-SdValue -Text $text -Key 'SD-P1-FALLS'
        prepareCount = $prepare
        screenshot  = $(if ($entered) { $Screenshot } else { '' })
    }
    $logPath = Join-Path $logDir "$stamp-sudden-death-entry.log"
    Set-Content -LiteralPath $logPath -Value $text
    Write-Host "gdb transcript -> $logPath"
    if ($JsonOut) {
        [void](New-Item -ItemType Directory -Force -Path (Split-Path -Parent $JsonOut))
        $report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $JsonOut
    }

    if (-not $entered) {
        Write-Host 'Last 20 lines of the gdb transcript:'
        ($text -split "`n") | Select-Object -Last 20 | ForEach-Object { "    $_" }
        # Name the stage, because each one fails for a different reason: no
        # `timer-live` means the countdown never started, and no `tie-check` means
        # the match never ended.
        throw ("Never entered Sudden Death (last stage: '$lastStage'). " +
               'If the tie-check stage printed non-zero scores, a fighter scored ' +
               'inside the shortened match -- lower -RemainTics and re-run.')
    }
    Write-Host ''
    Write-Host ("SUDDEN DEATH ENTERED. tie was {0}-{1} vs {2}-{3} (score-falls)." -f `
        $report.p0Score, $report.p0Falls, $report.p1Score, $report.p1Falls)
    if ($running) {
        Write-Host ("Setup COMPLETED and the Sudden Death match is running " +
                    "(prepare count $prepare).")
    } else {
        Write-Host ('Setup did NOT complete: entry was reached but the Sudden ' +
                    'Death match never began. The stall is inside ' +
                    'ndsBaseSCVSBattleStartSuddenDeath. This is the reported bug, ' +
                    'now reproduced deterministically.')
    }

    # Reaching the first Sudden Death update proves setup finished; it does NOT
    # prove the match runs. gdb has detached, so the emulator is now free-running
    # at full speed and the picture is the only instrument left -- which is also
    # how the owner sees the bug. Client-only hashes, so melonDS's title-bar FPS
    # readout cannot masquerade as motion.
    # The word-level diff. Equal word_count says nothing about content; this is
    # the comparison that does. Byte-identical streams mean the replayed third
    # of the stage is provably the same on both entries and the corruption is in
    # the five unreplayed segments; differing streams localise it to whichever
    # words changed.
    if ($MatchedCapture -and (Test-Path -LiteralPath $t36DumpA) -and
        (Test-Path -LiteralPath $t36DumpB)) {
        $ha = (Get-FileHash -LiteralPath $t36DumpA -Algorithm SHA256).Hash
        $hb = (Get-FileHash -LiteralPath $t36DumpB -Algorithm SHA256).Hash
        $la = (Get-Item -LiteralPath $t36DumpA).Length
        $lb = (Get-Item -LiteralPath $t36DumpB).Length
        Write-Host ''
        Write-Host ("T36 words match1      : {0} bytes  {1}" -f $la, $ha)
        Write-Host ("T36 words suddendeath : {0} bytes  {1}" -f $lb, $hb)
        if ($ha -eq $hb) {
            Write-Host ('T36 STREAMS ARE BYTE-IDENTICAL. The replayed segments ' +
                        'are provably the same on both entries, so the stage ' +
                        'corruption is NOT in the replayed third.')
        } else {
            $ba = [System.IO.File]::ReadAllBytes($t36DumpA)
            $bb = [System.IO.File]::ReadAllBytes($t36DumpB)
            $n = [Math]::Min($ba.Length, $bb.Length)
            $first = -1
            $diff = 0
            for ($i = 0; $i -lt $n; $i++) {
                if ($ba[$i] -ne $bb[$i]) {
                    if ($first -lt 0) { $first = $i }
                    $diff++
                }
            }
            Write-Host ("T36 STREAMS DIFFER: {0} of {1} bytes, first at byte " +
                        "{2} (word {3})." -f $diff, $n, $first,
                        [Math]::Floor($first / 4))
        }
    }

    if ($DiagnoseHang) {
        $f1 = Get-SdValue -Text $text -Key 'SD-PRESENT-1'
        $f2 = Get-SdValue -Text $text -Key 'SD-PRESENT-2'
        Write-Host ''
        if ($null -eq $f1) {
            Write-Host ('NO FRAME PRESENTS. ndsPlatformEndFrame was never ' +
                        'reached after Sudden Death started, so the guest is ' +
                        'stuck BEFORE presentation -- a dead loop, not a slow ' +
                        'scene.')
        } elseif ($f2 -gt $f1) {
            Write-Host (("ALIVE, NOT HUNG: frame counter advanced {0} -> {1} " +
                         "after Sudden Death started. The frozen picture is a " +
                         "stalled SCENE, not a stopped CPU.") -f $f1, $f2)
        } else {
            Write-Host (("One frame presented ({0}) and then the counter stopped " +
                         "at {1}. The loop is entered after the first present.") -f `
                         $f1, $f2)
        }
    }
    elseif ($running -and ($WatchSeconds -gt 0) -and ($window -ne [IntPtr]::Zero)) {
        Write-Host ''
        Write-Host "watching the Sudden Death match for ${WatchSeconds}s..."
        $hashes = @()
        $watchEnd = (Get-Date).AddSeconds($WatchSeconds)
        while ((Get-Date) -lt $watchEnd) {
            Start-Sleep -Seconds 5
            if ($emulator.HasExited) { break }
            $hashes += (Get-MelonDSWindowFrameHash -WindowHandle $window)
        }
        $distinct = ($hashes | Select-Object -Unique).Count
        $colors = Measure-MelonDSWindowDistinctColors -WindowHandle $window
        $report.watchSamples = $hashes.Count
        $report.watchDistinctFrames = $distinct
        $report.watchDistinctColors = $colors
        $moved = ($distinct -gt 1)
        $report.watchMoved = $moved
        if ($JsonOut) {
            $report | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $JsonOut
        }
        $after = Join-Path $logDir "$stamp-sudden-death-watch.png"
        [void](Save-MelonDSWindowCapture -WindowHandle $window -Path $after `
            -PreferPrintWindow)
        # Parenthesise the whole template before -f. PowerShell binds -f tighter
        # than +, so a concatenation formatted at the end leaves every placeholder
        # in the earlier fragments as literal text -- this printed "{0} samples"
        # on its first real reproduction.
        if ($moved) {
            Write-Host (("SUDDEN DEATH RUNS: {0} distinct frames across {1} " +
                         "samples. No freeze on this run.") -f `
                         $distinct, $hashes.Count)
        } else {
            Write-Host (("FROZEN: {0} samples over {1}s produced ONE distinct " +
                         "frame ({2} distinct colours, so the capture itself is " +
                         "live). Sudden Death is reproducibly stuck.") -f `
                         $hashes.Count, $WatchSeconds, $colors)
        }
        Write-Host "end-of-watch picture -> $after"
    }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
