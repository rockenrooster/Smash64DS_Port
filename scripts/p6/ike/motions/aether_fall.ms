# Aether's plunge (d02specialhi3), held while he falls: the falling hit, 3%
# (280, set knockback 40: they come down with him; Ultimate's frames
# (ultimateframedata, its frame N our N - 1) and the owner's Ultimate attack
# data (ike_attack_data.json: damage, angle, growth, set and base knockback;
# owner p6r)).
SetAfterImage is_itemswing=17 drawstatus=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=3 can_rebound=1 element=3 size=260 off_x=0 off_y=0 off_z=380 angle=280 knockback_scale=100 knockback_weight=40 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=3 can_rebound=1 element=3 size=240 off_x=0 off_y=0 off_z=120 angle=280 knockback_scale=100 knockback_weight=40 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
End value=0
