#ifndef SSB64_NDS_LB_TRANSITION_H
#define SSB64_NDS_LB_TRANSITION_H

#include <ssb_types.h>
#include <sys/obj.h>

typedef struct LBTransitionDesc
{
    u32 file_id;
    intptr_t o_dobjdesc;
    intptr_t o_anim_joint;
    s32 unk_lbtransition_0xC;
} LBTransitionDesc;

extern LBTransitionDesc dLBTransitionDescs[11];

GObj *lbTransitionMakeCamera(u32 id, s32 link, u32 link_priority,
                             u64 camera_mask);
void lbTransitionProcDisplay(GObj *gobj);
void lbTransitionProcUpdate(GObj *gobj);
GObj *lbTransitionMakeTransition(s32 transition_id, u32 id, s32 link,
                                 void (*proc_display)(GObj *), u8 dl_link_id,
                                 void (*proc_update)(GObj *));
void lbTransitionSetupTransition(void);

#endif
