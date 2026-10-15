# AttackHi4: up smash, one strong overhead swing (c04attackhi4). Ultimate's
# frames (ultimateframedata, its frame N our N - 1) and the owner's Ultimate
# attack data (ike_attack_data.json: damage, angle, growth, set and base
# knockback; owner p6r): 17% at 24-28 (ultimateframedata's; 85, growth 88,
# base 45 as the late hit's arc), 10% late at 29-30 (the data's: 65, growth
# 85, base 50).
SetSlopeContour flags=3
AsyncWait frames=10
PlaySmashVoice value=0
AsyncWait frames=16
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=24
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=17 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=85 knockback_scale=88 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=45
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=17 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=85 knockback_scale=88 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=45
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=17 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=85 knockback_scale=88 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=45
PlayFGMStoreInfo value=258
SyncWait frames=2
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=350 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=10 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=65 knockback_scale=85 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=10 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=65 knockback_scale=85 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=10 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=65 knockback_scale=85 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=50
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=44
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
