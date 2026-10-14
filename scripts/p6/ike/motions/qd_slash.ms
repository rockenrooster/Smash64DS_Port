# Quick Draw's slash on contact: Roy's forward-tilt clip and timing; 6 damage
# and up to 9 more by charge (nds_p4_ike.c), sent forward.
AsyncWait frames=4
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=7
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=250 angle=40 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=45
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=200 off_x=0 off_y=0 off_z=60 angle=40 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=45
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=6 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=40 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=45
PlayFGM value=cue:se_quickdraw_slash
SyncWait frames=4
ClearAttackCollAll value=0
AsyncWait frames=20
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
