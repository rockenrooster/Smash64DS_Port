param(
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 5
)
$ErrorActionPreference = 'Stop'
function Invoke-StatusLoopVerifier {
    param([string]$ScriptName)

    $params = @{
        MelonDS = $MelonDS
        Gdb = $Gdb
        RunnerSlot = $RunnerSlot
        DelaySeconds = $DelaySeconds
    }
    if ($NoBuild) { $params.NoBuild = $true }
    if ($PSBoundParameters.ContainsKey('GdbPort')) { $params.GdbPort = $GdbPort }

    & (Join-Path $PSScriptRoot $ScriptName) @params
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Invoke-StatusLoopVerifier 'verify-battle-mariofox-stage-mplivehit-status-loop-harness.ps1'
Invoke-StatusLoopVerifier 'verify-menu-chain-mariofox-stage-mplivehit-status-loop-harness.ps1'
Write-Output 'Stage MP live-hit continuous runtime verifier passed.'
