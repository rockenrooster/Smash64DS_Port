#ifndef SSB64_NDS_FIGHTER_DISPLAY_H
#define SSB64_NDS_FIGHTER_DISPLAY_H

#include <PR/gbi.h>
#include <PR/ultratypes.h>

struct GObj;
struct CObj;

void ndsFighterDisplayContractSetGeometryMode(u32 clear_mask, u32 set_mask);
void ndsFighterDisplayContractSetCycleType(u32 cycle_type);
void ndsFighterDisplayContractSetRenderMode(u32 mode1, u32 mode2);
void ndsFighterDisplayContractSetEnvColor(u8 r, u8 g, u8 b, u8 a);
void ndsFighterDisplayContractSetPrimColor(u8 r, u8 g, u8 b, u8 a);
void ndsFighterDisplayContractSetLightCount(u32 count);
void ndsFighterDisplayContractSetLight(const Light *light, u32 slot);
void ndsFighterDisplayContractResetSceneLight(void);
void ndsFighterDisplayContractSelectDL(const Gfx *dl);
u8 ndsFighterDisplayContractSetStageEnvColor(Gfx **dls);
sb32 ndsFighterDisplayContractCheckTargetInBounds(f32 pos_x, f32 pos_y);
void ndsFighterDisplayContractProjectTarget(struct CObj *cobj,
                                            Mtx44f matrix,
                                            Vec3f *pos,
                                            f32 *dist_x,
                                            f32 *dist_y);
void ndsFighterDisplayContractSubmit(struct GObj *fighter_gobj);

/* The fighter draw-contract memo's head boundary; see the block comment above
 * ndsFighterDisplayContractCapture in src/port/reloc_backend_renderer_dl.c.
 * Called once per capture from the import shim's gDPSetFogColor, which is
 * ftDisplayMainProcDisplay's last contract-visible action before the walk.
 * display_mode_master is supplied by the caller because the enum is not
 * visible in the renderer translation unit. */
void ndsFighterDisplayContractHeadBoundary(u32 sky_fog_alpha,
                                           u32 is_shade_fog,
                                           u32 display_mode_master);

/* On a memo hit, SkipRoot holds the live fighter root DObj and the shim's
 * DObjGetStruct substitutes the empty StubRoot, which collapses the walk
 * without writing anything in the live tree. Disarmed, SkipRoot equals
 * StubRoot, so the compare can never match a live root. void* so this header
 * does not need DObj to be complete. */
extern void *gNdsFtrDrawMemoSkipRoot;
extern void *gNdsFtrDrawMemoStubRoot;

#endif
