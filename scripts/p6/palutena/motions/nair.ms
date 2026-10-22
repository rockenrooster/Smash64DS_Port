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
# Owner p7h (2026-10-19): knockback growth 100 (was the launcher's 180).
# Owner (2026-10-21): multi-hit again, Fox's down air's way (a box re-made
# after ClearAttackCollAll hits afresh): four light hits of 2% at 7, 10, 13
# and 16 (two frames each, a frame off between), then the launcher, 6%, from
# 18 to 29 (14% in all). The light hits a fixed knockback (weight 25, growth
# 100, base 0: the same push at any damage, so the opponent stays in), angle
# 80 (up, so a rising opponent rises with her); the launcher 55, growth 100,
# base 30. Owner (2026-10-22, after playing it): the up tilt's hit sound (a
# heavy punch, level 2) on every hit, and the launcher 5% -> 6%. The whirl's
# sound and the swing still once.
AsyncWait frames=1
PlayFGM value=cue:se_attackair_n01
AsyncWait frames=5
SetAfterImage is_itemswing=24 drawstatus=0
AsyncWait frames=7
SetFlag1 value=40
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=275 off_x=41 off_y=272 off_z=-290 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=238 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
PlayFGMStoreInfo value=cue:se_swing_m
AsyncWait frames=9
ClearAttackCollAll value=0
AsyncWait frames=10
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=275 off_x=41 off_y=272 off_z=-290 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=238 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
AsyncWait frames=12
ClearAttackCollAll value=0
AsyncWait frames=13
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=275 off_x=41 off_y=272 off_z=-290 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=238 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
AsyncWait frames=15
ClearAttackCollAll value=0
AsyncWait frames=16
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=275 off_x=41 off_y=272 off_z=-290 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=238 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
AsyncWait frames=18
ClearAttackCollAll value=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=0 size=275 off_x=41 off_y=272 off_z=-290 angle=55 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=1 element=0 size=238 off_x=23 off_y=150 off_z=-160 angle=55 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=6 can_rebound=1 element=0 size=200 off_x=0 off_y=0 off_z=0 angle=55 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=30
AsyncWait frames=30
ClearAttackCollAll value=0
AsyncWait frames=32
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=40
SetFlag1 value=0
End value=0
