# The counter's strike: Marth's timing and damage (8), intangible while it
# swings.
SetHitStatusAll value=2
AsyncWait frames=3
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=4
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=350 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=8 can_rebound=1 element=3 size=200 off_x=0 off_y=0 off_z=265 angle=361 knockback_scale=35 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=8 can_rebound=1 element=3 size=200 off_x=0 off_y=0 off_z=70 angle=361 knockback_scale=35 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=8 can_rebound=1 element=3 size=200 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=35 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
MakeAttackColl attack_id=3 group_id=0 joint_id=6 damage=8 can_rebound=1 element=3 size=160 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=35 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
PlayFGMStoreInfo value=259
PlayVoiceStoreInfo value=cue:special_l01
SyncWait frames=8
SetHitStatusAll value=1
ClearAttackCollAll value=0
SyncWait frames=2
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
