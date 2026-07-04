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
$target = if ($HardwareTriangles) { 'smash64ds-battle-mariofox-stage-mpprocess-floor-loop-hwtri' } else { 'smash64ds-battle-mariofox-stage-mpprocess-floor-loop' }
$build = if ($HardwareTriangles) { 'build-battle-mariofox-stage-mpprocess-floor-loop-hwtri-harness' } else { 'build-battle-mariofox-stage-mpprocess-floor-loop-harness' }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'battle_mariofox_stage_mpprocess_floor_loop' `
    -Target $target `
    -Build $build `
    -ExpectedMode 67 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'Battle Mario/Fox stage MP floor-process-loop' `
    -HarnessSelectMessage 'Direct stage MP floor-process-loop harness did not select VSBattle from Maps.' `
    -HardwareTriangles:$HardwareTriangles `
    -RequireStageDraw `
    -RequireStageCollision `
    -RequireStageFloorEdge `
    -RequireStageMPProcessFloor
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
