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
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (-not $Build -and -not $NoBuild -and -not $List) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    Write-Output 'Building the normal opening ROM incrementally for P1Gate.'
    & make -C $root TARGET=smash64ds BUILD=build `
        NDS_DEV_SCENE_HARNESS=normal NDS_HARNESS_FAST_LOGIC=1 -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
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
