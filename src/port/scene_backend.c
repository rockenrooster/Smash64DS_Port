#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <nds/arm9/cache.h>
#include <nds/dma.h>
#include <nds/timers.h>
#include <PR/os.h>
#include <db/debug.h>
#include <ef/effect.h>
#include <ft/fighter.h>
#include <gm/gmsound.h>
#include <gr/ground.h>
#include <if/interface.h>
#include <mp/map.h>
#include <mn/menu.h>
#include <mv/movie.h>
#include <nds/nds_boot.h>
#include <nds/nds_audio_assets.h>
#include <nds/nds_audio_bgm.h>
#include <nds/nds_audio_fgm.h>
#include <nds/nds_controller.h>
#include <nds/nds_os.h>
#include <nds/nds_platform.h>
#include <nds/nds_reloc_assets.h>
#include <nds/nds_renderer.h>
#include <nds/nds_scene.h>
#include <nds/nds_scene_harness.h>
#include "nds_build_config.h"
#include "nds_scene_harness_config.h"
#include <nds/nds_startup.h>
#include <reloc_data.h>
#include <sc/scene.h>
#include <sys/audio.h>
#include <sys/controller.h>
#include <sys/debug.h>
#include <sys/malloc.h>
#include <sys/obj.h>
#include <sys/objman.h>
#include <sys/objhelper.h>
#include <sys/scheduler.h>
#include <sys/taskman.h>
#include <sys/video.h>
#include <wp/weapon.h>

extern volatile u32 gNdsFrameCounter;

s32 osResetType;

/* Original BattleShip task/object manager state.
 *
 * The object pool globals (sGCCommonHead, sGCProcessHead, ...) and the
 * taskman heap globals (gSYTaskmanGeneralHeap, gSYTaskmanGraphicsHeap, ...)
 * are now defined by the imported sys/objman.c and sys/taskman.c translation
 * units. This file no longer maintains a parallel hand-written object pool or
 * taskman heap mirror; it only:
 *   - backs the arena the original syTaskmanStartTask reads,
 *   - bounds the original task loop in the DS syTaskmanRunTask seam,
 *   - mirrors the original taskman cleanup tail and returns to scManager, and
 *   - records diagnostics that read the real object-manager state.
 *
 * The original flow is preserved: mnStartupStartScene -> syTaskmanStartTask ->
 * gcSetupObjman -> syTaskmanLoadScene -> mnStartupFuncStart ->
 * syTaskmanRunTask -> scManagerRunLoop -> mvOpeningRoomStartScene ->
 * mvOpeningRoomFuncStart -> one mvOpeningRoomFuncRun tick (then parked). */


/* Mechanical split: these project-owned backend slices are included to
 * preserve exact static linkage while reducing the scene_backend.c monolith.
 * Convert them to independent translation units only after adding explicit
 * narrow headers for the symbols each slice shares. */
#include "diagnostics.c"
#include "taskman_seam.c"
#include "reloc_backend.c"
#include "sprite_preview_backend.c"
#include "opening_movie_backend.c"
#include "title_backend.c"
