# AttackLw4: down smash (card: the wings down both sides). Owner p6i: the wings
# flap (art/p6/palutena/look/parts.json: the torso row's variants 1 folded, 3
# down to the floor at her front and back; art/p6/palutena/anims/dsmash.json,
# the owner's 3DS reference at 30 fps): folded behind her as she rises,
# flapped down at both sides as she drops (one hit at each, frames 10-14,
# sending low), folded as she rises, gone at 34.
SetModelPartID joint_id=6 modelpart_id=1
AsyncWait frames=9
SetModelPartID joint_id=6 modelpart_id=3
AsyncWait frames=10
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=6 damage=13 can_rebound=1 element=0 size=330 off_x=0 off_y=-230 off_z=330 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=13 can_rebound=1 element=0 size=330 off_x=0 off_y=-230 off_z=-360 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=6 damage=11 can_rebound=1 element=0 size=280 off_x=0 off_y=-80 off_z=200 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=3 group_id=0 joint_id=6 damage=11 can_rebound=1 element=0 size=280 off_x=0 off_y=-80 off_z=-220 angle=30 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
PlayFGMStoreInfo value=cue:se_smash_l01
PlaySmashVoice value=0
SyncWait frames=4
ClearAttackCollAll value=0
AsyncWait frames=24
SetModelPartID joint_id=6 modelpart_id=1
AsyncWait frames=34
SetModelPartID joint_id=6 modelpart_id=0
End value=0
