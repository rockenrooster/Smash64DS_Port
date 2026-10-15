# AttackAirF: forward air, Ultimate's (c05attackairf): the side kick, her left
# leg (the slit's) shot out ahead with a burst of light at the foot; one hit
# as Ultimate's (its 9-11, here 3-10). Ultimate's attack data (the owner's
# palutena_attack_data.json, p6r): 10%, 45, growth 92, base 31.
AsyncWait frames=3
SetFlag1 value=50
PlayVoiceStoreInfo value=cue:attack04
MakeAttackColl attack_id=0 group_id=0 joint_id=22 damage=10 can_rebound=1 element=0 size=240 off_x=0 off_y=0 off_z=0 angle=45 knockback_scale=92 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=31
MakeAttackColl attack_id=1 group_id=0 joint_id=21 damage=10 can_rebound=1 element=0 size=220 off_x=0 off_y=0 off_z=0 angle=45 knockback_scale=92 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=31
MakeAttackColl attack_id=2 group_id=0 joint_id=5 damage=10 can_rebound=1 element=0 size=240 off_x=0 off_y=0 off_z=0 angle=45 knockback_scale=92 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=31
PlayFGMStoreInfo value=cue:se_attackair_f01
AsyncWait frames=4
Effect joint_id=22 effect_id=31 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=11
ClearAttackCollAll value=0
AsyncWait frames=25
SetFlag1 value=0
End value=0
