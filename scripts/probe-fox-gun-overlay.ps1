[CmdletBinding()]
param(
    [string]$Build = 'build-c128-foxgun',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    [ValidateRange(1, 64)][int]$Draws = 4,
    [ValidatePattern('^[A-Za-z0-9][A-Za-z0-9._-]*$')]
    [string]$EvidenceLabel = '2026-08-12_fox-gun-overlay'
)

# BUGS.md "Fox's pistol model is missing" -- acceptance for the draw half.
#
# The state half has been green for a while (gNdsFighterModelPartOnCount rises
# 5x a match) and the gun still did not appear, so a counter alone cannot close
# this row: the question is whether 22 triangles reach the screen IN FOX'S HAND,
# and only a screenshot answers where.
#
# Event-driven, like probe-fox-blaster-native.ps1: the canonical level-3 Fox CPU
# stays on and GDB runs freely until the overlay has actually submitted, so no
# per-frame breakpoint distorts the pacing being looked at. The capture happens
# after $Draws submits, which is a frame where the gun is definitely out.
#
# What the counters add on top of the picture:
#   DrawCount      > 0                 the overlay ran
#   FailCount      = 0                 the texture upload never fell over
#   TriangleCount  = 22 x DrawCount    the whole mesh went, not a prefix
#   Bytes          = 288               256 texel + 32 palette, the exact asset
# A DrawCount of 0 with OnCount rising is the specific failure this row started
# from, and it would now be visible in one line instead of a cycle.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$captureHelper = Join-Path $PSScriptRoot 'capture-running-melonds-window.ps1'
$shot = Join-Path $root ('artifacts\visibility\' + $EvidenceLabel + '.png')
$artifact = Join-Path $root ('artifacts\verification\' + $EvidenceLabel + '.txt')
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.fox-gun-overlay.stdout.log'
$stderr = Join-Path $log_dir 'melonds.fox-gun-overlay.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$required = @(
    'ndsRendererSubmitFoxGun',
    'gNdsRendererFoxGunPrepareCount',
    'gNdsRendererFoxGunFailCount',
    'gNdsRendererFoxGunBytes',
    'gNdsRendererFoxGunDrawCount',
    'gNdsRendererFoxGunTriangleCount',
    'gNdsFighterModelPartOnCount',
    'battleship_wpFoxBlasterMakeWeapon',
    'gSCManagerBattleState'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Fox gun probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}

try {
    $config_state = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr, $shot -Force -ErrorAction SilentlyContinue
    # WindowStyle: visible-by-design
    # capture-running-melonds-window.ps1 needs a real top-level window handle.
    # Hidden, melonDS still runs but MainWindowHandle stays IntPtr.Zero and the
    # screenshot dies as a black PNG rather than as an error.
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melon_dir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Normal `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    $captureCommand =
        'shell pwsh.exe -NoProfile -File "{0}" -EmulatorProcessId {1} -Output "{2}" 2>&1' -f
        $captureHelper.Replace('\', '/'), $emulator.Id, $shot.Replace('\', '/')

    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),

        'set $draws = 0',
        'break ndsRendererSubmitFoxGun',
        'commands',
        'silent',
        'set $draws = $draws + 1',
        'printf "FOXGUN draw=%d tr=%d on=%u count=%u tris=%u fail=%u bytes=%u\n", $draws, gSCManagerBattleState->time_remain, gNdsFighterModelPartOnCount, gNdsRendererFoxGunDrawCount, gNdsRendererFoxGunTriangleCount, gNdsRendererFoxGunFailCount, gNdsRendererFoxGunBytes',
        ('if $draws < ' + $Draws),
        'continue',
        'end',
        'end',

        'continue',
        # Capture at the SHOT, not at an arbitrary gun-out frame. The standing
        # pre-fix control (artifacts/visibility/2026-08-09_fox-blaster-native-
        # promoted.png) is a firing frame with the arm extended and an empty
        # hand, and a candidate shot from a different pose cannot be stacked
        # against it -- which is exactly how a "visually intact" KEEP once got
        # written from the candidate alone.
        'delete breakpoints',
        'break battleship_wpFoxBlasterMakeWeapon',
        'continue',
        # Two more gun submits after the spawn, so the frame being grabbed is one
        # the overlay has actually presented. Stopping ON a submit captures the
        # frame before it, which is the frame without the gun.
        'delete breakpoints',
        'break ndsRendererSubmitFoxGun',
        'continue',
        'continue',
        $captureCommand,
        'printf "FOXGUNDONE draws=%d count=%u tris=%u fail=%u bytes=%u prepare=%u\n", $draws, gNdsRendererFoxGunDrawCount, gNdsRendererFoxGunTriangleCount, gNdsRendererFoxGunFailCount, gNdsRendererFoxGunBytes, gNdsRendererFoxGunPrepareCount',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'fox_gun_overlay_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # Report from the helper's capture file, not its return value: a run that
    # reaches its draw target and one that times out waiting for the last draw
    # produce the same evidence, and only the second throws.
    $captured = Join-Path $log_temp 'fox_gun_overlay_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $artifact -Force
        Get-Content -LiteralPath $artifact |
            Where-Object { $_ -match '^FOXGUN' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $artifact"
        if (Test-Path -LiteralPath $shot) {
            Write-Output "screenshot: $shot"
        } else {
            Write-Output 'screenshot: ABSENT -- the gun cannot be accepted on counters alone'
        }
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
