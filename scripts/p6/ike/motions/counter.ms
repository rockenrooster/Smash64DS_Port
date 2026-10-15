# Counter (d03speciallw): its window 8-32 (Ultimate's 9-33; flag0 for
# nds_p4_ike.c).
AsyncWait frames=8
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=350 rng_x=0 rng_y=0 rng_z=0
SetColAnim colanim_id=97 length=25
PlayFGM value=cue:se_counter
SetFlag0 value=1
SyncWait frames=25
SetFlag0 value=0
End value=0
