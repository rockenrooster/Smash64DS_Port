param(
    [string[]]$Steps = @('g', 'x')
)
# P2-2p8 Phase 1 slice 2c: the final runs, one at a time on runner slot 9.
#   g  the gate arms on the canonical ROM (word 0, word 2)
#   x  the battle-exit proof on the canonical ROM (word 0, word 2)
#   s  the exact-key shadow lab ROM (word 0 and word 2, shadow on; the
#      admission identity census at word 2)
#   c  the creation-time lab ROM (word default 2)
#   v  the two variant rosters (word 0; word 2 with the identity census)
$ErrorActionPreference = 'Continue'
$tools = 'D:\Stuff\DevFolder\Smash64DS_Port\artifacts\performance\2026-09-23_p2-2p8-phase1-slice2c\tools'
$run = Join-Path $tools 'run-s2c.ps1'
$exit = Join-Path $tools 'run-exit-s2c.ps1'
foreach ($step in $Steps) {
    switch ($step) {
        'g' {
            & $run -Arm g-admit0 -Admit 0
            & $run -Arm g-admit2 -Admit 2
        }
        'x' {
            try { & $exit -Arm x-admit0 -Admit 0 -Score 100 } catch { "x-admit0 FAILED: $_" }
            try { & $exit -Arm x-admit2 -Admit 2 -Score 100 } catch { "x-admit2 FAILED: $_" }
        }
        's' {
            & $run -Arm s-admit0-shadow -Admit 0 -Shadow 1 -Build build-p2p8-s2c-shadow
            & $run -Arm s-admit2-shadow -Admit 2 -Shadow 1 -IdCensus 1 -Build build-p2p8-s2c-shadow
        }
        'c' {
            & $run -Arm c-admit2 -Admit 2 -Build build-p2p8-s2c-creation
        }
        'v' {
            & $run -Arm v1-mfly-admit0 -Admit 0 -Build build-p2p8-s2c-roster-mfly
            & $run -Arm v1-mfly-admit2 -Admit 2 -IdCensus 1 -Build build-p2p8-s2c-roster-mfly
            & $run -Arm v2-cppn-admit0 -Admit 0 -Build build-p2p8-s2c-roster-cppn
            & $run -Arm v2-cppn-admit2 -Admit 2 -IdCensus 1 -Build build-p2p8-s2c-roster-cppn
        }
    }
    "STEP-DONE $step $(Get-Date -Format HH:mm:ss)"
}
