# Her rapid jab's loop (an appended Attack100Loop, Fox's routines; the owner's
# 3DS reference, 2026-10-14 p6i): the staff held ahead, a burst of light at its
# head four times a loop (frames 2, 8, 14, 20), its hits along the staff from
# its head to her grip (the grip's box owner 2026-10-22: enemies close to her
# were missed); each SetFlag1 lets the loop end unless A was pressed again
# (ftCommonAttack100LoopProcUpdate); at the clip's end the script waits for it
# to loop (PauseScript) and starts again (Fox's loop script's shape).
PlayFGM value=cue:se_attack100
AsyncWait frames=2
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=210 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=240 off_x=40 off_y=270 off_z=-290 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=270 off_x=2 off_y=16 off_z=-17 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
Effect joint_id=18 effect_id=31 flag=0 off_x=40 off_y=270 off_z=-290 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
SetFlag1 value=1
AsyncWait frames=8
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=210 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=240 off_x=40 off_y=270 off_z=-290 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=270 off_x=2 off_y=16 off_z=-17 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
Effect joint_id=18 effect_id=31 flag=0 off_x=40 off_y=270 off_z=-290 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
SetFlag1 value=1
AsyncWait frames=14
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=210 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=240 off_x=40 off_y=270 off_z=-290 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=270 off_x=2 off_y=16 off_z=-17 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
Effect joint_id=18 effect_id=31 flag=0 off_x=40 off_y=270 off_z=-290 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
SetFlag1 value=1
AsyncWait frames=20
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=210 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=240 off_x=40 off_y=270 off_z=-290 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=1 can_rebound=1 element=0 size=270 off_x=2 off_y=16 off_z=-17 angle=80 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
Effect joint_id=18 effect_id=31 flag=0 off_x=40 off_y=270 off_z=-290 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
SetFlag1 value=1
PauseScript value=0
SetFlag1 value=1
Goto -> self
