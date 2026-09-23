param(
    [Parameter(Mandatory = $true)][string]$Arm,
    [int]$Admit = 0,
    [int]$Route = 0,
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    # Results tic to halt on (sMNVSResultsTotalTimeTics), as capture-results-tic.ps1
    [int]$Tic = 160,
    [int]$TimeoutSeconds = 2400,
    [int]$BattleTimeoutSeconds = 900,
    [int]$Ignore = 600
)
# P2-2p8 Phase 1 slice 2c: the battle-exit proof. One whole four-CPU match
# played out past GAME SET into the VS Results scene on runner slot 9 / GDB
# 3423: the admission word is poked (whole u32) at the first frame-complete
# marker, as the sampler does; the emulator then halts at the end of Results
# tic $Tic (the capture-results-tic.ps1 stop), prints the exit counters (the
# lab flushes them at the exit and at the bank D return) and the window is
# captured.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2c'
. (Join-Path $root 'scripts\lib\melonds.ps1')
. (Join-Path $root 'scripts\lib\build-output.ps1')
. (Join-Path $root 'scripts\lib\melonds-screenshot.ps1')
$gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
$context = Initialize-MelonDSVerifierContext `
    -Root $root -MelonDS (Join-Path $root 'emulators\melonds\melonDS.exe') -RunnerSlot 9 `
    -GdbPort 3423 -GdbPortExplicit:$true -NoBuild:$true
$rom = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.nds'
$elf = Resolve-Smash64DSBuildOutput -Root $root -Target $Target -Build $Build -Extension '.elf'
$temp = Get-MelonDSVerifierTempDir -Root $root -RunnerSlot 9
$log = Join-Path $art "$Arm-scene.log"
$png = Join-Path $art "$Arm-results.png"
$configState = $null
$emulator = $null
$start = Get-Date
try {
    $configState = Enable-MelonDSGdbConfig `
        -MelonDSPath $context.MelonDSPath -GdbPort $context.GdbPort `
        -Arm7Port $context.Arm7Port -Persistent:([bool]$context.PersistentConfig) -MuteAudio
    $emulator = Start-Process -FilePath $context.MelonDSPath -ArgumentList $rom `
        -WorkingDirectory (Split-Path -Parent $context.MelonDSPath) `
        -RedirectStandardOutput (Join-Path $temp 'scene-s2c.melonds.out') `
        -RedirectStandardError (Join-Path $temp 'scene-s2c.melonds.err') `
        -WindowStyle Hidden -PassThru
    Wait-MelonDSGdbListener -Process $emulator -Port $context.GdbPort | Out-Null
    $emulator.WaitForInputIdle(20000) | Out-Null
    $script = Join-Path $temp 'scene-s2c.gdb'
    $stdout = Join-Path $temp 'scene-s2c.gdb.out'
    $stderr = Join-Path $temp 'scene-s2c.gdb.err'
    Remove-Item $stdout, $stderr -Force -ErrorAction SilentlyContinue
    [System.IO.File]::WriteAllLines($script, @(
        'set pagination off', 'set confirm off', 'set remotetimeout 60',
        "target remote 127.0.0.1:$($context.GdbPort)",
        'tbreak ndsBattlePlayableFrameCompleteMarker',
        'continue',
        "set variable gNdsFtrLeanRoute = $Route",
        "set variable gNdsFtrLeanAdmit = $Admit",
        'printf "POKED route=%u admit=%u frame=%u\n", gNdsFtrLeanRoute, gNdsFtrLeanAdmit, gNdsRendererProfileFrameCount',
        'tbreak ndsFtrLeanAdmitBattleExit',
        'continue',
        'printf "BATTLE-EXIT frame=%u status=%u time_remain=%u enters=%u\n", gNdsRendererProfileFrameCount, gSCManagerBattleState->game_status, gSCManagerBattleState->time_remain, gNdsSceneManagerEnterCount',
        'tbreak ndsSceneManagerEnter',
        'continue',
        'printf "SCENE-ENTER curr=%u prev=%u enters=%u frame=%u\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsSceneManagerEnterCount, gNdsRendererProfileFrameCount',
        'bt 8',
        'break ndsOsPostVBlank',
        "ignore `$bpnum $Ignore",
        'continue',
        'delete',
        'printf "LATER curr=%u prev=%u enters=%u frame=%u results_starts=%u tics=%d\n", gSCManagerSceneData.scene_curr, gSCManagerSceneData.scene_prev, gNdsSceneManagerEnterCount, gNdsRendererProfileFrameCount, gNdsVSResultsStartCount, sMNVSResultsTotalTimeTics',
        'printf "BANKD takes=%u returns=%u missed=%u lent=%u\n", gNdsVramBankDTakes, gNdsVramBankDReturns, gNdsVramBankDMissedExits, gNdsVramBankDLent',
        'bt 10',
        'info registers pc lr cpsr',
        'printf "DIAG-DONE\n"'
    ))
    $gdbProcess = Start-Process -FilePath $gdb -ArgumentList @('-q', '-x', $script, $elf) `
        -WorkingDirectory $root -RedirectStandardOutput $stdout -RedirectStandardError $stderr `
        -WindowStyle Hidden -PassThru
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    $battleDeadline = (Get-Date).AddSeconds($BattleTimeoutSeconds)
    $reached = $null
    $noExit = $false
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 1000
        $text = ''
        if (Test-Path -LiteralPath $stdout) {
            $text = Get-Content -LiteralPath $stdout -Raw
            if ($text -match 'DIAG-DONE') { $reached = 1; break }
        }
        if (((Get-Date) -gt $battleDeadline) -and ($text -notmatch 'BATTLE-EXIT')) {
            $noExit = $true
            break
        }
        if ($gdbProcess.HasExited) { break }
    }
    if (Test-Path -LiteralPath $stdout) { Copy-Item -LiteralPath $stdout -Destination $log -Force }
    if ($noExit) { Add-Content -LiteralPath $log -Value "`nNO BATTLE EXIT within ${BattleTimeoutSeconds}s" }
    if ($null -eq $reached) {
        Add-Content -LiteralPath $log -Value "`nNEVER REACHED Results tic $Tic within ${TimeoutSeconds}s"
        throw "Never reached Results tic $Tic"
    }
    if (-not $gdbProcess.HasExited) { Stop-Process -Id $gdbProcess.Id -Force }
}
finally {
    if ($emulator -and -not $emulator.HasExited) {
        Stop-Process -Id $emulator.Id -Force -ErrorAction SilentlyContinue
    }
    if ($configState) { Restore-MelonDSGdbConfig -State $configState }
}
