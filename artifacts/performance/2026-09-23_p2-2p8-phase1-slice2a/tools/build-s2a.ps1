param([string]$LogName = 'build.log')
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$lock = Join-Path $root 'builds\.p2p8-build.lock'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2a'
New-Item -ItemType Directory -Force -Path $art | Out-Null
$log = Join-Path $art $LogName
while (Test-Path -LiteralPath $lock) { Start-Sleep -Seconds 5 }
Set-Content -LiteralPath $lock -Value 'phase1' -NoNewline
try {
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
    Push-Location $root
    $start = Get-Date
    & make TARGET=smash64ds-p2-fourcpu-tickhud-hwtri BUILD=build-p2-fourcpu-tickhud *> $log
    $code = $LASTEXITCODE
    Pop-Location
    Add-Content -LiteralPath $log -Value ("`nMAKE_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds)
} finally {
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
