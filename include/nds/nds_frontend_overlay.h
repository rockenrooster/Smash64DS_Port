/* The front-end overlay and its loans (src/nds/nds_frontend_overlay.c). */

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
/* TRUE while the whole overlay is loaded and no battle borrows its tail: its
 * data (the select screens' Selected clips) may be read. */
s32 ndsFrontendOverlayTailResident(void);
/* The VS character select's loan of everything the overlay links before the
 * select's own block (linker/nds_frontend_overlay.ld: other scenes' code and
 * tables, which nothing in that scene runs or reads; check_frontend_overlay.py
 * rejects a reference from the block into it): its base and size, or NULL.
 * The next scene dispatch reloads the overlay before any of it runs again. */
void *ndsFrontendOverlayBorrowForSelect(u32 *bytes);

/* Scene-lifetime asset storage. The loan is separate from the existing taskman
 * allocation, whose newlib header and ownership must never be overwritten. */
void *ndsFrontendOverlayTryAlloc(size_t bytes, u32 alignment);
s32 ndsFrontendOverlayLoanContains(const void *ptr, size_t bytes);
void *ndsSceneAssetAlloc(size_t bytes, u32 alignment);
/* Atomic admission: preserve keep_free bytes in the taskman heap, including
 * alignment, and leave both cursors unchanged on rejection. */
void *ndsSceneAssetTryAlloc(size_t bytes, u32 alignment, size_t keep_free);
/* A VS battle's battle-lifetime data from the loaned range's idle tail, or
 * NULL (any other scene, or no room): the caller then takes its usual heap. */
void *ndsVSBattleOverlayTryAlloc(size_t bytes, u32 alignment);
/* TRUE in a VS battle scene (gSCManagerSceneData.scene_curr), for code that
 * does not include the scene header (the renderer). */
u32 ndsSceneIsVSBattle(void);

#endif
