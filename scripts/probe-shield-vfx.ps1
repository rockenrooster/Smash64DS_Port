[CmdletBinding()]
param(
    [string]$Build = 'build-r2-bothcpu',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 400)][int]$TimeoutSeconds = 300,
    # Frames the shield must have been continuously drawn before the capture.
    # Not 0: the shield grows in, so frame 0 of a guard is a dot. Not large
    # either -- a CPU releases guard quickly and a long wait never fires.
    [ValidateRange(1, 120)][int]$HoldFrames = 6
)

# ONE SCREENSHOT OF THE SHIELD, ON THE FRAME THE SHIELD IS ACTUALLY UP.
#
# BUGS.md "Shield VFX is not correct" was closed on source evidence -- the port
# shipped vertex alpha 0x60/0x50 where efManagerShieldProcDisplay
# (efmanager.c:4112) sets 0xC0 on both prim and env -- but opacity is a LOOK, and
# the owner asked to see it. A soak's final frame is always the results screen,
# so there was no shield frame to show.
#
# Catching one is not a matter of capturing often enough. A CPU guards for well
# under a second and the window is a few frames wide, so periodic capture is a
# lottery. This arms on the draw and spends the countdown at the frame marker,
# the same idiom probe-vfx-contracts.ps1 uses for the KO burst: gdb holds the
# emulator at the marker, so the window is showing the frame that armed it and
# not a later one.
#
# ndsEFManagerShieldProcDisplay runs once per frame per shielding fighter, so
# gNdsTask39FxShieldDrawCount advancing between two frame markers means a shield
# is on screen RIGHT NOW. Requiring it to advance HoldFrames times in a row
# rejects the one-frame flicker at guard start and lands on a grown bubble.
#
# Both CPUs, because a passive human slot never shields. Level-3 Fox guards on
# its own within a few seconds of engagement.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$artifact = Join-Path $root 'artifacts\verification\shield-vfx-probe.txt'
$capture_helper = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
$shot = 'artifacts/visibility/' + (Get-Date -Format 'yyyy-MM-dd') + '_shield-vfx.png'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.shield-vfx-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.shield-vfx-probe.stderr.log'
$config_state = $null
$emulator = $null

# gdb abandons a command file after its first error, so an absent symbol costs
# the whole run silently. Checked up front, as every harness here does.
$required = @(
    'ndsBattlePlayableFrameCompleteMarker',
    'gNdsTask39FxShieldDrawCount',
    'gNdsVisualEffectKindMask',
    'gNdsBattlePlayablePacingPresentedFrames'
)
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("Shield probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'set $prev_draws = 0',
        'set $run = 0',
        'set $best_run = 0',
        'set $shots = 0',
        'set $frames = 0',
        'set $shot_frame = -1',

        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        'set $frames = $frames + 1',

        # A shield is on screen this frame iff the draw counter moved since the
        # previous frame marker. Counting the RUN of such frames is what
        # separates a grown bubble from the first frame of a guard.
        'if gNdsTask39FxShieldDrawCount > $prev_draws',
        'set $run = $run + 1',
        'else',
        'set $run = 0',
        'end',
        'set $prev_draws = gNdsTask39FxShieldDrawCount',
        'if $run > $best_run',
        'set $best_run = $run',
        'end',

        ('if ($run == ' + $HoldFrames + ') && ($shots == 0)'),
        'set $shots = 1',
        'set $shot_frame = $frames',
        # pwsh, NOT powershell: lib\melonds.ps1 uses a PS7 ternary, so 5.1
        # cannot even parse it, and gdb's `shell` does not inherit the harness
        # shell. This exact line has been got wrong before.
        ('shell pwsh -NoProfile -ExecutionPolicy Bypass -File "' +
            $capture_helper + '" -EmulatorProcessId ' + $emulator.Id +
            ' -Output "' + $shot + '"'),
        'end',

        # THE CALLBACK STOPS ITSELF BY NOT CONTINUING -- the same rule
        # probe-vfx-contracts.ps1:578 records, and worth restating because the
        # first version of this script got it wrong in a way that still looked
        # like success. It printed a marker when done and then continued anyway,
        # so the marker fired every frame from then on: the capture landed
        # correctly, and the run still burned the full 300-second timeout and
        # ~20,000 lines of output to report it. A stop condition that does not
        # suppress the continue is not a stop condition.
        'set $done = 0',
        'if ($shots == 1) && ($frames > $shot_frame + 2)',
        'set $done = 1',
        'end',
        # Budget: a level-3 CPU guards within a few thousand frames. Spending
        # this without a shot is a real answer (no shield was drawn), not a hang.
        'if $frames > 5400',
        'set $done = 1',
        'end',
        'if $done == 0',
        'continue',
        'end',
        'end',

        'continue',

        'printf "SHIELDPROBE frames=%d shots=%d shot_frame=%d best_run=%d draws=%u kindmask=%u\n", $frames, $shots, $shot_frame, $best_run, gNdsTask39FxShieldDrawCount, gNdsVisualEffectKindMask',
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'shield_vfx_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifact) | Out-Null
    Set-Content -LiteralPath $artifact -Value $capture
    $capture | Select-String -Pattern 'SHIELDPROBE'
    Write-Output "probe capture: $artifact"
    Write-Output "screenshot:    $shot"
}
finally {
    if ($emulator -ne $null) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($config_state -ne $null) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
