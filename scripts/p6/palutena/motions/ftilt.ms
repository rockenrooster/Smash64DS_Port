# AttackS3: forward tilt (card: the staff spun ahead): Marth's forward tilt
AsyncWait frames=7
PlayVoiceStoreInfo value=cue:attack02
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=9 can_rebound=1 element=0 size=200 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=15
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=8 can_rebound=1 element=0 size=170 off_x=11 off_y=75 off_z=-80 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=15
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=8 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=15
PlayFGMStoreInfo value=cue:se_attackhard_s01
SyncWait frames=2
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
End value=0
