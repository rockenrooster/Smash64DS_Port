# Aether's landing (d02specialhi4): its hit at 0-1. Ultimate's frames
# (ultimateframedata, its frame N our N - 1) and the owner's Ultimate attack
# data (ike_attack_data.json: damage, angle, growth, set and base knockback;
# owner p6r): 6%, 60, growth 115, base 70.
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=260 off_x=0 off_y=0 off_z=375 angle=60 knockback_scale=115 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=6 can_rebound=1 element=3 size=260 off_x=0 off_y=0 off_z=0 angle=60 knockback_scale=115 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=70
PlayFGM value=cue:se_aether_land
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=14
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
