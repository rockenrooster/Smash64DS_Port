[CmdletBinding()]
param(
    [string]$Build = 'build-p2-shell',
    [string]$Target = 'smash64ds-p2-shell-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 900)][int]$TimeoutSeconds = 600,
    [string]$OutputPrefix = '',
    # Presents to step past after a state is reached. A stop AT
    # ndsPlatformEndFrame is BEFORE that frame reaches the screen, and the kit
    # composes into shadow OAM, so a capture taken at the state's own stop is
    # one state behind -- measured twice, once as a black screen and once as
    # the pre-move cursor.
    [ValidateRange(1, 32)][int]$PresentsAfterState = 6,
    # Capture only these states (names from the table below), in table order.
    # A direct-battle ROM has no shell symbols, so `-Only battle-intro,
    # fighter-entry-1,fighter-entry-2` is how a boot-into-battle lab build is
    # photographed; the symbol check below covers only the selected states.
    [string[]]$Only = @(),
    # Extra shots AFTER fighter-entry-1, each N steps after the previous one
    # (cumulative), named fighter-entry-1+<total>. A time series of the source
    # entry effect from one run: `-EntrySeries 16,32,64`. A step is one
    # ndsPlatformEndFrame, which this target reaches once per 60 Hz LOGIC
    # frame (two per presented frame), so +32 is source frame 32.
    # Strings, not [int[]]: invoked through `pwsh -File`, an [int[]] parameter
    # given `32,32,64` binds as the single int 323264 (culture-aware parsing
    # treats the commas as thousands separators), and two runs on 2026-08-23
    # then stepped three hundred thousand logic frames before their ceiling.
    # Each element is split on commas below.
    [string[]]$EntrySeries = @()
)

# P2-1g visibility, and the phase-level successor to
# scripts/menus/capture-p2-1f-sss.ps1. Captures EVERY shell screen at a KNOWN
# state in one run, for the owner's visual pass at the P2-1 close.
#
# WHY THIS EXISTS RATHER THAN `capture-melonds.ps1 -DelaySeconds`. The menu
# screens present far faster than real time under this emulator -- measured on
# three runs of the same ROM, the character select ran at 353, 717 and 738 fps
# against its own 60 Hz VBlank pacing -- so a wall-clock delay lands on a
# different screen every run, and the stage select's scripted visit is only 122
# presented frames wide. Two calibration runs missed it in both directions
# before the P2-1f version of this was written.
#
# THE LOCK IS THE SCREEN'S OWN ENTRY POINT. `scManagerRunLoop` dispatches each
# shell screen through its own exported `ndsMenuShellRun<X>`, so breaking there
# IS "screen X just opened" no matter how fast the host runs -- one symbol per
# screen, no hit arithmetic, and the printf beside each one reads
# `gNdsMenuShellScreen` back so the artifact records which screen was actually
# photographed rather than which one was intended. The two stage-select states
# keep P2-1f's finer lock (`ndsMenuShellSssShowSelection`, called once on entry
# and once per cursor move) because they differ by a cursor move inside one
# screen rather than by which screen is up.
#
# HOW THE CAPTURE HAPPENS WHILE HALTED. A gdb-halted melonDS keeps its window
# up showing the last presented frame, so the shot is taken from a stopped
# target through `capture-running-melonds-window.ps1` (gdb's own `shell`).
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
        (Get-Date -Format 'yyyy-MM-dd') + '_p2-shell')
}

# One entry per screenshot, in the order a cold boot reaches them.
$states = @(
    # P2-1h deleted the splash: boot reaches the title with no screen in
    # between, so the title IS the first capture of a cold boot.
    @{ Name = 'title';       Break = 'ndsMenuShellRunTitle' },
    @{ Name = 'main-menu';   Break = 'ndsMenuShellRunModeSelect' },
    @{ Name = 'vs-rules';    Break = 'ndsMenuShellRunVSMode' },
    @{ Name = 'css-default'; Break = 'ndsMenuShellRunCharSelect' },
    # The first CSS commit in the walk is reached only after accepted START and
    # its source 30-tic proceed wait. Input/update is parked for that wait, so
    # the last presented frame is the READY TO FIGHT state the player started
    # from. Unlike the tiny ShowReady helper, this seam survives optimization.
    @{ Name = 'css-ready';   Break = 'ndsMenuShellCssCommit'; Presents = 0 },
    @{ Name = 'sss-default'; Break = 'ndsMenuShellSssShowSelection' },
    # Hit 2 of the same symbol: the scripted RIGHT has moved the cursor. Slots
    # 7 and 8 are locked, so the move lands on 9 -- RANDOM.
    @{ Name = 'sss-random';  Break = 'ndsMenuShellSssShowSelection' },
    # Match-start presentation boundary. `scVSBattleStartBattle` is reached
    # only after the stage-select handoff has entered VSBattle. Advancing a
    # handful of presents from its entry photographs the source fighter-entry
    # window (Mario pipe / Fox Arwing), and is also the regression image for
    # the lower HUD: it must be absent from both SSS captures above and become
    # visible here once the VSBattle interface has actually displayed.
    @{ Name = 'battle-intro'; Break = 'scVSBattleStartBattle'; Presents = 8 },
    # The source intentionally waits before starting fighter entries, so a
    # fixed frame count from scene entry is a poor intro oracle. Capture the
    # first two actual ftCommonAppearSetStatus calls instead. With the canonical
    # Mario/Fox match these cover both fighter-specific entry paths regardless
    # of link-list order, and advancing after the call captures the animation /
    # effect rather than the pre-status frame at the breakpoint itself.
    @{ Name = 'fighter-entry-1'; Break = 'ftCommonAppearSetStatus'; Presents = 8 }
)
$entryTotal = 8
$EntrySeries = @($EntrySeries |
    ForEach-Object { "$_" -split '[,; ]+' } |
    Where-Object { $_ -ne '' } |
    ForEach-Object { [int]$_ })
