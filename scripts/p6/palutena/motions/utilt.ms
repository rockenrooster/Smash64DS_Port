# AttackHi3: up tilt, Ultimate's (c02attackhi3): on her knees the staff spins
# over her: four light hits (6, 12, 18, 24; each re-made) and a launching last
# (30-32). Ultimate's attack data (the owner's palutena_attack_data.json,
# p6r): the spin 0.5% (1), its pull (Ultimate's 190 with set knockback 30) a
# light upward set knockback here; the last 85, growth 195, base 60, its
# damage the latest patch's 4.5% (5; ultimateframedata, owner p6s; 1.0.0's 2.5).
# Owner p6w: its hit sound the forward air's (a heavy punch, level 2) and its
# hitboxes 1.15x the size.
# Owner p6x VFX plan (picks 2026-10-16): her staff's solid cyan-white trail
# (sword trail 24, spec sword_trails: row 14's grip-to-gem line) while it
# spins, 5 to 34.
AsyncWait frames=5
SetAfterImage is_itemswing=24 drawstatus=0
AsyncWait frames=6
PlayVoiceStoreInfo value=cue:attack03
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=265 off_x=41 off_y=272 off_z=-290 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=265 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=195 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=12
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=265 off_x=41 off_y=272 off_z=-290 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=265 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=195 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=18
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=265 off_x=41 off_y=272 off_z=-290 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=265 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=195 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=24
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=265 off_x=41 off_y=272 off_z=-290 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=265 off_x=23 off_y=150 off_z=-160 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=1 can_rebound=1 element=0 size=195 off_x=0 off_y=0 off_z=0 angle=100 knockback_scale=100 knockback_weight=15 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=0
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=30
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=299 off_x=41 off_y=272 off_z=-290 angle=85 knockback_scale=195 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=60
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=299 off_x=23 off_y=150 off_z=-160 angle=85 knockback_scale=195 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=60
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=5 can_rebound=1 element=0 size=218 off_x=0 off_y=0 off_z=0 angle=85 knockback_scale=195 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=60
PlayFGM value=cue:se_swing_m
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=34
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
