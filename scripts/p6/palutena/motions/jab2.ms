# Attack12: jab 2 (card: the light burst's finisher, the staff swung up): Marth's jab 2
AsyncWait frames=4
PlayVoiceStoreInfo value=cue:attack01
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=200 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=4 can_rebound=1 element=0 size=170 off_x=11 off_y=75 off_z=-80 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=4 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=25
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_swing_m
SyncWait frames=6
ClearAttackCollAll value=0
End value=0
