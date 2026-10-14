# Flare Blade's start (Roy's down special, Ike's Eruption; Roy's script at
# 0x80587150 plays his charge sound at frame 11). Owner playtest 2026-10-14
# (p6d): Ike's charge, Ultimate's four-second se_ike_special_N01, played as
# a one-shot ran on after the move was cancelled, interrupted or let go
# early. Here it is the fighter's loop sound, which a new status stops:
# nds_p4_roy.c carries it into the charge's wait, whose own loop sound then
# finds it playing and starts no second one.
HideItem value=0
AsyncWait frames=11
PlayLoopSFXStoreInfo value=cue:se_eruption_charge
End value=0
