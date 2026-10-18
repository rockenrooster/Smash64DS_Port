# Win2 (Results pose 2): Ultimate's second victory (j02win2; owner p6ac,
# 2026-10-17, as win1.ms), its sound script (sound_win2a): her win cue at
# 38, the staff's whirl at 48, its strike at 87, her landing at 112. Eyes
# open throughout, as Ultimate's.
SetModelPartID joint_id=6 modelpart_id=0
SetTexturePartID texturepart_id=0 frame=0
SetTexturePartID texturepart_id=1 frame=0
AsyncWait frames=38
PlayVoiceStoreInfo value=cue:win02
AsyncWait frames=48
PlayFGM value=cue:se_attackair_n01
AsyncWait frames=87
PlayFGM value=cue:se_attackhard_h01
AsyncWait frames=112
PlayFGM value=cue:se_landing01
End value=0
