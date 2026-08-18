#ifndef NDS_UI_KIT_DEMO_H
#define NDS_UI_KIT_DEMO_H

#include <PR/ultratypes.h>

/* P2-1c lab demo. Declared unconditionally and defined only under
 * NDS_P2_UI_KIT_DEMO -- a declaration guarded on a build-config macro makes a
 * header's contents depend on include order, which this repository has paid
 * for before. The CALL SITES in src/nds/main.c carry the guard.
 *
 * Update runs at the end of the main loop's frame body, AfterPresent
 * immediately after ndsPlatformEndFrame returns; the pair brackets the frame,
 * which is what makes gNdsUiKitDemoWorkHist ARM9 work per presented frame
 * rather than the frame period. */
void ndsUiKitDemoUpdate(void);
void ndsUiKitDemoAfterPresent(void);

extern volatile u32 gNdsUiKitDemoEntered;
extern volatile u32 gNdsUiKitDemoSceneKind;
extern volatile u32 gNdsUiKitDemoFrames;
extern volatile u32 gNdsUiKitDemoEnterFrameTicks;
extern volatile u32 gNdsUiKitDemoWorkTicksLast;
extern volatile u32 gNdsUiKitDemoWorkTicksMax;
extern volatile u32 gNdsUiKitDemoWorkHist[16];
extern volatile u32 gNdsUiKitDemoVBlankHist[4];
extern volatile u32 gNdsUiKitDemoVBlankMax;

#endif /* NDS_UI_KIT_DEMO_H */
