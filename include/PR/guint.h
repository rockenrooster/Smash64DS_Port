#ifndef SSB64_NDS_GUINT_H
#define SSB64_NDS_GUINT_H

#include <math.h>
#include <PR/gu.h>
#include <PR/mbi.h>

typedef union
{
    struct
    {
        unsigned int hi;
        unsigned int lo;
    } word;
    double d;
} du;

typedef union
{
    unsigned int i;
    float f;
} fu;

typedef float Matrix[4][4];

#define ROUND(d) (int)(((d) >= 0.0) ? ((d) + 0.5) : ((d)-0.5))
#define ABS(d) (((d) > 0) ? (d) : -(d))

extern float __libm_qnan_f;

#endif
