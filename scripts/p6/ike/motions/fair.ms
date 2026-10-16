# AttackAirF: forward air, the overhead arc in front (c05attackairf).
# Ultimate's frames (ultimateframedata, its frame N our N - 1) and the owner's
# Ultimate attack data (ike_attack_data.json: damage, angle, growth, set and
# base knockback; owner p6r): 11.5% (12), 30, growth 65, base 55, at 10-14;
# landing lag (flag1) to 41, none from 41 (autocancels).
SetFlag1 value=40
AsyncWait frames=6
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=10
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=12 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=525 angle=30 knockback_scale=65 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=55
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=12 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=250 angle=30 knockback_scale=65 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=55
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=12 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=30 knockback_scale=65 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=55
PlayFGMStoreInfo value=259
SyncWait frames=5
ClearAttackCollAll value=0
AsyncWait frames=26
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=41
SetFlag1 value=0
End value=0
