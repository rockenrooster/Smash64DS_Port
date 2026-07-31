#ifndef SSB64_NDS_GU_H
#define SSB64_NDS_GU_H

#include <math.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef FTOFIX32
#define FTOFIX32(x) (long)((x) * (float)0x00010000)
#endif

/* The inverse of FTOFIX32, spelled exactly as libultra's gu.h:22 spells it.
 * guMtxL2F needs it; it was missing here because no port translation unit had
 * compiled that function before. */
#ifndef FIX32TOF
#define FIX32TOF(x) ((float)(x) * (1.0f / (float)0x00010000))
#endif

#ifndef FTOFRAC8
#define FTOFRAC8(x) ((int)MIN(((x) * (128.0f)), 127.0f) & 0xff)
#endif

void guNormalize(float *x, float *y, float *z);

/* Defined by decomp/.../libultra/gu/mtxcatf.c, which the build compiles, but
 * declared nowhere until now -- so every translation unit that called them
 * wrote its own extern (battleship_wpmanager_core.c, battleship_gmcamera.c) and
 * lb/lbparticle.c, which cannot, got an implicit declaration instead.
 * Signatures match libultra's own gu.h. */
void guMtxIdentF(float mf[4][4]);
void guMtxCatF(float mf[4][4], float nf[4][4], float res[4][4]);

#endif
