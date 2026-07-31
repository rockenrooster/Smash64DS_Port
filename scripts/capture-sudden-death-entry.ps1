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
    make -C $root "TARGET=$Target" "BUILD=$Build" 'NDS_R2_BOTH_CPU=0'
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
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",

        # Stage 1: the match scene is up.
        'tbreak scVSBattleStartBattle',
        'continue',
        'printf "SD-STAGE=battle-start\n"',

        # Stage 2: past GO, with the countdown live. The condition matters --
        # `ifCommonTimerFuncRun` is a per-frame GObj proc that also runs BEFORE the
        # timer starts, and `ifCommonBattleUpdateInterfaceAll` resets the stamp and
        # the scheduler tic count at the GO transition (decomp ifcommon.c:3167).
        # Writing `time_remain` on an earlier hit would be overwritten by that.
        'tbreak ifCommonTimerFuncRun if sIFCommonTimerIsStarted != 0',
        'continue',
        'printf "SD-STAGE=timer-live\n"',
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
        'printf "SD-IS-RESET=%u\n", gSCManagerSceneData.is_reset',

        # Stage 4: entry. Breaking on the PORT wrapper, not the decomp one, is
        # deliberate -- the wrapper calls `ndsBaseSCVSBattleStartSuddenDeath()` and
        # only then increments the counter, so "entered" and "completed setup" are
        # separately observable and the stall localizes itself between them.
        'tbreak scVSBattleStartSuddenDeath',
        'continue',
        'printf "SD-STAGE=entered\n"',
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
        'printf "SD-HEAP-USED-ENTRY=%u\n", (unsigned)((char *)gSYTaskmanGeneralHeap.ptr - (char *)gSYTaskmanGeneralHeap.start)',
        'printf "SD-HEAP-FREE-ENTRY=%u\n", (unsigned)((char *)gSYTaskmanGeneralHeap.end - (char *)gSYTaskmanGeneralHeap.ptr)',
        'printf "SD-ARENA-FAIL-ENTRY=%u\n", gNdsTaskmanArenaAllocFailCount',

        # Stage 5: did setup finish, and does the Sudden Death match then run? If
        # the reported stall is real, one of these two never prints and the last
        # marker on stdout says which half owns it.
        'tbreak battleship_scvsbattle.c:scVSBattleFuncUpdate',
        'continue',
        'printf "SD-STAGE=running\n"',
        'printf "SD-PREPARE-COUNT=%u\n", gNdsSCVSBattleSuddenDeathPrepareCount',
        # The other side of the bracket. If the second setup pass ate most of the
        # remaining arena, this is where it shows.
        'printf "SD-HEAP-USED-RUN=%u\n", (unsigned)((char *)gSYTaskmanGeneralHeap.ptr - (char *)gSYTaskmanGeneralHeap.start)',
        'printf "SD-HEAP-FREE-RUN=%u\n", (unsigned)((char *)gSYTaskmanGeneralHeap.end - (char *)gSYTaskmanGeneralHeap.ptr)',
        'printf "SD-ARENA-FAIL-RUN=%u\n", gNdsTaskmanArenaAllocFailCount',
        # The heap-generation contract, proven engaged rather than assumed. A
        # second scene entry MUST have bumped the generation and MUST have made
        # the cache notice; a zero mismatch count here means the contract never
        # fired and the run proves nothing about it.
        'printf "SD-HEAP-GEN=%u\n", gNdsTaskmanHeapGeneration',
        'printf "SD-CACHE-GEN-MISMATCH=%u\n", gNdsR2AnimCacheArenaGenerationMismatches',
        'printf "SD-CACHE-RANGE-FAULT=%u\n", gNdsR2AnimCacheArenaRangeFaults',
        'printf "SD-CACHE-INVALIDATIONS=%u\n", gNdsR2AnimCacheArenaInvalidations',
        'printf "SD-CACHE-RESERVES=%u\n", gNdsR2AnimCacheArenaReserveCount',
        'printf "SD-DONE=1\n"'
    ) + $(if ($DiagnoseHang) { @(
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
    ) }))
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
                }
            }
            # In diagnose mode the run is not over at SD-DONE -- that is where
            # the interesting half starts.
            $finished = $(if ($DiagnoseHang) { 'SD-DIAG=end' } else { 'SD-DONE=1' })
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
