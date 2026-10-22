# Attack11: jab 1, Ultimate's (c00attack11): the staff drawn back and thrust
# ahead. Its hits along the staff (joint 18; owner p6r "jabs ... preventing
# hitting enemies": they were on the shield arm, behind her in this motion), at
# 4-6: its head, its middle, and one just past her grip reaching back to her
# middle (owner 2026-10-22: enemies close to her were missed, the boxes
# beginning about 260 ahead of her middle; one box, not two, for the DS's CPU).
# Ultimate's attack data (the owner's palutena_attack_data.json, p6r): 3%, 361,
# growth 20, base 20. Jab 2 may follow from frame 18.
AsyncWait frames=4
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=3 can_rebound=1 element=0 size=240 off_x=41 off_y=272 off_z=-290 angle=361 knockback_scale=20 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=20
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=3 can_rebound=1 element=0 size=200 off_x=23 off_y=150 off_z=-160 angle=361 knockback_scale=20 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=20
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=3 can_rebound=1 element=0 size=270 off_x=2 off_y=16 off_z=-17 angle=361 knockback_scale=20 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=0 knockback_base=20
PlayFGMStoreInfo value=cue:se_swing_s
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=18
SetFlag1 value=1
End value=0
