# AttackAirB: back air (card: a backfist with the shield arm; the owner's
# 3DS reference, 2026-10-14): the shield bashed out behind her, its hitbox
# at the shield's middle (joint 9, 94 along the forearm).
AsyncWait frames=7
SetFlag1 value=100
PlayVoiceStoreInfo value=cue:attack05
MakeAttackColl attack_id=0 group_id=0 joint_id=8 damage=13 can_rebound=1 element=0 size=280 off_x=180 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=15
MakeAttackColl attack_id=1 group_id=0 joint_id=9 damage=13 can_rebound=1 element=0 size=280 off_x=94 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=15
Effect joint_id=9 effect_id=31 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_attackair_b01
SyncWait frames=2
MakeAttackColl attack_id=0 group_id=0 joint_id=8 damage=10 can_rebound=1 element=0 size=250 off_x=150 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=9 damage=10 can_rebound=1 element=0 size=240 off_x=94 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=10
SyncWait frames=10
ClearAttackCollAll value=0
SyncWait frames=10
SetFlag1 value=0
End value=0
