# AttackAirLw: down air, the downward arc under him (c05attackairlw).
# Ultimate's frames (ultimateframedata, its frame N our N - 1) and the owner's
# Ultimate attack data (ike_attack_data.json: damage, angle, growth, set and
# base knockback; owner p6r): 15%, 270 (a meteor), growth 90, base 20, at
# 15-16; landing lag (flag1) from 4, none from 47 (autocancels).
AsyncWait frames=4
SetFlag1 value=40
AsyncWait frames=9
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=15
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=15 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=270 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=20
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=15 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=270 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=20
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=15 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=270 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=20
PlayFGMStoreInfo value=258
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=30
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=47
SetFlag1 value=0
End value=0
