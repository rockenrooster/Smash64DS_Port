# Win3 (Results pose 3): Ultimate's third victory (j02win3; owner p6ac,
# 2026-10-17, as win1.ms), its sound script (sound_win3): her win cue at 35
# (its sound at 21 and appeal_s02 at 67 are not among hers); her blink as
# Ultimate's visibility track (half 110, shut 111, half 116, open 119).
SetModelPartID joint_id=6 modelpart_id=0
SetTexturePartID texturepart_id=0 frame=0
SetTexturePartID texturepart_id=1 frame=0
AsyncWait frames=35
PlayVoiceStoreInfo value=cue:win03
AsyncWait frames=110
SetTexturePartID texturepart_id=0 frame=2
SetTexturePartID texturepart_id=1 frame=2
AsyncWait frames=111
SetTexturePartID texturepart_id=0 frame=3
SetTexturePartID texturepart_id=1 frame=3
AsyncWait frames=116
SetTexturePartID texturepart_id=0 frame=2
SetTexturePartID texturepart_id=1 frame=2
AsyncWait frames=119
SetTexturePartID texturepart_id=0 frame=0
SetTexturePartID texturepart_id=1 frame=0
End value=0
