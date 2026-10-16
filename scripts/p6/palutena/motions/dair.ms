# AttackAirLw: down air, Ultimate's (c05attackairlw): the downward kick, her
# left leg (the slit's) driven down with a burst of light at the foot, at 7-8
# (Ultimate's 10-11). Ultimate's attack data (the owner's
# palutena_attack_data.json, p6r): 11%, 55, growth 100, base 30 (Ultimate's
# sends, not a spike). Owner p6v, "wrong SFX": her burst's sound from 4, loud
# from the kick at 7 and no longer cut by the hit's (a stored sound is), and
# the hit's sound a kick's (it was a punch's).
# Owner p6w (the sparkle "no longer directly under palutena 100% of the
# time ... like the hitbox moves around"): the kick's hits and its sparkle
# on TopN (joint 0) where her foot meets them (115 below, 30 ahead), as
# Ultimate's are on its top bone, not on the swinging foot and knee (the
# foot's depth toward or away from the camera flips with her facing); the
# hits 1.25x the size (300 to 375, the knee's 220 to 275).
# Owner p6x: "still hard to hit ... meant to be a wide downwards sweeping
# kick (wide U shape)": the hits follow the kick again, on the foot (22)
# and the mid shin (21, 57 along it), 6-11 as the foot sweeps down from
# ahead, under her and up behind (the game tests each hit along its move
# from the frame before, so the six frames sweep the U; her hits lie in
# her plane, planar_hits, so the foot's depth does not move them); the
# sparkle stays under her on TopN; a white trail on the shin (sword
# trail 23, spec sword_trails) 5-13.
# Owner p6x follow-up (the hits drawn: frames 7-11 sat under her feet, the
# swing up behind her had none): the hits run to 13, 1.25x the size (foot
# 375 to 469, shin 275 to 344) and 50 units up the leg toward her body (the
# foot's along its -y, the ankle-to-knee line; the shin's 57 to 7 from the
# knee), so they cover her body more.
AsyncWait frames=4
PlayFGM value=cue:se_attackair_l01
AsyncWait frames=5
SetAfterImage is_itemswing=23 drawstatus=0
AsyncWait frames=6
SetFlag1 value=40
PlayVoiceStoreInfo value=cue:attack07
MakeAttackColl attack_id=0 group_id=0 joint_id=22 damage=11 can_rebound=1 element=0 size=469 off_x=0 off_y=-50 off_z=0 angle=55 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=1 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=21 damage=11 can_rebound=1 element=0 size=344 off_x=7 off_y=0 off_z=0 angle=55 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=1 knockback_base=30
AsyncWait frames=7
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=-115 off_z=30 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=14
ClearAttackCollAll value=0
SetAfterImage is_itemswing=0 drawstatus=-1
AsyncWait frames=22
SetFlag1 value=0
End value=0
