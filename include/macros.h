#ifndef SSB64_NDS_MACROS_H
#define SSB64_NDS_MACROS_H

#include <libc/math.h>
#include <PR/ultratypes.h>
#include <ssb_types.h>

#define SQUARE(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define ABSF(x) ((x) < 0.0F ? -(x) : (x))
#define TAKE_MAX(dst, src) do { if ((dst) < (src)) { (dst) = (src); } } while (0)
#define TAKE_MIN(dst, src) do { if ((dst) > (src)) { (dst) = (src); } } while (0)
#ifndef ARRAY_COUNT
#define ARRAY_COUNT(a) ((s32)(sizeof(a) / sizeof((a)[0])))
#endif
#define U8_MAX 0xFF
#define M_PI_F ((f32)M_PI)
#define M_DTOR_F(x) ((f32)((x) * (f32)M_DTOR))
#define F_CST_DTOR32(x) ((f32)((x) * (M_PI / 180.0)))
#define F_CLC_DTOR32(x) ((f32)(((x) * M_PI) / 180.0))
#define F_PCT_TO_DEC(x) ((f32)((x) * 0.01F))
#define PHYSICAL_TO_ROM(x) ((uintptr_t)(x) + 0xB0000000u)
#define TIME_SEC 60
#define TIME_MIN (TIME_SEC * 60)
#define I_SEC_TO_TICS(q) ((int)((q) * TIME_SEC))
#define I_MIN_TO_TICS(q) ((int)((q) * TIME_MIN))

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

/* GC_* bitfield helpers from BattleShip's macros.h. Required by the original
 * sys/objdef.h AObj event macros (aobjEvent32*), which the imported
 * task/object manager source compiles against. Mirrored verbatim so the port
 * does not diverge from the real ABI. */
#define GC_BITMASK(len) ((len) >= 32 ? 0xFFFFFFFFu : ((1u << (len)) - 1u))
#define GC_FIELDMASK(start, len) (GC_BITMASK(len) << (start))
#define GC_FIELDPREP(x, start, len) (((x) & GC_BITMASK(len)) << (start))
#define GC_FIELDSET(x, start, len) \
    (0 & ~GC_FIELDMASK(start, len) | GC_FIELDPREP(x, start, len))

/* From BattleShip macros.h: used by GOBJ_PRIORITY_DEFAULT (sys/objtypes.h). */
#define S32_MIN 0x80000000

/* From BattleShip macros.h: float sentinels used by the AObj animation system
 * (sys/objtypes.h, objanim.c). */
#define F32_MAX 3.40282346639e38F
#define F32_MIN (-F32_MAX)
#define F32_HALF (F32_MAX / 2)

#endif
