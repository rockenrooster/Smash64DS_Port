# Ike's up smash (AttackHi4): one wide arc from in front over his head to
# behind him (move card I3). Marth's up-tilt timing at 1.4 times its length:
# the sword in front and rising from frame 7, overhead and behind from 11
# to 18; Roy's final hit at his damage (13 and 12; Ike's 1.5x power is
# generate_p4_fighter.py OWNER_DAMAGE_SCALES), his launch.
SetSlopeContour flags=3
AsyncWait frames=4
SetAfterImage is_itemswing=17 drawstatus=0
PlaySmashVoice value=0
AsyncWait frames=7
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=13 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=250 angle=100 knockback_scale=125 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=12 can_rebound=1 element=3 size=200 off_x=0 off_y=0 off_z=50 angle=90 knockback_scale=125 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=12 can_rebound=1 element=3 size=160 off_x=0 off_y=0 off_z=0 angle=90 knockback_scale=125 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
PlayFGMStoreInfo value=258
SyncWait frames=4
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=350 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=13 can_rebound=1 element=3 size=220 off_x=0 off_y=0 off_z=250 angle=85 knockback_scale=125 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=12 can_rebound=1 element=3 size=200 off_x=0 off_y=0 off_z=50 angle=80 knockback_scale=125 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
MakeAttackColl attack_id=2 group_id=0 joint_id=16 damage=12 can_rebound=1 element=3 size=160 off_x=0 off_y=0 off_z=0 angle=80 knockback_scale=125 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=40
SyncWait frames=7
ClearAttackCollAll value=0
AsyncWait frames=31
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
