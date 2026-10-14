# AttackDash: the shield charge (D3), on Peach's dash attack
SetSlopeContour flags=4
AsyncWait frames=5
PlayVoiceStoreInfo value=cue:attack02
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=10 can_rebound=1 element=0 size=280 off_x=0 off_y=0 off_z=0 angle=70 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=10 can_rebound=1 element=0 size=300 off_x=0 off_y=120 off_z=140 angle=70 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
Effect joint_id=9 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_attackdash
SyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=7 can_rebound=1 element=0 size=260 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=20
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=7 can_rebound=1 element=0 size=280 off_x=0 off_y=120 off_z=140 angle=361 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=20
SyncWait frames=10
ClearAttackCollAll value=0
SetSlopeContour flags=0
End value=0
