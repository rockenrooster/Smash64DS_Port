param(
    [string]$Tag = 'c6',
    # roster name -> build dir suffix; each gets a route 0 and a route 1 capture
    [string[]]$Rosters = @('mfly', 'cppn', 'hi-mafo', 'hi-luyo', 'hi-capi', 'hi-puns', 'hi-dklk'),
    [string]$Frames = '150,260,420,640,900,1200',
    # rosters photographed at their first electric-skeleton draws instead
    [string[]]$SkeletonRosters = @('mfpn'),
    [int]$SkeletonShots = 4
)
# P2-2p8 Phase 1 slice 6: route 0 / route 1 capture pairs per roster on one ROM
# (capture-s6.ps1, runner slot 9), then capcmp6.py per pair. Output:
# captures/cap-<Tag>-<roster>-r<route>-*.png and captures-<Tag>.txt.
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice6'
$cap = Join-Path $art 'tools\capture-s6.ps1'
$cmp = Join-Path $art 'tools\capcmp6.py'
$out = Join-Path $art "captures-$Tag.txt"
Set-Content -LiteralPath $out -Value "slice 6 capture pairs ($Tag), route 0 vs route 1 on one ROM"
foreach ($r in $Rosters) {
    foreach ($route in 0, 1) {
        & $cap -Arm "cap-$Tag-$r-r$route" -Route $route -Build "build-p2p8-s6-$r" -Frames $Frames
    }
    Add-Content -LiteralPath $out -Value "`n== $r (frames $Frames)"
    Add-Content -LiteralPath $out -Value (& python $cmp "cap-$Tag-$r-r0" "cap-$Tag-$r-r1" --diff | Out-String)
}
foreach ($r in $SkeletonRosters) {
    foreach ($route in 0, 1) {
        & $cap -Arm "cap-$Tag-$r-r$route" -Route $route -Build "build-p2p8-s6-$r" -SkeletonShots $SkeletonShots
    }
    Add-Content -LiteralPath $out -Value "`n== $r (first $SkeletonShots electric-skeleton draws)"
    Add-Content -LiteralPath $out -Value (& python $cmp "cap-$Tag-$r-r0" "cap-$Tag-$r-r1" --diff | Out-String)
}
Get-Content -LiteralPath $out
