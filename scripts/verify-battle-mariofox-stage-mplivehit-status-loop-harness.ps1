param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$ImportBattleShipFTManager,
    [switch]$HardwareTriangles
)
$ErrorActionPreference = 'Stop'
$ImportBattleShipFTManager = $true
$target = if ($HardwareTriangles) { 'smash64ds-battle-mariofox-stage-mplivehit-status-loop-hwtri' } else { 'smash64ds-battle-mariofox-stage-mplivehit-status-loop' }
$build = if ($HardwareTriangles) { 'build-battle-mariofox-stage-mplivehit-status-loop-hwtri-harness' } else { 'build-battle-mariofox-stage-mplivehit-status-loop-harness' }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcrunall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -ImportBattleShipFTManager:$ImportBattleShipFTManager `
    -HardwareTriangles:$HardwareTriangles `
    -Harness 'battle_mariofox_stage_mplivehit_status_loop' `
    -Target $target `
    -Build $build `
    -ExpectedMode 161 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'Battle Mario/Fox Stage MP live-hit status-loop' `
    -HarnessSelectMessage 'Direct Mario/Fox Stage MP live-hit status-loop harness did not select VSBattle from Maps.'
