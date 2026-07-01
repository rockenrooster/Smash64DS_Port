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
    Harness = 'menu_chain_mariofox_stage_inishie_scale_loop'
    Target = 'smash64ds-menu-chain-mariofox-stage-inishie-scale-loop'
    Build = 'build-menu-chain-mariofox-stage-inishie-scale-loop-source-harness'
    ExpectedMode = 154
    ExpectedHarnessSceneCurr = 9
    ExpectedHarnessScenePrev = 1
    Label = 'Menu-chain Mario/Fox Stage Inishie scale-loop'
    HarnessSelectMessage = 'Menu-chain Mario/Fox Stage Inishie scale-loop harness did not start at VS Mode from Title.'
    RequireStageDraw = $true
    RequireStageInishieScaleLoop = $true
}
if ($NoBuild) { $params.NoBuild = $true }
if ($PSBoundParameters.ContainsKey('GdbPort')) { $params.GdbPort = $GdbPort }
& (Join-Path $PSScriptRoot 'verify-battle-mariofox-gcdrawall-loop-harness.ps1') @params
exit $LASTEXITCODE
