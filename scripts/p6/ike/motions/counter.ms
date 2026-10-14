# The taunt's counter: Marth's window (flag0, frames 6 to 30) and flash.
AsyncWait frames=6
Effect joint_id=18 effect_id=31 flag=0 off_x=0 off_y=0 off_z=350 rng_x=0 rng_y=0 rng_z=0
SetColAnim colanim_id=97 length=25
PlayFGM value=cue:se_counter
SetFlag0 value=1
SyncWait frames=24
SetFlag0 value=0
End value=0
