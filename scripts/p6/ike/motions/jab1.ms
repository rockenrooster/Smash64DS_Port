# Ike's jab 1 (Attack11): a left jab, ~3% (move card I1). Captain's jab 1
# timing on Captain's retargeted clip; the hitboxes on the left forearm and
# upper arm (joint ids 9 and 8, rows 5 and 4) with offsets along Roy's longer
# forearm (the fist at 140).
SetSlopeContour flags=3
AsyncWait frames=5
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=3 can_rebound=1 element=0 size=180 off_x=140 off_y=0 off_z=0 angle=361 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=8 damage=3 can_rebound=1 element=0 size=180 off_x=50 off_y=0 off_z=0 angle=361 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=8 damage=3 can_rebound=1 element=0 size=180 off_x=-50 off_y=0 off_z=0 angle=361 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=0
PlayFGMStoreInfo value=43
SyncWait frames=4
ClearAttackCollAll value=0
AsyncWait frames=9
SetFlag1 value=1
End value=0
