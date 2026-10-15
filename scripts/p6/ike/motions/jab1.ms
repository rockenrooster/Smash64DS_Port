# Attack11: jab 1, the left punch (c00attack11 frame for frame). Ultimate's
# frames (ultimateframedata, its frame N our N - 1) and the owner's Ultimate
# attack data (ike_attack_data.json: damage, angle, growth, set and base
# knockback; owner p6r): 2.5% (3), 361, growth 12, base 25, at 3-4; jab 2 may
# follow from 8. On the left forearm and upper arm (joints 9 and 8, the fist
# at 140).
SetSlopeContour flags=3
AsyncWait frames=3
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=3 can_rebound=1 element=0 size=180 off_x=140 off_y=0 off_z=0 angle=361 knockback_scale=12 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=1 group_id=0 joint_id=8 damage=3 can_rebound=1 element=0 size=180 off_x=50 off_y=0 off_z=0 angle=361 knockback_scale=12 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=2 group_id=0 joint_id=8 damage=3 can_rebound=1 element=0 size=180 off_x=-50 off_y=0 off_z=0 angle=361 knockback_scale=12 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=25
PlayFGMStoreInfo value=43
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=8
SetFlag1 value=1
End value=0
