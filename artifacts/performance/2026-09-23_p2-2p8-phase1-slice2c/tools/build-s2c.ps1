param(
    [string]$LogName = 'build.log',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [string]$Build = 'build-p2-fourcpu-tickhud',
    [string[]]$MakeArgs = @()
)
# P2-2p8 Phase 1 slice 2c build wrapper: waits for and holds the shared build
# lock, builds ONE target into ONE build dir (no -j), logs to the artifact dir.
$ErrorActionPreference = 'Continue'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$lock = Join-Path $root 'builds\.p2p8-build.lock'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice2c'
New-Item -ItemType Directory -Force -Path $art | Out-Null
$log = Join-Path $art $LogName
while (Test-Path -LiteralPath $lock) { Start-Sleep -Seconds 5 }
Set-Content -LiteralPath $lock -Value 'phase1' -NoNewline
try {
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
    Push-Location $root
    $start = Get-Date
    $all = @("TARGET=$Target", "BUILD=$Build") + $MakeArgs
    Set-Content -LiteralPath $log -Value ("make " + ($all -join ' '))
    & make @all *>> $log
    $code = $LASTEXITCODE
    Pop-Location
    Add-Content -LiteralPath $log -Value ("`nMAKE_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds)
} finally {
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
