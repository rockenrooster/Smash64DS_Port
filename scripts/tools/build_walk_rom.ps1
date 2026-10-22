param([Parameter(Mandatory)][string]$Build, [string]$Seed = '', [string[]]$Extra = @())
# The walk ROM (builds/<Build>/smash64ds.nds): the play ROM plus the menu walk
# (NDS_P2_MENU_WALK=1), which drives the real menus -- Mode Select, the 1P,
# bonus, Training and VS selects, the stage select, battles, Results -- for
# the audit drivers (audit_campaign/bonus/training/vswalk.ps1). The ship
# layout otherwise. Names so far: walk-uNN.
. "$PSScriptRoot\common.ps1"
& "$PSScriptRoot\build_play_rom.ps1" -Build $Build -Seed $Seed -Extra (@('NDS_P2_MENU_WALK=1') + $Extra)
