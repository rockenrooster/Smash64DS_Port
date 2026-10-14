# Aether's landing: a hit as the sword lands (no shockwave, card), then lag.
Effect joint_id=0 effect_id=13 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=6 can_rebound=1 element=3 size=260 off_x=0 off_y=0 off_z=200 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=6 can_rebound=1 element=3 size=260 off_x=0 off_y=0 off_z=0 angle=45 knockback_scale=100 knockback_weight=0 is_hit_ground_air=3 shield_damage=0 fgm_level=2 fgm_kind=5 knockback_base=50
PlayFGM value=cue:se_aether_land
PlayVoiceStoreInfo value=cue:special_h02
SyncWait frames=3
ClearAttackCollAll value=0
AsyncWait frames=12
SetAfterImage is_itemswing=0 drawstatus=-1
End value=0
