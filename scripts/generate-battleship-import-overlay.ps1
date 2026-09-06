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

Set-Content -LiteralPath (Join-Path $output '.stamp') -Value (
    "generated from pristine decomp at {0:o}" -f [DateTime]::UtcNow) -Encoding ascii
