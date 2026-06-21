/* Sprite library include file (N64 PR/sp.h compatibility header).
 *
 * Faithful copy of the N64 sprite library header. The imported BattleShip
 * source includes <PR/sp.h> for the Sprite/Bitmap types used by the object
 * and lb-common sprite code. Kept verbatim to match the original ABI; the DS
 * backend does not use the sp* sprite-drawing functions directly (display-list
 * translation is a later milestone). */
#ifndef SSB64_NDS_PR_SP_H
#define SSB64_NDS_PR_SP_H

#include <PR/mbi.h>
#include <PR/gbi.h>
#include <PR/ultratypes.h>

struct bitmap {
    s16 width;       /* Size across to draw in texels (done if 0) */
    s16 width_img;   /* Size across of bitmap in texels (done if 0) */
    s16 s;           /* Horizontal offset into bitmap */
    s16 t;           /* Vertical offset into base */
    void *buf;       /* Pointer to bitmap data */
    s16 actualHeight;/* True height of this bitmap piece */
    s16 LUToffset;   /* LUT base index */
};

typedef struct bitmap Bitmap;

struct sprite {
    s16 x, y;              /* Target position */
    s16 width, height;     /* Target size */
    f32 scalex, scaley;    /* Texel to pixel scale factor */
    s16 expx, expy;        /* Explosion spacing */
    u16 attr;              /* Attribute flags */
    s16 zdepth;            /* Z depth */
    u8 red;                /* Red component */
    u8 green;              /* Green component */
    u8 blue;               /* Blue component */
    u8 alpha;              /* Alpha component */
    s16 startTLUT;         /* Lookup table entry starting index */
    s16 nTLUT;             /* Total number of lookup table entries */
    int *LUT;              /* Pointer to lookup table */
    s16 istart;            /* Starting bitmap index */
    s16 istep;             /* Bitmap index step */
    s16 nbitmaps;          /* Total number of bitmaps */
    s16 ndisplist;         /* Total number of display-list words */
    s16 bmheight;          /* Bitmap texel height (used) */
    s16 bmHreal;           /* Bitmap texel height (real) */
    u8 bmfmt;              /* Bitmap format */
    u8 bmsiz;              /* Bitmap texel size */
    Bitmap *bitmap;        /* Pointer to first bitmap */
    Gfx *rsp_dl;           /* Pointer to RSP display list */
    Gfx *rsp_dl_next;      /* Pointer to next RSP display entry */
    s16 frac_s, frac_t;    /* Fractional texture offsets (5 fraction bits) */
};

typedef struct sprite Sprite;

/* For sprite->attr */
#define SP_TRANSPARENT 0x00000001
#define SP_CUTOUT      0x00000002
#define SP_HIDDEN      0x00000004
#define SP_Z           0x00000008
#define SP_SCALE       0x00000010
#define SP_FASTCOPY    0x00000020
#define SP_OVERLAP     0x00000040
#define SP_TEXSHIFT    0x00000080
#define SP_FRACPOS     0x00000100
#define SP_TEXSHUF     0x00000200
#define SP_EXTERN      0x00000400
#define SP_ARGUMENT    0x00000800
#define SP_CLOUD       0x00001000

#endif /* SSB64_NDS_PR_SP_H */
