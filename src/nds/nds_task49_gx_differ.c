/*
 * Task 49 -- GX equivalence differ capture recorder (Part 2 instrument).
 *
 * Mirrors the Task 34 stage-stream recorder shape
 * (src/nds/nds_renderer.c:460-580): a cold, noinline record function with
 * overflow and fault counters, and a per-frame reset keyed on
 * gNdsRendererProfileFrameCount. The difference is bracketing: this recorder
 * groups per OWNER (not per stage segment) and advances a binding index on
 * every MATRIX_LOAD4X4, so a host analyzer can recompose each binding's
 * composed clip matrix separately for the Tier 2 effective-transform compare.
 *
 * Default off (NDS_TASK49_GX_DIFFER ?= 0). Ships no runtime behavior at the
 * default; the entire TU is compiled out unless the flag is on. The single
 * nds_renderer.c writer hook is the NDS_TASK49_GX_DIFFER_RECORD call inside
 * the lean ndsRendererTask29GXRecord funnel.
 */

#include <ssb_types.h>
#include <nds/nds_startup.h>
#include <nds/nds_task49_gx_differ.h>

