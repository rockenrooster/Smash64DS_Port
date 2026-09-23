param(
    [string]$LogName = 'build-1p-pre-slice3.log',
    [string]$Target = 'smash64ds-p2-shell-freeplay-hwtri',
    [string]$Build = 'build-p2p8-s3-1p-pre'
)
# P2-2p8 Phase 1 slice 3, D3 attribution: build the 1P campaign lab ROM from
# the PRE-slice-3 sources (the slice's start-of-work backups) in its own build
# dir, under the shared build lock, then put the slice 3 sources back and
# prove them byte-identical. The lock is held for the whole swap, so no other
# build can see the swapped tree.
$ErrorActionPreference = 'Stop'
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$lock = Join-Path $root 'builds\.p2p8-build.lock'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice3'
$bk = 'C:\Users\Tyler\AppData\Local\Temp\claude\D--Stuff-DevFolder-Smash64DS-Port\31e5c78d-38d0-4d75-b683-91723230d2cc\scratchpad\backup-slice3'
$final = Join-Path $bk 'final'
$log = Join-Path $art $LogName
# tree path -> pre-slice-3 source
$pre = [ordered]@{
    'Makefile'                                = (Join-Path $bk 'Makefile')
    'include\nds\renderer_fighter_lean.h'     = (Join-Path $bk 'include_nds_renderer_fighter_lean.h')
    'src\nds\nds_ftr_lean_kernel.c'           = (Join-Path $bk 'src_nds_nds_ftr_lean_kernel.c')
    'src\nds\nds_renderer_native_common.c'    = (Join-Path $bk 'src_nds_nds_renderer_native_common.c')
    'src\nds\nds_renderer_preamble.c'         = (Join-Path $bk 'src_nds_nds_renderer_preamble.c')
    'src\port\renderer_adapter_fighter.c'     = (Join-Path $bk 'src_port_renderer_adapter_fighter.c')
    'src\port\renderer_adapter_matrix.c'      = (Join-Path $bk 'src_port_renderer_adapter_matrix.c')
    'src\port\renderer_fighter_lean.c'        = (Join-Path $bk 'src_port_renderer_fighter_lean.c')
    'src\port\reloc_backend_compat_shims.c'   = (Join-Path $bk 'src_port_reloc_backend_compat_shims.c.pre-slice3')
    'src\nds\nds_platform.c'                  = (Join-Path $bk 'a14\nds_platform.c')
}
foreach ($k in $pre.Keys) {
    if (-not (Test-Path -LiteralPath $pre[$k])) { throw "missing pre-slice-3 source for $k" }
}
while (Test-Path -LiteralPath $lock) { Start-Sleep -Seconds 5 }
Set-Content -LiteralPath $lock -Value 'phase1-slice3-pre-ab' -NoNewline
$hashes = @{}
try {
    New-Item -ItemType Directory -Force -Path $final | Out-Null
    foreach ($k in $pre.Keys) {
        $p = Join-Path $root $k
        $name = ($k -replace '[\\/]', '_')
        Copy-Item -LiteralPath $p -Destination (Join-Path $final $name) -Force
        $hashes[$k] = (Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash
    }
    foreach ($k in $pre.Keys) {
        Copy-Item -LiteralPath $pre[$k] -Destination (Join-Path $root $k) -Force
    }
    $env:DEVKITPRO = 'C:/devkitPro'
    $env:DEVKITARM = 'C:/devkitPro/devkitARM'
    Push-Location $root
    $start = Get-Date
    $all = @("TARGET=$Target", "BUILD=$Build", 'NDS_P2_MENU_WALK=1')
    Set-Content -LiteralPath $log -Value ("make " + ($all -join ' ') + "   (PRE-slice-3 sources)")
    $ErrorActionPreference = 'Continue'
    & make @all *>> $log
    $code = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    Pop-Location
    Add-Content -LiteralPath $log -Value ("`nMAKE_EXIT=$code elapsed=" + ((Get-Date) - $start).TotalSeconds)
} finally {
    # Put the slice 3 sources back and prove them byte-identical.
    $bad = @()
    foreach ($k in $pre.Keys) {
        $name = ($k -replace '[\\/]', '_')
        $src = Join-Path $final $name
        if (Test-Path -LiteralPath $src) {
            Copy-Item -LiteralPath $src -Destination (Join-Path $root $k) -Force
            $h = (Get-FileHash -LiteralPath (Join-Path $root $k) -Algorithm SHA256).Hash
            if ($hashes.ContainsKey($k) -and ($h -ne $hashes[$k])) { $bad += $k }
        }
    }
    Add-Content -LiteralPath $log -Value ("RESTORED slice 3 sources; mismatches: " + $bad.Count + " " + ($bad -join ','))
    Remove-Item -LiteralPath $lock -Force -ErrorAction SilentlyContinue
}
