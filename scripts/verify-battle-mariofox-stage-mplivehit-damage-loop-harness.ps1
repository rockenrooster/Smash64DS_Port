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
    Harness = 'battle_mariofox_stage_mplivehit_damage_loop'
    Target = 'smash64ds-battle-mariofox-stage-mplivehit-damage-loop'
    Build = 'build-battle-mariofox-stage-mplivehit-damage-loop-harness'
    ExpectedMode = 159
    ExpectedHarnessSceneCurr = 22
    ExpectedHarnessScenePrev = 21
    Label = 'Battle Mario/Fox Stage MP live-hit damage-loop'
    HarnessSelectMessage = 'Direct Mario/Fox Stage MP live-hit damage-loop harness did not select VSBattle from Maps.'
    RequireStageDraw = $true
    RequireStageMPPassiveRecoverLoop = $true
    RequireStageMPDamageRecoverLoop = $true
    RequireStageMPLiveHitDamageLoop = $true
}
if ($NoBuild) { $params.NoBuild = $true }
if ($PSBoundParameters.ContainsKey('GdbPort')) { $params.GdbPort = $GdbPort }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') @params
exit $LASTEXITCODE
