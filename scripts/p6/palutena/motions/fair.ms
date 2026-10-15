# AttackAirF: forward air (the owner's 3DS reference, 2026-10-14; the owner:
# "follow the 3DS ones" over the card's flying kick). The owner's half-speed
# high-resolution clip shows a flying kick: her left leg (the slit's) shot
# out level ahead with a burst of light at the foot, leaning back with the
# staff raised behind her head. The kick's frames and damage, its hitboxes
# on the leg (joint 22, the left ankle; joint 21, the left knee) and her
# hips (joint 5), the burst at the foot.
AsyncWait frames=3
SetFlag1 value=50
PlayVoiceStoreInfo value=cue:attack04
MakeAttackColl attack_id=0 group_id=0 joint_id=22 damage=10 can_rebound=1 element=0 size=240 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=15
MakeAttackColl attack_id=1 group_id=0 joint_id=21 damage=10 can_rebound=1 element=0 size=220 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=15
MakeAttackColl attack_id=2 group_id=0 joint_id=5 damage=10 can_rebound=1 element=0 size=240 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=15
PlayFGMStoreInfo value=cue:se_attackair_f01
AsyncWait frames=4
Effect joint_id=22 effect_id=31 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=11
MakeAttackColl attack_id=0 group_id=0 joint_id=22 damage=7 can_rebound=1 element=0 size=240 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=21 damage=7 can_rebound=1 element=0 size=220 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=5 damage=7 can_rebound=1 element=0 size=240 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=0
SyncWait frames=14
SetFlag1 value=0
ClearAttackCollAll value=0
End value=0
