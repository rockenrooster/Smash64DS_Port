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
    Harness = 'menu_chain_mariofox_stage_mpdamage_recover_loop'
    Target = 'smash64ds-menu-chain-mariofox-stage-mpdamage-recover-loop'
    Build = 'build-menu-chain-mariofox-stage-mpdamage-recover-loop-harness'
    ExpectedMode = 158
    ExpectedHarnessSceneCurr = 9
    ExpectedHarnessScenePrev = 1
    Label = 'Menu-chain Mario/Fox Stage MP damage-recover-loop'
    HarnessSelectMessage = 'Menu-chain Mario/Fox Stage MP damage-recover-loop harness did not select VS Mode from Title.'
    RequireStageDraw = $true
    RequireStageMPPassiveRecoverLoop = $true
    RequireStageMPDamageRecoverLoop = $true
}
if ($NoBuild) { $params.NoBuild = $true }
if ($PSBoundParameters.ContainsKey('GdbPort')) { $params.GdbPort = $GdbPort }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') @params
exit $LASTEXITCODE
