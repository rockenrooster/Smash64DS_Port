param([Parameter(Mandatory)][string]$Out, [string]$Compare = '')
# Makefile refactor proof (P8 G): `make -n -B` for every live build
# configuration -- the play, walk, wireless-lab and root ROMs, the gate ROM
# and its profiling, soak and VRAM-census variants -- each into its own scratch
# build directory (builds/mkn-<config>), one listing per configuration in
# $Out. With -Compare <an earlier $Out>, each listing is diffed against that
# one and the run fails on any difference. Builds serialise on the tools'
# mutex; never edit sources while it runs.
. "$PSScriptRoot\common.ps1"
$ErrorActionPreference = 'Stop'
Set-Location $S64Repo
Use-S64DevkitPro
New-Item -ItemType Directory -Force $Out | Out-Null
$labLd = Join-Path $S64Repo 'builds\mkn-lab\nds_hot_text.lab.ld'
New-Item -ItemType Directory -Force (Split-Path $labLd) | Out-Null
$text = [IO.File]::ReadAllText((Join-Path $S64Repo 'linker\nds_hot_text.ld'))
$text = $text.Replace('*(.text.ndsRendererProfileSetOwner)', '/* LAB sweep: evicted */')
foreach ($sym in $S64SweepEvictions) { $text = $text.Replace("*(.text.$sym)", "/* LAB sweep: evicted $sym */") }
[IO.File]::WriteAllText($labLd, $text)
$gate = @('TARGET=smash64ds-p2-fourcpu-tickhud-hwtri', 'NDS_LAB_FOURCPU_WORDS=1',
    'NDS_STAGE_YOSTER=1', 'NDS_STAGE_CASTLE=1', 'NDS_STAGE_JUNGLE=1', 'NDS_STAGE_ZEBES=1',
    'NDS_STAGE_HYRULE=1', 'NDS_STAGE_YAMABUKI=1', 'NDS_STAGE_INISHIE=1', 'NDS_STAGE_SECTOR=1',
    'NDS_PIKACHU=1', 'NDS_YOSHI=1', 'NDS_NESS=1', 'NDS_PURIN=1',
    "NDS_HOT_TEXT_LINKER_SCRIPT=$($labLd -replace '\\', '/')") + $S64ShipFlags
$profile = @('NDS_CPU_PROFILE=1', 'NDS_CPU_PROFILE_START=800', 'NDS_CPU_PROFILE_FRAMES=800',
             'NDS_CPU_PROFILE_PER_FRAME_REGION=1')
$configs = [ordered]@{
    play = @('TARGET=smash64ds') + $S64PlayFlags
    walk = @('TARGET=smash64ds', 'NDS_MENU_WALK=1') + $S64PlayFlags
    net  = @('TARGET=smash64ds', 'NDS_NET_LAB_LOBBY=1', 'NDS_NET_LAB_SWEEP=1') + $S64PlayFlags
    root = @('TARGET=smash64ds')
    gate = $gate
    prof = $gate + $profile
    soak = $gate + @('NDS_R2_SOAK_MATCH_MINUTES=5')
    vram = $gate + @('NDS_DIAG_COUNTERS=1')
}
$bad = 0
foreach ($name in $configs.Keys) {
    $build = "builds/mkn-$name"
    New-Item -ItemType Directory -Force $build | Out-Null
    $root = (Join-Path $S64Repo $build) -replace '\\', '/'
    $file = Join-Path $Out "$name.txt"
    Invoke-S64Serialized {
        make -n -B "BUILD=$build" "NDS_OUTPUT_ROOT=$root" @($configs[$name]) *> $file
    }
    $line = "{0,-5} {1} lines" -f $name, (Get-Content $file).Count
    if ($Compare -ne '') {
        $ref = Join-Path $Compare "$name.txt"
        $same = (Test-Path $ref) -and ((Get-FileHash $ref).Hash -eq (Get-FileHash $file).Hash)
        $line += $(if ($same) { '  IDENTICAL' } else { '  DIFFERS' })
        if (-not $same) { $bad++ }
    }
    $line
}
if ($Compare -ne '') { if ($bad -ne 0) { "MAKE DRY IDENTITY FAILED ($bad)"; exit 1 } else { 'MAKE DRY IDENTITY PASSED' } }
