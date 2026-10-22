# AttackDash: the shield charge (D3), Ultimate's motion (c00attackdash), its
# travel the TransN slot's: the bash at 6-9, the slide to 19. Hits at the
# shield's middle (joint 9, 94 along the forearm) and her torso. Ultimate's
# attack data (the owner's palutena_attack_data.json, p6r): 60, growth 90,
# base 70; late 60, growth 70, base 60; the damage the latest patch's
# (ultimateframedata, owner p6s): 11%, late 7%.
# Owner p7h (2026-10-19): base knockback 0.6x -- 70 -> 42, the late hit's
# 60 -> 36.
# Owner (2026-10-19, after play-p7k): its hit sound the forward tilt's (a
# heavy punch, level 2).
# Owner (2026-10-21): angle 60 -> 30, both hits.
# Owner (2026-10-22): base knockback 42 -> 40 (the late hit's 36 kept).
SetSlopeContour flags=4
AsyncWait frames=6
PlayVoiceStoreInfo value=cue:attack02
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=11 can_rebound=1 element=0 size=300 off_x=94 off_y=0 off_z=0 angle=30 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=11 can_rebound=1 element=0 size=300 off_x=0 off_y=120 off_z=140 angle=30 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=40
Effect joint_id=9 effect_id=41 flag=0 off_x=94 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_attackdash
SyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=7 can_rebound=1 element=0 size=280 off_x=94 off_y=0 off_z=0 angle=30 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=36
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=7 can_rebound=1 element=0 size=280 off_x=0 off_y=120 off_z=140 angle=30 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=36
SyncWait frames=10
ClearAttackCollAll value=0
SetSlopeContour flags=0
End value=0
