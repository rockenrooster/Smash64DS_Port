# AttackLw4: down smash, the sweep in front then behind (c04attacklw4).
# Ultimate's frames (ultimateframedata, its frame N our N - 1) and the owner's
# Ultimate attack data (ike_attack_data.json: damage, angle, growth, set and
# base knockback; owner p6r): 16% in front at 12-14, 19% behind at 31-32 (48,
# growth 100, base 40), 9% late at 33-35 (61, base 30).
SetSlopeContour flags=3
AsyncWait frames=4
SetAfterImage is_itemswing=17 drawstatus=0
SetSlopeContour flags=4
AsyncWait frames=8
SetTexturePartID texturepart_id=1 frame=6
PlayVoiceStoreInfo value=1445
AsyncWait frames=12
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=16 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=48 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=16 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=48 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=16 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=48 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=3 group_id=0 joint_id=6 damage=16 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=48 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
PlayFGMStoreInfo value=258
Effect joint_id=0 effect_id=16 flag=0 off_x=0 off_y=0 off_z=-100 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=26
PlaySmashVoice value=0
AsyncWait frames=31
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=19 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=48 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=19 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=48 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=19 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=48 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=3 group_id=0 joint_id=6 damage=19 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=48 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
PlayFGMStoreInfo value=258
Effect joint_id=0 effect_id=15 flag=0 off_x=0 off_y=0 off_z=-350 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=9 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=61 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=9 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=61 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=9 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=61 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=30
ClearAttackCollID value=3
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=44
SetAfterImage is_itemswing=0 drawstatus=-1
SetTexturePartID texturepart_id=1 frame=0
AsyncWait frames=60
SetSlopeContour flags=3
End value=0
