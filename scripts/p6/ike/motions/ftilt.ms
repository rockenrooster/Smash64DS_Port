# AttackS3: forward tilt, the horizontal cleave (c01attacks3s). Ultimate's
# frames (ultimateframedata, its frame N our N - 1) and the owner's Ultimate
# attack data (ike_attack_data.json: damage, angle, growth, set and base
# knockback; owner p6r): 13.5% (14), 361, growth 97, base 30, at 11-12. Its
# hits ahead of him at TopN offsets, as Ultimate's (its top bone): the blade
# sweeps round through his side at those frames and lies ahead at 13.
SetSlopeContour flags=3
AsyncWait frames=5
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=11
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=14 can_rebound=1 element=3 size=300 off_x=0 off_y=330 off_z=650 angle=361 knockback_scale=97 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=14 can_rebound=1 element=3 size=300 off_x=0 off_y=330 off_z=360 angle=361 knockback_scale=97 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=14 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=97 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=30
PlayFGMStoreInfo value=259
SyncWait frames=1
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=175 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=1
ClearAttackCollAll value=0
AsyncWait frames=28
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
