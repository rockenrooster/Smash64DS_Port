# AttackS4: forward smash (card: wings conjured and flapped forward). Owner p6i
# follow-up: the wings are an effect of their own with a skeleton
# (scripts/p6/p6_wings.py, art/p6/palutena/wings/fsmash.json; made and posed
# every tic of the status by src/port/nds_p4_palutena.c). Keyed on the
# owner's half-speed 3DS clip (2026-10-14 p6j; ours 0-14 = its 12-28):
# sprouting behind her shoulders (the sparkle at 4), a big V raised over her
# as she hops (11), swung up ahead (13) and laid out long and low ahead to
# the floor at the hit (frame 14, SSB64's speed), held through her bow to
# 31, curling up into a V behind her and gone at 44. The hits are on the
# wings, as TopN offsets (the inner and the outer half of their reach ahead
# of her, out to about 850: radius half the size), and on the staff.
AsyncWait frames=4
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=416 off_z=-177 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=14
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=380 off_x=0 off_y=240 off_z=600 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=380 off_x=0 off_y=220 off_z=270 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=14 can_rebound=1 element=0 size=230 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
Effect joint_id=0 effect_id=19 flag=0 off_x=0 off_y=0 off_z=-180 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_smash_s01
PlaySmashVoice value=0
SyncWait frames=4
ClearAttackCollAll value=0
End value=0
