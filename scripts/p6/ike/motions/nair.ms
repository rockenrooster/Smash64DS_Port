# AttackAirN: neutral air, the spin round him (c05attackairn). Ultimate's
# frames (ultimateframedata, its frame N our N - 1) and the owner's Ultimate
# attack data (ike_attack_data.json: damage, angle, growth, set and base
# knockback; owner p6r): 7.5% (8) at 9-13, 6% late at 14-21
# (ultimateframedata's), 80, growth 105, base 35; landing lag (flag1) from 2,
# none from 49 (autocancels).
AsyncWait frames=2
SetFlag1 value=40
AsyncWait frames=7
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=9
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=8 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=80 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=35
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=8 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=80 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=35
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=8 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=35
MakeAttackColl attack_id=3 group_id=0 joint_id=6 damage=8 can_rebound=1 element=0 size=220 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=35
PlayFGMStoreInfo value=260
SyncWait frames=5
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=80 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=35
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=80 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=35
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=6 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=35
MakeAttackColl attack_id=3 group_id=0 joint_id=6 damage=6 can_rebound=1 element=0 size=220 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=35
SyncWait frames=8
ClearAttackCollAll value=0
AsyncWait frames=30
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=49
SetFlag1 value=0
End value=0
