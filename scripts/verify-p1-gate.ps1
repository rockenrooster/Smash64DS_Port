param(
    [switch]$Build,
    [switch]$NoBuild,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$DelaySeconds = 5,
    [int]$RunnerSlot = -1,
    [int]$GdbPort = 3333,
    [switch]$List,
    [switch]$SkipRegistryCheck
)
$ErrorActionPreference = 'Stop'
if (-not $List) {
    throw 'P1Gate execution is retired while its compile-distinct legs are migrated to the two-ROM runtime model. Use verify-current.ps1 or verify-boundary.ps1.'
}
$params = @{
    Profile = 'P1Gate'
    MelonDS = $MelonDS
    Gdb = $Gdb
    DelaySeconds = $DelaySeconds
    RunnerSlot = $RunnerSlot
}
if ($PSBoundParameters.ContainsKey('GdbPort')) { $params.GdbPort = $GdbPort }
if ($Build) { $params.Build = $true }
if ($NoBuild) { $params.NoBuild = $true }
if ($List) { $params.List = $true }
if ($SkipRegistryCheck) { $params.SkipRegistryCheck = $true }
& (Join-Path $PSScriptRoot 'verify-all.ps1') @params
exit $LASTEXITCODE
