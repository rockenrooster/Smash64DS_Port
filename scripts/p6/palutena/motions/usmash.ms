# AttackHi4: up smash (card: the light pillar ahead of her): Marth's up smash,
# the staff raised. Owner playtest 2026-10-14, round 2: ONE column of light in
# front of her, exactly vertical, as Ultimate's: an effect her code makes at
# flag0, 260 ahead on her floor, world-aligned (nds_p4_palutena.c; its cards
# in her special file 2, scripts/p6/p6_gate.py), 1,560 tall, no other effects.
# Four 390-unit spheres stacked from the floor to its top at its middle (TopN
# z 260) are its hitboxes.
SetFlag0 value=0
AsyncWait frames=11
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlaySmashVoice value=0
AsyncWait frames=12
SetFlag0 value=1
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=390 off_x=0 off_y=195 off_z=260 angle=90 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=390 off_x=0 off_y=585 off_z=260 angle=90 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=14 can_rebound=1 element=0 size=390 off_x=0 off_y=975 off_z=260 angle=90 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=14 can_rebound=1 element=0 size=390 off_x=0 off_y=1365 off_z=260 angle=90 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
PlayFGMStoreInfo value=cue:se_smash_h01
SyncWait frames=6
ClearAttackCollAll value=0
End value=0
