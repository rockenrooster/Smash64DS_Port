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
    Harness = 'battle_mariofox_stage_mppassive_recover_loop'
    Target = 'smash64ds-battle-mariofox-stage-mppassive-recover-loop'
    Build = 'build-battle-mariofox-stage-mppassive-recover-loop-harness'
    ExpectedMode = 155
    ExpectedHarnessSceneCurr = 22
    ExpectedHarnessScenePrev = 21
    Label = 'Battle Mario/Fox Stage MP Passive recover-loop'
    HarnessSelectMessage = 'Direct Mario/Fox Stage MP Passive recover-loop harness did not select VSBattle from Maps.'
    RequireStageDraw = $true
    RequireStageMPPassiveRecoverLoop = $true
}
if ($NoBuild) { $params.NoBuild = $true }
if ($PSBoundParameters.ContainsKey('GdbPort')) { $params.GdbPort = $GdbPort }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') @params
exit $LASTEXITCODE
