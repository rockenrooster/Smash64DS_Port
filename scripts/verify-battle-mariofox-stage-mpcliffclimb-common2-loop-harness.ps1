param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5,
    [switch]$HardwareTriangles,
    [switch]$SoftwarePreview
)
$ErrorActionPreference = 'Stop'
$HardwareTriangles = -not $SoftwarePreview
$target = if ($HardwareTriangles) { 'smash64ds-battle-mariofox-stage-mpcliffclimb-common2-loop-hwtri' } else { 'smash64ds-battle-mariofox-stage-mpcliffclimb-common2-loop' }
$build = if ($HardwareTriangles) { 'build-battle-mariofox-stage-mpcliffclimb-common2-loop-hwtri-harness' } else { 'build-battle-mariofox-stage-mpcliffclimb-common2-loop-harness' }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -HardwareTriangles:$HardwareTriangles `
    -Harness 'battle_mariofox_stage_mpcliffclimb_common2_loop' `
    -Target $target `
    -Build $build `
    -ExpectedMode 117 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'Battle Mario/Fox Stage MP Cliff-climb common2-loop' `
    -HarnessSelectMessage 'Direct Mario/Fox Stage MP Cliff-climb common2-loop harness did not select VSBattle from Maps.' `
    -RequireStageDraw `
    -RequireStageMPCliffClimbCommon2
exit $LASTEXITCODE
