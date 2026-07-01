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
    Harness = 'menu_chain_mariofox_stage_mplivehit_status_loop'
    Target = 'smash64ds-menu-chain-mariofox-stage-mplivehit-status-loop'
    Build = 'build-menu-chain-mariofox-stage-mplivehit-status-loop-harness'
    ExpectedMode = 162
    ExpectedHarnessSceneCurr = 9
    ExpectedHarnessScenePrev = 1
    Label = 'Menu-chain Mario/Fox Stage MP live-hit status-loop'
    HarnessSelectMessage = 'Menu-chain Mario/Fox Stage MP live-hit status-loop harness did not select VS Mode from Title.'
    RequireStageDraw = $true
    RequireStageMPPassiveRecoverLoop = $true
    RequireStageMPDamageRecoverLoop = $true
    RequireStageMPLiveHitDamageLoop = $true
    RequireStageMPLiveHitStatusLoop = $true
}
if ($NoBuild) { $params.NoBuild = $true }
if ($PSBoundParameters.ContainsKey('GdbPort')) { $params.GdbPort = $GdbPort }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') @params
exit $LASTEXITCODE
