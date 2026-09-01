param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 4623,
    [int]$RunnerSlot = -1,
    [Parameter(Mandatory=$true)][string]$Build,
    [string]$Target = 'smash64ds-results-lab-hwtri',
    [switch]$NoBuild,
    # -1 leaves the configured default alone. Setting this at runtime keeps
    # matched-source screenshots on the exact same ROM while selecting the
    # native (8/9) or generic (0) renderer arm.
    [ValidateRange(-1, 9)][int]$RendererFastRunMode = -1,
    # The SOURCE clock, not the wall clock. See the header comment.
    [ValidateRange(1, 100000)][int]$Tic = 160,
    [Parameter(Mandatory=$true)][string]$Output,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 600
)

# Capture the VS Results screen at an exact SOURCE tic.
#
# WHY THIS EXISTS. Comparing two Results builds needs both captures taken at the
# same point on the source's own timeline. A wall-clock delay cannot do that:
# the whole point of a Results optimization is that the candidate runs faster,
# so the same delay lands the two arms on DIFFERENT source tics and the diff
# shows the scene's own animation rather than the change under test. R2-07 R2b
# paid for that once already and it is now a standing rule. `capture-melonds.ps1`
# keys on wall clock, and its exact-frame path
# (`capture-cut-g-exact-frames.ps1`) conditions on `gNdsRendererProfileFrameCount`
# at `ndsBattlePlayableFrameCompleteMarker` -- both battle-only symbols that the
# Results loop never reaches. Hence a Results-shaped sibling rather than another
# parameter on either.
#
# HOW. `sMNVSResultsTotalTimeTics` is the scene's own tic counter, incremented
# once per Results update. Break on the presentation call conditioned on it, so
# the emulator halts with the frame for that exact tic on screen, then capture
# the window. Same tic on both arms means the only difference left in the pair
# is the code.
#
# Pair it with `smash64ds-results-lab-hwtri` (harness mode `results_playable`),
# which boots straight into Results -- a capture then costs seconds instead of a
# full emulated match.

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

# Build here rather than trusting whatever is on disk. soak-freeze-watch.ps1
# shipped a -NoBuild switch that built nothing and silently soaked a stale ROM
# for a day; the switch means what its name says in this script.
if (-not $NoBuild) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    make -C $root "TARGET=$Target" "BUILD=$Build"
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
foreach ($path in @($rom, $elf)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "Required capture input is missing: $path"
    }
}

