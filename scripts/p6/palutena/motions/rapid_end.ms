# Her rapid jab's finisher (an appended Attack100End; the owner's 3DS
# reference, 2026-10-14 p6i): a last thrust with a burst of light that
# launches (frame 3), the staff swung up overhead, back to her wait.
AsyncWait frames=3
PlayVoiceStoreInfo value=cue:attack02
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=3 can_rebound=1 element=0 size=260 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=130 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=3 can_rebound=1 element=0 size=300 off_x=40 off_y=270 off_z=-290 angle=361 knockback_scale=130 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
Effect joint_id=18 effect_id=31 flag=0 off_x=40 off_y=270 off_z=-290 rng_x=0 rng_y=0 rng_z=0
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
PlayFGM value=cue:se_attack100end
SyncWait frames=3
ClearAttackCollAll value=0
End value=0
