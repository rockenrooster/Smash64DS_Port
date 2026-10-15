# Quick Draw's slash on contact (d01specialsattack). Ultimate's frames
# (ultimateframedata, its frame N our N - 1) and the owner's Ultimate attack
# data (ike_attack_data.json: damage, angle, growth, set and base knockback;
# owner p6r): 6% and up to 7 more by charge (nds_p4_ike.c; Ultimate's 6-13),
# 60, growth 88, base 70 (owner p6r: "it hardly knocks the enemy back"), at
# 1-2.
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=1
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=420 angle=60 knockback_scale=88 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=200 angle=60 knockback_scale=88 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=6 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=60 knockback_scale=88 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
PlayFGM value=cue:se_quickdraw_slash
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=20
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
