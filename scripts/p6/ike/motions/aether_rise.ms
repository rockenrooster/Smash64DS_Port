# Aether's rise: the crouch, the jump at frame 9 (flag2 for nds_p4_ike.c)
# with armour to frame 18 (flag1), three hits that carry upward.
AsyncWait frames=6
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=9
SetFlag2 value=1
SetFlag1 value=1
PlayFGM value=cue:se_aether
PlayVoiceStoreInfo value=cue:special_h01
LoopBegin count=3
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=2 can_rebound=0 element=3 size=280 off_x=0 off_y=0 off_z=260 angle=90 knockback_scale=100 knockback_weight=100 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=2 can_rebound=0 element=3 size=220 off_x=0 off_y=0 off_z=40 angle=90 knockback_scale=100 knockback_weight=100 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=2 can_rebound=0 element=3 size=220 off_x=0 off_y=0 off_z=0 angle=90 knockback_scale=100 knockback_weight=100 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
SyncWait frames=1
LoopEnd value=0
SetFlag1 value=0
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=350 rng_x=0 rng_y=0 rng_z=0
End value=0