foreach ($step in $EntrySeries) {
    $entryTotal += $step
    $states += @{ Name = ('fighter-entry-1+' + $entryTotal);
                  Break = 'ndsPlatformEndFrame'; Presents = $step - 1 }
}
$states += @{ Name = 'fighter-entry-2'; Break = 'ftCommonAppearSetStatus'; Presents = 8 }

if ($Only.Count -gt 0) {
    $unknown = @($Only | Where-Object { $name = $_; -not ($states | Where-Object { $_.Name -eq $name }) })
    if ($unknown.Count -gt 0) {
        throw ('Unknown -Only state(s): ' + ($unknown -join ', '))
    }
    $states = @($states | Where-Object { ($Only -contains $_.Name) -or ($_.Name -like 'fighter-entry-1+*') })
}
$required = @('ndsPlatformEndFrame') +
    @($states | ForEach-Object { $_.Break } | Select-Object -Unique)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
# A boot-into-battle lab ROM links no menu shell; its captures still work,
# they just cannot report the shell screen/cursor in the marker line.
$hasShell = $symbols -contains 'gNdsMenuShellScreen'
if (-not $hasShell -and ($states | Where-Object { $_.Break -like 'ndsMenuShell*' })) {
    throw ('{0} has no menu shell; select only battle states with -Only' -f $elf)
}
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("p2-shell capture symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$config_state = $null
$emulator = $null
$capture = Join-Path $scripts 'capture-running-melonds-window.ps1'

try {
    # BreakOnStartup: the first state's symbol (ndsMenuShellRunTitle) runs
    # ONCE per screen entry, and an unthrottled guest reaches the title
    # before gdb has connected -- two 2026-08-22 runs attached inside the
    # title's own animation and waited the full timeout for a breakpoint
    # that could no longer fire. Hold ARM9 at reset until the script's
    # first `continue`, the way the battle proof harness does.
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -BreakOnStartup -MuteAudio
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
    # the completed DS presentation (capture-melonds.ps1's own lesson).
    [void][Smash64DSWindowCapture]::SetForegroundWindow(
        $emulator.MainWindowHandle)
    Start-Sleep -Milliseconds 300

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort)
    )
    foreach ($state in $states) {
        $statePresents = if ($state.ContainsKey('Presents')) {
            [int]$state.Presents
        } else {
            $PresentsAfterState
        }
        $commands += @(
            'delete',
            ('break ' + $state.Break)
        )
        if ($state.ContainsKey('Condition')) {
            # `$bpnum` is GDB's last-created breakpoint number. Breakpoint IDs
            # keep increasing across `delete`, so hard-coding `condition 1`
            # would silently condition only the first state in this sequence.
            $commands += ('condition $bpnum ' + $state.Condition)
        }
        $commands += @(
            'continue',
            $(if ($hasShell) {
                ('printf "SHELLCAP ' + $state.Name +
                 ' screen=%u sss_slot=%u sss_gkind=%02x\n", gNdsMenuShellScreen, ' +
                 'gNdsMenuShellSssCursorSlot, gNdsMenuShellSssCursorGkind')
            } else {
                ('printf "SHELLCAP ' + $state.Name + '\n"')
            }),
            'delete',
            'break ndsPlatformEndFrame'
        )
        # P2-1i. The fire's own counters at every state, which is what makes
        # ONE run carry both halves of the proof: on the title they must climb
        # once per presented frame with enable=1/disable=0, and on every later
        # screen they must be FROZEN with enable==disable -- the negative
        # control. Printed at the state's entry (before the presents below) and
        # again after them, so the title's delta over a known number of
        # presents is readable rather than inferred.
        # Index 0 is NDS_MENU_SHELL_SCREEN_TITLE (nds_menu_shell.h:48).
        $fire_args = if ($hasShell) {
            (' en=%u dis=%u frame=%u reveal=%u titleframes=%u\n", ' +
            'gNdsTitleFireEnableCount, gNdsTitleFireDisableCount, ' +
            'gNdsTitleFireFrameCount, gNdsTitleFireRevealFrame, ' +
            'gNdsMenuShellFrames[0]')
        } else { '\n"' }
        $commands += @(
            ('printf "SHELLFIRE ' + $state.Name + ' enter' + $fire_args))
        # SEPARATE CONTINUES, and that is load-bearing: `continue N` sets the
        # IGNORE COUNT of the breakpoint that last stopped, and the one that
        # last stopped was just deleted, so gdb would continue exactly once and
        # say nothing.
        if ($statePresents -gt 0) {
            $commands += @(1..$statePresents | ForEach-Object { 'continue' })
        }
        $commands += @(
            ('printf "SHELLFIRE ' + $state.Name + ' shot' + $fire_args))
        $commands += @(
            ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' + $capture +
             '" -EmulatorProcessId ' + $emulator.Id + ' -Output "' +
             ($OutputPrefix + '-' + $state.Name + '.png') + '"')
        )
    }
    $commands += @(
        $(if ($hasShell) {
            'printf "SHELLCAP done move=%u blocked=%u csscommit=%u ssscommit=%u\n", gNdsMenuShellSssMoveCount, gNdsMenuShellSssBlockedCount, gNdsMenuShellCssCommitCount, gNdsMenuShellSssCommitCount'
        } else { 'printf "SHELLCAP done\n"' }),
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'p2_shell_capture.gdb' `
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