$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot $RunnerSlot
$configState = $null
$emulator = $null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port `
        -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    # WindowStyle: visible-by-design -- the window IS the instrument here. A
    # hidden melonDS has no MainWindowHandle, so the capture below would throw
    # (or, worse, write a blank surface) while the emulation ran perfectly.
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'capture-results.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'capture-results.melonds.err') `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null
    $emulator.Refresh()
    $window = $emulator.MainWindowHandle
    if ($window -eq [IntPtr]::Zero) {
        throw 'melonDS opened no main window, so nothing can be captured.'
    }

    Write-Host ("capture: {0} [{1}] at Results tic {2}" -f `
        [System.IO.Path]::GetFileName($rom), $Build, $Tic)

    # One attach, one conditional breakpoint, one continue. The stub serves a
    # single session per emulation run, so this cannot be retried.
    $script = Join-Path $temp 'capture-results.gdb'
    $stdout = Join-Path $temp 'capture-results.gdb.out'
    $stderr = Join-Path $temp 'capture-results.gdb.err'
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $modeCommands = if ($RendererFastRunMode -ge 0) {
        @(
            "set variable gNdsRendererFastRunMode = $RendererFastRunMode",
            'printf "RESULTS_FAST_MODE=%u\n", gNdsRendererFastRunMode'
        )
    } else { @() }
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)"
    ) + $modeCommands + @(
        # ndsPlatformEndFrame closes the presented frame, so halting here leaves
        # the completed picture for this tic on the screen.
        "tbreak ndsPlatformEndFrame if sMNVSResultsTotalTimeTics == $Tic",
        'continue',
        # Print what was actually reached. A capture that silently stopped
        # somewhere else would otherwise look like a successful pair.
        'printf "REACHED-TIC=%d\n", sMNVSResultsTotalTimeTics',
        'printf "RESULTS-STARTS=%u\n", gNdsVSResultsStartCount'
    ))
    $gdbProcess = Start-Process -FilePath $Gdb `
        -ArgumentList @('-q', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr -WindowStyle Hidden -PassThru

    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $reached = $null
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 500
        if (Test-Path -LiteralPath $stdout) {
            $text = Get-Content -LiteralPath $stdout -Raw
            if ($text -match 'REACHED-TIC=(\d+)') {
                $reached = [int]$Matches[1]
                break
            }
        }
        if ($gdbProcess.HasExited) { break }
    }
    if ($null -eq $reached) {
        if (Test-Path -LiteralPath $stdout) {
            Write-Host 'GDB never reported the tic. Last output:'
            Get-Content -LiteralPath $stdout -Tail 15 | ForEach-Object { "    $_" }
        }
        throw "Never reached Results tic $Tic within ${TimeoutSeconds}s."
    }
    if ($reached -ne $Tic) {
        throw "Halted at Results tic $reached, not the requested $Tic."
    }

    # FOREGROUND FIRST. Save-MelonDSWindowCapture reads the DESKTOP REGION the
    # window occupies, not the window's own back buffer, so whatever is stacked
    # on top of melonDS is what lands in the file. Measured 2026-07-30: the
    # first version of this script omitted the call and captured the owner's
    # browser for both arms -- two screenshots of an unrelated web page that
    # diffed at 67.7% and looked exactly like a catastrophic visual regression.
    # Every other capture harness in this repo foregrounds before shooting
    # (`capture-melonds.ps1:444`, `:462`); this one has to as well.
    #
    # Safe to do here precisely because the core is halted at the breakpoint:
    # bringing the window forward cannot advance the emulation, so the picture
    # still belongs to the requested tic. The settle delay is for the compositor
    # to finish raising the window, not for the guest.
    # READ THE WINDOW, NOT THE DESKTOP. The default capture path calls
    # CopyFromScreen, which copies whatever pixels occupy the window's screen
    # rectangle and does not throw when something is stacked on top. Measured
    # 2026-07-30, twice: this script wrote two "matched-tic" pairs that were
    # actually screenshots of the owner's browser, because Windows had refused
    # to raise melonDS. They diffed at 67.7% of pixels with a max channel delta
    # of 255 and read exactly like a catastrophic visual regression in the
    # change under test. Nothing flagged it; only opening the image did.
    #
    # PrintWindow asks the window to render itself, so occlusion is irrelevant,
    # no foreground raise is needed, and this cannot photograph the operator's
    # desktop by mistake. That last property is the important one: a capture
    # harness must never be able to write somebody's screen into artifacts/.
    [void](New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Output))
    [void](Save-MelonDSWindowCapture -WindowHandle $window -Path $Output `
        -PreferPrintWindow)

    # PrintWindow's failure mode is a blank surface on GPU-composited windows,
    # not an error, so prove the file has a picture in it before calling it
    # evidence. Measured from the FILE, not from a second window read: the
    # helper's sampler takes its own CopyFromScreen bitmap, which would put the
    # desktop back into the one check meant to catch a bad capture.
    $written = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Output).Path)
    try {
        $seen = New-Object 'System.Collections.Generic.HashSet[int]'
        for ($y = 0; $y -lt $written.Height; $y += 16) {
            for ($x = 0; $x -lt $written.Width; $x += 16) {
                [void]$seen.Add($written.GetPixel($x, $y).ToArgb())
            }
        }
        $colors = $seen.Count
    } finally {
        $written.Dispose()
    }
    if ($colors -lt 8) {
        Remove-Item -LiteralPath $Output -Force -ErrorAction SilentlyContinue
        throw ("PrintWindow returned a near-uniform surface ($colors distinct " +
               "colours), which means it captured nothing. Deleted $Output " +
               "rather than leave a blank file that looks like a result.")
    }
    Write-Host "captured Results tic $reached ($colors distinct colours) -> $Output"

    if (-not $gdbProcess.HasExited) { Stop-Process -Id $gdbProcess.Id -Force }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
