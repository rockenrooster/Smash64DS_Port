# AttackAirHi: up air (card: a twirl, the halo above her): the staff raised
# straight up (art/p6/palutena/anims, key poses on the owner's 3DS reference, 2026-10-14), her halo (the head row's part 2) shown through the hits
SetModelPartID joint_id=12 modelpart_id=2
AsyncWait frames=5
SetFlag1 value=50
PlayVoiceStoreInfo value=cue:attack06
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=9 can_rebound=1 element=0 size=320 off_x=0 off_y=680 off_z=0 angle=80 knockback_scale=95 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=25
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=8 can_rebound=1 element=0 size=200 off_x=23 off_y=150 off_z=-160 angle=80 knockback_scale=95 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=20
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=7 can_rebound=1 element=0 size=170 off_x=11 off_y=75 off_z=-80 angle=80 knockback_scale=95 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=20
Effect joint_id=0 effect_id=42 flag=0 off_x=0 off_y=680 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_attackair_h01
SyncWait frames=6
ClearAttackCollAll value=0
AsyncWait frames=26
SetFlag1 value=0
SetModelPartID joint_id=12 modelpart_id=0
End value=0
