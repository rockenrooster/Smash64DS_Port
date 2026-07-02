#ifndef SSB64_NDS_FTCHAR_COMPAT_STATUS_H
#define SSB64_NDS_FTCHAR_COMPAT_STATUS_H

#include <ft/fighter.h>

#define NDS_FT_STATUS_ENTRY(motion_, motion_attack_, ga_, projectile_, \
                            status_attack_, update_, interrupt_, physics_, \
                            map_) \
    { \
        { (motion_), (motion_attack_) }, \
        { 0, FALSE, (ga_), (projectile_), (status_attack_) }, \
        (update_), (interrupt_), (physics_), (map_) \
    }

#define NDS_FT_STATUS_STUB \
    NDS_FT_STATUS_ENTRY(nFTCommonMotionNull, nFTMotionAttackIDNone, \
                        nMPKineticsGround, FALSE, nFTStatusAttackIDNone, \
                        NULL, NULL, NULL, NULL)

#define NDS_FT_STATUS_STUB4 \
    NDS_FT_STATUS_STUB, NDS_FT_STATUS_STUB, \
    NDS_FT_STATUS_STUB, NDS_FT_STATUS_STUB

#define NDS_FT_STATUS_STUB16 \
    NDS_FT_STATUS_STUB4, NDS_FT_STATUS_STUB4, \
    NDS_FT_STATUS_STUB4, NDS_FT_STATUS_STUB4

#endif
