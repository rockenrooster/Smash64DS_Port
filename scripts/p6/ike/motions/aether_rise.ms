# Aether's throw, leap and spin (d02specialhi1, then d02specialhi2 from 18).
# Ultimate's frames (ultimateframedata, its frame N our N - 1) and the owner's
# Ultimate attack data (ike_attack_data.json: damage, angle, growth, set and
# base knockback; owner p6r): armour (flag1) 4-34; Ragnell thrown up at 14,
# its hit 6% at 96 with set knockback 140 (its trail off from the throw to the
# catch at 46: it would streak after the flying blade) (14-17, on the flying
# blade); the leap at 40 (flag2: nds_p4_ike.c rises on Ultimate's curve and
# hangs for the spin); 1% hits at 43, 46 and 49 (90, set knockback 50:
# Ultimate's 367 pulls them in); the falling hit, 3% (280, set knockback 40),
# from 50 into the plunge.
AsyncWait frames=4
SetFlag1 value=1
AsyncWait frames=6
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=13
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=14
PlayFGM value=cue:se_aether
PlayVoiceStoreInfo value=cue:special_h01
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=0 element=3 size=280 off_x=0 off_y=0 off_z=380 angle=96 knockback_scale=100 knockback_weight=140 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=0 element=3 size=260 off_x=0 off_y=0 off_z=120 angle=96 knockback_scale=100 knockback_weight=140 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=0
SyncWait frames=4
ClearAttackCollAll value=0
AsyncWait frames=34
SetFlag1 value=0
AsyncWait frames=40
SetFlag2 value=1
AsyncWait frames=43
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=0 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=90 knockback_scale=100 knockback_weight=50 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=0 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=90 knockback_scale=100 knockback_weight=50 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=0 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=90 knockback_scale=100 knockback_weight=50 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=46
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=350 rng_x=0 rng_y=0 rng_z=0
SetAfterImage is_itemswing=17 drawstatus=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=0 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=90 knockback_scale=100 knockback_weight=50 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=0 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=90 knockback_scale=100 knockback_weight=50 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=0 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=90 knockback_scale=100 knockback_weight=50 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=49
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=0 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=90 knockback_scale=100 knockback_weight=50 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=0 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=90 knockback_scale=100 knockback_weight=50 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=0 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=90 knockback_scale=100 knockback_weight=50 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
SyncWait frames=1
ClearAttackCollAll value=0
AsyncWait frames=50
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=3 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=280 knockback_scale=100 knockback_weight=40 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=3 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=280 knockback_scale=100 knockback_weight=40 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=3 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=280 knockback_scale=100 knockback_weight=40 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
End value=0
