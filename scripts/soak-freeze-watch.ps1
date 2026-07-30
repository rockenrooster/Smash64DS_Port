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
    [ValidateRange(1, 720)][int]$MinutesToRun = 20,
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
            # An instrument that has never seen the picture MOVE has not observed
            # a freeze; it has failed to capture. Distinguish the two, because a
            # black or stale grab would otherwise report a freeze on every run
            # and every one of those reports would be worthless.
            if ($distinct -le 1) {
                $verdict = 'CAPTURE-STATIC'
                $diagnosis = ('the window hash never changed at all, so the ' +
                    'capture is suspect rather than the ROM')
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

    if ($verdict -in @('FROZEN-PICTURE', 'CAPTURE-STATIC')) {
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
            'detach'))
        $gdbProcess = Start-Process -FilePath $Gdb `
            -ArgumentList @('-q', '-batch', '-x', $gdbScript, $elf) `
            -WorkingDirectory $root -RedirectStandardOutput $gdbOut `
            -RedirectStandardError $gdbErr -WindowStyle Hidden -PassThru
        if (-not $gdbProcess.WaitForExit(120000)) {
            Stop-Process -Id $gdbProcess.Id -Force
            Write-Host 'freeze capture attach TIMED OUT -- the stub could not halt the core.'
        } else {
            $capture = Get-Content $gdbOut -Raw
            Write-Host '--- freeze capture'
            Write-Host $capture
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
