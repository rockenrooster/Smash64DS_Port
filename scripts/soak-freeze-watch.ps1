[CmdletBinding()]
param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4619,
    [int]$RunnerSlot = -1,
    [string]$Build = 'build-r2-bothcpu',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [switch]$NoBuild,
    [ValidateRange(2, 120)][int]$PollSeconds = 10,
    # Owner, 2026-07-29: *"for a soak 5 mins tops, because I don't think the
    # results screen ever ends"*, then *"5 mins is too long for the stress ROM,
    # should be like 2.5 min"*. Both hold: 5 is the ceiling, 2.5 is the default,
    # and the default Build below IS the stress ROM. A longer run buys no coverage
    # -- the ROM reaches Results once and stays there, because mnVSResultsCheckExit
    # (decomp mnvsresults.c:266) exits on a START_BUTTON tap with no timeout. A
    # measured both-CPU match completes well inside 2.5 minutes: one full match
    # reported gNdsVSResultsStartCount=1 with 2,043 presented frames. Fractional
    # on purpose -- this was [int] and could not express the owner's number.
    [ValidateRange(0.5, 5.0)][double]$MinutesToRun = 2.5,
    # Consecutive identical frames needed to call it frozen. Two screens of a DS
    # game in motion never render byte-identically, but legitimately static
    # moments exist -- and the longest is much longer than it sounds. This port's
    # scene transitions reload both fighters' asset sets by string path through
    # NitroFS, and the board measured that hand-off as roughly THIRTY SECONDS of
    # dead air with the last frame still on screen. At 4 samples x 10s the trip
    # threshold was 40s, barely past it, and a Sudden Death scene load duly
    # tripped as a freeze while the PC sat on a working `cmp` in the renderer.
    # 8 puts the threshold at 80s, safely clear of the measured dead air.
    [ValidateRange(2, 20)][int]$IdenticalFramesToTrip = 8,
    # Seconds after launch to tap START once. 0 disables. Results exits only on
    # START (mnVSResultsCheckExit, decomp mnvsresults.c:266) and only after
    # sMNVSResultsAllowExitWait -- 410 Results tics for a normal result, which at
    # the measured ~10.1 VBlanks/tic lands around 139 s from launch. Pick a value
    # past that or the tap is swallowed and the run proves nothing.
    #
    # KEEP REMATCH/SUDDEN-DEATH RUNS SHORT. Owner, 2026-07-30: these loops "go on
    # far longer than they need to". Measured timings for the canonical config:
    # Results is reachable around t+170 s, and the rematch fires on the first
    # press that wins the foreground race. Everything after roughly 30 s of match
    # two is repetition -- the corruption is visible immediately and the counters
    # are already latched. So the useful shape is
    #   -MinutesToRun 3.5 -PressStartSeconds 165 -PressStartCount 2
    # which is the floor: ~170 s is the match itself (one game minute runs ~136 s
    # of wall clock at the measured rate) and cannot be cut without changing the
    # match timer. Do not default to 5 minutes for these; 5 is the ceiling for a
    # freeze soak, not the setting for an input experiment.
    [ValidateRange(0, 300)][int]$PressStartSeconds = 0,
    # How many times to repeat that press, one per poll. See the comment at the
    # press site: a single synthetic press wins the foreground race only about
    # half the time.
    [ValidateRange(1, 20)][int]$PressStartCount = 6,
    [string]$JsonOut = ''
)

