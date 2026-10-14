# Ike's up tilt (AttackHi3): his own (the owner's 3DS reference, 2026-10-14;
# art/p6/ike/anims/utilt.json): a crouch, a hop with Ragnell raised level
# overhead, a low sweep down to a crouch. Roy's up-tilt hitboxes (joint 18
# the blade, tip and base; 16 the forearm) on its two phases: the rise and
# the overhead hold (frames 9-20, sent upward) and the sweep (22-27, sent
# forward, weaker). Damage is Roy's (Ike's 1.5x is OWNER_DAMAGE_SCALES).
SetSlopeContour flags=3
AsyncWait frames=7
SetAfterImage is_itemswing=17 drawstatus=0
AsyncWait frames=9
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=8 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=250 angle=88 knockback_scale=105 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=30
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=10 can_rebound=1 element=3 size=160 off_x=-50 off_y=0 off_z=50 angle=90 knockback_scale=110 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
PlayFGMStoreInfo value=259
MakeAttackColl attack_id=0 group_id=0 joint_id=16 damage=10 can_rebound=1 element=3 size=160 off_x=0 off_y=0 off_z=0 angle=90 knockback_scale=110 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
SyncWait frames=4
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=175 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=7
ClearAttackCollAll value=0
AsyncWait frames=22
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=7 can_rebound=1 element=3 size=180 off_x=0 off_y=0 off_z=250 angle=40 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=7 can_rebound=1 element=3 size=160 off_x=-50 off_y=0 off_z=50 angle=40 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=30
SyncWait frames=5
ClearAttackCollAll value=0
AsyncWait frames=36
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
