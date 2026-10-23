param([string]$Build = 'build-vram-v4', [int[]]$Gkinds = @(4), [string]$Kinds = '16777472', [string]$Content = '336661769',
      [string]$Tag = 'vmap', [int]$Slot = 11, [int]$Frames = 900, [int]$Seconds = 200)
# P8 V4, the texture VRAM map: on a census lab ROM (vram_census.ps1's build,
# NDS_DIAG_COUNTERS=1) run a four-CPU match on each stage of -Gkinds and, at
# presented frame -Frames, dump every live texture name (address, bytes,
# format, size, owner bucket, palette bytes) and the census's creating-site
# PCs, and each texture's content (texel hash, colours, alpha levels, 4x4 fit:
# ndsVramCensusContent); then scripts/tools/vram_map.py turns each record into
# a static HTML page of banks A-D in artifacts/visibility/vram/<Build>/ (no
# JavaScript).
. "$PSScriptRoot\common.ps1"
foreach ($g in $Gkinds) {
    $name = "$Tag-g$g"
    $lines = @('break ndsLabCensusFrameHook', 'commands', 'silent',
               'printf "X MAPHDR frame=%u count=%u sites=%u vramcnt=%x\n", gNdsFrameCounter, gNdsVramCensusMapCount, gNdsVramCensusSiteCount, *(unsigned int *)0x04000240',
               'set $i = 0',
               'while $i < gNdsVramCensusMapCount',
               'printf "X ROW %u %x %u %u %u %u %u %x %u %u %u\n", gNdsVramCensusMap[$i].name, gNdsVramCensusMap[$i].addr, gNdsVramCensusMap[$i].bytes, gNdsVramCensusMap[$i].bucket, gNdsVramCensusMap[$i].format, gNdsVramCensusMap[$i].size, gNdsVramCensusMap[$i].pal_bytes, gNdsVramCensusMap[$i].hash, gNdsVramCensusMap[$i].colors, gNdsVramCensusMap[$i].alphas, gNdsVramCensusMap[$i].flags',
               'set $i = $i + 1',
               'end',
               'set $i = 0',
               'while $i < gNdsVramCensusBlockCount',
               'printf "X BLOCK %x %u %u\n", gNdsVramCensusBlockMap[$i].addr, gNdsVramCensusBlockMap[$i].bytes, gNdsVramCensusBlockMap[$i].used',
               'set $i = $i + 1',
               'end',
               'set $i = 0',
               'while $i < gNdsVramCensusSiteCount',
               'printf "X SITE %u %x\n", $i, gNdsVramCensusSitePc[$i]',
               'set $i = $i + 1',
               'end',
               'printf "X MAPEND\n"',
               'delete', 'detach', 'quit', 'end')
    & "$PSScriptRoot\probe_session.ps1" -Build $Build -Rom 'smash64ds-p2-fourcpu-tickhud-hwtri' -Slot $Slot -Tag $name `
        -Seconds $Seconds -Pokes @("set var gNdsLabFourCpuGkind = $g", "set var gNdsLabFourCpuKinds = $Kinds",
                                   "set var gNdsLabP4Content = $Content", 'set var gNdsVramCensusEnable = 1',
                                   "set var gNdsLabCensusFrame = $Frames") `
        -Lines $lines -StopWithGdb | Out-Null
    $log = Join-Path $S64Probes "$name.log"
    python (Join-Path $PSScriptRoot 'vram_map.py') --log $log --elf (Join-Path $S64Repo "builds\$Build\smash64ds-p2-fourcpu-tickhud-hwtri.elf") `
        --out (Join-Path $S64Repo "artifacts\visibility\vram\$Build\$name.html") --title "$Build $name (gkind $g, frame $Frames)"
}
