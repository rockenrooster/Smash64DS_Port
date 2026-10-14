# Attack12: jab 2 (owner p6i): the staff thrust ahead at her hip with a light
# burst (art/p6/palutena/anims/jab2.json, the owner's 3DS reference), a
# lighter hit than before so the rapid jab links (it follows from frame 12
# when A is mashed: her rapid jab rows, spec kind_tables).
AsyncWait frames=4
PlayVoiceStoreInfo value=cue:attack01
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=3 can_rebound=1 element=0 size=220 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=15
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=170 off_x=11 off_y=75 off_z=-80 angle=80 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=15
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=50 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=15
Effect joint_id=18 effect_id=31 flag=0 off_x=23 off_y=150 off_z=-160 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_swing_m
SyncWait frames=6
ClearAttackCollAll value=0
AsyncWait frames=12
SetFlag1 value=1
End value=0
