# Attack11: jab 1 (card: a staff thrust): the staff thrust ahead at hip height
# (art/p6/palutena/anims, key poses on the owner's 3DS reference, 2026-10-14).
AsyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=3 can_rebound=1 element=0 size=200 off_x=23 off_y=150 off_z=-160 angle=70 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=1 element=0 size=170 off_x=11 off_y=75 off_z=-80 angle=70 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=1 element=0 size=150 off_x=0 off_y=0 off_z=0 angle=70 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
PlayFGMStoreInfo value=cue:se_swing_s
SyncWait frames=5
ClearAttackCollAll value=0
AsyncWait frames=18
SetFlag1 value=1
End value=0
