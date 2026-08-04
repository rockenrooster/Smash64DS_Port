[CmdletBinding()]
param(
    [string]$Build = 'build-c59-flag1',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(30, 1800)][int]$TimeoutSeconds = 900,
    [ValidateRange(1, 200)][int]$Hits = 40,
    [string]$Artifact = ''
)

# DOES A SOURCE EFFECT'S TEXTURE REACH ITS POLYGON?
#
# gNdsEffectRendererTextureReadyCount and TextureRejectCount are deltas taken
# around one effect submit (reloc_backend_movement.c:13226), so BOTH reading 0
# means the renderer never attempted a bind at all -- not that a bind failed.
# That was the shield's state through cycle 58: right place, right size, right
# per-player colour, and a SQUARE, because ndsRendererHardwareUseTexture refused
# the list before any texture work happened.
#
# This probe reads the accumulating counters at each source-effect submit, so
# consecutive hits give the per-submit delta. It is not shield-specific: the
# rebirth halo (link 10) and the impact wave (link 10) carry the same
# G_CC_BLENDPE combine and the same question.
#
# The break is efManagerShieldProcDisplay, source's own display proc, which is a
# real exported function -- so r0 is the effect GObj at the symbol entry. Its
# wrapper ndsRendererAdapterSubmitEffectDObjTree is NOT usable: the linker folds
# it away entirely (nm on the c59 ELF has no such symbol), which is exactly the
# "a counter with no compiled writer reads 0" trap in breakpoint form.
#
# Counters read at entry describe every submit up to the PREVIOUS frame, so the
# per-frame delta is the difference between consecutive hits; link15 is the
# discriminator, since only the shield and Fox's reflector ride DL link 15 and
# the reflector never spawns in an automated window. Every value printed is a
# register or a code-published global -- no stack object is read, because this
# stub cannot serve them (BUGS.md, cycle 55).

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
        (Get-Date -Format 'yyyy-MM-dd') + '_effect-texture-probe.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melon_dir = Split-Path -Parent $context.MelonDSPath
$log_dir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $log_dir 'melonds.effect-texture-probe.stdout.log'
$stderr = Join-Path $log_dir 'melonds.effect-texture-probe.stderr.log'
$config_state = $null
$emulator = $null

# A gdb batch aborts every command after the first absent symbol, so check the
# whole read set here, not only the breakpoint target.
$required = @(
    'efManagerShieldProcDisplay',
    'gSCManagerBattleState',
    'gNdsEffectRendererDObjDrawCount',
    'gNdsEffectRendererLink15DrawCount',
    'gNdsEffectRendererTriangleCount',
    'gNdsEffectRendererTextureReadyCount',
    'gNdsEffectRendererTextureRejectCount'
)
$symbols = & $nm $elf | ForEach-Object { ($_ -split '\s+')[-1] }
$missing = $required | Where-Object { $symbols -notcontains $_ }
if ($missing.Count -gt 0) {
    throw ("Effect texture probe symbols absent from {0}: {1}" -f $elf, ($missing -join ', '))
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
        'break *efManagerShieldProcDisplay',
        'commands 1',
        'silent',
        'set $n = $n + 1',
        'printf "EFXTEX n=%d tr=%d gobj=%p draw=%u link15=%u tris=%u texready=%u texreject=%u\n", $n, gSCManagerBattleState->time_remain, $r0, gNdsEffectRendererDObjDrawCount, gNdsEffectRendererLink15DrawCount, gNdsEffectRendererTriangleCount, gNdsEffectRendererTextureReadyCount, gNdsEffectRendererTextureRejectCount',
        ("if `$n < {0}" -f $Hits),
        'continue',
        'end',
        'end',
        'continue',
        'printf "EFXTEXDONE n=%d draw=%u link15=%u tris=%u texready=%u texreject=%u\n", $n, gNdsEffectRendererDObjDrawCount, gNdsEffectRendererLink15DrawCount, gNdsEffectRendererTriangleCount, gNdsEffectRendererTextureReadyCount, gNdsEffectRendererTextureRejectCount',
        'detach',
        'quit'
    )

    $capture = Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'effect_texture_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) | Out-Null
    Set-Content -LiteralPath $Artifact -Value $capture.Stdout
    $capture.Stdout -split "`n" |
        Where-Object { $_ -match 'EFXTEX' } |
        ForEach-Object { Write-Output $_ }
    Write-Output "probe capture: $Artifact"
}
finally {
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $config_state) {
        Restore-MelonDSGdbConfig -State $config_state
    }
}
