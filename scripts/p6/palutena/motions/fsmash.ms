# AttackS4: forward smash, Ultimate's (c03attacks4s): the wings an effect of
# their own (scripts/p6/p6_wings.py, art/p6/palutena/wings/fsmash.json,
# nds_p4_palutena.c), laid out ahead to the floor at the hit (14-17). The hits
# on the wings as TopN offsets (the inner half of their reach close, the outer
# far) and on the staff. Ultimate's attack data (the owner's
# palutena_attack_data.json, p6r) and ultimateframedata: close 16%, far 13%,
# 361, growth 102, base 40.
AsyncWait frames=4
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=416 off_z=-177 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=14
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=13 can_rebound=1 element=0 size=380 off_x=0 off_y=240 off_z=600 angle=361 knockback_scale=102 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=380 off_x=0 off_y=220 off_z=270 angle=361 knockback_scale=102 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=16 can_rebound=1 element=0 size=230 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=102 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
Effect joint_id=0 effect_id=19 flag=0 off_x=0 off_y=0 off_z=-180 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_smash_s01
PlaySmashVoice value=0
SyncWait frames=4
ClearAttackCollAll value=0
End value=0
