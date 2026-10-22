param([Parameter(Mandatory)][string]$Build, [string]$Seed = '', [string[]]$Extra = @())
# THE gate ROM (builds/<Build>): the four-CPU sweep target in the ship layout
# (every content, P5, the 1P game, the menu shell and UI kit, ship telemetry,
# the freeze report) with the sweep's ITCM evictions. A bare sweep ROM has a
# ~979 KB arena against the ship's ~641 KB and is not a gate (memory
# sweep-must-use-ship-layout). Names so far: build-sweep-sNN.
. "$PSScriptRoot\common.ps1"
& "$PSScriptRoot\build_sweep_rom.ps1" -Build $Build -Seed $Seed -EvictSyms $S64SweepEvictions `
    -Extra ($S64ShipFlags + $Extra)
$cfg = Join-Path $S64Repo "builds\$Build\nds_build_config.h"
foreach ($k in 'NDS_P2_1P_GAME', 'NDS_P2_MENU_SHELL', 'NDS_P4', 'NDS_P5') {
    $line = Select-String -Path $cfg -Pattern "#define $k " | Select-Object -First 1
    "$k -> $($line.Line)"
}
