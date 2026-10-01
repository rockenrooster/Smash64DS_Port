#ifndef NDS_FRONTEND_OVERLAY_H
#define NDS_FRONTEND_OVERLAY_H

#include <stddef.h>
#include <PR/ultratypes.h>

/* Called by the resident source dispatcher before invoking a scene entry.
 * VS lends the front-end's whole range; a 1P, bonus or training battle
 * lends its tail (after the 1P battle-time head, __nds_frontend_1p_end).
 * Nested campaign menus and scenes retain it. */
void ndsFrontendOverlayPrepareDispatch(u32 kind);
void ndsFrontendOverlayBeginScene(u32 kind);
void ndsFrontendOverlayEndScene(void);
/* After a 1P, bonus or training battle returns into the campaign: reload the
 * overlay if the battle borrowed its tail, before any tail code runs. */
void ndsFrontendOverlayRestoreTail(void);

/* Scene-lifetime asset storage. The loan is separate from the existing taskman
 * allocation, whose newlib header and ownership must never be overwritten. */
void *ndsFrontendOverlayTryAlloc(size_t bytes, u32 alignment);
void *ndsSceneAssetAlloc(size_t bytes, u32 alignment);
/* Atomic admission: preserve keep_free bytes in the taskman heap, including
 * alignment, and leave both cursors unchanged on rejection. */
void *ndsSceneAssetTryAlloc(size_t bytes, u32 alignment, size_t keep_free);

#endif
