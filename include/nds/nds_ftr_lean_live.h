#ifndef NDS_FTR_LEAN_LIVE_H
#define NDS_FTR_LEAN_LIVE_H

/* Whether this build's fighter renderer is the lean path
 * (include/nds/renderer_fighter_lean.h). Its own header so code ahead of
 * that one -- the old executor in renderer_adapter_fighter.c, compiled only
 * where lean is not (2026-10-05, owner: delete the old machinery) -- can
 * test it. Mirrors NDS_FIGHTER_PACKET_LIVE (src/nds/nds_renderer_preamble.c):
 * the lean lists live in the packet arena and reuse its patch helpers. */
#define NDS_FTR_LEAN_LIVE 1

#endif
