/*
 * P2-3f5. Falcon Dive's VICTIM side.
 *
 * The source keeps this in ft/ftcommon, not in ft/ftchar/ftcaptain, and that is
 * the right ownership: `nFTCommonStatusCaptureCaptain` is a COMMON status, so
 * the fighter that ends up in it is any fighter Falcon grabbed, running common
 * callbacks the shared status table already names.  P2-3r10 counted this as one
 * of the fifteen absent ftcommon TUs; Falcon is simply the first caller.
 *
 * The grabber half is battleship_captain.c (ftcaptainspecialhi.c).
 */
#include <ft/fighter.h>
#include <macros.h>

/* BattleShip ftcommon.h:314-318.  The broad decomp header is intentionally not
 * part of the port ABI mirror, so restate the five constants this TU consumes
 * beside the code that consumes them. */
#define FTCOMMON_CAPTURECAPTAIN_MASK_THROW (1 << 1)
#define FTCOMMON_CAPTURECAPTAIN_MASK_NOUPDATE (1 << 2)
#define FTCOMMON_CAPTURECAPTAIN_JOINT 29
#define FTCOMMON_CAPTURECAPTAIN_FRAME_BEGIN 4.0F
#define FTCOMMON_CAPTURECAPTAIN_ANIM_SPEED 0.0F

#ifndef DObjGetStruct
#define DObjGetStruct(gobj) ((DObj *)((gobj)->obj))
#endif

/* decomp src/sys/vector.h; vector.c is compiled from the overlay. */
extern f32 syVectorNorm3D(Vec3f *dst);
extern f32 syVectorMag3D(Vec3f *src);
extern Vec3f *syVectorAdd3D(Vec3f *dst, Vec3f *add);
extern Vec3f *syVectorSub3D(Vec3f *dst, Vec3f *sub);

/*
 * THE VICTIM TETHER IS A RELOC TABLE READ, not a constant.
 *
 * `dCaptainMainMotion_0x0000` (decomp relocData/235_CaptainMainMotion.c:38) is
 * a 27-entry `Vec2h` array -- one grab offset per victim FTKind -- and the
 * source reaches it through the reloc symbol below at file offset 0x0
 * (tools/reloc_data_symbols.us.txt:3862, and the JP table agrees).  The port's
 * reloc backend resolves `ll*` symbols by taking the ADDRESS of a uintptr_t
 * holding the file offset, which is why this is a variable and not a #define
 * (see llFoxMainMotionLwReflectorFTSpecialColl in battleship_fox_reflector.c).
 */
uintptr_t llCaptainMainMotionSpecialHiVec2h = 0x0u;

void ftCommonCaptureCaptainRelease(GObj *fighter_gobj);
void ftCommonCaptureCaptainProcPhysics(GObj *fighter_gobj);
void ftCommonCaptureCaptainUpdatePositions(GObj *fighter_gobj,
                                           GObj *capture_gobj, Vec3f *pos);

#include "../../decomp/BattleShip-main/decomp/src/ft/ftcommon/ftcommoncapturecaptain.c"
