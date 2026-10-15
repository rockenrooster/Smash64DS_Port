# AttackLw4: down smash, Ultimate's (c04attacklw4) frame for frame (owner p6r:
# "down smash too fast use ultimate timing"): the wings an effect of their own
# (art/p6/palutena/wings/dsmash.json, turned to show their face once flat on
# the floor); the sparkle as the staff goes up (5), the slam and its dust at
# 16, its hits 16-18 (Ultimate's 17-19) as TopN offsets along the wings, both
# sides: close 15%, far 13% (ultimateframedata), sending low (30, growth 100,
# base 30).
AsyncWait frames=5
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=560 off_z=-40 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=16
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=13 can_rebound=1 element=0 size=360 off_x=0 off_y=110 off_z=480 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=13 can_rebound=1 element=0 size=360 off_x=0 off_y=110 off_z=-480 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=15 can_rebound=1 element=0 size=340 off_x=0 off_y=180 off_z=190 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=15 can_rebound=1 element=0 size=340 off_x=0 off_y=180 off_z=-190 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
PlayFGMStoreInfo value=cue:se_smash_l01
PlaySmashVoice value=0
SyncWait frames=3
ClearAttackCollAll value=0
End value=0
