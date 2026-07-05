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
$target = if ($HardwareTriangles) { 'smash64ds-menu-chain-mariofox-stage-mpcliffattack-floor-loop-hwtri' } else { 'smash64ds-menu-chain-mariofox-stage-mpcliffattack-floor-loop' }
$build = if ($HardwareTriangles) { 'build-menu-chain-mariofox-stage-mpcliffattack-floor-loop-hwtri-harness' } else { 'build-menu-chain-mariofox-stage-mpcliffattack-floor-loop-harness' }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'menu_chain_mariofox_stage_mpcliffattack_floor_loop' `
    -Target $target `
    -Build $build `
    -ExpectedMode 104 `
    -ExpectedHarnessSceneCurr 9 `
    -ExpectedHarnessScenePrev 1 `
    -Label 'Menu-chain Mario/Fox Stage MP Cliff-attack floor-loop' `
    -HarnessSelectMessage 'Menu-chain Mario/Fox Stage MP Cliff-attack floor-loop harness did not select VS Mode from Title.' `
    -RequireStageDraw `
    -RequireStageMPCliffAttackFloor `
    -HardwareTriangles:$HardwareTriangles
exit $LASTEXITCODE
