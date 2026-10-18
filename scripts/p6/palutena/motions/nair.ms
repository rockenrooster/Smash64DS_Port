# AttackAirN: neutral air, Ultimate's (c05attackairn): the staff whirled round
# her. Owner p6ac (2026-10-17): played at 0.75 (art/p6/palutena/anims/nair.json,
# every frame below 4/3 of what it was), one hit instead of Ultimate's five
# light ones and a launcher, its damage the forward tilt's (6% then 7%,
# re-made live so it hits once, as motions/ftilt.ms), its hitboxes 1.25x the
# launcher's; the launcher's 55, growth 180, base 30 (Ultimate's attack data,
# the owner's palutena_attack_data.json, p6r).
# Her whirl's sound once, from frame 1 (owner p6v: it played at every light
# hit, five over each other).
# Owner p6x VFX plan (picks 2026-10-16): her staff's solid cyan-white trail
# (sword trail 24, spec sword_trails: her staff's line) while it spins, 5 to
# 32.
AsyncWait frames=1
PlayFGM value=cue:se_attackair_n01
AsyncWait frames=5
SetAfterImage is_itemswing=24 drawstatus=0
AsyncWait frames=7
SetFlag1 value=40
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=0 size=275 off_x=41 off_y=272 off_z=-290 angle=55 knockback_scale=180 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=1 element=0 size=238 off_x=23 off_y=150 off_z=-160 angle=55 knockback_scale=180 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=6 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=55 knockback_scale=180 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
PlayFGMStoreInfo value=cue:se_swing_m
AsyncWait frames=18
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=7 can_rebound=1 element=0 size=275 off_x=41 off_y=272 off_z=-290 angle=55 knockback_scale=180 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=7 can_rebound=1 element=0 size=238 off_x=23 off_y=150 off_z=-160 angle=55 knockback_scale=180 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=7 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=55 knockback_scale=180 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
AsyncWait frames=30
ClearAttackCollAll value=0
AsyncWait frames=32
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=40
SetFlag1 value=0
End value=0
