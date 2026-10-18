# AttackLw3: down tilt, Ultimate's (c02attacklw3): from her crouch the staff
# spun along the floor, at 6-9. Ultimate's attack data (the owner's
# palutena_attack_data.json, p6r): 75, growth 100, base 38; the damage the
# latest patch's (ultimateframedata, owner p6s): 8.5% (9) to a grounded
# opponent, 5% to one in the air -- the staff's head and middle ground-only
# (ids 0-1) and air-only (2-3).
# Owner p6ac (2026-10-17): its staff spin twice as long (art/p6/palutena/
# anims/dtilt.json: Ultimate's 13-28 over clip 6-23), the hit where it was.
# Owner p6x VFX plan (picks 2026-10-16): her staff's solid cyan-white trail
# (sword trail 24, spec sword_trails: her staff's line) while it spins, 5 to
# 22.
AsyncWait frames=5
SetAfterImage is_itemswing=24 drawstatus=0
AsyncWait frames=6
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=9 can_rebound=1 element=0 size=200 off_x=41 off_y=272 off_z=-290 angle=75 knockback_scale=100 knockback_weight=0 is_hit_ground_air=2 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=38
MakeAttackColl attack_id=1 group_id=0 joint_id=18 damage=9 can_rebound=1 element=0 size=170 off_x=23 off_y=150 off_z=-160 angle=75 knockback_scale=100 knockback_weight=0 is_hit_ground_air=2 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=38
MakeAttackColl attack_id=2 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=200 off_x=41 off_y=272 off_z=-290 angle=75 knockback_scale=100 knockback_weight=0 is_hit_ground_air=1 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=38
MakeAttackColl attack_id=3 group_id=0 joint_id=18 damage=5 can_rebound=1 element=0 size=170 off_x=23 off_y=150 off_z=-160 angle=75 knockback_scale=100 knockback_weight=0 is_hit_ground_air=1 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=38
PlayFGMStoreInfo value=cue:se_attackhard_l01
SyncWait frames=4
ClearAttackCollAll value=0
AsyncWait frames=22
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
