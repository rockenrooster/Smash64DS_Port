# AttackDash: the shield charge (D3). Owner p6i: a lunge before the bash, more
# force (art/p6/palutena/anims/dash.json, key poses on the owner's 3DS
# reference at 30 fps): she gathers (frames 0-4), lunges with the shield
# punched out (the bash, frames 6-10; its own TransN lunge curve) and slides
# on it (the late hit to 20). The shield's hitbox at its middle (joint 9, the
# left forearm, 94 along it), her body's on the torso.
SetSlopeContour flags=4
AsyncWait frames=6
PlayVoiceStoreInfo value=cue:attack02
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=10 can_rebound=1 element=0 size=300 off_x=94 off_y=0 off_z=0 angle=70 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=10 can_rebound=1 element=0 size=300 off_x=0 off_y=120 off_z=140 angle=70 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=40
Effect joint_id=9 effect_id=41 flag=0 off_x=94 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_attackdash
SyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=7 can_rebound=1 element=0 size=280 off_x=94 off_y=0 off_z=0 angle=361 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=20
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=7 can_rebound=1 element=0 size=280 off_x=0 off_y=120 off_z=140 angle=361 knockback_scale=70 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=20
SyncWait frames=10
ClearAttackCollAll value=0
SetSlopeContour flags=0
End value=0
