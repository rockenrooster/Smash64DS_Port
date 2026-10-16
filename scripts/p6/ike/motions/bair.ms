# AttackAirB: back air, the thrust behind him (c05attackairb). Ultimate's
# frames (ultimateframedata, its frame N our N - 1) and the owner's Ultimate
# attack data (ike_attack_data.json: damage, angle, growth, set and base
# knockback; owner p6r): 14%, 361, growth 100, base 30, at 6-8; landing lag
# (flag1) from 2, none from 34 (autocancels).
AsyncWait frames=2
SetFlag1 value=40
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=6
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=14 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=525 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=14 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=250 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=14 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=30
PlayFGMStoreInfo value=259
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=22
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=34
SetFlag1 value=0
End value=0
