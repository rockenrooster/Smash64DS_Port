# AttackHi4: up smash (card: the light pillar ahead of her): Marth's up smash,
# the staff raised; her light pillar stands where the column of hitboxes does.
# Owner playtest 2026-10-14: a light burst of several sprites (the hips row's
# variants 1-5 in turn: ignite, burst, sustain, fade, thin; parts.json), 1.25x
# taller (1,560), always in front of her (each card on both sides of her in
# depth), and hitboxes matching it: four 390-unit spheres stacked from the
# floor to its top at its middle (TopN z 260); white sparkles, no green ring.
AsyncWait frames=11
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlaySmashVoice value=0
AsyncWait frames=12
SetModelPartID joint_id=5 modelpart_id=1
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=390 off_x=0 off_y=195 off_z=260 angle=90 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=390 off_x=0 off_y=585 off_z=260 angle=90 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=14 can_rebound=1 element=0 size=390 off_x=0 off_y=975 off_z=260 angle=90 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=14 can_rebound=1 element=0 size=390 off_x=0 off_y=1365 off_z=260 angle=90 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
Effect joint_id=0 effect_id=30 flag=0 off_x=0 off_y=300 off_z=260 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_smash_h01
SyncWait frames=2
SetModelPartID joint_id=5 modelpart_id=2
Effect joint_id=0 effect_id=30 flag=0 off_x=0 off_y=900 off_z=260 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
SetModelPartID joint_id=5 modelpart_id=3
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=1450 off_z=260 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
SetModelPartID joint_id=5 modelpart_id=4
SyncWait frames=2
SetModelPartID joint_id=5 modelpart_id=5
SyncWait frames=2
SetModelPartID joint_id=5 modelpart_id=0
End value=0
