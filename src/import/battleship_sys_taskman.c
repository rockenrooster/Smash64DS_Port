/* Compile the original BattleShip task-manager translation unit.
 *
 * Provides real syTaskmanStartTask, syTaskmanLoadScene, syTaskmanInitGeneralHeap,
 * syTaskmanMalloc, the general/graphics heap globals, and the per-context
 * DL/graphics/RDP setup.
 *
 * The original syTaskmanRunTask is the full per-frame task/object loop, which
 * needs the threading scheduler, display-list pipeline, and RSP/RDP backend
 * that are not yet imported. The original source is compiled with an
 * SSB64_TARGET_NDS guard that omits only that loop definition; the strong DS
 * bounded seam in src/port/scene_backend.c receives syTaskmanLoadScene's call,
 * runs startup updates, mirrors the original cleanup tail, and returns to the
 * original scene manager before task_draw. */

#include "../../decomp/BattleShip-main/decomp/src/sys/taskman.c"