# Unattended freeze watch. R2-06's "soak clean" clause has had no instrument --
# the board says so in as many words -- and the owner reports "lots of freeze
# bugs that seem random", so this is the missing instrument rather than a
# one-off probe.
#
# HOW IT DETECTS, and why it is not GDB-based: melonDS's GDB stub serves exactly
# ONE session per emulation run. Measured 2026-07-29 -- after a clean `detach`
# the TCP listener is still bound and still accepts a connection, but the second
# session produces no output and the client hangs until its timeout. So a polled
# GDB watchdog cannot be built at all, and the breakpoint-driven alternative
# stops the core on every presented frame and runs the ROM roughly twelve times
# slower than real time, which changes the timing of anything race-shaped -- the
# worst possible property in a hunt for a random freeze.
#
# It therefore watches the screen, which is also how the owner sees a freeze:
# SHA-256 over the window pixels every PollSeconds. Frames in motion never hash
# equal; IdenticalFramesToTrip consecutive equal hashes is a frozen picture.
#
# The single GDB session is then spent where it is worth the most -- on the
# freeze itself, once. It captures the PC, a backtrace, the register file,
# REG_IME/IE/IF, GXSTAT and the IPC registers in one attach, because those are
# what separate a game-code loop from an interrupts-disabled VBlank wait from an
# audio/IPC handshake from a geometry-engine deadlock, and a random freeze that
# must be reproduced twice to be diagnosed will not be diagnosed.

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

# This script exposed a -NoBuild switch and never built anything: the switch only
# sets SMASH64DS_VERIFY_NO_BUILD, which OTHER verifiers read. So a soak silently
# ran whatever ROM happened to be on disk. Measured 2026-07-30: a source fix went
# in, `make` succeeded for the default target, the soak was started without
# -NoBuild, and it soaked a tickhud ROM from the previous evening -- a stale
# result that reads exactly like a real one, because a stale ROM boots and its
# picture moves. Only the missing counter symbols gave it away. Build here, like
# every census harness does, so the switch means what its name says.
if (-not $NoBuild) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    make -C $root "TARGET=$Target" "BUILD=$Build"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
foreach ($path in @($rom, $elf)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required soak file is missing: $path"
    }
}
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$logDir = Join-Path $root 'artifacts\verification\freeze-soak'

# The one GDB attach this run gets, wherever it is spent. Both the freeze capture
# and the clean-run counter read go through here.
#
# This function was CALLED before it existed. The clean-run read at the bottom of
# the script referenced Invoke-SoakGdb from the day it was written and nothing
# defined it, so every NO-FREEZE verdict this instrument has ever produced was
# pixels-only -- no match count, no arena size, no overflow latch -- and the
# failure was invisible because it lands after the verdict is already printed.
# Third defect of that exact shape in one day: a verification step was added and
# never confirmed to run. Returns the captured text, or $null if nothing landed;
# a timeout still returns whatever GDB flushed, because this attach cannot be
# retried (melonDS serves one stub session per emulation run).
function Invoke-SoakGdb {
    param(
        [Parameter(Mandatory=$true)][string]$Tag,
        [Parameter(Mandatory=$true)][string[]]$Commands,
        [int]$TimeoutSeconds = 120
    )

    $script = Join-Path $temp "soak-$Tag.gdb"
    $stdout = Join-Path $temp "soak-$Tag.out"
    $stderr = Join-Path $temp "soak-$Tag.err"

    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 30',
        "target remote 127.0.0.1:$($context.GdbPort)") + $Commands + @('detach'))
    $process = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-batch', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $process.Id -Force
        Write-Host "$Tag attach TIMED OUT -- the stub could not halt the core."
    }
    if (Test-Path -LiteralPath $stdout) {
        return Get-Content -LiteralPath $stdout -Raw
    }
    return $null
}

