#include <ft/fighter.h>
#include <if/interface.h>
#include <sc/scene.h>
#include <sys/objman.h>
#include <sys/taskman.h>

sb32 ftParamCheckSetColAnimID(GMColAnim *colanim, s32 colanim_id,
                              s32 length);
void ftParamResetColAnim(GMColAnim *colanim);
sb32 ftMainUpdateColAnim(GMColAnim *colanim, GObj *fighter_gobj,
                         sb32 is_muted, sb32 is_effect_skip);

#define ifScreenFlashSetColAnimID ndsBattleShipIFScreenFlashSetColAnimID
#include "../../decomp/BattleShip-main/decomp/src/if/ifscreenflash.c"
#undef ifScreenFlashSetColAnimID
