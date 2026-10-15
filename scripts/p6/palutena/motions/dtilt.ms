# AttackLw3: down tilt, Ultimate's (c02attacklw3): from her crouch the staff
# spun along the floor, at 6-9. Ultimate's attack data (the owner's
# palutena_attack_data.json, p6r): 5%, 75, growth 100, base 38.
AsyncWait frames=6
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=200 off_x=41 off_y=272 off_z=-290 angle=75 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=38
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=170 off_x=23 off_y=150 off_z=-160 angle=75 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=38
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=5 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=75 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=38
PlayFGMStoreInfo value=cue:se_attackhard_l01
SyncWait frames=4
ClearAttackCollAll value=0
End value=0
