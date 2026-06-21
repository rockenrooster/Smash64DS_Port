param(
    [switch]$Build,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Gdb = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gdb.exe'
)

$ErrorActionPreference = 'Stop'

if ($Build) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    & make -C (Resolve-Path (Join-Path $PSScriptRoot '..')).Path -j4
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

& (Join-Path $PSScriptRoot 'verify-opening-movie-speed.ps1') `
    -MelonDS $MelonDS `
    -Gdb $Gdb
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
