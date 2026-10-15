# AttackLw4: down smash (card: the wings down both sides). Owner p6i
# follow-up: the wings are an effect of their own with a skeleton
# (scripts/p6/p6_wings.py, art/p6/palutena/wings/dsmash.json; made and posed
# every tic of the status by src/port/nds_p4_palutena.c), turned to the
# camera's plane. Keyed on the owner's half-speed 3DS clip (2026-10-14
# p6j; ours 0-10 = its 11-29): turned to the camera, a ring of wings over
# her head (the sparkle at 1), flung open into two thin crescents (9), down
# to the floor both sides and flat along it at the hit (frame 10, the dust)
# as she drops into a deep squat, held there to 33, rising into an arch, gull
# wings, a wide shallow V, gone at 49. The hits are on the wings, as TopN
# offsets (the inner and outer half of each wing's reach, front and back,
# low, out to about 650: radius half the size), sending low.
AsyncWait frames=1
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=560 off_z=-40 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=10
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=13 can_rebound=1 element=0 size=360 off_x=0 off_y=110 off_z=480 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=13 can_rebound=1 element=0 size=360 off_x=0 off_y=110 off_z=-480 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=11 can_rebound=1 element=0 size=340 off_x=0 off_y=180 off_z=190 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=11 can_rebound=1 element=0 size=340 off_x=0 off_y=180 off_z=-190 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
PlayFGMStoreInfo value=cue:se_smash_l01
PlaySmashVoice value=0
SyncWait frames=4
ClearAttackCollAll value=0
End value=0
