param(
    [string]$LogName = 'build-pre.log',
    [string]$Target = 'smash64ds-p2-fourcpu-tickhud-hwtri',
    [string]$Build = 'build-p2p8-s6-pre',
    [string]$SeedFrom = '',
    [string[]]$MakeArgs = @()
)
# P2-2p8 Phase 1 slice 6 A/B baseline (from slice 5's build-pre-s5.ps1):
# build the PRE-slice-6 sources of the files this slice edits (the start-of-
# work backups) on TODAY's tree, in their own build dir, under the shared build
# lock; then put the slice 6 sources back and prove them byte-identical. The
# lock is held for the whole swap, so no other build can see the swapped tree.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$lock = Join-Path $root 'builds\.p2p8-build.lock'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice6'
$bk = 'C:\Users\Tyler\AppData\Local\Temp\claude\D--Stuff-DevFolder-Smash64DS-Port\31e5c78d-38d0-4d75-b683-91723230d2cc\scratchpad\backup-slice6'
$final = Join-Path $bk 'current'
$log = Join-Path $art $LogName
$pre = [ordered]@{
    'include\nds\renderer_fighter_lean.h'     = (Join-Path $bk 'include_nds_renderer_fighter_lean.h')
    'src\nds\nds_renderer_native_common.c'    = (Join-Path $bk 'src_nds_nds_renderer_native_common.c')
    'src\port\renderer_adapter_fighter.c'     = (Join-Path $bk 'src_port_renderer_adapter_fighter.c')
    'src\port\renderer_adapter_matrix.c'      = (Join-Path $bk 'src_port_renderer_adapter_matrix.c')
    'src\port\renderer_fighter_lean.c'        = (Join-Path $bk 'src_port_renderer_fighter_lean.c')
    'src\port\nds_match_config.c'             = (Join-Path $bk 'src_port_nds_match_config.c')
    'Makefile'                                = (Join-Path $bk 'Makefile')
}
foreach ($k in $pre.Keys) {
    if (-not (Test-Path -LiteralPath $pre[$k])) { throw "missing pre-slice-6 source for $k" }
}
while (Test-Path -LiteralPath $lock) { Start-Sleep -Seconds 5 }
Set-Content -LiteralPath $lock -Value 'phase1-slice6-pre' -NoNewline
$hashes = @{}
try {
    New-Item -ItemType Directory -Force -Path $final | Out-Null
    $dst = Join-Path $root "builds\$Build"
    if ((-not (Test-Path -LiteralPath $dst)) -and ($SeedFrom -ne '')) {
        Copy-Item -LiteralPath (Join-Path $root "builds\$SeedFrom") -Destination $dst -Recurse
    }
    foreach ($k in $pre.Keys) {
        $p = Join-Path $root $k
        $name = ($k -replace '[\\/]', '_')
        Copy-Item -LiteralPath $p -Destination (Join-Path $final $name) -Force
        $hashes[$k] = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash
    }
    foreach ($k in $pre.Keys) {
        Copy-Item -LiteralPath $pre[$k] -Destination (Join-Path $root $k) -Force
        (Get-Item -LiteralPath (Join-Path $root $k)).LastWriteTime = Get-Date
    }
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
    Push-Location $root
    $start = Get-Date
    $all = @("TARGET=$Target", "BUILD=$Build") + $MakeArgs
    Set-Content -LiteralPath $log -Value ("make " + ($all -join ' ') + "   (PRE-slice-6 lean sources)")
    $ErrorActionPreference = 'Continue'
    & make @all *>> $log
    $code = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    Pop-Location
    $nds = Join-Path $dst "$Target.nds"
    $sha = if (Test-Path -LiteralPath $nds) { (Get-FileHash -LiteralPath $nds -Algorithm SHA256).Hash } else { 'none' }
    Add-Content -LiteralPath $log -Value ("`nMAKE_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds + " sha256=$sha")
} finally {
    $bad = @()
    foreach ($k in $pre.Keys) {
        $name = ($k -replace '[\\/]', '_')
        $src = Join-Path $final $name
        if (Test-Path -LiteralPath $src) {
            Copy-Item -LiteralPath $src -Destination (Join-Path $root $k) -Force
            (Get-Item -LiteralPath (Join-Path $root $k)).LastWriteTime = Get-Date
            $h = (Get-FileHash -LiteralPath (Join-Path $root $k) -Algorithm SHA256).Hash
            if ($hashes.ContainsKey($k) -and ($h -ne $hashes[$k])) { $bad += $k }
        }
    }
    Add-Content -LiteralPath $log -Value ("RESTORED slice 6 sources; mismatches: " + $bad.Count + " " + ($bad -join ','))
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
