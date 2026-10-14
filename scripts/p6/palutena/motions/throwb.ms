# ThrowB: back throw (card: lifts and flings the victim behind her):
# Peach's clip, timing and throw parameters, made telekinetic as the
# forward throw is: psionic glows at her hands until the release (frame 26:
# Peach's waits, the 13 after the 16 passing at once), her throw sound at
# the fling.
SetThrow -> ram:0x805957dc
SetSlopeContour flags=1
PlaySmashVoice value=0
PlayFGM value=18
AsyncWait frames=4
SetHitStatusAll value=2
Effect joint_id=17 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=7
Effect joint_id=0 effect_id=19 flag=0 off_x=0 off_y=0 off_z=-256 rng_x=0 rng_y=0 rng_z=0
Effect joint_id=10 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
Effect joint_id=17 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=3
Effect joint_id=10 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=16
Effect joint_id=-1 effect_id=33 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
PlayFGM value=cue:se_throw
Effect joint_id=17 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=4
Effect joint_id=10 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=4
Effect joint_id=17 effect_id=41 flag=0 off_x=0 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
AsyncWait frames=26
Effect joint_id=26 effect_id=0 flag=0 off_x=300 off_y=0 off_z=0 rng_x=0 rng_y=0 rng_z=0
ClearAttackCollAll value=0
SetFlag2 value=2
SetHitStatusAll value=1
End value=0
