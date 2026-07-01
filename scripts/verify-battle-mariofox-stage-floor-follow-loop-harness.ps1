param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb `
    -GdbPort $GdbPort `
    -RunnerSlot $RunnerSlot `
    -NoBuild:$NoBuild `
    -DelaySeconds $DelaySeconds `
    -Harness 'battle_mariofox_stage_floor_follow_loop' `
    -Target 'smash64ds-battle-mariofox-stage-floor-follow-loop' `
    -Build 'build-battle-mariofox-stage-floor-follow-loop-harness' `
    -ExpectedMode 63 `
    -ExpectedHarnessSceneCurr 22 `
    -ExpectedHarnessScenePrev 21 `
    -Label 'Battle Mario/Fox stage floor-follow-loop' `
    -HarnessSelectMessage 'Direct stage floor-follow-loop harness did not select VSBattle from Maps.' `
    -RequireStageDraw `
    -RequireStageCollision `
    -RequireStageFloorFollow
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
