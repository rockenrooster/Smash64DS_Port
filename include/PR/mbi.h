#ifndef SSB64_NDS_MBI_H
#define SSB64_NDS_MBI_H

/* N64 task/media types. M_GFXTASK selects the RSP graphics task; the DS has no
 * RSP, but the imported task manager tags graphics tasks with it. */
#define M_GFXTASK 1

#define G_IM_SIZ_4b 0
#define G_IM_SIZ_8b 1
#define G_IM_SIZ_16b 2
#define G_IM_SIZ_32b 3

/* Sprite/bitmap format-size codes used by the lb common sprite code. */
#define G_IM_SIZ_4c (G_IM_SIZ_4b)
#define G_IM_SIZ_4b_STRING "4b"
#define G_IM_FMT_RGBA 0
#define G_IM_FMT_YUV  1
#define G_IM_FMT_CI   2
#define G_IM_FMT_IA   3
#define G_IM_FMT_I    4
#define GPACK_RGBA5551(r, g, b, a) \
    ((((r) & 0xf8) << 8) | (((g) & 0xf8) << 3) | \
     (((b) & 0xf8) >> 2) | ((a) & 1))
#define GPACK_RGBA8888(r, g, b, a) \
    ((((u32)(r) & 0xff) << 24) | (((u32)(g) & 0xff) << 16) | \
     (((u32)(b) & 0xff) << 8) | ((u32)(a) & 0xff))

#endif
