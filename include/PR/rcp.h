#ifndef SSB64_NDS_RCP_H
#define SSB64_NDS_RCP_H

#include <PR/ultratypes.h>

#define SP_DMEM_START 0x04000000u
#define SP_IMEM_START 0x04001000u

#define VI_CTRL_TYPE_16            0x00002u
#define VI_CTRL_TYPE_32            0x00003u
#define VI_CTRL_GAMMA_DITHER_ON    0x00004u
#define VI_CTRL_GAMMA_ON           0x00008u
#define VI_CTRL_DIVOT_ON           0x00010u
#define VI_CTRL_SERRATE_ON         0x00040u
#define VI_CTRL_ANTIALIAS_MASK     0x00300u
#define VI_CTRL_ANTIALIAS_MODE_1   0x00100u
#define VI_CTRL_ANTIALIAS_MODE_2   0x00200u
#define VI_CTRL_ANTIALIAS_MODE_3   0x00300u
#define VI_CTRL_DITHER_FILTER_ON   0x10000u

u32 ndsN64IoRead(u32 address);
#define IO_READ(address) ndsN64IoRead((u32)(address))

#endif