$configState = $null
$emulator = $null
$samples = @()
$verdict = 'RUNNING'
$diagnosis = $null
$capture = $null
try {
    # The GDB stub is enabled but deliberately unused unless a freeze trips, so
    # the one available session is still free at the moment it is needed.
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    # WindowStyle: visible-by-design -- the window IS the instrument here. A
    # hidden melonDS has no MainWindowHandle and no desktop pixels, so the frame
    # hash would be constant and every run would report a freeze.
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'soak.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'soak.melonds.err') `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null
    $emulator.Refresh()
    $window = $emulator.MainWindowHandle
    if ($window -eq [IntPtr]::Zero) {
        throw 'melonDS opened no main window, so the screen cannot be watched.'
    }
    Write-Host ("soak: {0} [{1}] slot {2}, hashing the window every {3}s for {4}m; trip at {5} identical frames" -f `
        [System.IO.Path]::GetFileName($rom), $Build, $RunnerSlot, $PollSeconds,
        $MinutesToRun, $IdenticalFramesToTrip)

    $started = Get-Date
    $deadline = $started.AddMinutes($MinutesToRun)
    $previousHash = $null
    $identical = 0
    $distinct = 0
    # Repeat the press. SetForegroundWindow is refused whenever another process
    # owns the foreground, so a single synthetic press is unreliable: measured
    # 2026-07-30, two identical runs gave gNdsVSResultsPadMask 0x1000 and then 0.
    # Repeating on the poll cadence makes at least one land without needing the
    # focus race to be won on the first try. A deterministic alternative exists
    # (the controller playback path in `src/port/controller_backend.c`, already
    # driven by verify-battle-mariofox-gcrunall-loop-harness.ps1) and is the
    # right instrument if this ever needs to be exact rather than eventual.
    $startPresses = 0
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds $PollSeconds
        # R2-07: the Results screen exits on a START tap and nothing else, so a
        # passive soak can never reach match two -- which is exactly the state the
        # rematch redirect has to be tested in. One timed press turns this into a
        # two-match soak. ENTER is melonDS's START; the window must be foregrounded
        # first or SendKeys goes to whatever else has focus.
        if (($PressStartSeconds -gt 0) -and ($startPresses -lt $PressStartCount) -and
            ((Get-Date) -ge $started.AddSeconds($PressStartSeconds))) {
            # HOLD the key, do not tap it. SendKeys presses and releases within
            # milliseconds; the Results screen renders at roughly 6 FPS, so a tap
            # that short almost never falls inside a guest input sample and
            # `button_tap` never sees the edge. Measured: a SendKeys ENTER at
            # t+151s left gNdsVSResultsRematchCount at 0 with Results still
            # ticking. keybd_event with a real down/up pair spans several guest
            # frames, which is what a player actually does.
            [void][Smash64DSWindowCapture]::SetForegroundWindow($window)
            Start-Sleep -Milliseconds 400
            [Smash64DSWindowCapture]::keybd_event(0x0D, 0, 0, [UIntPtr]::Zero)
            Start-Sleep -Milliseconds 500
            [Smash64DSWindowCapture]::keybd_event(0x0D, 0, 2, [UIntPtr]::Zero)
            $startPresses++
            Write-Host ("  t+{0,5}s  held START (ENTER) 500 ms  [{1}/{2}]" -f
                [int]((Get-Date) - $started).TotalSeconds, $startPresses,
                $PressStartCount)
        }
        $emulator.Refresh()
        if ($emulator.HasExited) {
            $verdict = 'EMULATOR-EXITED'
            $diagnosis = "melonDS exited with code $($emulator.ExitCode)"
            break
        }
        $hash = Get-MelonDSWindowFrameHash -WindowHandle $window
        $elapsed = [int]((Get-Date) - $started).TotalSeconds
        if ($hash -eq $previousHash) {
            $identical++
        } else {
            if ($identical -gt 0) {
                Write-Host ("  t+{0,5}s  moving again after {1} identical frame(s)" -f $elapsed, $identical)
            }
            $identical = 0
            $distinct++
        }
        $samples += [pscustomobject]@{
            elapsedSeconds = $elapsed
            frameHash = $hash.Substring(0, 16)
            identicalRun = $identical
        }
        $previousHash = $hash
        if ($identical -ge ($IdenticalFramesToTrip - 1)) {
            # Never having seen the picture MOVE is either a broken capture or a
            # ROM that died before it animated anything -- and since the hash went
            # chrome-free the second is the common case, so decide it from pixels
            # instead of assuming. A uniform grab is the instrument's fault; a
            # detailed one that never changes is the ROM's.
            if ($distinct -le 1) {
                $colors = Measure-MelonDSWindowDistinctColors -WindowHandle $window
                if ($colors -le 2) {
                    $verdict = 'CAPTURE-STATIC'
                    $diagnosis = ("the guest area holds only $colors distinct " +
                        'colour(s), so the capture is suspect rather than the ROM')
                } else {
                    $verdict = 'FROZEN-FROM-START'
                    $diagnosis = ("the picture never changed once across " +
                        "$($identical + 1) samples yet holds $colors distinct " +
                        'colours, so the ROM drew a frame and then stopped')
                }
            } else {
                $verdict = 'FROZEN-PICTURE'
                $diagnosis = ("{0} consecutive identical window hashes over {1}s, " +
                    "after {2} distinct frames" -f ($identical + 1),
                    (($identical + 1) * $PollSeconds), $distinct)
            }
            break
        }
        if (($distinct % 6) -eq 0) {
            Write-Host ("  t+{0,5}s  alive, {1} distinct frames so far" -f $elapsed, $distinct)
        }
    }
    if ($verdict -eq 'RUNNING') { $verdict = 'NO-FREEZE' }
    Write-Host ''
    Write-Host "verdict: $verdict$(if ($diagnosis) { " -- $diagnosis" })"

    # A clean run leaves the one GDB session unspent, so spend it here. Without
    # this, "N minutes with a changing picture" is all the soak can claim -- and
    # that is much weaker than it sounds, because a ROM parked on the animating
    # results screen also shows changing frames. gNdsVSResultsStartCount ticks
    # once per results-scene start, so it is the match count, and it is the number
    # that decides whether a soak exercised match teardown and rematch at all or
    # simply watched one match's aftermath for twenty-five minutes.
    if ($verdict -eq 'NO-FREEZE') {
        $cleanFields = @(
            'sVBlankCount',
            'gNdsBattlePlayablePacingPresentedFrames',
            'dSYTaskmanUpdateCount',
            'gNdsVSResultsStartCount',
            'gNdsVSResultsTickCount',
            'gNdsSyMallocOverflowCount',
            # A clean run still has to prove it did not fall off the arena-search
            # cliff: below 0x130000 = 1245184 the build has quietly lost ~237 KB
            # of game heap AND the Task 36 replay path, and a soak that only
            # watched pixels would call that healthy.
            'gNdsTaskmanArenaChosenSize',
            'gNdsTaskmanArenaAllocFailCount',
            # Minutes, seeded to 1 at scene_harness.c:182. Read so the soak can
            # say how much of its own runtime was actually gameplay.
            'gSCManagerTransferBattleState.time_limit',
            'gNdsR2AnimCacheArenaUsedBytes',
            'gNdsR2AnimCacheArenaOverflows',
            'gNdsR2AnimCacheFills',
            'gNdsR2AnimCacheHits',
            'gNdsR2AnimCacheRejects',
            # R2-07 R1. The Battle -> Results hand-off is the visible half of the
            # owner's complaint -- ~30 s of dead air with the last battle frame
            # still on screen -- and this soak already reaches Results exactly
            # once, so it is the cheapest place to price it. FuncStart is the whole
            # task-start; SetupFiles is the fighter-asset half inside it, so the
            # difference is the scene's own file list plus construction.
            # ...and the enclosing span, because FuncStart turned out to be only
            # 21,851,904 ticks (0.65 s) of a hand-off recorded at ~30 s. Battle
            # taskman exit to the first Results tick, so it is the dead air itself.
            'gNdsVSResultsTransitionTicks',
            'gNdsVSResultsFuncStartTicks',
            'gNdsVSResultsSetupFilesTicks',
            'gNdsVSResultsSetupFilesCalls',
            # The reveal, which is the dead air as the player experiences it. The
            # source holds the wallpaper to Results tic 80 and the panels to tic
            # 120, so these are per-frame cost x 80 and x 120 -- divide by
            # 33,514,000 for seconds.
            'gNdsVSResultsToWallpaperTicks',
            'gNdsVSResultsToResultsTicks',
            # R2-07: START on Results restarts the match. Non-zero proves the
            # redirect fired, which is the only way a soak reaches match two --
            # a passive one still cannot, because nothing presses START.
            'gNdsVSResultsRematchCount',
            # Sticky input evidence. SeenMask 0 after a press means the key never
            # reached the guest; SeenMask non-zero with TapMask 0 means the edge
            # is wrong. START_BUTTON is 0x1000.
            'gNdsVSResultsInputPollCount',
            'gNdsVSResultsPadMask',
            'gNdsVSResultsInputSeenMask',
            'gNdsVSResultsInputTapMask',
            # The publish interlock. Suppressed counts the second
            # syControllerUpdateGlobalData of a single read -- each of those used
            # to overwrite a live button_tap with zero. EdgeSeenMask non-zero
            # with PublishedTapMask zero would mean the edge is computed and then
            # lost after the publish, which is a different defect again.
            # All seven are reset on the first Results tick, so they are Results
            # only and divide by gNdsVSResultsTickCount. Run-global versions of
            # these lied once already: a soak presses START on a wall-clock
            # schedule, so early presses land during the battle.
            'gNdsControllerReadCount',
            'gNdsControllerReadEdgeCount',
            'gNdsControllerPublishCount',
            'gNdsControllerPublishSuppressedCount',
            'gNdsControllerPublishTapNonzeroCount',
            'gNdsControllerEdgeSeenMask',
            'gNdsControllerPublishedTapMask',
            # Second-entry preparation. A rematch and a Sudden Death are both
            # re-entries into the battle scene, and both were drawing against
            # resources the scene load had torn down. PrepareCount must rise once
            # per battle entry; a non-zero ViolationCount means the texture cache
            # was discarded while still marked prepared.
            'gNdsRendererBattleStaticTexturePrepareCount',
            'gNdsRendererBattleStaticTextureViolationCount',
            'gNdsSCVSBattleSuddenDeathPrepareCount',
            # Did the rematch actually re-enter the battle scene through the
            # scene manager's dispatch loop? AdapterCount rises once per
            # scManagerFuncUpdate, so 2 means scVSBattleStartScene ran twice and
            # syTaskmanStartTask rewound the general heap for match two. GObjCount
            # and MallocCount separate "rewound and refilled" from "piled on top".
            # Non-zero proves a same-kind scene re-entry was caught as stale.
            'gNdsRelocSceneReentryEvictCount',
            # Non-zero proves the native OAM path's retained texture names were
            # dropped with the VRAM behind them. Must rise once per scene change.
            'gNdsIFCommonNativeOamTextureDiscardCount',
            'gNdsSCVSBattleLifecycleArenaAdapterCount',
            'gNdsSCVSBattleOriginalGObjCount',
            'gNdsTaskmanMallocCount',
            # Tick-HUD brackets, read while match two is running. The rematch
            # slowdown is sustained (~3x) rather than a rare excursion, so one
            # sample names the owning bracket against match one's known figures:
            # FTR 392,896, SRC 471,232, STG 177,088. Attribute before cutting --
            # this bug has already produced three plausible-but-wrong causes.
            'gNdsTickHudFighterTicks',
            'gNdsTickHudStageTicks',
            'gNdsTickHudSourceTicks',
            'gNdsTickHudForegroundTicks',
            'gNdsTickHudBackgroundTicks',
            'gNdsTickHudAudioTicks',
            'gNdsTickHudFlushTicks',
            'gNdsTickHudVBlankWaitTicks',
            # The source controller THREAD is live (syMainThread5 osStartThreads
            # syControllerThreadMain), and its read/publish use the raw decomp
            # symbols, so the port wrapper cannot see them. PollCount counts every
            # osContGetReadData including the thread's, so PollCount well above
            # ReadCount is the second pipeline. WaitUpdate non-NULL means the
            # thread's unguarded publish branch (controller.c:484) is reachable.
            'gNdsControllerPollCount',
            'sSYControllerWaitUpdate',
            # The gcRunAll bracket. gcRunAll IS the Results task_update, so it is
            # the span between the publish (leaves button_tap holding START) and
            # mnVSResultsCheckExit inside it (reads zero). EntryTapMask zero means
            # the tap is already gone before task_update begins and the publish
            # value never reached this side; Alive>0 with Lost==Alive means it is
            # destroyed inside.
            'gNdsGcRunAllEntryTapMask',
            'gNdsGcRunAllExitTapMask',
            'gNdsGcRunAllTapAliveCount',
            'gNdsGcRunAllTapLostCount',
            # The exit gate itself, read from the scene's own statics rather than
            # inferred from the taskman tick count -- those are two different
            # clocks and only this one gates mnVSResultsCheckExit. TotalTimeTics
            # below AllowExitWait means START is being ignored for a reason that
            # has nothing to do with the input pipeline.
            'sMNVSResultsTotalTimeTics',
            'sMNVSResultsAllowExitWait',
            # The only port writers to gSYControllerDevices[].button_tap outside
            # the publish. Nonzero on Results would make one of them the third
            # writer; zero rules the whole fighter-script family out.
            'gNdsFighterProcessLoopControllerBridgeCount',
            'gNdsFighterProcessLoopP0InputApplyCount',
            'gNdsFighterSchedulerLoopP0InputApplyCount')
        $format = (, '%u' * $cleanFields.Count) -join ','
        # The ROM's own per-iteration sample ring, read in the SAME single stop.
        # A point read of gNdsTickHud*Ticks is ONE frame -- those globals are
        # zeroed at the top of every presentation-loop iteration
        # (taskman_seam.c:5077, :7769) -- and reading a distribution from one
        # sample has now cost this campaign three withdrawn conclusions. The ring
        # holds the last NDS_TICK_HUD_WINDOW presented iterations, so after a
        # rematch its contents are match-TWO frames, which is exactly the
        # population in question. Bucket 1 is Fighters, 2 is Stage.
        $progress = Invoke-SoakGdb -Tag 'clean' -TimeoutSeconds 90 -Commands @(
            "printf `"CLEAN=$format\n`", $($cleanFields -join ', ')",
            'printf "RINGHEAD=%u,%u\n", sBattleTickHudRingHead, sBattleTickHudRingCount',
            'echo RINGFIGHTERS=\n',
            'output sBattleTickHudRing[1]',
            'echo \n')
        if ($null -eq $progress) {
            Write-Host 'end-of-run attach failed; match count unknown for this run.'
        } else {
            $match = [regex]::Match($progress, 'CLEAN=([0-9,]+)')
            if (-not $match.Success) {
                # GDB's printf is all-or-nothing: one unresolvable name aborts the
                # whole statement, so every counter is lost together. That used to
                # be SILENT -- this branch did not exist -- and a soak whose ROM
                # predated a new counter reported nothing but the pixel verdict,
                # which is indistinguishable from a soak that simply had nothing
                # to say. Echo what GDB actually replied; the "No symbol ... in
                # current context" line names the offending counter directly.
                Write-Host 'end-of-run counter read produced no CLEAN= line. GDB said:'
                # (ring output, if any, is echoed below with the rest of the reply)
                foreach ($line in ($progress -split "`r?`n" |
                    Where-Object { $_.Trim() -ne '' } | Select-Object -Last 12)) {
                    Write-Host "    $line"
                }
            } else {
                $values = $match.Groups[1].Value -split ','
                # Keyed by NAME, not position. Adding a counter to $cleanFields
                # used to silently renumber every read below it; the arena pair
                # was inserted after both indices in use purely by luck.
                $counter = @{}
                Write-Host 'progress over the run:'
                for ($i = 0; $i -lt $cleanFields.Count; $i++) {
                    Write-Host ("    {0,-40} {1}" -f $cleanFields[$i], $values[$i])
                    $counter[$cleanFields[$i]] = [uint32]$values[$i]
                    $samples += [pscustomobject]@{
                        counter = $cleanFields[$i]; value = [uint32]$values[$i] }
                }
                $presented = $counter['gNdsBattlePlayablePacingPresentedFrames']
                $matches_run = $counter['gNdsVSResultsStartCount']
                # Owner, 2026-07-29: *"if you want to run a longer soak for any
                # reason, then you also need to change the match timer to match
                # the soak time"*. Read from the ROM rather than assumed, so the
                # two cannot drift: gSCManagerTransferBattleState.time_limit is in
                # MINUTES (scene_harness.c:182 seeds 1). Past that the match is
                # over and every remaining second watches a Results screen that
                # never exits, so the extra time proves nothing about gameplay.
                $matchMinutes = $counter['gSCManagerTransferBattleState.time_limit']
                if (($matchMinutes -gt 0u) -and ($MinutesToRun -gt $matchMinutes)) {
                    # -f binds TIGHTER than +, so a format applied after a
                    # concatenation formats only the last fragment and leaves every
                    # earlier {0} literal. Parenthesise the concatenation.
                    Write-Host (("  NOTE: soaked {0} min against a {1}-minute match " +
                        'timer, so only the first {1} min covered gameplay. Raise ' +
                        'the match timer to soak longer.') -f $MinutesToRun, $matchMinutes)
                }
                $arena = $counter['gNdsTaskmanArenaChosenSize']
                if ($arena -lt 1245184u) {
                    Write-Host ((
                        "  WARNING: taskman arena {0} is BELOW the 0x130000 floor " +
                        "after {1} failed steps -- this build lost ~237 KB of game " +
                        'heap and the Task 36 replay path. Check static BSS growth.'
                        ) -f $arena, $counter['gNdsTaskmanArenaAllocFailCount'])
                }
                Write-Host ("  => {0} results-scene start(s), i.e. {0} completed match(es)" -f $matches_run)
                # A run that never presented a battle frame has not been soaked;
                # it has failed to boot, and calling that NO-FREEZE is exactly the
                # false negative that cost this instrument two withdrawn verdicts.
                if ($presented -eq 0u) {
                    $verdict = 'NEVER-STARTED'
                    $diagnosis = ('the ROM presented zero battle frames, so nothing ' +
                        'was soaked -- the picture moved, but not because of gameplay')
                    Write-Host "verdict CORRECTED to $verdict -- $diagnosis"
                } elseif ($matches_run -ge 1u) {
                    # Reaching Results is the natural END of a soak, not a bonus:
                    # mnVSResultsCheckExit (decomp mnvsresults.c:266) returns TRUE
                    # only on a START_BUTTON tap and has no timeout, and the DS
                    # results loop (taskman_seam.c:6968) is bounded by updates only
                    # when NDS_HARNESS_FAST_LOGIC != 0 -- which every shipped target
                    # pins to 0. So an unattended ROM sits in Results forever, by
                    # original design. No passive soak can ever see match two.
                    Write-Host ("  battle completed and Results is up. A passive soak " +
                        "CANNOT reach match {0}: Results exits on START only." -f `
                        ($matches_run + 1u))
                } else {
                    Write-Host '  WARNING: no match completed. This run says nothing' `
                        'about match teardown, rematch, or cross-match drift.'
                }
            }
        }
    }

    if ($verdict -in @('FROZEN-PICTURE', 'FROZEN-FROM-START', 'CAPTURE-STATIC')) {
        [void](New-Item -ItemType Directory -Force -Path $logDir)
        $stamp = Get-Date -Format 'yyyy-MM-dd_HHmmss'
        $shot = Join-Path $logDir "$stamp-frozen.png"
        [void](Save-MelonDSWindowCapture -WindowHandle $window -Path $shot)
        Write-Host "frozen frame saved to $shot"

        $capture = Invoke-SoakGdb -Tag 'freeze' -TimeoutSeconds 120 -Commands @(
            'printf "FREEZE-PC=%p\n", $pc',
            # The whole freeze class is decomp malloc.c:30's `while (TRUE);`, which
            # compiles to a single self-branch. One instruction at the PC therefore
            # names the mechanism outright -- `b.n <self>` is a spin, anything else
            # is not -- and it needs no second sample, which matters because the
            # stub allows exactly one session and stops freeze the emulator, so
            # elapsed guest time cannot be sampled twice from inside one attach.
            'x/1i $pc',
            'backtrace 40',
            'info registers',
            'printf "REG_IME=%08x\n", *(unsigned int *)0x04000208',
            'printf "REG_IE=%08x\n", *(unsigned int *)0x04000210',
            'printf "REG_IF=%08x\n", *(unsigned int *)0x04000214',
            'printf "GXSTAT=%08x\n", *(unsigned int *)0x04000600',
            'printf "IPCSYNC=%08x\n", *(unsigned int *)0x04000180',
            'printf "IPCFIFOCNT=%08x\n", *(unsigned int *)0x04000184',
            ('printf "COUNTERS=%u,%u,%u,%u\n", sVBlankCount, ' +
             'gNdsBattlePlayablePacingPresentedFrames, dSYTaskmanUpdateCount, ' +
             'gNdsVSResultsTickCount'),
            # Separate printfs on purpose: one missing symbol fails its whole
            # command, and COUNTERS must not be lost to an absent arena counter.
            ('printf "ANIMARENA=%u,%u\n", gNdsR2AnimCacheArenaUsedBytes, ' +
             'gNdsR2AnimCacheArenaOverflows'),
            # The taskman arena is chosen by a downward calloc search at boot, so
            # ANY growth in static BSS can push it over the 0x130000 = 1245184
            # cliff and cost ~237 KB in one step. A heap-exhaustion freeze is
            # therefore not fully diagnosed without the size that was actually
            # secured -- print it next to the request that failed.
            ('printf "TASKARENA=%u,%u\n", gNdsTaskmanArenaChosenSize, ' +
             'gNdsTaskmanArenaAllocFailCount'),
            ('printf "MALLOCOVF=%u,id=%u,req=%u,head=%u,lr=%08x\n", ' +
             'gNdsSyMallocOverflowCount, gNdsSyMallocOverflowArenaID, ' +
             'gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom, ' +
             'gNdsSyMallocOverflowCallerLR'))
        if ($capture) {
            Write-Host '--- freeze capture'
            Write-Host $capture
            # A static picture is not proof of a hang, and the capture already
            # holds the discriminator: `x/1i $pc` on a spin disassembles to a
            # branch to its OWN address, which is what decomp malloc.c:30's
            # `while (TRUE);` and ndsSyMallocOverflowHalt both compile to. Any
            # other instruction means the ARM9 was executing real work when it was
            # halted, and a long scene load looks exactly like a freeze from
            # outside. Say which one was observed rather than assuming the worse.
            $pcMatch = [regex]::Match($capture, 'FREEZE-PC=(0x[0-9a-fA-F]+)')
            $spinning = $false
            if ($pcMatch.Success) {
                $pcText = $pcMatch.Groups[1].Value
                $spinning = $capture -match
                    ('=>\s*' + [regex]::Escape($pcText) + '[^\r\n]*\bb(?:\.n|\.w)?\s+' +
                     [regex]::Escape($pcText) + '\b')
            }
            if (-not $spinning) {
                $verdict = "$verdict-UNCONFIRMED"
                Write-Host ''
                Write-Host ("verdict DOWNGRADED to $verdict -- the PC is not a " +
                    'self-branch, so the core was executing when halted. This is a ' +
                    'stall or a slow scene load, NOT the allocator spin class. ' +
                    'Compare against the ~30s NitroFS scene-load dead air before ' +
                    'treating it as a hang.')
            } else {
                Write-Host ''
                Write-Host ('spin CONFIRMED: the PC branches to itself, which is ' +
                    'the `while (TRUE);` allocator-exhaustion signature.')
            }
        } else {
            Write-Host 'freeze capture produced nothing; the attach failed outright.'
        }
        Set-Content -LiteralPath (Join-Path $logDir "$stamp-$verdict.txt") -Value (
            @("rom=$rom", "build=$Build", "verdict=$verdict",
              "diagnosis=$diagnosis", "screenshot=$shot", '') +
            ($samples | Format-Table -AutoSize | Out-String) + $capture)
        Write-Host "capture written to artifacts\verification\freeze-soak\$stamp-$verdict.txt"
    }

    if ($JsonOut) {
        [void](New-Item -ItemType Directory -Force `
            -Path (Split-Path -Parent $JsonOut))
        @{
            rom = $rom
            build = $Build
            runnerSlot = $RunnerSlot
            verdict = $verdict
            diagnosis = $diagnosis
            pollSeconds = $PollSeconds
            minutesToRun = $MinutesToRun
            identicalFramesToTrip = $IdenticalFramesToTrip
            samples = $samples
        } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $JsonOut
    }
    if ($verdict -ne 'NO-FREEZE') { exit 2 }
} finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
