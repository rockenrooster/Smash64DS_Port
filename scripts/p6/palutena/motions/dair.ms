# AttackAirLw: down air (the owner's 3DS reference, 2026-10-14; the owner:
# "follow the 3DS ones" over the card's roundhouse kick). The owner's
# half-speed high-resolution clip shows a stomp: turned to the camera, arms
# spread, her left leg (the slit's) driven straight down with a burst of
# light at the foot. The kick's frames and damage, its hitboxes on the leg
# (joint 22, the left ankle; joint 21, the left knee); the first a spike.
AsyncWait frames=7
SetFlag1 value=40
PlayVoiceStoreInfo value=cue:attack07
MakeAttackColl attack_id=0 group_id=0 joint_id=22 damage=12 can_rebound=1 element=0 size=300 off_x=0 off_y=0 off_z=0 angle=290 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=21 damage=10 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=290 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=10
PlayFGMStoreInfo value=cue:se_attackair_l01
Effect joint_id=22 effect_id=31 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=22 damage=9 can_rebound=1 element=0 size=280 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=21 damage=8 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=10
SyncWait frames=12
SetFlag1 value=0
ClearAttackCollAll value=0
End value=0
