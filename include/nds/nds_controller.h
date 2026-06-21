#ifndef SSB64_NDS_CONTROLLER_H
#define SSB64_NDS_CONTROLLER_H

#include <PR/ultratypes.h>

extern volatile u32 gNdsControllerPollCount;

int ndsControllerBackendSelfTest(void);

#endif
