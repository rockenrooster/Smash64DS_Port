# AttackAirF: forward air (card: a flying kick): Mario's neutral air on Peach's tree
AsyncWait frames=3
SetFlag1 value=50
PlayVoiceStoreInfo value=cue:attack04
MakeAttackColl attack_id=0 group_id=0 joint_id=26 damage=10 can_rebound=1 element=0 size=240 off_x=10 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=1 knockback_base=15
MakeAttackColl attack_id=1 group_id=0 joint_id=21 damage=10 can_rebound=1 element=0 size=240 off_x=10 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=1 knockback_base=15
MakeAttackColl attack_id=2 group_id=0 joint_id=5 damage=10 can_rebound=1 element=0 size=260 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=1 knockback_base=15
PlayFGMStoreInfo value=cue:se_attackair_f01
SyncWait frames=8
MakeAttackColl attack_id=0 group_id=0 joint_id=26 damage=7 can_rebound=1 element=0 size=240 off_x=10 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=1 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=21 damage=7 can_rebound=1 element=0 size=240 off_x=10 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=1 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=5 damage=7 can_rebound=1 element=0 size=260 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=1 knockback_base=0
SyncWait frames=14
SetFlag1 value=0
ClearAttackCollAll value=0
End value=0
