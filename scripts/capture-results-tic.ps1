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
    # Start hidden; the shared capture helper locates the owned window at the stop.
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'capture-results.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'capture-results.melonds.err') `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null

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
        'break ndsSyMallocOverflowHalt', 'commands', 'silent',
        'printf "RESULTS-OOM request=%u free=%u arena=%u\n", gNdsSyMallocOverflowRequest, gNdsSyMallocOverflowHeadroom, gNdsTaskmanArenaChosenSize',
        'bt', 'quit 2', 'end',
        # ndsPlatformEndFrame closes the presented frame, so halting here leaves
        # the completed picture for this tic on the screen.
        "tbreak ndsPlatformEndFrame if (gSCManagerSceneData.scene_curr == nSCKindVSResults) && (sMNVSResultsTotalTimeTics == $Tic)",
        'continue',
        'printf "RESULTS-CHECK fighters=%u pack=%u extern=%u native=%u free=%u\n", gNdsVSResultsFighterCount, gNdsPreviewPackFailure, gNdsBattleCoreExternFailure, gNdsRendererNativeFailure.count, gNdsTaskmanGeneralHeapFreeMin',
        'if (gNdsVSResultsFighterCount == 0) || gNdsPreviewPackFailure || gNdsBattleCoreExternFailure || gNdsRendererNativeFailure.count',
        'quit 2', 'end',
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

    & (Join-Path $root 'scripts/capture-running-melonds-window.ps1') `
        -EmulatorProcessId $emulator.Id -Output $Output
    Write-Host "captured Results tic $reached -> $Output"

    if (-not $gdbProcess.HasExited) { Stop-Process -Id $gdbProcess.Id -Force }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
