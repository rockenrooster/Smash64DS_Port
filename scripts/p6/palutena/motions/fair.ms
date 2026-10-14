# AttackAirF: forward air (the owner's 3DS reference, 2026-10-14; the owner:
# "follow the 3DS ones" over the card's flying kick): the staff thrust ahead
# level with a light burst at its head. The kick's frames and damage, its
# hitboxes on the staff (joint 18: its head 220 and 110 along it) and the
# forearm (joint 16).
AsyncWait frames=3
SetFlag1 value=50
PlayVoiceStoreInfo value=cue:attack04
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=10 can_rebound=1 element=0 size=240 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=15
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=10 can_rebound=1 element=0 size=220 off_x=11 off_y=75 off_z=-80 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=15
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=10 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=15
PlayFGMStoreInfo value=cue:se_attackair_f01
AsyncWait frames=4
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=11
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=7 can_rebound=1 element=0 size=240 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=7 can_rebound=1 element=0 size=220 off_x=11 off_y=75 off_z=-80 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=7 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=0
SyncWait frames=14
SetFlag1 value=0
ClearAttackCollAll value=0
End value=0
