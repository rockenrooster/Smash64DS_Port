# Quick Draw's dash: a hit on the sword and the body that holds the foe
# (fixed knockback) while the code cuts to the slash; the charge's blue glow
# goes on, with the large dash dust and the dash's whoosh.
Effect joint_id=0 effect_id=20 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SetColAnim colanim_id=126 length=0
SetAfterImage is_itemswing=17 drawstatus=0
PlayFGM value=cue:se_quickdraw
PlayVoiceStoreInfo value=cue:special_s01
MakeAttackColl attack_id=0 group_id=0 joint_id=18 damage=4 can_rebound=0 element=3 size=240 off_x=0 off_y=0 off_z=250 angle=361 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
MakeAttackColl attack_id=1 group_id=0 joint_id=6 damage=4 can_rebound=0 element=3 size=260 off_x=0 off_y=0 off_z=0 angle=361 knockback_scale=100 knockback_weight=10 is_hit_ground_air=3 shield_damage=0 fgm_level=1 fgm_kind=5 knockback_base=0
End value=0
