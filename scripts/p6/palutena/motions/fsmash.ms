# AttackS4: forward smash (card: wings conjured and flapped forward). Owner p6i:
# the wings animate (art/p6/palutena/look/parts.json: the torso row's variants
# 1 folded, 2 swept forward; art/p6/palutena/anims/fsmash.json, the owner's 3DS
# reference at 30 fps): folded behind her as she leans back with the staff,
# swept forward low as she lunges (the hit on the wings and the staff, frames
# 14-18), folded back, gone at 36.
SetModelPartID joint_id=6 modelpart_id=1
AsyncWait frames=9
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=416 off_z=-177 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=13
SetModelPartID joint_id=6 modelpart_id=2
AsyncWait frames=14
MakeAttackColl attack_id=0 group_id=0 joint_id=6 damage=16 can_rebound=1 element=0 size=330 off_x=0 off_y=60 off_z=330 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=16 can_rebound=1 element=0 size=300 off_x=0 off_y=-60 off_z=200 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=14 can_rebound=1 element=0 size=230 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=30
Effect joint_id=0 effect_id=19 flag=0 off_x=0 off_y=0 off_z=-180 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_smash_s01
PlaySmashVoice value=0
SyncWait frames=4
ClearAttackCollAll value=0
AsyncWait frames=27
SetModelPartID joint_id=6 modelpart_id=1
AsyncWait frames=36
SetModelPartID joint_id=6 modelpart_id=0
End value=0
