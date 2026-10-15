# Attack13: jab 3, Ragnell swung up over him (c00attack13). Ultimate's frames
# (ultimateframedata, its frame N our N - 1) and the owner's Ultimate attack
# data (ike_attack_data.json: damage, angle, growth, set and base knockback;
# owner p6r): 5%, 45, growth 100, base 60, at 4-7, the hit over his head at 4
# only.
SetSlopeContour flags=3
AsyncWait frames=2
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=3
PlayFGMStoreInfo value=259
AsyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=5 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=60
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=5 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=60
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=5 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=60
MakeAttackColl attack_id=3 group_id=0 joint_id=6 damage=5 can_rebound=1 element=3 size=200 off_x=0 off_y=520 off_z=0 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=60
SyncWait frames=1
ClearAttackCollID value=3
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=16
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
