param(
    [Parameter(Mandatory)][string]$Build,
    [string]$Seed = '',
    [string]$Gkind = '',
    [string[]]$Extra = @(),
    # More hot-text sections to leave out of ITCM, for this lab build only.
    [string[]]$EvictSyms = @(),
    # Input sections (full names, e.g. .bss.gNdsFooCount) to leave out of the
    # DTCM hot list, for this lab build only: a diagnostics build's counters
    # take DTCM past the boot stack's low-water ceiling.
    [string[]]$EvictSections = @(),
    # A shipping build whose NitroFS fills the files the four-CPU target does
    # not stage (default: the newest builds/play-*).
    [string]$NitroFrom = ''
)
# The four-CPU sweep ROM (TARGET=smash64ds-p2-fourcpu-tickhud-hwtri): the
# boot-poked stage, roster, P4 content and item words (NDS_LAB_FOURCPU_WORDS)
# and nothing else of the lab; every VS stage admitted; the lab-only ITCM
# evictions in a per-build copy of the hot-text linker script; the shipping
# build's whole NitroFS. build_gate_rom.ps1 adds the ship layout on top: use
# that one for any measurement.
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
Set-Location $S64Repo
Use-S64DevkitPro
$rom = 'smash64ds-p2-fourcpu-tickhud-hwtri.nds'
$ld = 'linker/nds_hot_text.ld'
$text = [IO.File]::ReadAllText("$PWD/$ld")
$labText = $text.Replace('*(.text.ndsRendererProfileSetOwner)', '/* LAB sweep: evicted */')
foreach ($sym in $EvictSyms) {
    $labText = $labText.Replace("*(.text.$sym)", "/* LAB sweep: evicted $sym */")
}
foreach ($sec in $EvictSections) {
    $labText = $labText.Replace("*($sec)", "/* LAB sweep: evicted $sec */")
}
Copy-S64BuildSeed -Build $Build -Seed $Seed -DropRoms @($rom)
New-Item -ItemType Directory -Force "builds/$Build" | Out-Null
$labLd = Join-Path $PWD "builds/$Build/nds_hot_text.lab.ld"
$old = if (Test-Path $labLd) { [IO.File]::ReadAllText($labLd) } else { '' }
if ($old -ne $labText) { [IO.File]::WriteAllText($labLd, $labText) }
$makeArgs = @("TARGET=$($rom -replace '\.nds$', '')", "BUILD=builds/$Build", 'NDS_LAB_FOURCPU_WORDS=1',
    'NDS_P2_STAGE_YOSTER=1', 'NDS_P2_STAGE_CASTLE=1', 'NDS_P2_STAGE_JUNGLE=1', 'NDS_P2_STAGE_ZEBES=1',
    'NDS_P2_STAGE_HYRULE=1', 'NDS_P2_STAGE_YAMABUKI=1', 'NDS_P2_STAGE_INISHIE=1', 'NDS_P2_STAGE_SECTOR=1',
    # The sweep pokes any kind; only admitted kinds are compiled (a missing one wedges melonDS).
    'NDS_P2_PIKACHU=1', 'NDS_P2_YOSHI=1', 'NDS_P2_NESS=1', 'NDS_P2_PURIN=1',
    "NDS_HOT_TEXT_LINKER_SCRIPT=$($labLd -replace '\\', '/')")
if ($Gkind -ne '') { $makeArgs += "NDS_LAB_FOURCPU_SWEEP_GKIND=$Gkind" }
$makeArgs += $Extra
if ($NitroFrom -eq '') {
    $NitroFrom = Get-ChildItem builds -Directory -Filter 'play-*' |
        Where-Object { Test-Path (Join-Path $_.FullName 'nitrofs') } |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1 |
        ForEach-Object { "builds/$($_.Name)/nitrofs" }
}
Invoke-S64Serialized {
    make @makeArgs *> "builds/$Build-lab.log"
    if ($LASTEXITCODE -ne 0) { throw "make failed ($Build)" }
    $full = (Resolve-Path $NitroFrom).Path
    $dst = (Resolve-Path "builds/$Build/nitrofs").Path
    $n = 0
    Get-ChildItem -Recurse -File $full | ForEach-Object {
        $to = $dst + $_.FullName.Substring($full.Length)
        if (-not (Test-Path $to)) {
            New-Item -ItemType Directory -Force (Split-Path $to) | Out-Null
            Copy-Item $_.FullName $to
            $n++
        }
    }
    if ($n -gt 0) {
        [IO.File]::Delete("$PWD/builds/$Build/$rom")
        make @makeArgs *> "builds/$Build-lab-pack.log"
        if ($LASTEXITCODE -ne 0) { throw "repack failed ($Build)" }
    }
    "$Build ok copied=$n from=$NitroFrom files=$((Get-ChildItem -Recurse -File "builds/$Build/nitrofs").Count)"
}
