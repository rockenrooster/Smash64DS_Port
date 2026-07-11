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

#endif
