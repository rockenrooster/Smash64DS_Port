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
# Owner p7h follow-up (2026-10-19): knockback growth 110, base 35 (were
# 152 and 50).
# Owner (2026-10-21): electric (element 2) with the zap hit sound (fgm_kind
# 4, nSYAudioFGMShockM), damage 2x (6 -> 12, 7 -> 14), knockback growth 0.7x
# (110 -> 77), and the boxes cover her upper half and above her head: the
# staff's two kept, the TopN box above her head 368 -> 400 (centre 806 up,
# radius 200: 606 to 1,006), and a fourth TopN box the staff box 0's size
# (299) over her upper body, its top meeting the head box (centre 456 up:
# 306 to 606; her half height 303, head top 606); the launcher's each 23
# bigger as before. TopN offsets are scaled by her size (1.185), radii are
# not.
# Owner (2026-10-22): multi-hit, as Ultimate's and neutral air's (a box
# re-made after ClearAttackCollAll hits afresh): five light hits of 2% at 10,
# 14, 18, 22 and 26 (Ultimate's 5, 7, 9, 11, 13 at the 0.5 rate; two frames
# on, two off), then the launcher, 12%, from 28 to 31: 22% in all. The light
# hits a fixed knockback (weight 25, growth 100, base 0: the same push at
# any damage, so the opponent stays in), angle 90 (straight up, over her);
# the launcher 88, growth 77, base 35. All electric with the zap sound, the
# boxes as above (the launcher's 23 bigger).
AsyncWait frames=8
PlayFGM value=cue:se_attackair_h01
AsyncWait frames=10
SetFlag1 value=50
PlayVoiceStoreInfo value=cue:attack06
Effect joint_id=0 effect_id=42 flag=0 off_x=0 off_y=680 off_z=0 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=400 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=299 off_x=0 off_y=385 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
AsyncWait frames=12
ClearAttackCollAll value=0
AsyncWait frames=14
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=400 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=299 off_x=0 off_y=385 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
AsyncWait frames=16
ClearAttackCollAll value=0
AsyncWait frames=18
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=400 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=299 off_x=0 off_y=385 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
AsyncWait frames=20
ClearAttackCollAll value=0
AsyncWait frames=22
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=400 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=299 off_x=0 off_y=385 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
AsyncWait frames=24
ClearAttackCollAll value=0
AsyncWait frames=26
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=299 off_x=41 off_y=272 off_z=-290 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=2 size=253 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=400 off_x=0 off_y=680 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=2 can_rebound=1 element=2 size=299 off_x=0 off_y=385 off_z=0 angle=90 knockback_scale=100 knockback_weight=25 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=0
AsyncWait frames=28
ClearAttackCollAll value=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=12 can_rebound=1 element=2 size=322 off_x=41 off_y=272 off_z=-290 angle=88 knockback_scale=77 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=35
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=12 can_rebound=1 element=2 size=276 off_x=23 off_y=150 off_z=-160 angle=88 knockback_scale=77 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=35
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=12 can_rebound=1 element=2 size=423 off_x=0 off_y=680 off_z=0 angle=88 knockback_scale=77 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=35
MakeAttackColl attack_id=3 group_id=0 joint_id=0 damage=12 can_rebound=1 element=2 size=322 off_x=0 off_y=385 off_z=0 angle=88 knockback_scale=77 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=4 knockback_base=35
AsyncWait frames=32
ClearAttackCollAll value=0
AsyncWait frames=64
SetFlag1 value=0
End value=0
