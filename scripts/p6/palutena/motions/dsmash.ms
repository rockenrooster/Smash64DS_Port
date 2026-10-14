# AttackLw4: down smash (card: the wings down both sides; for now the staff sweeps both sides): Marth's down smash
AsyncWait frames=6
Effect joint_id=0 effect_id=16 flag=0 off_x=0 off_y=0 off_z=-100 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=13 can_rebound=1 element=0 size=200 off_x=23 off_y=150 off_z=-160 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=11 can_rebound=1 element=0 size=170 off_x=11 off_y=75 off_z=-80 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=11 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
PlayFGMStoreInfo value=cue:se_smash_l01
PlaySmashVoice value=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=19
Effect joint_id=0 effect_id=15 flag=0 off_x=0 off_y=0 off_z=350 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=13 can_rebound=1 element=0 size=200 off_x=23 off_y=150 off_z=-160 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=11 can_rebound=1 element=0 size=170 off_x=11 off_y=75 off_z=-80 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=11 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
PlayFGMStoreInfo value=cue:se_swing_l
SyncWait frames=4
ClearAttackCollAll value=0
End value=0
