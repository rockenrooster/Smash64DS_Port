# Win1 (Results pose 1): Ultimate's first victory (j02win1; owner p6ac,
# 2026-10-17: the pose was Peach's clip, her wings out and motionless --
# Ultimate's wins have no wings, so none here), its sound script
# (sound_win1a): her win cue at 15, her hop at 23 (Ultimate's appeal_s02 at
# 33 is not among her sounds). Eyes open throughout, as Ultimate's.
SetModelPartID joint_id=6 modelpart_id=0
SetTexturePartID texturepart_id=0 frame=0
SetTexturePartID texturepart_id=1 frame=0
AsyncWait frames=15
PlayVoiceStoreInfo value=cue:win01
AsyncWait frames=23
PlayFGM value=cue:se_jump02
End value=0
