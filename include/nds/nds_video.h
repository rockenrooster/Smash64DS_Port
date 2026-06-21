#ifndef SSB64_NDS_VIDEO_BOOTSTRAP_H
#define SSB64_NDS_VIDEO_BOOTSTRAP_H

#include <PR/ultratypes.h>

#define NDS_VIDEO_BOOTSTRAP_PASS 0x56494430u

extern volatile u32 gNdsVideoBootstrapResult;

void ndsVideoBootstrapStart(void);
void ndsVideoBootstrapUpdate(void);

#endif

