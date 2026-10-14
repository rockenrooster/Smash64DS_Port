# Attack11: jab 1 (owner p6i: "A attack Jabs should be a continous attack just
# like the 3DS"): the shield pushed ahead (art/p6/palutena/anims/jab1.json,
# key poses on the owner's 3DS reference at 30 fps), its hit on the shield
# (joint 9, the left forearm: the shield's middle 94 along it) and the arm;
# jab 2 may follow from frame 18.
AsyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=9 damage=3 can_rebound=1 element=0 size=230 off_x=94 off_y=0 off_z=0 angle=70 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
MakeAttackColl attack_id=1 group_id=0 joint_id=9 damage=2 can_rebound=1 element=0 size=170 off_x=0 off_y=0 off_z=0 angle=70 knockback_scale=80 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=10
PlayFGMStoreInfo value=cue:se_swing_s
SyncWait frames=5
ClearAttackCollAll value=0
AsyncWait frames=18
SetFlag1 value=1
End value=0
