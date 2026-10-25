param([Parameter(Mandatory)][string]$Build, [string]$Seed = '', [int]$Minutes = 5, [string[]]$Extra = @())
# The soak ROM (builds/<Build>): the gate ROM with matches of $Minutes minutes
# (NDS_SOAK_MATCH_MINUTES), for the four-player all-items soaks
# (audit_soak.ps1). Its tick HUD costs ~24.8 KB of arena against the play ROM,
# so its heap low-water reads that much below what ships. Names so far:
# build-soak-sNN.
. "$PSScriptRoot\common.ps1"
& "$PSScriptRoot\build_gate_rom.ps1" -Build $Build -Seed $Seed `
    -Extra (@("NDS_SOAK_MATCH_MINUTES=$Minutes") + $Extra)
