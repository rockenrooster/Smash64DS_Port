# The counterattack (d03speciallwhit): intangible to 3, its hit at 3-4.
# Ultimate's frames (ultimateframedata, its frame N our N - 1) and the owner's
# Ultimate attack data (ike_attack_data.json: damage, angle, growth, set and
# base knockback; owner p6r): 10%, 361, growth 100, base 48.
SetHitStatusAll value=2
AsyncWait frames=2
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=3
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=350 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=10 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=48
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=10 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=48
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=10 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=48
MakeAttackColl attack_id=3 group_id=0 joint_id=6 damage=10 can_rebound=1 element=3 size=160 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=48
PlayFGMStoreInfo value=259
PlayVoiceStoreInfo value=cue:special_l01
SyncWait frames=1
SetHitStatusAll value=1
SyncWait frames=1
ClearAttackCollAll value=0
AsyncWait frames=20
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
