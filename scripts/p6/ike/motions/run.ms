# Run: Roy's script on Roy's run (owner p6r: "revert running animation ...
# remove footstep SFX"): his dust at each step, no step sound.
SetSlopeContour flags=3
SyncWait frames=4
Effect joint_id=0 effect_id=11 flag=0 off_x=0 off_y=0 off_z=60 rng_x=0 rng_y=0 rng_z=0
SyncWait frames=10
Effect joint_id=0 effect_id=11 flag=0 off_x=0 off_y=0 off_z=60 rng_x=0 rng_y=0 rng_z=0
PauseScript value=0
Goto -> self
