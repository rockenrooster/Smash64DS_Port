# Appeal: down taunt (card: lets go of the staff, brushes her hair back,
# retakes the staff): the staff stood upright on the floor and let go,
# her hand brushing her hair back (art/p6/palutena/anims/appeal.json, key
# poses on the owner's 3DS reference, 2026-10-14), a sparkle at her head
# (owner p6i: no burst; her halo removed, owner 2026-10-17); her voice
# and swing cues where Peach's script has its own. Owner p6x: her line "Ready
# when you are" (appeal02: 1.2 s, five syllables, a voiced R onset; appeal01
# has an early S).
SetSlopeContour flags=3
MakeRumble length=0 rumble_id=6
PlayVoiceStoreInfo value=cue:appeal02
SyncWait frames=12
Effect joint_id=12 effect_id=31 flag=0 off_x=0 off_y=120 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayFGMStoreInfo value=cue:se_appeal_l01
SyncWait frames=34
SyncWait frames=2
PlayFGMStoreInfo value=cue:se_swing_m
SyncWait frames=30
AsyncWait frames=60
SetFlag1 value=1
End value=0
