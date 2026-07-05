param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [switch]$HardwareTriangles,
    [switch]$SoftwarePreview,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
$HardwareTriangles = -not $SoftwarePreview
$target = if ($HardwareTriangles) { 'smash64ds-battle-mariofox-stage-mpcliffwait-floor-loop-hwtri' } else { 'smash64ds-battle-mariofox-stage-mpcliffwait-floor-loop' }
$build = if ($HardwareTriangles) { 'build-battle-mariofox-stage-mpcliffwait-floor-loop-hwtri-harness' } else { 'build-battle-mariofox-stage-mpcliffwait-floor-loop-harness' }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'battle_mariofox_stage_mpcliffwait_floor_loop' `
    -Target $target `
    -Build $build `
    -ExpectedMode 101 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'Battle Mario/Fox Stage MP Cliff-wait floor-loop' `
    -HarnessSelectMessage 'Direct Mario/Fox Stage MP Cliff-wait floor-loop harness did not select VSBattle from Maps.' `
    -RequireStageDraw `
    -RequireStageMPCliffWaitFloor `
    -HardwareTriangles:$HardwareTriangles
exit $LASTEXITCODE
