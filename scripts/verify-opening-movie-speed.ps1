param(
    [switch]$Build,
    [switch]$Visible,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe',
    [int]$GdbPort = 3333,
    [int]$RunnerSlot = -1,
    [switch]$NoBuild,
    [int]$DelaySeconds = 80,
    [double]$MinHostFps = 30.0
)
$ErrorActionPreference = 'Stop'
$sample = Join-Path $PSScriptRoot 'sample-runtime-speed.ps1'
$sampleParams = @{
    MelonDS = $MelonDS
    Gdb = $Gdb
    GdbPort = $GdbPort
    RunnerSlot = $RunnerSlot
    NoBuild = $NoBuild
    DelaySeconds = $DelaySeconds
    RequireTitle = $true
    MinRoomTick = 1320
    MinActionScenes = 9
    MinActionFrames = 324
    MinHostFps = $MinHostFps
}
if ($Build) {
    $sampleParams.Build = $true
}
if ($Visible) {
    $sampleParams.Visible = $true
}
& $sample @sampleParams
