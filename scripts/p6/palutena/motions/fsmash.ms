# AttackS4: forward smash (card: wings conjured and flapped forward). Owner p6i
# follow-up: the wings are an effect of their own with a skeleton
# (scripts/p6/p6_wings.py, art/p6/palutena/wings/fsmash.json; made and posed
# every tic of the status by src/port/nds_p4_palutena.c): sprouting small
# behind her shoulders and growing as they rise (the sparkle at 9), whipped
# down in front and stretched long, level at the hit (frames 14-17),
# retracting through the hold, rising into a V and gone at 43. The hits are
# on the wings, as TopN offsets (the inner and the outer half of their reach
# ahead of her: radius half the size), and on the staff.
AsyncWait frames=9
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=416 off_z=-177 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=14
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=380 off_x=0 off_y=260 off_z=540 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=380 off_x=0 off_y=210 off_z=230 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=14 can_rebound=1 element=0 size=230 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
Effect joint_id=0 effect_id=19 flag=0 off_x=0 off_y=0 off_z=-180 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_smash_s01
PlaySmashVoice value=0
SyncWait frames=4
ClearAttackCollAll value=0
End value=0
