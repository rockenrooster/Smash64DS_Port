# AttackS4: forward smash (card: the wings, next; for now the staff swung as Peach swings her pan)
SetModelPartID joint_id=18 modelpart_id=1
AsyncWait frames=13
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=416 off_z=-177 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=15
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=16 can_rebound=1 element=0 size=230 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=14 can_rebound=1 element=0 size=200 off_x=11 off_y=75 off_z=-80 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=14 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
Effect joint_id=0 effect_id=19 flag=0 off_x=0 off_y=0 off_z=-180 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_smash_s01
PlaySmashVoice value=0
SyncWait frames=4
ClearAttackCollAll value=0
End value=0
