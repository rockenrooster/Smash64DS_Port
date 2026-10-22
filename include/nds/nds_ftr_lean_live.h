#ifndef NDS_FTR_LEAN_LIVE_H
#define NDS_FTR_LEAN_LIVE_H

/* The fighter renderer is the lean path (include/nds/renderer_fighter_lean.h)
 * in every build; the old executor it replaced is deleted (2026-10-05, owner:
 * delete the old machinery). Only the generated owner images still test this
 * (scripts/fighters/generate_nds_native_owner_images.py). */
#define NDS_FTR_LEAN_LIVE 1

#endif
