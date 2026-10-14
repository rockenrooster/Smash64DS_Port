# ThrowF: forward throw (card: a telekinetic toss forward): Peach's clip,
# timing and throw parameters (her victims' thrown rows pair with them),
# made telekinetic: psionic glows at her hands while she holds the victim,
# her throw sound with Peach's flash and dust, the release at frame 23.
SetThrow -> ram:0x80595740
PlaySmashVoice value=0
PlayFGM value=18
AsyncWait frames=4
SetHitStatusAll value=2
Effect joint_id=17 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
Effect joint_id=10 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
Effect joint_id=17 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
Effect joint_id=10 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=15
PlayFGM value=cue:se_throw
Effect joint_id=0 effect_id=31 flag=0 off_x=0 off_y=256 off_z=192 rng_x=0 rng_y=0 rng_z=0
Effect joint_id=0 effect_id=19 flag=0 off_x=0 off_y=0 off_z=-150 rng_x=0 rng_y=0 rng_z=0
Effect joint_id=-1 effect_id=33 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
Effect joint_id=17 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
Effect joint_id=10 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=23
SetFlag2 value=1
SetSlopeContour flags=1
SetHitStatusAll value=1
End value=0
