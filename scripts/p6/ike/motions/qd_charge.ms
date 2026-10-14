# Quick Draw's charge: the stance while B is held (nds_p4_ike.c counts it).
# Owner playtest 2026-10-14 (the charge had no effect or sound): his blue
# charge glow (Roy's charge colour animation 126, which Ike's remap draws
# blue), the charge's sounds and a twinkle on the blade every ten frames.
Effect joint_id=0 effect_id=11 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SetColAnim colanim_id=126 length=0
PlayFGM value=cue:se_quickdraw_charge
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=300 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=8
PlayFGM value=cue:se_quickdraw_charge2
LoopBegin count=5
SyncWait frames=10
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=300 rng_x=0 rng_y=0 rng_z=0
LoopEnd value=0
End value=0
