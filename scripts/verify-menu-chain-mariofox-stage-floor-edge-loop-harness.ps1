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
$target = if ($HardwareTriangles) { 'smash64ds-menu-chain-mariofox-stage-floor-edge-loop-hwtri' } else { 'smash64ds-menu-chain-mariofox-stage-floor-edge-loop' }
$build = if ($HardwareTriangles) { 'build-menu-chain-mariofox-stage-floor-edge-loop-hwtri-harness' } else { 'build-menu-chain-mariofox-stage-floor-edge-loop-harness' }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'menu_chain_mariofox_stage_floor_edge_loop' `
    -Target $target `
    -Build $build `
    -ExpectedMode 66 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox stage floor-edge-loop' `
    -HarnessSelectMessage 'Menu-chain stage floor-edge-loop harness did not select VS Mode from Title.' `
    -HardwareTriangles:$HardwareTriangles `
    -RequireStageDraw `
    -RequireStageCollision `
    -RequireStageFloorEdge
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
