# AttackHi4: up smash (card: the light pillar ahead of her): Marth's up smash, the staff raised
AsyncWait frames=11
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlaySmashVoice value=0
AsyncWait frames=12
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=300 off_x=0 off_y=350 off_z=260 angle=90 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=0 damage=16 can_rebound=1 element=0 size=300 off_x=0 off_y=700 off_z=260 angle=90 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=0 damage=14 can_rebound=1 element=0 size=300 off_x=0 off_y=1050 off_z=260 angle=90 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=3 group_id=0 joint_id=18 damage=12 can_rebound=1 element=0 size=200 off_x=23 off_y=150 off_z=-160 angle=90 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
Effect joint_id=0 effect_id=43 flag=0 off_x=0 off_y=350 off_z=260 rng_x=0 rng_y=0 rng_z=0
Effect joint_id=0 effect_id=41 flag=0 off_x=0 off_y=700 off_z=260 rng_x=0 rng_y=0 rng_z=0
Effect joint_id=0 effect_id=41 flag=0 off_x=0 off_y=1050 off_z=260 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_smash_h01
SyncWait frames=6
ClearAttackCollAll value=0
End value=0
