# AttackAirHi: up air, the arc over his head (c05attackairhi). Ultimate's
# frames (ultimateframedata, its frame N our N - 1) and the owner's Ultimate
# attack data (ike_attack_data.json: damage, angle, growth, set and base
# knockback; owner p6r): 11%, 80, growth 94, base 55, at 12-17; landing lag
# (flag1) from 5, none from 50 (autocancels).
AsyncWait frames=5
SetFlag1 value=40
AsyncWait frames=8
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=12
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=11 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=80 knockback_scale=94 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=55
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=11 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=80 knockback_scale=94 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=55
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=11 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=94 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=55
PlayFGMStoreInfo value=259
SyncWait frames=6
ClearAttackCollAll value=0
AsyncWait frames=32
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=50
SetFlag1 value=0
End value=0
