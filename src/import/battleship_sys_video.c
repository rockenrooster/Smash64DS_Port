/* Shared DS video seam: the original BattleShip video translation unit plus
 * the DS blackout mirror.
 *
 * Source behaviour is unchanged: mn/mncommon/mncongra.c:369-393 latches
 * SYVIDEO_FLAG_BLACKOUT once (5-frame wait to Title) and
 * sc/sccommon/scstaffroll.c:2236-2248 latches it once (-1 -> -2 to
 * Startup/OpeningRoom). The N64 VI honors that flag at the scheduler's frame
 * boundary (the setFlags call only queues flags); on DS the latch is the
 * master-brightness state in src/port/video_blackout.c, so syVideoSetFlags
 * mirrors BLACKOUT/NOBLACKOUT onto it and syVideoInit mirrors the setup
 * flags (all setups carry NOBLACKOUT, and scheduler.c:456-463 order makes
 * NOBLACKOUT win a combined word). The mirror only latches software state;
 * ndsVideoBlackoutCommit applies the registers from the post-VBlank window
 * of ndsPlatformEndFrame, matching source VI scheduling. Every next scene
 * recovers through syVideoInit, so no scene wrapper carries per-scene or
 * per-frame reset work.
 *
 * The source N64 framebuffer clear loops to 0x80400000 (mncongra.c:407-409,
 * :429-431; scstaffroll.c:2326-2328, :2336-2338) NEVER run here: the Congra /
 * Credits StartScene wrappers replace only the platform start and skip them,
 * since mapping the address macro to a DS buffer and running them would
 * overwrite RAM.
 *
 * Header note: decomp sys/video.c includes "video.h", which resolves to its
 * own sibling decomp/src/sys/video.h, while the wrappers below need the
 * port-canonical <sys/video.h> (identical flags/setup ABI, DS framebuffer
 * extent + latch declarations). Including both would redefine SYVideoSetup,
 * so the port header is included first and _SYVIDEO_H_ suppresses the
 * sibling; video.c needs nothing only the sibling provides (no setup-table
 * macros, no framebuffer/zbuffer externs -- only pointers passed in). */

#include <sys/video.h>

#define _SYVIDEO_H_

#define syVideoSetFlags ndsBaseSyVideoSetFlags
#define syVideoInit ndsBaseSyVideoInit

#include "../../decomp/BattleShip-main/decomp/src/sys/video.c"

#undef syVideoSetFlags
#undef syVideoInit

void ndsBaseSyVideoSetFlags(u32 flags);
void ndsBaseSyVideoInit(SYVideoSetup *video_setup);

static void ndsApplyVideoBlackoutFlags(u32 flags)
{
    if ((flags & SYVIDEO_FLAG_BLACKOUT) != 0u)
    {
        ndsVideoSetBlackout(TRUE);
    }
    /* scheduler.c:456-463 applies NOBLACKOUT second if both are present. */
    if ((flags & SYVIDEO_FLAG_NOBLACKOUT) != 0u)
    {
        ndsVideoSetBlackout(FALSE);
    }
}

void syVideoSetFlags(u32 flags)
{
    ndsBaseSyVideoSetFlags(flags);
    ndsApplyVideoBlackoutFlags(flags);
}

void syVideoInit(SYVideoSetup *video_setup)
{
    ndsBaseSyVideoInit(video_setup);
    ndsApplyVideoBlackoutFlags(video_setup->flags);
}
