# AttackAirN: neutral air, Ultimate's (c05attackairn): the staff whirled round
# her, its hits Ultimate's: five light ones (5, 8, 11, 14, 17; each re-made:
# Ultimate's 1%, 100, its set knockback 65 a light 10 here so they stay in)
# and a launching last (21-22: 5% from ultimateframedata, 55, growth 180, base
# 30, Ultimate's attack data (the owner's palutena_attack_data.json, p6r)).
# Her whirl's sound once, from frame 1 (owner p6v: it played at every light
# hit, five over each other; the one sound's swooshes peak at 6, 15 and 21).
# Owner p6x VFX plan (picks 2026-10-16): her staff's solid cyan-white trail
# (sword trail 24, spec sword_trails: row 14's grip-to-gem line) while it
# spins, 4 to 24.
AsyncWait frames=1
PlayFGM value=cue:se_attackair_n01
AsyncWait frames=4
SetAfterImage is_itemswing=24 drawstatus=0
AsyncWait frames=5
SetFlag1 value=40
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=200 off_x=41 off_y=272 off_z=-290 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=170 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=8
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=200 off_x=41 off_y=272 off_z=-290 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=170 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=11
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=200 off_x=41 off_y=272 off_z=-290 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=170 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=14
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=200 off_x=41 off_y=272 off_z=-290 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=170 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=17
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=200 off_x=41 off_y=272 off_z=-290 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=170 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=21
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=220 off_x=41 off_y=272 off_z=-290 angle=55 knockback_scale=180 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=190 off_x=23 off_y=150 off_z=-160 angle=55 knockback_scale=180 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=5 can_rebound=1 element=0 size=160 off_x=0 off_y=0 off_z=0 angle=55 knockback_scale=180 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
PlayFGMStoreInfo value=cue:se_swing_m
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=24
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=30
SetFlag1 value=0
End value=0
