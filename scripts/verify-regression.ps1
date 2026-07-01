param(
    [switch]$Build,
    [switch]$NoBuild,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [ValidateSet('Regression','RegressionFast')]
    [string]$Profile = 'Regression',
    [int]$DelaySeconds = 5,
    [int]$ShardCount = 1,
    [int]$ShardIndex = 0,
    [int]$RunnerSlot = -1,
    [int]$GdbPort = 3333,
    [switch]$List,
    [switch]$SkipRegistryCheck
)
$ErrorActionPreference = 'Stop'
$params = @{
    Profile = $Profile
    MelonDS = $MelonDS
    Gdb = $Gdb
    DelaySeconds = $DelaySeconds
    ShardCount = $ShardCount
    ShardIndex = $ShardIndex
    RunnerSlot = $RunnerSlot
}
if ($PSBoundParameters.ContainsKey('GdbPort')) { $params.GdbPort = $GdbPort }
if ($Build) { $params.Build = $true }
if ($NoBuild) { $params.NoBuild = $true }
if ($List) { $params.List = $true }
if ($SkipRegistryCheck) { $params.SkipRegistryCheck = $true }
& (Join-Path $PSScriptRoot 'verify-all.ps1') @params
exit $LASTEXITCODE
