/* Compile BattleShip's original transition translation unit. */
#include <PR/gbi.h>
#include <PR/ultratypes.h>
#include <lb/transition.h>
#include <reloc_data.h>
#include <sys/obj.h>
#include <sys/objhelper.h>
#include <sys/objman.h>
#include <sys/taskman.h>
#include <sys/video.h>

extern void *gSYSchedulerCurrentFramebuffer;
extern void gcSetupCustomDObjs(GObj *gobj, DObjDesc *dobjdesc, DObj **dobjs,
                               u8 tk1, u8 tk2, u8 tk3);
extern void gcAddAnimJointAll(GObj *gobj, AObjEvent32 **anim_joint,
                              f32 start_frame);
extern void gcPlayAnimAll(GObj *gobj);
extern f32 syUtilsTan(f32 angle);
extern s32 syUtilsRandIntRange(s32 range);

#define _LIBRARY_H_
#define lbTransitionSetupTransition ndsBaseLBTransitionSetupTransition

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-int"
#include "../../decomp/BattleShip-main/decomp/src/lb/lbtransition.c"
#pragma GCC diagnostic pop

#undef lbTransitionSetupTransition

void lbTransitionSetupTransition(void)
{
    static sb32 is_offsets_resolved;
    s32 i;

    if (is_offsets_resolved == FALSE)
    {
        for (i = 0; i < ARRAY_COUNT(dLBTransitionDescs); i++)
        {
            dLBTransitionDescs[i].o_dobjdesc =
                *(const uintptr_t *)dLBTransitionDescs[i].o_dobjdesc;
            dLBTransitionDescs[i].o_anim_joint =
                *(const uintptr_t *)dLBTransitionDescs[i].o_anim_joint;
        }
        is_offsets_resolved = TRUE;
    }
    if (gSYSchedulerCurrentFramebuffer == NULL)
    {
        gSYSchedulerCurrentFramebuffer = &gSYFramebufferSets[0];
    }
    ndsBaseLBTransitionSetupTransition();
}
