param(
    [switch]$Build,
    [string]$MelonDS = (Join-Path $PSScriptRoot '..\emulators\melonds\melonDS.exe'),
    [string]$Rom = (Join-Path $PSScriptRoot '..\smash64ds.nds')
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\melonds.ps1')
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$melonDsPath = if ([System.IO.Path]::IsPathRooted($MelonDS)) {
    $MelonDS
} else {
    Join-Path $root $MelonDS
}
$melonDsDir = Split-Path -Parent $melonDsPath
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
if (-not (Test-Path $melonDsPath)) {
    throw "melonDS executable not found: $melonDsPath"
}
if (-not (Test-Path $romPath)) {
    throw "ROM not found: $romPath. Run `make -j16` first or pass -Build."
}
$config = Join-Path $melonDsDir 'melonDS.toml'
if (Test-Path -LiteralPath $config) {
    $configText = Get-Content -LiteralPath $config -Raw
    $dualScreenConfig = Set-MelonDSDualScreenLayout -Text $configText
    if ($dualScreenConfig -ne $configText) {
        Set-Content -LiteralPath $config -Value $dualScreenConfig -NoNewline
    }
}
Write-Output 'Launching melonDS with live visual debug HUD.'
Write-Output 'Display: natural stacked top and bottom screens at equal size.'
Write-Output 'Top rail: self-test, boot chain, startup func, taskman, Opening Room boundary.'
Write-Output 'Previews: native-size original N64Logo sprite plus bounded Opening Room DObj DL slice.'
Write-Output 'Moving top-screen markers are disabled; detailed live values stay on the bottom screen and verifier.'
Write-Output 'Bottom text redraws only when diagnostics change, so steady-state captures stay stable.'
Start-Process -FilePath $melonDsPath -ArgumentList "`"$romPath`"" -WorkingDirectory $melonDsDir
