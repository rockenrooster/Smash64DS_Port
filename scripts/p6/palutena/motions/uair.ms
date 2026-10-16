# AttackAirHi: up air, Ultimate's (c05attackairhi): the twirl, the staff
# raised over her and her halo (the head row's part 2) through the hits:
# Ultimate's multi-hit, five light hits (5, 7, 9, 11, 13; each re-made: 1%,
# its pull, Ultimate's 367, as 90 with a light set knockback here) and a
# launching last at 14-15 (5%, 88, growth 152, base 50; Ultimate's attack data
# (the owner's palutena_attack_data.json, p6r)).
# Owner p6w: its sound once (it played at each of five light hits and the
# last), from 4 so its shimmer peaks at the last hit (14); its hitboxes
# 1.15x the size.
SetModelPartID joint_id=12 modelpart_id=2
AsyncWait frames=4
PlayFGM value=cue:se_attackair_h01
AsyncWait frames=5
SetFlag1 value=50
PlayVoiceStoreInfo value=cue:attack06
Effect joint_id=0 effect_id=42 flag=0 off_x=0 off_y=680 off_z=0 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=1 can_rebound=1 element=0 size=368 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=1
ClearAttackCollAll value=0
AsyncWait frames=7
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=1 can_rebound=1 element=0 size=368 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=1
ClearAttackCollAll value=0
AsyncWait frames=9
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=1 can_rebound=1 element=0 size=368 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=1
ClearAttackCollAll value=0
AsyncWait frames=11
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=1 can_rebound=1 element=0 size=368 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=1
ClearAttackCollAll value=0
AsyncWait frames=13
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=1 can_rebound=1 element=0 size=368 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
SyncWait frames=1
ClearAttackCollAll value=0
AsyncWait frames=14
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=322 off_x=41 off_y=272 off_z=-290 angle=88 knockback_scale=152 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=50
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=276 off_x=23 off_y=150 off_z=-160 angle=88 knockback_scale=152 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=50
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=5 can_rebound=1 element=0 size=391 off_x=0 off_y=680 off_z=0 angle=88 knockback_scale=152 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=50
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=32
SetFlag1 value=0
SetModelPartID joint_id=12 modelpart_id=0
End value=0
