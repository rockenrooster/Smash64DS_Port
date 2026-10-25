# Counter / Reflect (card: Marth's numbers): the window (flag0) counters a hit
# and reflects projectiles (nds_p4_palutena.c), from frame 1 through 40 (owner
# 2026-10-23; it was 6 to 28), its glow (the colour animation) over it. The
# shield's flash and its sound stay at the raise (6). Its voice at 4 is the
# code's (nds_p4_palutena.c PAL_COUNTER_VOICE_*): "counter", an effort line
# or nothing, at random (owner 2026-10-25, the 3DS's).
AsyncWait frames=1
SetFlag0 value=1
SetColAnim colanim_id=97 length=40
AsyncWait frames=6
Effect joint_id=9 effect_id=71 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayFGM value=cue:se_special_l01
AsyncWait frames=41
SetFlag0 value=0
End value=0
