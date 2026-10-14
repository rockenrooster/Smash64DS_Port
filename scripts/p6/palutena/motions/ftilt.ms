# AttackS3: forward tilt (card: the staff spun ahead). Owner p6i: as the 3DS's
# (art/p6/palutena/anims/ftilt.json, the owner's reference at 30 fps): a twirl,
# then the staff twirled at arm's length like a baton: three hits (frames 9,
# 16, 23; each re-made so it can hit again) and a launching last (30), 9%.
AsyncWait frames=9
PlayVoiceStoreInfo value=cue:attack02
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=240 off_x=23 off_y=150 off_z=-160 angle=70 knockback_scale=40 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=240 off_x=-11 off_y=-75 off_z=80 angle=70 knockback_scale=40 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=1 element=0 size=170 off_x=0 off_y=0 off_z=0 angle=70 knockback_scale=40 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=30
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=16
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=240 off_x=23 off_y=150 off_z=-160 angle=70 knockback_scale=40 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=240 off_x=-11 off_y=-75 off_z=80 angle=70 knockback_scale=40 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=1 element=0 size=170 off_x=0 off_y=0 off_z=0 angle=70 knockback_scale=40 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=30
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=23
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=240 off_x=23 off_y=150 off_z=-160 angle=70 knockback_scale=40 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=240 off_x=-11 off_y=-75 off_z=80 angle=70 knockback_scale=40 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=1 element=0 size=170 off_x=0 off_y=0 off_z=0 angle=70 knockback_scale=40 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=30
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=30
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=3 can_rebound=1 element=0 size=270 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=120 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=3 can_rebound=1 element=0 size=270 off_x=-11 off_y=-75 off_z=80 angle=361 knockback_scale=120 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=3 can_rebound=1 element=0 size=190 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=120 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
ClearAttackCollAll value=0
End value=0
