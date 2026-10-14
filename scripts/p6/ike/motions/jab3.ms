# Ike's jab 3 (an appended Attack13): an upward slash, ~6%, launches (move
# card I1). Marth's second-jab timing on its clip: the sword (joint id 18,
# tip and base) and the right forearm (16) from frame 4 to 10.
SetSlopeContour flags=3
AsyncWait frames=2
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=200 off_x=0 off_y=0 off_z=250 angle=80 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=60 angle=80 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=5 can_rebound=1 element=3 size=160 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=45
PlayFGMStoreInfo value=259
SyncWait frames=6
ClearAttackCollAll value=0
AsyncWait frames=20
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
