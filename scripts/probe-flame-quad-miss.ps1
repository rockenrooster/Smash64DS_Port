[CmdletBinding()]
param(
    [string]$Build = 'build-c127-fire',
    [string]$Target = 'smash64ds-battle-playable-tickhud-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    [ValidateRange(1, 32)][int]$Hits = 3,
    [string]$Artifact = ''
)

# BUGS.md "Missing fire burn effects" -- RE-OPENED 2026-08-12 after the owner
# playtested build-c127-fire and still saw no burn.
#
# The static chain is already resolved and this run exists to confirm it on the
# exact ROM the owner played, with no rebuild:
#
#   efManagerFlameLRMakeEffect  ->  lbParticleMakeScriptID(bank, 0x12)
#                                   (decomp efmanager.c:2538)
#   script 0x12                 ->  texture 12   (read from the packed bank)
#   texture 12                  ->  PACKED, but its quad cell was in
#                                   quads.excluded of
#                                   docs/optimization/NDS_PARTICLE_BANKS.generated.json
#   runtime                     ->  ndsParticleQuadFrameFor returns NULL and
#                                   battleship_lbparticle.c:3698 takes the
#                                   `continue` that deliberately draws NOTHING
#                                   rather than a neighbouring cell
#
# So a FlameLR particle is created, positioned, scaled and updated correctly and
# emits zero pixels. Every counter the previous acceptance read stays green.
#
# PREDICTIONS, written before the run:
#   * gNdsParticleQuadMissCount rises across the flame hits.
#   * gNdsParticleQuadMissMask has BIT 12 set  (0x00001000). Bits, not hex
#     digits -- reading this mask as digits has already cost this project a
#     round of chasing textures the pack does not carry.
#   * gNdsParticleTextureUseMask does NOT have bit 12 set, because the use mask
#     is stamped on the draw path the miss skips.
#   * FlameRandom/FlameStatic take script 0x55 -> texture 15, which IS admitted,
#     so bit 15 should appear in the USE mask and not in the miss mask.
# If bit 12 is clear, the static chain is wrong and the fix must not be built.

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
        (Get-Date -Format 'yyyy-MM-dd') + '_flame-quad-miss.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.flame-quad-miss.stdout.log'
$stderr = Join-Path $log_dir 'melonds.flame-quad-miss.stderr.log'
$log_temp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root 'artifacts\verifier-temp\default'
}
$config_state = $null
$emulator = $null

$required = @(
    'efManagerFlameLRMakeEffect',
    'efManagerFlameRandomMakeEffect',
    'gNdsParticleQuadMissCount',
    'gNdsParticleQuadMissMask',
    'gNdsParticleQuadEmitCount',
    'gNdsParticleTextureUseMask',
    'gSCManagerBattleState'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = @($required | Where-Object { $symbols -notcontains $_ })
if ($missing.Count -gt 0) {
    throw ("Flame quad-miss probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
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

        'set $n = 0',
        # BOTH makers, because which one fires first depends on the burn's
        # strength and on the CPU fight. A FlameLR-only breakpoint waited out a
        # 900-second run on build-c129-foxfire without ever hitting, and a probe
        # that can only observe half of an effect is a probe that reports
        # "nothing happened" for the wrong reason.
        'break efManagerFlameLRMakeEffect',
        'break efManagerFlameRandomMakeEffect',
        'commands 1-2',
        'silent',
        'set $n = $n + 1',
        'printf "FLAME hit=%d tr=%d miss=%u missmask=%08x usemask=%08x emit=%u\n", $n, gSCManagerBattleState->time_remain, gNdsParticleQuadMissCount, gNdsParticleQuadMissMask[0], gNdsParticleTextureUseMask[0], gNdsParticleQuadEmitCount',
        ('if $n < ' + $Hits),
        'continue',
        'end',
        'end',
        'continue',
        # The masks that matter are stamped on the DRAW pass, so read them a few
        # frames after the last creation rather than at it -- and halt on a frame
        # BOUNDARY so the screenshot is a whole presented frame rather than a
        # half-drawn one.
        'delete breakpoints',
        'break ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'continue',
        $captureCommand,
        'continue',
        'printf "FLAMEDONE miss=%u missmask=%08x missframes=%08x usemask=%08x emit=%u max=%u\n", gNdsParticleQuadMissCount, gNdsParticleQuadMissMask[0], gNdsParticleQuadMissFrameMask, gNdsParticleTextureUseMask[0], gNdsParticleQuadEmitCount, gNdsParticleQuadEmitMax',
        'printf "FLAMEBITS bit12_miss=%d bit12_use=%d bit15_miss=%d bit15_use=%d\n", (gNdsParticleQuadMissMask[0] >> 12) & 1, (gNdsParticleTextureUseMask[0] >> 12) & 1, (gNdsParticleQuadMissMask[0] >> 15) & 1, (gNdsParticleTextureUseMask[0] >> 15) & 1',
        'detach',
        'quit'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'flame_quad_miss_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    $captured = Join-Path $log_temp 'flame_quad_miss_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^FLAME' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
