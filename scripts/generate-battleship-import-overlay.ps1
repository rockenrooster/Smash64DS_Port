param(
    [Parameter(Mandatory = $true)]
    [string]$OutputRoot
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$sourceRoot = Join-Path $root 'decomp/BattleShip-main/decomp'
$patchRoot = Join-Path $PSScriptRoot 'import-overlays/battleship'

# These are legacy DS adaptations whose semantics still live inside imported
# BattleShip translation units.  They are applied only to an ephemeral build
# overlay.  `decomp/` itself is immutable and is never a patch destination.
# ftmain and mntitle are intentionally absent: their DS behavior lives directly
# in src/import now.
$patches = [ordered]@{
    'src/ft/ftanim.c'                    = 'src_ft_ftanim.patch'
    'src/mn/mncommon/mnstartup.c'        = 'src_mn_mncommon_mnstartup.patch'
    'src/mn/mnoption/mnbackupclear.c'      = 'src_mn_mnoption_mnbackupclear.patch'
    'src/mv/mvopening/mvopeningroom.c'   = 'src_mv_mvopening_mvopeningroom.patch'
    'src/sc/scmanager.c'                 = 'src_sc_scmanager.patch'
    'src/sc/sc1pmode/sc1pgame.c'         = 'src_sc_sc1pmode_sc1pgame.patch'
    'src/sc/sccommon/scvsbattle.c'       = 'src_sc_sccommon_scvsbattle.patch'
    'src/sc/sccommon/scstaffroll.c'      = 'src_sc_sccommon_scstaffroll.patch'
    'src/sys/objanim.c'                  = 'src_sys_objanim.patch'
    'src/sys/objhelper.c'                = 'src_sys_objhelper.patch'
    'src/sys/objman.c'                   = 'src_sys_objman.patch'
    'src/sys/taskman.c'                  = 'src_sys_taskman.patch'
}

$output = if ([IO.Path]::IsPathRooted($OutputRoot)) {
    [IO.Path]::GetFullPath($OutputRoot)
} else {
    [IO.Path]::GetFullPath((Join-Path $root $OutputRoot))
}

if (-not $output.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
    throw "BattleShip overlay must be generated inside the repository: $output"
}

if (Test-Path -LiteralPath $output) {
    Remove-Item -LiteralPath $output -Recurse -Force
}
New-Item -ItemType Directory -Path $output -Force | Out-Null

foreach ($relativePath in $patches.Keys) {
    $source = Join-Path $sourceRoot $relativePath
    $destination = Join-Path $output $relativePath
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Missing pristine BattleShip source: $relativePath"
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination
}

# git apply is deliberately run from the project repository, with --directory
# pointing at the generated tree.  It never writes below decomp/.
$overlayRelative = [IO.Path]::GetRelativePath($root, $output).Replace('\', '/')
Push-Location $root
try {
    foreach ($relativePath in $patches.Keys) {
        $patch = Join-Path $patchRoot $patches[$relativePath]
        if (-not (Test-Path -LiteralPath $patch -PathType Leaf)) {
            throw "Missing import-overlay patch: $patch"
        }
        & git apply --whitespace=nowarn "--directory=$overlayRelative" $patch
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to generate BattleShip import overlay for $relativePath"
        }
    }
} finally {
    Pop-Location
}

# Staffroll narrow tables (2026-09-06): the overlaid scstaffroll.c stores the
# four credit character-ID tables as s8 instead of s32 (CompanyIDs rides the
# same patch; its enum initializers already fit). The pristine
# credits/*.encoded initializers spell the space/line-break sentinels as
# 32-bit hex (0xffffffdf/0xffffffc9), which would warn under -Wall as s8
# initializers, so the overlay carries narrowed copies spelling every value
# exactly: non-negatives verbatim, negatives as signed decimals. Any value
# outside s8 range throws here and fails the build instead of truncating.
$narrowCredits = @('staff', 'titles', 'info', 'companies')
$narrowDir = Join-Path $output 'src/sc/sccommon/credits'
New-Item -ItemType Directory -Path $narrowDir -Force | Out-Null
foreach ($name in $narrowCredits) {
    $encoded = Get-Content -LiteralPath (Join-Path $sourceRoot "src/credits/$name.credits.encoded") -Raw -Encoding UTF8
    $out = @()
    foreach ($token in ($encoded -split ',')) {
        $t = $token.Trim()
        if ($t.Length -eq 0) { continue }
        $v = [Convert]::ToUInt32($t, 16)
        if ($v -ge 0x80000000u) {
            $s = [int64]$v - 0x100000000
            if ($s -lt -128) { throw "Staffroll narrow table ${name}: value $t outside s8 range" }
            $out += $s.ToString()
        } else {
            if ($v -gt 127) { throw "Staffroll narrow table ${name}: value $t outside s8 range" }
            $out += $t
        }
    }
    if ($out.Count -eq 0) { throw "Staffroll narrow table ${name}: no values decoded" }
    [System.IO.File]::WriteAllText(
        (Join-Path $narrowDir "$name.credits.narrow"),
        (($out -join ',') + ",`n"),
        [System.Text.Encoding]::ASCII)
}

Set-Content -LiteralPath (Join-Path $output '.stamp') -Value (
    "generated from pristine decomp at {0:o}" -f [DateTime]::UtcNow) -Encoding ascii
