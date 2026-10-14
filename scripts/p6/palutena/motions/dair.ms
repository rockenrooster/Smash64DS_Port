# AttackAirLw: down air (the owner's 3DS reference, 2026-10-14; the owner:
# "follow the 3DS ones" over the card's roundhouse kick): the shield swept
# under her from front to back. The kick's frames and damage, its hitboxes
# on the shield (joint 9, the left forearm: the shield's middle 94 along it)
# and the upper arm (joint 8); the first a spike while the shield passes
# under her.
AsyncWait frames=7
SetFlag1 value=40
PlayVoiceStoreInfo value=cue:attack07
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=12 can_rebound=1 element=0 size=300 off_x=94 off_y=0 off_z=0 angle=290 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=8 damage=10 can_rebound=1 element=0 size=200 off_x=70 off_y=0 off_z=0 angle=290 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=0 knockback_base=10
PlayFGMStoreInfo value=cue:se_attackair_l01
SyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=9 can_rebound=1 element=0 size=280 off_x=94 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=8 damage=8 can_rebound=1 element=0 size=200 off_x=70 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=10
SyncWait frames=12
SetFlag1 value=0
ClearAttackCollAll value=0
End value=0
