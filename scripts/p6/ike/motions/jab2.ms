# Ike's jab 2 (Attack12): a front kick, ~3% (move card I1). Captain's
# forward-tilt kick at 0.65 of its length: his hit window (frames 9-15) lands
# at 6-10; the hitboxes along the left shin and on the thigh (joint ids 27
# and 26, rows 23 and 22).
SetSlopeContour flags=2
AsyncWait frames=6
PlayFGMStoreInfo value=42
MakeAttackColl attack_id=0 group_id=0 joint_id=27 damage=3 can_rebound=1 element=0 size=220 off_x=200 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=1 knockback_base=6
MakeAttackColl attack_id=1 group_id=0 joint_id=27 damage=3 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=1 knockback_base=6
MakeAttackColl attack_id=2 group_id=0 joint_id=26 damage=3 can_rebound=1 element=0 size=140 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=1 knockback_base=6
SyncWait frames=5
ClearAttackCollAll value=0
AsyncWait frames=13
SetFlag1 value=1
End value=0
