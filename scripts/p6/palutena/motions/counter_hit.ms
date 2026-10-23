# The counterattack (Ultimate's d03speciallwattack): a light blast from the
# staff and in front of her, intangible while it strikes. Ultimate's attack
# data (the owner's palutena_attack_data.json, p6r): 10%, 361, growth 65, base
# 30.
# Owner (2026-10-21): 1.1x damage (10 -> 11) and base knockback (30 -> 33);
# its sound the forward tilt's: the swing (se_swing_m) in place of Ultimate's
# se_special_l02, the hit the heavy punch (fgm_level 2) as before.
# Owner (2026-10-23): its damage scales with the hit it counted, as
# Ultimate's (1.3x, 11 to 50: nds_p4_palutena.c PAL_COUNTER_MUL); the 11
# here is that floor, replaced every tic.
SetHitStatusAll value=2
AsyncWait frames=6
Effect joint_id=18 effect_id=29 flag=0 off_x=35 off_y=146 off_z=-160 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=11 can_rebound=1 element=0 size=300 off_x=35 off_y=146 off_z=-160 angle=361 knockback_scale=65 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=33
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=11 can_rebound=1 element=0 size=360 off_x=0 off_y=320 off_z=320 angle=361 knockback_scale=65 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=33
PlayFGM value=cue:se_swing_m
PlayVoiceStoreInfo value=cue:special_l02
SyncWait frames=4
SetHitStatusAll value=1
ClearAttackCollAll value=0
End value=0
