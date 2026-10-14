# Counter / Reflect (card: Marth's numbers): the window (flag0) from frame
# 6 to 28 counters a hit and reflects projectiles (nds_p4_palutena.c).
AsyncWait frames=4
PlayVoiceStoreInfo value=cue:special_l01
AsyncWait frames=6
Effect joint_id=9 effect_id=71 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SetColAnim colanim_id=97 length=22
PlayFGM value=cue:se_special_l01
SetFlag0 value=1
SyncWait frames=22
SetFlag0 value=0
End value=0
