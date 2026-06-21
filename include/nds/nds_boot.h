#ifndef SSB64_NDS_BOOT_H
#define SSB64_NDS_BOOT_H

#include <PR/ultratypes.h>

#define NDS_BOOT_SCHEDULER_READY (1u << 0)
#define NDS_BOOT_AUDIO_READY      (1u << 1)
#define NDS_BOOT_CONTROLLER_READY (1u << 2)
#define NDS_BOOT_SCENE_REACHED    0x53430000u
#define NDS_BOOT_EXPECTED         (NDS_BOOT_SCENE_REACHED | 7u)

extern volatile u32 gNdsOriginalBootStage;

#endif

