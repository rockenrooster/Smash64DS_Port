# AttackAirB: back air, Ultimate's (c05attackairb): the back-handed slap with
# the shield behind her, at 7-9 (Ultimate's 8-10). Ultimate's attack data (the
# owner's palutena_attack_data.json, p6r): 12%, 361, growth 95, base 30.
AsyncWait frames=7
SetFlag1 value=100
PlayVoiceStoreInfo value=cue:attack05
MakeAttackColl attack_id=0 group_id=0 joint_id=8 damage=12 can_rebound=1 element=0 size=280 off_x=180 off_y=0 off_z=0 angle=361 knockback_scale=95 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=9 damage=12 can_rebound=1 element=0 size=280 off_x=94 off_y=0 off_z=0 angle=361 knockback_scale=95 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=30
Effect joint_id=9 effect_id=31 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_attackair_b01
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=29
SetFlag1 value=0
End value=0
