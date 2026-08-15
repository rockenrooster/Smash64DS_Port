[CmdletBinding()]
param(
    [string]$Build = 'build-c163-battlepack',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 3600)][int]$TimeoutSeconds = 900,
    [ValidateRange(1, 2000)][int]$Hits = 420,
    [string]$Artifact = ''
)

# R2-04 E2: `battle_playable lower-screen rolling FPS counter did not sample
# actual presentation cadence`. The harness reads four globals in one GDB
# printf and recomputes fps_x10 from the frame/tick window published beside it;
# the recompute disagrees. Two explanations are already dead: a second writer
# (there is exactly one non-reset writer per field, inside one REG_IME=0 block)
# and a cache-line straddle (all four share one 32-byte line).
#
# WHAT NOBODY HAS DONE IS WATCH THE GROUP CHANGE. The four fields are published
# together every ~0.5 s. If the group is coherent, all four move on the same
# presented frame. If one half of the group reaches the debugger a publication
# later than the other, that is directly visible as a SPLIT UPDATE: x10 moves on
# one frame and sc/fw/tw on a later one (or the reverse).
#
# NO REBUILD. Every value below already exists in the shipped ROM; this only
# samples them once per presented frame instead of once per run.
#
# The breakpoint is ndsBattlePlayableFrameCompleteMarker -- the same marker the
# battle_playable harness stops on -- so what this probe sees is what the assert
# sees, one frame at a time.

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$nm = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-nm.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_fpshud-publication.txt')
}

$required = @(
    'ndsBattlePlayableFrameCompleteMarker',
    'scVSBattleStartBattle',
    'gNdsBattlePlayableHudFpsX10',
    'gNdsBattlePlayableHudFpsSampleCount',
    'gNdsBattlePlayableHudFpsFrameWindow',
    'gNdsBattlePlayableHudFpsTickWindow',
    'gNdsBattlePlayablePacingPresentedFrames',
    'gNdsBattlePlayablePacingVBlanks'
)
$nm_lines = & $nm $elf
$symbols = $nm_lines | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("FPS-HUD probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
}
foreach ($name in @('gNdsBattlePlayableHudFpsX10',
                    'gNdsBattlePlayableHudFpsSampleCount',
                    'gNdsBattlePlayableHudFpsFrameWindow',
                    'gNdsBattlePlayableHudFpsTickWindow')) {
    foreach ($line in $nm_lines) {
        $parts = $line -split '\s+'
        if ($parts.Count -ge 3 -and $parts[-1] -eq $name) {
            Write-Output ("addr {0} {1}" -f $parts[0], $name)
        }
    }
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.fpshud-publication.stdout.log'
$stderr = Join-Path $log_dir 'melonds.fpshud-publication.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

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
        'set $n = 0',
        'tbreak scVSBattleStartBattle',
        'continue',
        'printf "FPSP battle entered\n"',
        'break ndsBattlePlayableFrameCompleteMarker',
        'commands',
        'silent',
        'set $n = $n + 1',
        'printf "FPSP %d x10=%u sc=%u fw=%u tw=%u pres=%u vbl=%u\n", $n, gNdsBattlePlayableHudFpsX10, gNdsBattlePlayableHudFpsSampleCount, gNdsBattlePlayableHudFpsFrameWindow, gNdsBattlePlayableHudFpsTickWindow, gNdsBattlePlayablePacingPresentedFrames, gNdsBattlePlayablePacingVBlanks',
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        'continue',
        'printf "FPSPDONE n=%d x10=%u sc=%u fw=%u tw=%u\n", $n, gNdsBattlePlayableHudFpsX10, gNdsBattlePlayableHudFpsSampleCount, gNdsBattlePlayableHudFpsFrameWindow, gNdsBattlePlayableHudFpsTickWindow',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'fpshud_publication_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    # From the capture file, not the helper's return value: a probe that runs to
    # its hit cap may still exit by timeout, and the capture holds every sample
    # taken before that.
    $captured = Join-Path $log_temp 'fpshud_publication_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
