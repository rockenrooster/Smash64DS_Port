# Ike's jab 3 (an appended Attack13): a downward swing, ~6%, launches
# (owner playtest 2026-10-14; move card I1 had an upward slash). Marth's
# forward smash spliced (art/p6/ike/anims/jab3.json): the sword overhead at
# frame 3, down by 5; the sword (joint id 18, tip and base) and the right
# forearm (16) from frame 4 to 7.
SetSlopeContour flags=3
AsyncWait frames=2
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=3
PlayFGMStoreInfo value=259
AsyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=250 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=200 off_x=0 off_y=0 off_z=60 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=5 can_rebound=1 element=3 size=160 off_x=0 off_y=0 off_z=0 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=12
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
