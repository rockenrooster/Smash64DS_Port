#ifndef SSB64_NDS_SCENE_BACKEND_H
#define SSB64_NDS_SCENE_BACKEND_H

#include <PR/ultratypes.h>

#define NDS_SCENE_BOUNDARY_PASS 0x53434e45u

extern volatile u32 gNdsSceneBoundaryResult;
extern volatile u32 gNdsSceneBoundaryKind;

#endif
