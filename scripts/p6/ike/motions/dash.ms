# Ike's dash attack (AttackDash): a heavy swing (move card I3). Marth's
# dash-attack timing at 1.15 times its length (the swing from frame 14 to
# 18); Roy's three hitboxes at his damage (Ike's 1.5x power is
# generate_p4_fighter.py OWNER_DAMAGE_SCALES; 1.2x here before). Owner
# playtest 2026-10-14 (p6d): sent away at 30 degrees, more horizontal (80
# before).
Effect joint_id=0 effect_id=16 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=9
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=14
PlayFGMStoreInfo value=258
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=10 can_rebound=1 element=3 size=200 off_x=0 off_y=0 off_z=330 angle=30 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=10
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=15 can_rebound=1 element=3 size=180 off_x=-50 off_y=0 off_z=50 angle=30 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=10
MakeAttackColl attack_id=2 group_id=0 joint_id=15 damage=15 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=0 angle=30 knockback_scale=90 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=20
SyncWait frames=1
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=175 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=4
ClearAttackCollAll value=0
AsyncWait frames=25
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
