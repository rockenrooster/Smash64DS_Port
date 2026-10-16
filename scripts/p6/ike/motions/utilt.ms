# AttackHi3: up tilt, Ragnell swept up over him (c02attackhi3). Ultimate's
# frames (ultimateframedata, its frame N our N - 1) and the owner's Ultimate
# attack data (ike_attack_data.json: damage, angle, growth, set and base
# knockback; owner p6r): 12% at 10-15 (ultimateframedata's), 8% late at 16-20
# (the data's), 85, growth 90, base 50.
SetSlopeContour flags=3
AsyncWait frames=7
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=10
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=12 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=525 angle=85 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=12 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=250 angle=85 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=12 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=85 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
PlayFGMStoreInfo value=259
SyncWait frames=6
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=8 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=525 angle=85 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=8 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=250 angle=85 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=8 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=85 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=50
SyncWait frames=5
ClearAttackCollAll value=0
AsyncWait frames=30
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
