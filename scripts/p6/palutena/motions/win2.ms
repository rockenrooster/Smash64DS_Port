# Win2 (Results pose 2): Peach's second victory clip and its script (eyes
# closed, her win cue where Peach's voice was, then open), her halo (the
# head row's part 2) above her.
SetModelPartID joint_id=12 modelpart_id=2
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
