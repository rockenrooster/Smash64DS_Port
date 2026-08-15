param(
    [switch]$Force,
    [switch]$VerifyOnly
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

# decomp/BattleShip-main is a build input, not only a reference: the Makefile
# compiles decomp/src/sys in place, puts decomp/src on the include path, and
# copies NitroFS payloads out of BattleShip_o2r/ and decomp/assets/us/relocData.
# It is gitignored (/decomp/) because it is third-party source plus ROM-derived
# data, so this script reconstructs it from upstream at the pinned commits the
# port was written against.
#
# The two trees are pinned independently and deliberately. The on-disk snapshot
# is a GitHub zip of BattleShip main plus a SEPARATE checkout of the upstream
# decompilation - it is NOT the decomp submodule BattleShip itself pins, and
# `git submodule update` would silently swap the behavioral reference.
$battleShipUrl = 'https://github.com/JRickey/BattleShip.git'
$battleShipPin = '62b513e9895ac5a4e833102e098afdfc05c9a48c'  # main, 2026-06-10
$decompUrl = 'https://github.com/VetriTheRetri/ssb-decomp-re.git'
$decompPin = 'e6f3eee68dbe19fbac87914b613ff4ea6f29e251'      # main, 2026-06-12

$destination = Join-Path $root 'decomp/BattleShip-main'
$decompDestination = Join-Path $destination 'decomp'
$patchDir = Join-Path $PSScriptRoot 'decomp-patches/battleship'

# The port's own edits to the decompilation, keyed on -DSSB64_TARGET_NDS. Every
# one of these files is compiled into the ROM through a src/import wrapper, so
# they are build input and must survive a re-fetch.
#
# Patching a file that scripts/stages/generate_nds_native_stage.py pins costs a
# verifier run if you forget: the M3 stage falsifier hashes its TEXT_INPUTS and
# aborts Boundary with "SHA256 x != pinned y" before anything runs. objanim.c is
# pinned there; re-pin it in the same change and say why the bytes moved.
$patches = [ordered]@{
    'src/ft/ftanim.c'                = 'src_ft_ftanim.patch'
    'src/ft/ftmain.c'                = 'src_ft_ftmain.patch'
    'src/mn/mncommon/mnstartup.c'    = 'src_mn_mncommon_mnstartup.patch'
    'src/mn/mncommon/mntitle.c'      = 'src_mn_mncommon_mntitle.patch'
    'src/mv/mvopening/mvopeningroom.c' = 'src_mv_mvopening_mvopeningroom.patch'
    'src/sc/scmanager.c'             = 'src_sc_scmanager.patch'
    'src/sys/objanim.c'              = 'src_sys_objanim.patch'
    'src/sys/objhelper.c'            = 'src_sys_objhelper.patch'
    'src/sys/objman.c'               = 'src_sys_objman.patch'
    'src/sys/taskman.c'              = 'src_sys_taskman.patch'
}

function Invoke-Git {
    param([string[]]$Arguments)
    $output = & git @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw ('git {0} failed: {1}' -f ($Arguments -join ' '), ($output -join "`n"))
    }
    return $output
}

function Test-PatchApplied {
    param([string]$RelativePath)
    $file = Join-Path $decompDestination $RelativePath
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { return $false }
    return (Select-String -LiteralPath $file -Pattern 'SSB64_TARGET_NDS' -Quiet -SimpleMatch)
}

if ($VerifyOnly) {
    if (-not (Test-Path -LiteralPath $decompDestination -PathType Container)) {
        throw ('Missing {0}. Run this script without -VerifyOnly to fetch it.' -f $decompDestination)
    }
    $unpatched = @($patches.Keys | Where-Object { -not (Test-PatchApplied $_) })
    if ($unpatched.Count -gt 0) {
        throw ('BattleShip reference is missing its SSB64_TARGET_NDS port edits: {0}' -f
            ($unpatched -join ', '))
    }
    $missingRom = @(
        'assets/us/relocData'
    ) | Where-Object {
        -not (Test-Path -LiteralPath (Join-Path $decompDestination $_) -PathType Container)
    }
    $missingO2r = -not (Test-Path -LiteralPath (Join-Path $destination 'BattleShip_o2r') -PathType Container)
    if ($missingRom.Count -gt 0 -or $missingO2r) {
        Write-Output 'BattleShip source present; ROM-derived payloads absent (see the note below).'
    } else {
        Write-Output 'BattleShip reference verified: pinned source, port edits, and ROM-derived payloads all present.'
    }
    exit 0
}

if (Test-Path -LiteralPath $destination) {
    if (-not $Force) {
        throw ('{0} already exists. Re-run with -Force to replace it, or use -VerifyOnly to check the existing tree.' -f $destination)
    }
    Write-Output ('Removing existing {0} ...' -f $destination)
    Remove-Item -LiteralPath $destination -Recurse -Force
}

# core.autocrlf=false keeps the working tree byte-identical to the pinned blobs,
# which is what lets the patches below apply and lets a hash comparison mean
# anything on Windows.
Write-Output ('Cloning BattleShip @ {0} ...' -f $battleShipPin.Substring(0, 9))
Invoke-Git @('-c', 'core.autocrlf=false', 'clone', '--no-checkout',
    '--no-recurse-submodules', $battleShipUrl, $destination) | Out-Null
Invoke-Git @('-C', $destination, '-c', 'core.autocrlf=false', 'checkout',
    '--detach', $battleShipPin) | Out-Null

Write-Output ('Cloning ssb-decomp-re @ {0} ...' -f $decompPin.Substring(0, 9))
if (Test-Path -LiteralPath $decompDestination) {
    Remove-Item -LiteralPath $decompDestination -Recurse -Force
}
Invoke-Git @('-c', 'core.autocrlf=false', 'clone', '--no-checkout',
    $decompUrl, $decompDestination) | Out-Null
Invoke-Git @('-C', $decompDestination, '-c', 'core.autocrlf=false', 'checkout',
    '--detach', $decompPin) | Out-Null

foreach ($relativePath in $patches.Keys) {
    $patchFile = Join-Path $patchDir $patches[$relativePath]
    if (-not (Test-Path -LiteralPath $patchFile -PathType Leaf)) {
        throw ('Missing patch {0}' -f $patchFile)
    }
    Invoke-Git @('-C', $decompDestination, 'apply', '--whitespace=nowarn', $patchFile) | Out-Null
    if (-not (Test-PatchApplied $relativePath)) {
        throw ('Patch applied but {0} has no SSB64_TARGET_NDS marker.' -f $relativePath)
    }
    Write-Output ('  patched {0}' -f $relativePath)
}

Write-Output ''
Write-Output 'BattleShip source reference restored.'
Write-Output ''
Write-Output 'NOT restored, because it is ROM-derived and cannot be redistributed:'
Write-Output '  decomp/BattleShip-main/decomp/assets/     (make extract, needs your own baserom.us.z64)'
Write-Output '  decomp/BattleShip-main/BattleShip_o2r/    (BattleShip asset export)'
Write-Output 'The NitroFS payload rules in the Makefile read both paths, so a build'
Write-Output 'needs them. See decomp/BattleShip-main/decomp/README.md for extraction.'
exit 0
