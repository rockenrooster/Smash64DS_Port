param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [int]$Route = 0,
    [int]$Admit = 2,
    [string]$Build = 'build-p2p8-s6',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    # Presented frames to photograph (gNdsBattlePlayablePacingPresentedFrames,
    # a DTCM word published at the frame-complete marker). The replay digest of
    # a route 1 / route 0 pair on one ROM is identical frame for frame, so the
    # same frame number is the same game state in both arms.
    [string]$Frames = '150,260,420,640,900,1200',
    # Instead of fixed frames: the first N draws that select the electric
    # skeleton (ndsRendererNativeFighterSetRootProgram with program 0xFE, which
    # both routes call on the draw), each photographed $SettleMarkers
    # frame-complete markers later (the render of frame F is on screen after
    # the next swap).
    [int]$SkeletonShots = 0,
    [int]$SettleMarkers = 2,
    [int]$TimeoutSeconds = 1200
)
# P2-2p8 Phase 1 slice 6: route-matched captures on one ROM, runner slot 9 /
# GDB 3423. The route and admission words are poked (whole u32) at the first
# frame-complete marker, as the sampler does; then at each stop gdb's `shell`
# runs shot-s6.ps1 while the core is halted, so every picture is that stop's.
# melonDS runs VISIBLE (a hidden window has no handle to capture).
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice6'
$shots = Join-Path $art 'captures'
New-Item -ItemType Directory -Force -Path $shots | Out-Null
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS (Join-Path $root 'emulators\melonds\melonDS.exe') -RunnerSlot 9 `
    -GdbPort 3423 -GdbPortExplicit:$true -NoBuild:$true
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot 9
$log = Join-Path $art "$Arm-capture.log"
$shot = Join-Path $art 'tools\shot-s6.ps1'
$configState = $null
$emulator = $null
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'capture-s6.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'capture-s6.melonds.err') -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null
    $script = Join-Path $temp 'capture-s6.gdb'
    $stdout = Join-Path $temp 'capture-s6.gdb.out'
    $stderr = Join-Path $temp 'capture-s6.gdb.err'
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
    if ($SkeletonShots -gt 0) {
        for ($i = 1; $i -le $SkeletonShots; $i++) {
            $png = Join-Path $shots ("{0}-skel{1}.png" -f $Arm, $i)
            $lines += @(
                'break ndsRendererNativeFighterSetRootProgram if $r1 == 254',
                'continue',
                'delete',
                "printf `"SKEL $i frame=%u owner=%u`\n`", gNdsBattlePlayablePacingPresentedFrames, `$r0"
            )
            for ($m = 1; $m -le $SettleMarkers; $m++) {
                $lines += @('tbreak ndsBattlePlayableFrameCompleteMarker', 'continue')
            }
            $lines += @(
                "printf `"STOP skel=$i frame=%u`\n`", gNdsBattlePlayablePacingPresentedFrames",
                "shell pwsh -NoProfile -File $shot -ProcessId $($emulator.Id) -Path $png",
                # step past this toggle's frames before arming the next one
                'break ndsBattlePlayableFrameCompleteMarker', 'ignore $bpnum 40', 'continue', 'delete'
            )
        }
    } else {
        foreach ($f in @($Frames -split ',' | ForEach-Object { [int]$_ })) {
            $png = Join-Path $shots ("{0}-f{1:d4}.png" -f $Arm, $f)
            $lines += @(
                "break ndsBattlePlayableFrameCompleteMarker if gNdsBattlePlayablePacingPresentedFrames == $f",
                'continue',
                'delete',
                "printf `"STOP frame=%u logic=%u`\n`", gNdsBattlePlayablePacingPresentedFrames, gNdsBattlePlayablePacingLogicFrames",
                "shell pwsh -NoProfile -File $shot -ProcessId $($emulator.Id) -Path $png"
            )
        }
    }
    $lines += @('printf "CAPTURE-DONE\n"', 'detach', 'quit')
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
    Add-Content -LiteralPath $log -Value "`nROM $romSha route=$Route admit=$Admit frames=$Frames skeleton=$SkeletonShots"
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
