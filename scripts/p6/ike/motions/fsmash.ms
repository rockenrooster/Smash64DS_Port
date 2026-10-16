# AttackS4: forward smash, the overhead slam (c03attacks4s), in place (owner
# p6r: "he also lunges too far forward"; Ultimate's step in and back is on the
# body). Ultimate's frames (ultimateframedata, its frame N our N - 1) and the
# owner's Ultimate attack data (ike_attack_data.json: damage, angle, growth,
# set and base knockback; owner p6r): 19% as the blade comes over at 30-31,
# 22% as it slams down at 32-34 (ultimateframedata's), 57, growth 86, base 40;
# Ragnell then flat on the floor.
AsyncWait frames=8
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=14
SetTexturePartID texturepart_id=1 frame=6
PlaySmashVoice value=0
AsyncWait frames=30
SetSlopeContour flags=4
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=19 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=525 angle=57 knockback_scale=86 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=19 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=250 angle=57 knockback_scale=86 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=19 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=57 knockback_scale=86 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
PlayFGMStoreInfo value=258
SyncWait frames=2
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=22 can_rebound=1 element=3 size=240 off_x=0 off_y=0 off_z=525 angle=57 knockback_scale=86 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=22 can_rebound=1 element=3 size=240 off_x=0 off_y=0 off_z=250 angle=57 knockback_scale=86 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=22 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=57 knockback_scale=86 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=760 rng_x=0 rng_y=0 rng_z=0
PlayFGM value=299
SyncWait frames=3
ClearAttackCollAll value=0
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=60
SetTexturePartID texturepart_id=1 frame=0
AsyncWait frames=72
SetSlopeContour flags=0
End value=0
