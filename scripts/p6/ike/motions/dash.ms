# AttackDash: the rising slash (c00attackdash), its travel the TransN slot's.
# Ultimate's frames (ultimateframedata, its frame N our N - 1) and the owner's
# Ultimate attack data (ike_attack_data.json: damage, angle, growth, set and
# base knockback; owner p6r): 14% at 14-15 (ultimateframedata's), 9% late at
# 16-18 (the data's), 45, growth 100, base 60.
Effect joint_id=0 effect_id=16 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=10
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=14
PlayFGMStoreInfo value=258
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=14 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=60
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=14 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=60
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=14 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=60
SyncWait frames=1
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=175 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=1
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=9 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=60
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=9 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=60
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=9 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=60
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=32
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
