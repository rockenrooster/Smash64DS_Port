#ifndef SSB64_NDS_RESULTS_OAM_H
#define SSB64_NDS_RESULTS_OAM_H

#include <PR/ultratypes.h>

struct GObj;

void ndsResultsOamEnter(void);
void ndsResultsOamExit(void);
s32 ndsResultsOamIsActive(void);
void ndsResultsOamBeginFrame(void);
s32 ndsResultsOamDrawGObj(struct GObj *gobj);
void ndsResultsOamCommit(void);
u32 ndsResultsOamBakeGObj(struct GObj *gobj);
s32 ndsResultsOamEmitTintPlane(u32 source_alpha);
s32 ndsResultsOamEmitFillRect(s32 sx0, s32 sy0, s32 sx1, s32 sy1);

extern volatile u32 gNdsResultsOamEnterCount;
extern volatile u32 gNdsResultsOamExitCount;
extern volatile u32 gNdsResultsOamBeginFrameCount;
extern volatile u32 gNdsResultsOamDrawGObjCount;
extern volatile u32 gNdsResultsOamDrawSObjCount;
extern volatile u32 gNdsResultsOamBakeCellCount;
extern volatile u32 gNdsResultsOamPaletteCount;
extern volatile u32 gNdsResultsOamEmitCount;
extern volatile u32 gNdsResultsOamCommitCount;
extern volatile u32 gNdsResultsOamRollbackCount;
extern volatile u32 gNdsResultsOamVramBytes;
/* R01-A evidence pair. Every counter above reads healthy whether or not a
 * bitmap sprite is actually visible: the first-place badge was prepared,
 * baked, emitted and given an OAM slot while being written fully
 * transparent. `BmpEmit` counts OBJs emitted in SpriteColorFormat_Bmp and
 * `BmpZeroAlpha` the subset written with alpha 0, which libnds defines as
 * invisible. BmpEmit > 0 with BmpZeroAlpha == 0 is the repaired state. */
extern volatile u32 gNdsResultsOamBmpEmitCount;
extern volatile u32 gNdsResultsOamBmpZeroAlphaCount;

extern volatile u32 gNdsResultsOamFailureInactive;
extern volatile u32 gNdsResultsOamFailureUnsupportedFormat;
extern volatile u32 gNdsResultsOamFailureBadProvenance;
extern volatile u32 gNdsResultsOamFailureTileOverflow;
extern volatile u32 gNdsResultsOamFailureCellSlotsFull;
extern volatile u32 gNdsResultsOamFailureVramFull;
extern volatile u32 gNdsResultsOamFailurePaletteFull;
extern volatile u32 gNdsResultsOamFailureOamFull;

#endif /* SSB64_NDS_RESULTS_OAM_H */
