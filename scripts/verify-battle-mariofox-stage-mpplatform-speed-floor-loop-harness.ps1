param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
$params = @{
    MelonDS = $MelonDS
    Gdb = $Gdb
    RunnerSlot = $RunnerSlot
    DelaySeconds = $DelaySeconds
    Harness = 'battle_mariofox_stage_mpplatform_speed_floor_loop'
    Target = 'smash64ds-battle-mariofox-stage-mpplatform-speed-floor-loop'
    Build = 'build-battle-mariofox-stage-mpplatform-speed-floor-loop-harness'
    ExpectedMode = 151
    ExpectedHarnessSceneCurr = 22
    ExpectedHarnessScenePrev = 21
    Label = 'Battle Mario/Fox Stage MP platform-speed floor-loop'
    HarnessSelectMessage = 'Direct Mario/Fox Stage MP platform-speed floor-loop harness did not select VSBattle from Maps.'
    RequireStageDraw = $true
    RequireStageMPPlatformSpeedFloor = $true
}
if ($NoBuild) { $params.NoBuild = $true }
if ($PSBoundParameters.ContainsKey('GdbPort')) { $params.GdbPort = $GdbPort }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') @params
exit $LASTEXITCODE
