# AttackHi4: up smash, Ultimate's (c04attackhi4): the staff raised, ONE column
# of light ahead of her (owner playtest 2026-10-14: an effect her code makes
# at flag0, 325 ahead on her floor; attacks 0 and 1 its lower and upper
# halves, 400 wide, moved between their ends every frame by nds_p4_palutena.c
# PAL_PILLAR_COLUMN; 2 and 3 at their middles for the first frame), up 26
# frames (12-37). Ultimate's damage by height and time (ultimateframedata):
# early (12-28) 16% low, 12% high; late (29-37) 13% and 9%; angle 88
# (Ultimate's attack data (the owner's palutena_attack_data.json, p6r)); the
# knockback the owner's (p6h: 1.5x), then 25% less (owner p7h, 2026-10-19:
# growth 158/150 -> 118/112, base 60 -> 45, each x0.75 rounded half to even).
# Owner p7h follow-up (2026-10-19): growth 100 and base 40 on both halves
# (12-28), the late boxes (29-37) 0.75x of those (75, 30); the boxes back
# to 274 ahead in script units, which her model size (attr->size 1.185)
# makes 325 in game, the pillar effect's own distance (PAL_PILLAR_AHEAD,
# world units): at 325 they stood 385 ahead, past the pillar's middle.
# Owner (2026-10-19, after play-p7k): growth 110 early and 85 late, the
# late base 35; the stronger lower half (attacks 0 and 2) hits with the
# forward tilt's sound (a heavy punch, level 2).
# Owner (2026-10-19): the column as tall as the light pillar plus 6. The
# pillar draws to about 2,048 above her floor (measured: its 2,500-tall
# cards stop at the DS's v16 vertex range, 2,047.9 at the x16 factor), so
# attack 1's sweep tops out at 1,565 (x her size 1.185 = 1,854, its 200
# radius above: 2,054; it was 2,300, 2,925); the halves meet at 883,
# attacks 2 and 3 at their middles (542, 1,224); nds_p4_palutena.c's
# column with it.
# Owner 2026-10-22 (the 3DS's in the owner's quarter-speed clip): the
# sparkle at the staff's gem as she swings it up (frame 4, the 3DS's burst
# 8 frames before the column), her gem's glow at the thrust
# (nds_p4_palutena.c sNdsP4PalutenaGlowWindows).
SetFlag0 value=0
AsyncWait frames=4
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=11
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlaySmashVoice value=0
AsyncWait frames=12
SetFlag0 value=1
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=400 off_x=0 off_y=200 off_z=274 angle=88 knockback_scale=110 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=12 can_rebound=1 element=0 size=400 off_x=0 off_y=883 off_z=274 angle=88 knockback_scale=110 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=400 off_x=0 off_y=542 off_z=274 angle=88 knockback_scale=110 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=12 can_rebound=1 element=0 size=400 off_x=0 off_y=1224 off_z=274 angle=88 knockback_scale=110 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
PlayFGM value=cue:se_smash_h01
SyncWait frames=17
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=13 can_rebound=1 element=0 size=400 off_x=0 off_y=200 off_z=274 angle=88 knockback_scale=85 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=35
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=9 can_rebound=1 element=0 size=400 off_x=0 off_y=883 off_z=274 angle=88 knockback_scale=85 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=35
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=13 can_rebound=1 element=0 size=400 off_x=0 off_y=542 off_z=274 angle=88 knockback_scale=85 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=35
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=9 can_rebound=1 element=0 size=400 off_x=0 off_y=1224 off_z=274 angle=88 knockback_scale=85 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=35
SyncWait frames=9
ClearAttackCollAll value=0
End value=0
