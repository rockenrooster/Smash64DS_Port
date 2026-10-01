#ifndef NDS_SOURCE2D_H
#define NDS_SOURCE2D_H

#include <PR/ultratypes.h>

struct GObj;

/* Native OBJ presentation for the source 2D scenes that have no native screen
 * of their own (the 1P intro, stage clear, continue, challenger, message and
 * congratulations scenes): every visible SObj a source display callback draws
 * is baked into OBJ cells (bank E) at its mapped size and placed in source
 * draw order. The source scene keeps every position, scale, colour and
 * timing; this only presents them. See src/nds/nds_source2d.c. */

/* Activates the tenant when `scene` is one of its scenes (a no-op otherwise).
 * Called by the source-menu pump before the scene's first frame; while a
 * transition holds the previous frame the entry waits for the Thaw. */
void ndsSource2DEnterForScene(u32 scene);
void ndsSource2DEnterPending(void);
void ndsSource2DExit(void);
s32 ndsSource2DIsActive(void);
void ndsSource2DBeginFrame(void);
/* Presents every visible SObj of `gobj`; a sprite it cannot present records
 * a native sprite failure. Returns nonzero when the tenant owns the draw. */
s32 ndsSource2DDrawGObj(struct GObj *gobj);
/* A no-attribute draw (lbCommonDrawSObjNoAttr): RGBA/CI texels are multiplied
 * by the prim colour its display callback set (0xRRGGBB; 0xffffff = none). */
s32 ndsSource2DDrawGObjModulated(struct GObj *gobj, u32 modulate_rgb);
/* VBlank-side commit from ndsPlatformEndFrame: staged palettes and OAM. */
void ndsSource2DCommit(void);

#endif
