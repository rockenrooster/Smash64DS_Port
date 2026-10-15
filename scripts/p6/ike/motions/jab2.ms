# Attack12: jab 2, the forward kick with his right foot (c00attack12).
# Ultimate's frames (ultimateframedata, its frame N our N - 1) and the owner's
# Ultimate attack data (ike_attack_data.json: damage, angle, growth, set and
# base knockback; owner p6r): 2.5% (3), 361, growth 15, base 30, at 2-3; jab 3
# may follow from 8. On his right foot, shin and thigh (joints 33, 32, 31:
# rows 29, 28, 27; the hits were on his standing left shin before).
SetSlopeContour flags=2
AsyncWait frames=2
PlayFGMStoreInfo value=42
MakeAttackColl attack_id=0 group_id=0 joint_id=33 damage=3 can_rebound=1 element=0 size=220 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=15 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=1 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=32 damage=3 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=15 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=1 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=31 damage=3 can_rebound=1 element=0 size=160 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=15 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=1 knockback_base=30
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=8
SetFlag1 value=1
End value=0
