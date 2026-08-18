[CmdletBinding()]
param(
    [string]$Build = 'build-p2-1f-sss',
    [string]$Target = 'smash64ds-p2-1f-sss-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 900)][int]$TimeoutSeconds = 300,
    [string]$OutputPrefix = ''
)

# P2-1f visibility. Captures the stage select at KNOWN screen states.
#
# WHY THIS EXISTS RATHER THAN `capture-melonds.ps1 -DelaySeconds`. The menu
# screens present far faster than real time under this emulator -- measured on
# three runs of the same ROM, the character select ran at 353, 717 and 738 fps
# against its own 60 Hz VBlank pacing -- so a wall-clock delay lands on a
# different screen every run, and the stage select's scripted visit is only 122
# presented frames wide. Two calibration runs missed it in both directions
# before this was written. The state lock here is the SCREEN'S OWN code: gdb
# breaks on `ndsMenuShellSssShowSelection`, which the screen calls exactly once
# on entry and once per cursor move (mnMapsMakeNameAndEmblem's role), so hit 1
# IS "the stage select just opened" and hit 2 IS "the cursor just moved" no
# matter how fast the host runs.
#
# HOW THE CAPTURE HAPPENS WHILE HALTED. A gdb-halted melonDS keeps its window
# up showing the last presented frame, so the shot is taken from a stopped
# target through `capture-running-melonds-window.ps1` (gdb's own `shell`).
# After each state is reached the script steps forward a few PRESENTS -- the
# selection change is composed into shadow OAM and only reaches the screen at
# the next `ndsPlatformEndFrame` -- so what is captured is the state after the
# move, not the frame before it.
#
# Nothing here writes guest memory.

$ErrorActionPreference = 'Stop'
$scripts = Split-Path -Parent $PSScriptRoot
$root = Split-Path -Parent $scripts
. (Join-Path $scripts 'lib\melonds.ps1')
. (Join-Path $scripts 'lib\gdb-markers.ps1')
. (Join-Path $scripts 'lib\build-output.ps1')
. (Join-Path $scripts 'lib\melonds-screenshot.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($OutputPrefix)) {
    $OutputPrefix = Join-Path $root ('artifacts\visibility\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-1f-sss')
}

$required = @('ndsMenuShellSssShowSelection', 'ndsPlatformEndFrame')
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("p2-1f capture symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.p2-1f-capture.stdout.log'
$stderr = Join-Path $log_dir 'melonds.p2-1f-capture.stderr.log'
$config_state = $null
$emulator = $null
$capture = Join-Path $scripts 'capture-running-melonds-window.ps1'

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    # WindowStyle: visible-by-design -- this harness photographs the emulator
    # window, and a hidden launch leaves MainWindowHandle at IntPtr.Zero, so
    # the run succeeds and every PNG comes out black. Exactly the case
    # check-melonds-policy.ps1's per-call-site exemption exists for.
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -PassThru
    $deadline = (Get-Date).AddSeconds(30)
    do {
        Start-Sleep -Milliseconds 250
        $emulator.Refresh()
    } while (($emulator.MainWindowHandle -eq [IntPtr]::Zero) -and
             (-not $emulator.HasExited) -and ((Get-Date) -lt $deadline))
    if ($emulator.HasExited -or ($emulator.MainWindowHandle -eq [IntPtr]::Zero)) {
        throw 'melonDS did not present a window to capture.'
    }
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    # Foreground once, while emulation is still running: bringing the window
    # forward after the target is halted can capture a Qt repaint instead of
    # the completed DS presentation (capture-melonds.ps1's own lesson). The
    # window is left at whatever size it opens, which is why the assertions
    # against these PNGs run with -WindowScaledCapture.
    [void][Smash64DSWindowCapture]::SetForegroundWindow(
        $emulator.MainWindowHandle)
    Start-Sleep -Milliseconds 300

    $shot = {
        param([string]$Name)
        @(
            'delete',
            'break ndsPlatformEndFrame',
            # SIX SEPARATE CONTINUES, and both halves of that are load-bearing.
            # SEPARATE, because `continue 6` sets the IGNORE COUNT of the
            # breakpoint that last stopped -- and the one that last stopped was
            # just deleted, so gdb continues exactly once and says nothing.
            # SIX, because a stop AT ndsPlatformEndFrame is before that frame
            # is presented: the first capture written this way came out one
            # state behind on both shots (a black screen for "the stage select
            # just opened", and the pre-move cursor for "the cursor moved").
            'continue', 'continue', 'continue',
            'continue', 'continue', 'continue',
            ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
             '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' +
             ($OutputPrefix + '-' + $Name + '.png') + '"')
        )
    }

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        # Hit 1: the stage select has just opened. mnMapsInitVars put the
        # cursor on the cell maps_vsmode_gkind names -- Dream Land on a cold
        # boot -- and the other eight grounds draw the locked plate.
        'break ndsMenuShellSssShowSelection',
        'continue',
        'printf "SSSCAP default slot=%u gkind=%02x\n", gNdsMenuShellSssCursorSlot, gNdsMenuShellSssCursorGkind'
    ) + (& $shot 'default') + @(
        # Hit 2: the scripted RIGHT has moved the cursor. Slots 7 and 8 are
        # locked, so the move lands on 9 -- RANDOM.
        'delete',
        'break ndsMenuShellSssShowSelection',
        'continue',
        'printf "SSSCAP random slot=%u gkind=%02x\n", gNdsMenuShellSssCursorSlot, gNdsMenuShellSssCursorGkind'
    ) + (& $shot 'random') + @(
        'printf "SSSCAP done move=%u blocked=%u\n", gNdsMenuShellSssMoveCount, gNdsMenuShellSssBlockedCount',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'p2_1f_sss_capture.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
