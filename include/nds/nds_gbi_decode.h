#ifndef SSB64_NDS_GBI_DECODE_H
#define SSB64_NDS_GBI_DECODE_H

#include <PR/ultratypes.h>

#define NDS_GBI_F3DEX2_TRI_PACKED_MASK 0x00ffffffu

static inline s32 ndsGBIDecodeF3DEX2Vtx(u32 w0, u32 max_vtx,
                                        u32 *out_v0, u32 *out_count)
{
    u32 count = (w0 >> 12) & 0xffu;
    u32 end = (w0 >> 1) & 0x7fu;

    if ((out_v0 == NULL) || (out_count == NULL))
    {
        return 0;
    }
    if ((count == 0u) || (count > max_vtx))
    {
        return 0;
    }
    if ((end < count) || (end > max_vtx))
    {
        return 0;
    }
    *out_v0 = end - count;
    *out_count = count;
    return 1;
}

static inline u32 ndsGBIDecodeF3DEX2Tri1(u32 w0)
{
    return w0 & NDS_GBI_F3DEX2_TRI_PACKED_MASK;
}

static inline u32 ndsGBIDecodeF3DEX2Tri2First(u32 w0)
{
    return w0 & NDS_GBI_F3DEX2_TRI_PACKED_MASK;
}

static inline u32 ndsGBIDecodeF3DEX2Tri2Second(u32 w1)
{
    return w1 & NDS_GBI_F3DEX2_TRI_PACKED_MASK;
}

static inline void ndsGBIDecodePackedTriIndices(u32 packed, u32 *out_i0,
                                                u32 *out_i1, u32 *out_i2)
{
    if (out_i0 != NULL)
    {
        *out_i0 = ((packed >> 16) & 0xffu) / 2u;
    }
    if (out_i1 != NULL)
    {
        *out_i1 = ((packed >> 8) & 0xffu) / 2u;
    }
    if (out_i2 != NULL)
    {
        *out_i2 = (packed & 0xffu) / 2u;
    }
}

#endif
