param(
    [Parameter(Mandatory = $true)][string]$Build,
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 6,
    [ValidateRange(10, 600)][int]$SettleSeconds = 90,
    [ValidateRange(30, 900)][int]$TimeoutSeconds = 300,
    [string]$Artifact = '',
    # Extra u32 globals to read at the same stop, for testing a
    # specific hypothesis about why the frame never arrived.
    [string[]]$ExtraGlobals = @(),
    # Stop at this symbol instead of interrupting wherever the guest is.
    # The register dump below then shows its arguments in r0-r3, which is
    # how a bad pointer ARGUMENT is told apart from a bad dereference.
    [string]$BreakSymbol = '',
    # A fault inside the first seconds is already in the handler by the
    # time an ordinary attach lands, so the breakpoint never arms. Halt the
    # ARM9 at startup for those, and skip the settle.
    [switch]$BreakOnStartup,
    # Arbitrary gdb print expressions evaluated at the same stop,
    # for state that is not a plain global (struct fields, casts).
    [string[]]$Expressions = @()
)

# WHERE DID THE BATTLE STOP.
#
# `probe-arena-overflow.ps1` answers "did an allocation halt", and the tick-HUD
# sampler answers "did it reach frame N", but between them sat a gap that cost
# three build/run cycles on 2026-09-02: a ROM that allocates fine and still
# never presents a frame. The sampler can only report UNRESOLVED there, because
# its liveness probe cannot attach while the main GDB session holds the port.
#
# This probe lets the ROM run untouched for -SettleSeconds, then interrupts it
# once and reports the two things that separate the cases: whether the presented
# -frame counter is advancing, and what the CPU is actually executing. A stalled
# counter with a backtrace is a hang to fix; an advancing counter is a slow run
# to re-time. Nothing here is fighter-specific -- any target whose ELF carries
# the pacing counter can be its patient.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_battle-progress.txt')
}

$required = @(
    'gNdsBattlePlayablePacingPresentedFrames',
    'gNdsRelocAssetOpenFailCount',
    'gNdsRelocAssetFighterStreamFailures',
    'gNdsTaskmanArenaChosenSize',
    'gNdsTaskmanArenaAllocFailCount'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw "battle-progress probe symbols absent from ${elf}: $($missing -join ', ')"
}

$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.battle-progress.stdout.log'
$stderr = Join-Path $log_dir 'melonds.battle-progress.stderr.log'
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
        -GdbPort $context.GdbPort -Persistent `
        -BreakOnStartup:$BreakOnStartup -MuteAudio
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

    # The guest runs free while GDB is detached, so the settle happens before
    # the first attach rather than inside the session.
    if (-not $BreakOnStartup) {
        Start-Sleep -Seconds $SettleSeconds
    }

    $commands = [System.Collections.Generic.List[string]]::new()
    $commands.AddRange([string[]]@(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        $(if (-not [string]::IsNullOrWhiteSpace($BreakSymbol)) {
            "break $BreakSymbol"
        }),
        $(if (-not [string]::IsNullOrWhiteSpace($BreakSymbol)) {
            'continue'
        }),
        ('printf "PROGRESS A presented=%u openfail=%u streamfail=%u ' +
         'arena=%u allocfail=%u\n", ' +
         'gNdsBattlePlayablePacingPresentedFrames, ' +
         'gNdsRelocAssetOpenFailCount, gNdsRelocAssetFighterStreamFailures, ' +
         'gNdsTaskmanArenaChosenSize, gNdsTaskmanArenaAllocFailCount'),
        'bt 24',
        $(if ($ExtraGlobals.Count -ne 0) {
            $fmt = ($ExtraGlobals | ForEach-Object { "$_=%u" }) -join ' '
            $eol = [string][char]92 + 'n'
            'printf "PROGRESS EXTRA ' + $fmt + $eol + '", ' +
                ($ExtraGlobals -join ', ')
        }),
        # A ROM sitting in calico's __excpt_entry has already faulted:
        # the banked registers are the only record of where.
        $(foreach ($e in $Expressions) { "p $e" }),
        'info registers',
        'info symbol $pc',
        'info symbol $lr',
        'x/4i $lr - 8',
        'continue &',
        'shell powershell -NoProfile -Command "Start-Sleep -Seconds 8"',
        'interrupt',
        ('printf "PROGRESS B presented=%u openfail=%u streamfail=%u\n", ' +
         'gNdsBattlePlayablePacingPresentedFrames, ' +
         'gNdsRelocAssetOpenFailCount, gNdsRelocAssetFighterStreamFailures'),
        'bt 24',
        'detach',
        'quit'
    ))
    $commands = [System.Collections.Generic.List[string]]::new(
        [string[]]@($commands | Where-Object {
            -not [string]::IsNullOrWhiteSpace($_) }))

    try {
        Invoke-GdbMarkerScript `
            -Gdb $gdb -Elf $elf -Root $root -Commands $commands.ToArray() `
            -ScriptName 'battle_progress_probe.gdb' `
            -TimeoutSeconds $TimeoutSeconds | Out-Null
    } catch {
        Write-Output ('battle-progress capture ended: ' +
            $_.Exception.Message.Split("`n")[0])
    }
}
finally {
    $captured = Join-Path $log_temp 'battle_progress_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force `
            -Path (Split-Path -Parent $Artifact) | Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^(PROGRESS |#\d|r\d|lr_usr|cpsr)' }
        Write-Output ("probe capture: " + $Artifact)
    }
    if ($null -ne $emulator) {
        try { $emulator.CloseMainWindow() | Out-Null } catch { }
        if (-not $emulator.WaitForExit(4000)) {
            Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
        }
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state | Out-Null
    }
}
