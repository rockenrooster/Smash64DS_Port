# AttackLw3: down tilt, the low sweep from his crouch (c02attacklw3).
# Ultimate's frames (ultimateframedata, its frame N our N - 1) and the owner's
# Ultimate attack data (ike_attack_data.json: damage, angle, growth, set and
# base knockback; owner p6r): 8%, 80, growth 70, base 70, at 6-7.
SetSlopeContour flags=3
AsyncWait frames=3
SetSlopeContour flags=4
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=6
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=8 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=80 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=8 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=80 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=8 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
PlayFGMStoreInfo value=259
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=18
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=24
SetSlopeContour flags=3
End value=0
