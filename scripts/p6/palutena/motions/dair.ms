# AttackAirLw: down air, Ultimate's (c05attackairlw): the downward kick, her
# left leg (the slit's) driven down with a burst of light at the foot, at 7-8
# (Ultimate's 10-11). Ultimate's attack data (the owner's
# palutena_attack_data.json, p6r): 11%, 55, growth 100, base 30 (Ultimate's
# sends, not a spike). Owner p6v, "wrong SFX": her burst's sound from 4, loud
# from the kick at 7 and no longer cut by the hit's (a stored sound is), and
# the hit's sound a kick's (it was a punch's).
AsyncWait frames=4
PlayFGM value=cue:se_attackair_l01
AsyncWait frames=7
SetFlag1 value=40
PlayVoiceStoreInfo value=cue:attack07
MakeAttackColl attack_id=0 group_id=0 joint_id=22 damage=11 can_rebound=1 element=0 size=300 off_x=0 off_y=0 off_z=0 angle=55 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=1 knockback_base=30
MakeAttackColl attack_id=1 group_id=0 joint_id=21 damage=11 can_rebound=1 element=0 size=220 off_x=0 off_y=0 off_z=0 angle=55 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=1 knockback_base=30
Effect joint_id=22 effect_id=31 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=2
ClearAttackCollAll value=0
AsyncWait frames=22
SetFlag1 value=0
End value=0
