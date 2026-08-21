param(
    [string]$Build = 'build-battle-playable-proof-hwtri-harness',
    [string]$Target = 'smash64ds-battle-playable-proof-hwtri',
    [ValidateRange(1, 8)][int]$RunnerSlot = 7,
    [ValidateRange(10, 120)][int]$TimeoutSeconds = 30,
    [string]$Artifact = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
. (Join-Path $PSScriptRoot 'lib\gdb-markers.ps1')
. (Join-Path $PSScriptRoot 'lib\build-output.ps1')

$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
if ([string]::IsNullOrWhiteSpace($Artifact)) {
    $Artifact = Join-Path $root ('artifacts\verification\' +
        (Get-Date -Format 'yyyy-MM-dd') + '_dreamland-wallpaper-seed.txt')
}
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS '' -RunnerSlot $RunnerSlot -NoBuild
$melonDir = Split-Path -Parent $context.MelonDSPath
$logDir = Get-MelonDSVerifierLogDir -Root $root -RunnerSlot $RunnerSlot
$stdout = Join-Path $logDir 'melonds.dreamland-wallpaper-seed.stdout.log'
$stderr = Join-Path $logDir 'melonds.dreamland-wallpaper-seed.stderr.log'
$logTemp = if (-not [string]::IsNullOrWhiteSpace($env:SMASH64DS_VERIFY_TEMP_DIR)) {
    $env:SMASH64DS_VERIFY_TEMP_DIR
} else {
    Join-Path $root ('artifacts\verifier-temp\slot' + $RunnerSlot)
}
$configState = $null
$emulator = $null

try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath `
        -GdbPort $context.GdbPort -Persistent -MuteAudio
    Remove-Item -LiteralPath $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $emulator = Start-Process `
        -FilePath $context.MelonDSPath `
        -ArgumentList $rom `
        -WorkingDirectory $melonDir `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -WindowStyle Hidden `
        -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null

    # QueueIdentity is called once when capture begins and again after
    # FinishSeed has populated the opaque-count/state counters. Ignore the
    # first call and stop on the first completed seed, so this proof is reached
    # during boot rather than after an arbitrary gameplay delay.
    $commands = @(
        'set pagination off',
        'set confirm off',
        'set remotetimeout 20',
        ("target remote 127.0.0.1:{0}" -f $context.GdbPort),
        'break ndsPlatformFastWallpaperQueueIdentity',
        'commands',
        'silent',
        'if (gNdsFastWallpaperSeedSuccessCount + gNdsFastWallpaperSeedFailureCount) != 0',
        'printf "WSEED state=%u attempt=%u success=%u failure=%u degraded=%u opaque=%u clampx=%u clampy=%u clampscale=%u invalid=%u restore=%u\n", gNdsFastWallpaperState, gNdsFastWallpaperSeedAttemptCount, gNdsFastWallpaperSeedSuccessCount, gNdsFastWallpaperSeedFailureCount, gNdsFastWallpaperStaticDegradedCount, gNdsFastWallpaperSeedOpaquePixelCount, gNdsFastWallpaperClampXCount, gNdsFastWallpaperClampYCount, gNdsFastWallpaperClampScaleCount, gNdsFastWallpaperInvalidTransformCount, gNdsFastWallpaperSeedRestoreMismatchCount',
        'detach',
        'quit',
        'end',
        'continue',
        'end',
        'continue'
    )

    Invoke-GdbMarkerScript `
        -Gdb $gdb -Elf $elf -Root $root -Commands $commands `
        -ScriptName 'dreamland_wallpaper_seed_probe.gdb' `
        -TimeoutSeconds $TimeoutSeconds | Out-Null
}
finally {
    $captured = Join-Path $logTemp 'dreamland_wallpaper_seed_probe.gdb.out'
    if (Test-Path -LiteralPath $captured) {
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Artifact) |
            Out-Null
        Copy-Item -LiteralPath $captured -Destination $Artifact -Force
        Get-Content -LiteralPath $Artifact |
            Where-Object { $_ -match '^WSEED ' } |
            ForEach-Object { Write-Output $_ }
        Write-Output "probe capture: $Artifact"
    }
    if ($null -ne $emulator) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($null -ne $configState) {
        Restore-MelonDSGdbConfig -State $configState
    }
}
