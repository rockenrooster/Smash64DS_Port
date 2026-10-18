# Win2 (Results pose 2): Peach's second victory clip and its script (eyes
# closed, her win cue where Peach's voice was, then open); no halo (owner
# 2026-10-17: removed from the game).
SetTexturePartID texturepart_id=0 frame=3
SetTexturePartID texturepart_id=1 frame=3
AsyncWait frames=38
PlayVoiceStoreInfo value=cue:win02
AsyncWait frames=80
SetTexturePartID texturepart_id=0 frame=2
SetTexturePartID texturepart_id=1 frame=2
AsyncWait frames=82
SetTexturePartID texturepart_id=0 frame=4
SetTexturePartID texturepart_id=1 frame=4
End value=0
