#ifndef SSB64_NDS_TYPES_H
#define SSB64_NDS_TYPES_H

#include <PR/ultratypes.h>

typedef f32 Mtx44f[4][4];

typedef struct Vec3h { s16 x, y, z; } Vec3h;
typedef struct Vec3i { s32 x, y, z; } Vec3i;
typedef struct Vec3f { f32 x, y, z; } Vec3f;
typedef struct Vec2b { s8 x, y; } Vec2b;
typedef struct Vec2f { f32 x, y; } Vec2f;
typedef struct Vec2h { s16 x, y; } Vec2h;
typedef struct Vec2i { s32 x, y; } Vec2i;

typedef s8 sb8;
typedef s16 sb16;
typedef s32 sb32;
typedef u8 ub8;
typedef u16 ub16;
typedef u32 ub32;

typedef struct SYColorRGB { u8 r, g, b; } SYColorRGB;
typedef struct SYColorRGBA { u8 r, g, b, a; } SYColorRGBA;
typedef struct SYColorRGBPair { SYColorRGB prim, env; } SYColorRGBPair;
typedef union SYColorPack { SYColorRGBA s; u32 pack; } SYColorPack;
typedef struct SYRectangle { s32 ulx, uly, lrx, lry; } SYRectangle;

#endif
