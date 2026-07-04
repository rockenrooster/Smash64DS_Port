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
$target = if ($HardwareTriangles) { 'smash64ds-menu-chain-mariofox-stage-mpwall-floor-loop-hwtri' } else { 'smash64ds-menu-chain-mariofox-stage-mpwall-floor-loop' }
$build = if ($HardwareTriangles) { 'build-menu-chain-mariofox-stage-mpwall-floor-loop-hwtri-harness' } else { 'build-menu-chain-mariofox-stage-mpwall-floor-loop-harness' }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'menu_chain_mariofox_stage_mpwall_floor_loop' `
    -Target $target `
    -Build $build `
    -ExpectedMode 80 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox Stage MP wall-floor-loop' `
    -HarnessSelectMessage 'Menu-chain Mario/Fox Stage MP wall-floor-loop harness did not start at VS Mode from Title.' `
    -HardwareTriangles:$HardwareTriangles `
    -RequireStageDraw `
    -RequireStageCollision `
    -RequireStageFloorEdge `
    -RequireStageMPUpdateFloor `
    -RequireStageMPSweepFloor `
    -RequireStageMPCrossFloor `
    -RequireStageMPAdjustFloor `
    -RequireStageMPEdgeFloor `
    -RequireStageMPWallFloor
exit $LASTEXITCODE
