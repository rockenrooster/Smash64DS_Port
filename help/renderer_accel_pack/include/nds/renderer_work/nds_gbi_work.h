#ifndef SSB64_NDS_GBI_WORK_H
#define SSB64_NDS_GBI_WORK_H

#include <stddef.h>
#include <stdint.h>
#include <PR/ultratypes.h>
#include <PR/gbi.h>

#define NDS_GBI_OP_VTX              0x01u
#define NDS_GBI_OP_CULLDL           0x03u
#define NDS_GBI_OP_TRI1             0x05u
#define NDS_GBI_OP_TRI2             0x06u
#define NDS_GBI_OP_TEXTURE          0xD7u
#define NDS_GBI_OP_GEOMETRYMODE     0xD9u
#define NDS_GBI_OP_MOVEWORD         0xDBu
#define NDS_GBI_OP_DL               0xDEu
#define NDS_GBI_OP_ENDDL            0xDFu
#define NDS_GBI_OP_SETOTHERMODE_L   0xE2u
#define NDS_GBI_OP_SETOTHERMODE_H   0xE3u
#define NDS_GBI_OP_RDPLOADSYNC      0xE6u
#define NDS_GBI_OP_RDPPIPESYNC      0xE7u
#define NDS_GBI_OP_RDPTILESYNC      0xE8u
#define NDS_GBI_OP_RDPFULLSYNC      0xE9u
#define NDS_GBI_OP_SETSCISSOR       0xEDu
#define NDS_GBI_OP_RDPSETOTHERMODE  0xEFu
#define NDS_GBI_OP_SETTILESIZE      0xF2u
#define NDS_GBI_OP_LOADBLOCK        0xF3u
#define NDS_GBI_OP_SETTILE          0xF5u
#define NDS_GBI_OP_FILLRECT         0xF6u
#define NDS_GBI_OP_SETFILLCOLOR     0xF7u
#define NDS_GBI_OP_SETFOGCOLOR      0xF8u
#define NDS_GBI_OP_SETBLENDCOLOR    0xF9u
#define NDS_GBI_OP_SETPRIMCOLOR     0xFAu
#define NDS_GBI_OP_SETENVCOLOR      0xFBu
#define NDS_GBI_OP_SETCOMBINE       0xFCu
#define NDS_GBI_OP_SETTIMG          0xFDu

#define NDS_GBI_FMT_RGBA 0u
#define NDS_GBI_FMT_YUV  1u
#define NDS_GBI_FMT_CI   2u
#define NDS_GBI_FMT_IA   3u
#define NDS_GBI_FMT_I    4u

#define NDS_GBI_SIZ_4B  0u
#define NDS_GBI_SIZ_8B  1u
#define NDS_GBI_SIZ_16B 2u
#define NDS_GBI_SIZ_32B 3u

static inline u32 ndsGbiOpcode(u32 w0) { return w0 >> 24; }
static inline u32 ndsGbiColorR(u32 w1) { return (w1 >> 24) & 0xFFu; }
static inline u32 ndsGbiColorG(u32 w1) { return (w1 >> 16) & 0xFFu; }
static inline u32 ndsGbiColorB(u32 w1) { return (w1 >> 8) & 0xFFu; }
static inline u32 ndsGbiColorA(u32 w1) { return w1 & 0xFFu; }

#endif
