# AppearR / AppearL: entry (card: walks out of golden halo-shaped doors).
# Her gate -- special file 2, made by her entry case
# (src/port/nds_p4_palutena.c) -- stands before her, its doors parting from
# tic 10 and the gate shrinking away from tic 84. Marth's entry clip on her
# tree, the staff where his sword is; her halo (the head row's part 2) shows
# while she stands in the doorway; sparkles as the doors part, her entry
# sound and her line; her shadow from the doors' parting (flag2).
SetModelPartID joint_id=12 modelpart_id=2
AsyncWait frames=10
SetFlag2 value=1
PlayFGMStoreInfo value=cue:se_appear01
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=450 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=22
Effect joint_id=0 effect_id=30 flag=0 off_x=0 off_y=450 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayVoiceStoreInfo value=cue:appeal02
AsyncWait frames=84
SetModelPartID joint_id=12 modelpart_id=0
End value=0
