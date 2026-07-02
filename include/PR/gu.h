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

#ifndef FTOFRAC8
#define FTOFRAC8(x) ((int)MIN(((x) * (128.0f)), 127.0f) & 0xff)
#endif

void guNormalize(float *x, float *y, float *z);

#endif
