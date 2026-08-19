[CmdletBinding()]
param(
    [string]$Build = 'build-p2-1k-d',
    [string]$Target = 'smash64ds-p2-shell-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 900)][int]$TimeoutSeconds = 420,
    [string]$OutputPrefix = '',
    # Which presented title frames to photograph. Defaults bracket the
    # animation: one early pose, the peak-cost pose the bake names (19), one
    # late pose, and one well past the tic-220 snap so the settled screen is in
    # the same run as the moving ones.
    [int[]]$Presents = @(10, 19, 34, 60, 140)
)

# P2-1k (d) visibility. Photographs the title's pop animation MID-FLIGHT, which
# is the one thing `capture-p2-shell.ps1` structurally cannot do: it locks on a
# screen's own entry point and shoots six presents later, so every title shot it
# takes is the same early pose.
#
# THE LOCK IS THE PRESENT, and the pose is READ rather than assumed. The
# animation samples itself from `ndsPlatformVBlankCount`, so the pose a given
# present carries depends on what that frame cost -- which is the whole point of
# driving it from VBlanks, and it means a script that counted presents and then
# CLAIMED a pose would be writing fiction on any frame that took two VBlanks.
# Every shot prints `gNdsUiKitTitleAnimPose` beside it, and the artifact records
# the pose that was photographed.
#
# ONE OFF-BY-ONE, STATED. A stop at `ndsPlatformEndFrame` is BEFORE that frame
# reaches the panel, and the pose for it was drawn by the update just above, so
# the image on screen at stop N is pose N-1's. The printed `pose` is therefore
# one ahead of the photograph, and `shot` in the marker line is the corrected
# number.
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
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-1k-d')
}

$required = @('ndsPlatformEndFrame', 'ndsMenuShellRunTitle',
              'gNdsUiKitTitleAnimPose', 'gNdsUiKitTitleAnimArmCount',
              'gNdsUiKitTitleAnimSettleCount',
              'gNdsUiKitTitleAnimLoadFailCount',
              'gNdsUiKitTitleAnimMaxTicks', 'gNdsTitleFireFrameCount',
              'gNdsMenuShellVBlankHist', 'gNdsMenuShellVBlankMax',
              'gNdsMenuShellWorkMax')
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("p2-1k-d capture symbols absent from {0}: {1}" -f $elf,
           ($missing -join ', '))
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$config_state = $null
$emulator = $null
$capture = Join-Path $scripts 'capture-running-melonds-window.ps1'

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    # WindowStyle: visible-by-design -- this harness photographs the emulator
    # window, and a hidden launch leaves MainWindowHandle at IntPtr.Zero, so
    # the run succeeds and every PNG comes out black.
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
    [void][Smash64DSWindowCapture]::SetForegroundWindow(
        $emulator.MainWindowHandle)
    Start-Sleep -Milliseconds 300

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'break ndsMenuShellRunTitle',
        'continue',
        'delete',
        'break ndsPlatformEndFrame'
    )
    $reached = 0
    foreach ($present in ($Presents | Sort-Object)) {
        $step = $present - $reached
        if ($step -lt 1) {
            throw "capture-p2-1k-title-anim: -Presents must be increasing."
        }
        # SEPARATE CONTINUES: `continue N` sets the ignore count of the
        # breakpoint that last stopped, which is not what is wanted here.
        $commands += @(1..$step | ForEach-Object { 'continue' })
        $commands += @(
            ('printf "ANIMCAP present=' + $present +
             ' pose=%u shot=%u arm=%u settle=%u fail=%u frames=%u ' +
             'maxticks=%u maxpose=%u fire=%u\n", gNdsUiKitTitleAnimPose, ' +
             'gNdsUiKitTitleAnimPose - 1, gNdsUiKitTitleAnimArmCount, ' +
             'gNdsUiKitTitleAnimSettleCount, gNdsUiKitTitleAnimLoadFailCount, ' +
             'gNdsUiKitTitleAnimFrameCount, gNdsUiKitTitleAnimMaxTicks, ' +
             'gNdsUiKitTitleAnimMaxPose, gNdsTitleFireFrameCount'),
            # PRESS START shares the animation's row band, so it is redrawn
            # from the kit's cache after every animated frame. `pdraw` climbing
            # with the frame count is what separates "the redraw ran" from "the
            # label happened to survive that pose's dirty rectangle".
            ('printf "ANIMPS present=' + $present +
             ' pdraw=%u perase=%u\n", gNdsUiKitSurfaceDrawCachedCount, ' +
             'gNdsUiKitSurfaceEraseCachedCount'),
            # THE CADENCE, SPLIT BY SUBTRACTION. The shell's per-screen VBlank
            # histogram is cumulative over the whole title dwell, so a single
            # reading cannot say whether an interval of 2 belonged to the
            # animation window or to the still screen after it. Printing it at
            # two presents and differencing them can: the delta between the
            # last animation present and a late one is pure steady state.
            # FOUR buckets, matching NDS_MENU_SHELL_VBLANK_BUCKETS and
            # probe-p2-shell.ps1's own MSVB line; a fifth index would silently
            # read the NEXT screen's row rather than fail.
            ('printf "ANIMVB present=' + $present +
             ' v1=%u v2=%u v3=%u v4=%u max=%u work=%u draws=%u\n", ' +
             'gNdsMenuShellVBlankHist[0][0], gNdsMenuShellVBlankHist[0][1], ' +
             'gNdsMenuShellVBlankHist[0][2], gNdsMenuShellVBlankHist[0][3], ' +
             'gNdsMenuShellVBlankMax[0], gNdsMenuShellWorkMax[0], ' +
             'gNdsUiKitTitleAnimFrameCount'),
            ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
             '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' +
             ($OutputPrefix + '-present' + $present + '.png') + '"')
        )
        $reached = $present
    }
    $commands += @('detach', 'quit')

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'p2_1k_title_anim_capture.gdb' `
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
