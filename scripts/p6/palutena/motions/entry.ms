# AppearR / AppearL: entry (card: walks out of golden halo-shaped doors).
# Her gate -- special file 2, made by her entry case
# (src/port/nds_p4_palutena.c) -- stands behind the stage's plane (380 units
# back, scripts/p6/p6_gate.py FRONT), its doors parting toward the camera
# from tic 10 and the gate shrinking away from tic 84. Ultimate's entry
# (art/p6/palutena/anims/entry.json): she walks out of it toward the camera,
# hidden by its doors until they part, and stops on the plane (owner p6v).
# Her halo (the head row's part 2) shows until the gate shrinks; sparkles at
# the doors as they part, her entry sound and her line; her shadow (flag2) once
# she nears the plane (it is drawn under her entry position).
SetModelPartID joint_id=12 modelpart_id=2
AsyncWait frames=10
PlayFGMStoreInfo value=cue:se_appear01
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=450 off_z=-380 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=22
Effect joint_id=0 effect_id=30 flag=0 off_x=0 off_y=450 off_z=-380 rng_x=0 rng_y=0 rng_z=0
PlayVoiceStoreInfo value=cue:appeal02
AsyncWait frames=84
SetModelPartID joint_id=12 modelpart_id=0
AsyncWait frames=96
SetFlag2 value=1
End value=0
