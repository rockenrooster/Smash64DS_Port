param(
    # Two-fighter HIGH arms to build (names after build-p2p8-s6-hi-), in order.
    [string[]]$Arms = @('dklk', 'mafo', 'luyo', 'capi', 'puns', 'saki'),
    [string]$Tag = 'c6'
)
# P2-2p8 Phase 1 slice 6: build the two-fighter HIGH lab arms, then re-make
# each one and compare the ROM sha256 (a lab-flag build can package or compile
# against a shared generated output another configuration just rewrote; the
# re-make must reproduce the ROM). A mismatch re-makes once more and reports
# whether the second re-make is stable.
$root = 'D:\Stuff\DevFolder\Smash64DS_Port'
$art = Join-Path $root 'artifacts\performance\2026-09-23_p2-2p8-phase1-slice6'
$b = Join-Path $art 'tools\build-s6.ps1'
$flags = @{
    'dklk' = @('NDS_LAB_FOURCPU_KINDS=2 5 2 5', 'NDS_LAB_FOURCPU_TWO=1')
    'mafo' = @('NDS_LAB_FOURCPU_KINDS=0 1 0 1', 'NDS_LAB_FOURCPU_TWO=1')
    'luyo' = @('NDS_LAB_FOURCPU_KINDS=4 6 4 6', 'NDS_P2_YOSHI=1', 'NDS_LAB_FOURCPU_TWO=1')
    'capi' = @('NDS_LAB_FOURCPU_KINDS=7 9 7 9', 'NDS_P2_PIKACHU=1', 'NDS_LAB_FOURCPU_TWO=1')
    'puns' = @('NDS_LAB_FOURCPU_KINDS=10 11 10 11', 'NDS_P2_PURIN=1', 'NDS_P2_NESS=1', 'NDS_LAB_FOURCPU_TWO=1')
    'saki' = @('NDS_LAB_FOURCPU_KINDS=3 8 3 8', 'NDS_LAB_FOURCPU_TWO=1')
}
function Sha($log) { ((Get-Content (Join-Path $art $log) -Tail 1) -replace '.*sha256=', '') }
foreach ($a in $Arms) {
    & $b -LogName "build-$Tag-hi-$a.log" -Build "build-p2p8-s6-hi-$a" -MakeArgs $flags[$a] -ForceParticleDeps
}
foreach ($a in $Arms) {
    & $b -LogName "build-$Tag-hi-$a-remake.log" -Build "build-p2p8-s6-hi-$a" -MakeArgs $flags[$a] -ForceParticleDeps
    $first = Sha "build-$Tag-hi-$a.log"
    $re = Sha "build-$Tag-hi-$a-remake.log"
    if ($first -ne $re) {
        & $b -LogName "build-$Tag-hi-$a-remake2.log" -Build "build-p2p8-s6-hi-$a" -MakeArgs $flags[$a] -ForceParticleDeps
        $re2 = Sha "build-$Tag-hi-$a-remake2.log"
        "hi-$a first=$first remake=$re remake2=$re2 stable=$($re -eq $re2)"
    } else {
        "hi-$a first=$first remake=$re same=True"
    }
}
