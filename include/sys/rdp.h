#ifndef SSB64_NDS_SYS_RDP_H
#define SSB64_NDS_SYS_RDP_H

#include <sys/objdef.h>
#include <sys/objtypes.h>

void syRdpSetViewport(Vp *viewport, f32 ulx, f32 uly, f32 lrx, f32 lry);
void syRdpSetDefaultViewport(Vp *vp);
void syRdpSetFuncLights(void (*func_lights)(Gfx**));
void syRdpResetSettings(Gfx **dl);

extern Vp gSYRdpViewport;

#endif
