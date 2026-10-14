# Explosive Flame (card): the cast, then three 1% bursts and a 15% blast at
# the distance the stick picked (nds_p4_palutena.c keeps every TopN hitbox
# there, shows a twinkle on the spot at flag2 and draws the fire at flag0
# and flag1). Not a projectile: nothing reflects it. Owner playtest
# 2026-10-14: her "Explosive flame!" call (Ultimate's side-special voice),
# the twinkle six frames before the first burst, the hitboxes twice the size.
AsyncWait frames=4
PlayVoiceStoreInfo value=cue:special_s01
Effect joint_id=18 effect_id=31 flag=0 off_x=35 off_y=146 off_z=-160 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=12
SetFlag2 value=1
AsyncWait frames=18
PlayFGM value=cue:se_special_n01
SetFlag0 value=1
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=1 can_rebound=0 element=1 size=640 off_x=0 off_y=320 off_z=1000 angle=90 knockback_scale=0 knockback_weight=30 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=1 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
SyncWait frames=3
SetFlag0 value=1
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=1 can_rebound=0 element=1 size=640 off_x=0 off_y=320 off_z=1000 angle=90 knockback_scale=0 knockback_weight=30 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=1 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
SyncWait frames=3
SetFlag0 value=1
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=1 can_rebound=0 element=1 size=640 off_x=0 off_y=320 off_z=1000 angle=90 knockback_scale=0 knockback_weight=30 is_hit_ground_air=3 shield_damage=0 fgm_level=0 fgm_kind=1 knockback_base=0
SyncWait frames=2
ClearAttackCollAll value=0
SyncWait frames=4
PlayFGM value=cue:se_special_n02
SetFlag1 value=1
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=15 can_rebound=0 element=1 size=860 off_x=0 off_y=320 off_z=1000 angle=361 knockback_scale=95 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=1 knockback_base=40
SyncWait frames=3
ClearAttackCollAll value=0
End value=0
