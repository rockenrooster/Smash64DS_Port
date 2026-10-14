# Catch: grab (card: a telekinetic clutch; Peach's grab box, reach +20%):
# Peach's grab clip, script and catch parameters, the box pushed out (its
# far edge from 504 to 604 units ahead) with a psionic flash where it
# closes.
SetThrow -> ram:0x805956bc
SetSlopeContour flags=3
Effect joint_id=0 effect_id=11 flag=0 off_x=0 off_y=0 off_z=-120 rng_x=0 rng_y=60 rng_z=0
AsyncWait frames=6
MakeAttackColl attack_id=0 group_id=0 joint_id=0 damage=1 can_rebound=1 element=0 size=340 off_x=0 off_y=288 off_z=264 angle=361 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=0 knockback_base=0
Effect joint_id=0 effect_id=41 flag=0 off_x=0 off_y=288 off_z=400 rng_x=0 rng_y=0 rng_z=0
PlayFGM value=19
SyncWait frames=1
ClearAttackCollAll value=0
End value=0
