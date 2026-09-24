param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [Parameter(Mandatory = $true)][string]$ScriptBody,  # gdb lines run after the first frame-complete marker
    [int]$Route = 2,
    [int]$Admit = 2,
    [string]$Build = 'build-p2p8-s6-mfpn',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [int]$EndFrame = 1990,
    [int]$TimeoutSeconds = 900
)
# P2-2p8 Phase 1 slice 6: a gdb diagnostic run on one ROM, runner slot 9 / GDB
# 3423 (capture-s6.ps1's launcher without the pictures). The route and
# admission words are poked at the first frame-complete marker, then
# $ScriptBody (a file of gdb lines: breakpoints with `commands` that print
# registers -- stack and heap reads are stale on the cache-accurate fork) runs
# until presented frame $EndFrame. Log: <Arm>-gdb.log.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice6'
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS (Join-Path $root 'emulators\melonds\melonDS.exe') -RunnerSlot 9 `
    -GdbPort 3423 -GdbPortExplicit:$true -NoBuild:$true
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot 9
$log = Join-Path $art "$Arm-gdb.log"
$configState = $null
$emulator = $null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'gdbdiag-s6.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'gdbdiag-s6.melonds.err') -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $script = Join-Path $temp 'gdbdiag-s6.gdb'
    $stdout = Join-Path $temp 'gdbdiag-s6.gdb.out'
    $stderr = Join-Path $temp 'gdbdiag-s6.gdb.err'
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    $lines = @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        "set variable gNdsFtrLeanRoute = $Route",
        "set variable gNdsFtrLeanAdmit = $Admit",
        'printf "POKED route=%u admit=%u presented=%u\n", gNdsFtrLeanRoute, gNdsFtrLeanAdmit, gNdsBattlePlayablePacingPresentedFrames'
    )
    $lines += Get-Content -LiteralPath $ScriptBody
    $lines += @(
        "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames >= $EndFrame",
        'continue',
        'printf "DIAG-DONE presented=%u\n", gNdsBattlePlayablePacingPresentedFrames',
        'detach', 'quit'
    )
    [System.IO.File]::WriteAllLines($script, $lines)
    $gdbProcess = Start-Process -FilePath $gdb -ArgumentList @('-q', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        if ($gdbProcess.HasExited) { break }
    }
    if (Test-Path -LiteralPath $stdout) { Copy-Item -LiteralPath $stdout -Destination $log -Force }
    if (Test-Path -LiteralPath $stderr) { Add-Content -LiteralPath $log -Value (Get-Content -LiteralPath $stderr -Raw) }
    if (-not $gdbProcess.HasExited) {
        Stop-Process -Id $gdbProcess.Id -Force
        Add-Content -LiteralPath $log -Value "`nTIMEOUT after $TimeoutSeconds s"
    }
    $romSha = (Get-FileHash -LiteralPath $rom -Algorithm SHA256).Hash
    Add-Content -LiteralPath $log -Value "`nROM $romSha route=$Route admit=$Admit"
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
