param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [int]$Admit = 2,
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [int]$Hits = 4,
    [int]$TimeoutSeconds = 900
)
# P2-2p8 Phase 1 slice 2c (lab): the call chain of every fighter texture
# upload outside the admitted list. Runner slot 9 / GDB 3423; the admission
# word is poked at the first frame-complete marker as the sampler does; a
# conditional breakpoint on the lab census's upload note (slot 4 = Kirby in
# the stress roster, after the admission) prints a backtrace and the key.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2c'
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS (Join-Path $root 'emulators\melonds\melonDS.exe') -RunnerSlot 9 `
    -GdbPort 3423 -GdbPortExplicit:$true -NoBuild:$true
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot 9
$log = Join-Path $art "$Arm-bt.log"
$configState = $null
$emulator = $null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'bt-s2c.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'bt-s2c.melonds.err') `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null
    $script = Join-Path $temp 'bt-s2c.gdb'
    $stdout = Join-Path $temp 'bt-s2c.gdb.out'
    $stderr = Join-Path $temp 'bt-s2c.gdb.err'
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $lines = @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        'set variable gNdsFtrLeanRoute = 0',
        "set variable gNdsFtrLeanAdmit = $Admit",
        'printf "POKED admit=%u\n", gNdsFtrLeanAdmit',
        'break ndsVramCensusNoteCacheKey if (gNdsVramCensusDrawSlotPlus1 == 4) && (gNdsFtrAdmitLab.done != 0)'
    )
    for ($i = 1; $i -le $Hits; $i++) {
        $lines += @(
            'continue',
            "printf `"HIT $i frame=%u image=%08x tlut=%08x fmt=%u siz=%u\n`", gNdsRendererProfileFrameCount, key->image, key->tlut_image, key->image_format, key->image_size",
            'bt 16'
        )
    }
    $lines += @('printf "BT-DONE\n"')
    [System.IO.File]::WriteAllLines($script, $lines)
    $gdbProcess = Start-Process -FilePath $gdb -ArgumentList @('-q', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        if ((Test-Path -LiteralPath $stdout) -and ((Get-Content -LiteralPath $stdout -Raw) -match 'BT-DONE')) { break }
        if ($gdbProcess.HasExited) { break }
    }
    if (Test-Path -LiteralPath $stdout) { Copy-Item -LiteralPath $stdout -Destination $log -Force }
    if (Test-Path -LiteralPath $stderr) { Add-Content -LiteralPath $log -Value (Get-Content -LiteralPath $stderr -Raw) }
    if (-not $gdbProcess.HasExited) { Stop-Process -Id $gdbProcess.Id -Force }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
