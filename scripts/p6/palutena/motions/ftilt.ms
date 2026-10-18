# AttackS3: forward tilt, Ultimate's (c01attacks3): the staff twirled ahead at
# arm's length, one hit over two windows as Ultimate's (its 14-23 and 24-35):
# 6% at 9-19, 7% at 20-31 (re-made live, so it hits once). Ultimate's attack
# data (the owner's palutena_attack_data.json, p6r): 40, growth 80, base 78.
# Owner p6w: its hit sound the forward air's (a heavy punch, level 2) and its
# hitboxes 1.15x the size.
# Owner p6x VFX plan (picks 2026-10-16): her staff's solid cyan-white trail
# (sword trail 24, spec sword_trails: row 14's grip-to-gem line) while it
# spins, 8 to 33.
AsyncWait frames=8
SetAfterImage is_itemswing=24 drawstatus=0
AsyncWait frames=9
PlayVoiceStoreInfo value=cue:attack02
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=0 size=276 off_x=41 off_y=272 off_z=-290 angle=40 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=78
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=1 element=0 size=276 off_x=23 off_y=150 off_z=-160 angle=40 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=78
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=6 can_rebound=1 element=0 size=195 off_x=0 off_y=0 off_z=0 angle=40 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=78
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=20
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=7 can_rebound=1 element=0 size=276 off_x=41 off_y=272 off_z=-290 angle=40 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=78
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=7 can_rebound=1 element=0 size=276 off_x=23 off_y=150 off_z=-160 angle=40 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=78
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=7 can_rebound=1 element=0 size=195 off_x=0 off_y=0 off_z=0 angle=40 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=78
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=32
ClearAttackCollAll value=0
AsyncWait frames=33
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
