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
    # results screen ever ends"*. A longer run does not buy more coverage while
    # that is true -- the ROM reaches the results scene once and stays there, so
    # minute six onward watches the same stuck scene as minute five. Raise this
    # ceiling only once a match is known to hand back to another match.
    [ValidateRange(1, 5)][int]$MinutesToRun = 5,
    # Consecutive identical frames needed to call it frozen. Two screens of a DS
    # game in motion never render byte-identically, but a legitimately static
    # moment does exist (a settled results screen, a pause), so require several.
    [ValidateRange(2, 20)][int]$IdenticalFramesToTrip = 4,
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
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$logDir = Join-Path $root 'artifacts\verification\freeze-soak'

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
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Seconds $PollSeconds
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
            'gNdsR2AnimCacheArenaUsedBytes',
            'gNdsR2AnimCacheArenaOverflows',
            'gNdsR2AnimCacheFills',
            'gNdsR2AnimCacheHits',
            'gNdsR2AnimCacheRejects')
        $format = (, '%u' * $cleanFields.Count) -join ','
        $progress = Invoke-SoakGdb -Tag 'clean' -TimeoutSeconds 90 -Commands @(
            "printf `"CLEAN=$format\n`", $($cleanFields -join ', ')")
        if ($null -eq $progress) {
            Write-Host 'end-of-run attach failed; match count unknown for this run.'
        } else {
            $match = [regex]::Match($progress, 'CLEAN=([0-9,]+)')
            if ($match.Success) {
                $values = $match.Groups[1].Value -split ','
                Write-Host 'progress over the run:'
                for ($i = 0; $i -lt $cleanFields.Count; $i++) {
                    Write-Host ("    {0,-40} {1}" -f $cleanFields[$i], $values[$i])
                    $samples += [pscustomobject]@{
                        counter = $cleanFields[$i]; value = [uint32]$values[$i] }
                }
                $presented = [uint32]$values[1]
                $matches_run = [uint32]$values[3]
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

        $gdbScript = Join-Path $temp 'soak-freeze.gdb'
        $gdbOut = Join-Path $temp 'soak-freeze.out'
        $gdbErr = Join-Path $temp 'soak-freeze.err'
        Remove-Item $gdbOut, $gdbErr -Force -ErrorAction SilentlyContinue
        [System.IO.File]::WriteAllLines($gdbScript, @(
            'set pagination off', 'set confirm off', 'set remotetimeout 30',
            "target remote 127.0.0.1:$($context.GdbPort)",
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
            # Separate printf on purpose: one missing symbol fails its whole
            # command, and COUNTERS must not be lost to an absent arena counter.
            ('printf "ANIMARENA=%u,%u\n", gNdsR2AnimCacheArenaUsedBytes, ' +
             'gNdsR2AnimCacheArenaOverflows'),
            'detach'))
        $gdbProcess = Start-Process -FilePath $Gdb `
            -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
            -WorkingDirectory $root -RedirectStandardOutput $gdbOut `
            -RedirectStandardError $gdbErr -WindowStyle Hidden -PassThru
        $timedOut = -not $gdbProcess.WaitForExit(120000)
        if ($timedOut) {
            Stop-Process -Id $gdbProcess.Id -Force
            Write-Host 'freeze capture attach TIMED OUT -- the stub could not halt the core.'
        }
        # Read what landed either way. A timeout still leaves the commands that
        # completed before it on disk, and this is the only attach available, so
        # discarding a partial capture discards the whole run's diagnosis.
        if (Test-Path -LiteralPath $gdbOut) {
            $capture = Get-Content $gdbOut -Raw
            if ($capture) {
                Write-Host "--- freeze capture$(if ($timedOut) { ' (partial)' })"
                Write-Host $capture
            }
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
