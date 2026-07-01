#ifndef SSB64_NDS_ITEM_H
#define SSB64_NDS_ITEM_H

#include <PR/ultratypes.h>

typedef struct ITStruct {
    s32 type;
    s32 weight;
} ITStruct;

enum {
    nITTypeDamage = 0,
    nITTypeSwing = 1,
    nITTypeShoot = 2,
    nITTypeThrow = 3,
    nITTypeTouch = 4,
    nITTypeConsume = 5,
    nITTypeFighter = 6
};

enum {
    nITWeightHeavy = 0,
    nITWeightLight = 1
};

#define itGetStruct(item_gobj) ((ITStruct *)(item_gobj)->user_data.p)

void itManagerMakeAppearActor(void);

#endif
