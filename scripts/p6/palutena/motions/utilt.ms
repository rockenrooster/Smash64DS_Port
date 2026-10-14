# AttackHi3: up tilt (card: the staff spun overhead). Owner p6i: as the 3DS's
# (art/p6/palutena/anims/utilt.json, the owner's reference at 30 fps): on her
# knees the staff spins overhead with four small hits (frames 6, 12, 18, 24;
# each re-made so it can hit again) and a launching last (30), 8% in all.
AsyncWait frames=6
PlayVoiceStoreInfo value=cue:attack03
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=230 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=230 off_x=-11 off_y=-75 off_z=80 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=170 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=12
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=230 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=230 off_x=-11 off_y=-75 off_z=80 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=170 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=18
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=230 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=230 off_x=-11 off_y=-75 off_z=80 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=170 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=24
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=230 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=230 off_x=-11 off_y=-75 off_z=80 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=170 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=30
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=4 can_rebound=1 element=0 size=260 off_x=23 off_y=150 off_z=-160 angle=88 knockback_scale=110 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=45
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=4 can_rebound=1 element=0 size=260 off_x=-11 off_y=-75 off_z=80 angle=88 knockback_scale=110 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=45
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=4 can_rebound=1 element=0 size=190 off_x=0 off_y=0 off_z=0 angle=88 knockback_scale=110 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=45
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
ClearAttackCollAll value=0
End value=0
