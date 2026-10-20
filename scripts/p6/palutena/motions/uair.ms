# AttackAirHi: up air, Ultimate's (c05attackairhi): the twirl, the staff
# raised over her (no halo: owner 2026-10-17).
# Owner 2026-10-18: neutral air's damage and hit sound. One hit instead of
# Ultimate's five light ones (5, 7, 9, 11, 13; 1% each) and a launcher (14-15,
# 5%), as motions/nair.ms (owner p6ac): 6% from the first light hit's frame,
# 7% from the launcher's (re-made live, so it hits once), nair's hit sound
# (fgm_level 1); the launcher's 88, growth 152, base 50 throughout
# (Ultimate's attack data, the owner's palutena_attack_data.json, p6r). The
# light hits' boxes until 14, the launcher's after (owner p6w: 1.15x).
# Owner p6w: its sound once, from 4, so its shimmer peaks at the launch (14).
# Owner p7h (2026-10-19): played at 0.5 (art/p6/palutena/anims/uair.json),
# every frame below twice what the notes above say: the sound from 8, the
# hit from 10, the launcher from 28, the boxes cleared at 32, the landing
# flag at 64.
AsyncWait frames=8
PlayFGM value=cue:se_attackair_h01
AsyncWait frames=10
SetFlag1 value=50
PlayVoiceStoreInfo value=cue:attack06
Effect joint_id=0 effect_id=42 flag=0 off_x=0 off_y=680 off_z=0 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=0 size=299 off_x=41 off_y=272 off_z=-290 angle=88 knockback_scale=152 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=50
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=1 element=0 size=253 off_x=23 off_y=150 off_z=-160 angle=88 knockback_scale=152 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=50
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=6 can_rebound=1 element=0 size=368 off_x=0 off_y=680 off_z=0 angle=88 knockback_scale=152 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=50
AsyncWait frames=28
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=7 can_rebound=1 element=0 size=322 off_x=41 off_y=272 off_z=-290 angle=88 knockback_scale=152 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=50
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=7 can_rebound=1 element=0 size=276 off_x=23 off_y=150 off_z=-160 angle=88 knockback_scale=152 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=50
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=7 can_rebound=1 element=0 size=391 off_x=0 off_y=680 off_z=0 angle=88 knockback_scale=152 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=50
AsyncWait frames=32
ClearAttackCollAll value=0
AsyncWait frames=64
SetFlag1 value=0
End value=0
