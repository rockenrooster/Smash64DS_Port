param(
    [switch]$Build,
    [string]$NoGba = (Join-Path $PSScriptRoot '..\emulators\nogba\NO$GBA.EXE'),
    [string]$Rom = (Join-Path $PSScriptRoot '..\smash64ds.nds'),
    [string[]]$EmulatorArgs = @()
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$noGbaPath = if ([System.IO.Path]::IsPathRooted($NoGba)) {
    $NoGba
} else {
    Join-Path $root $NoGba
}
$noGbaDir = Split-Path -Parent $noGbaPath
$romPath = if ([System.IO.Path]::IsPathRooted($Rom)) {
    $Rom
} else {
    Join-Path $root $Rom
}
if ($Build) {
    if (-not $env:DEVKITPRO) { $env:DEVKITPRO = 'C:/devkitPro' }
    if (-not $env:DEVKITARM) { $env:DEVKITARM = 'C:/devkitPro/devkitARM' }
    & make -C $root -j16
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
if (-not (Test-Path $noGbaPath)) {
    throw "no`$gba executable not found: $noGbaPath. Place NO`$GBA.EXE in emulators\nogba or pass -NoGba."
}
if (-not (Test-Path $romPath)) {
    throw "ROM not found: $romPath. Run `make -j16` first or pass -Build."
}
Write-Output 'Launching no$gba for interactive DS hardware/debugger inspection.'
Write-Output 'Use this for VRAM/OAM/register/timing inspection; melonDS remains the automated GDB verifier.'
$argsList = @("`"$romPath`"") + $EmulatorArgs
Start-Process -FilePath $noGbaPath -ArgumentList $argsList -WorkingDirectory $noGbaDir
