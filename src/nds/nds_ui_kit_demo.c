/* P2-1c demo surface. LAB ONLY -- it exists to make the kit visible and
 * measurable before P2-1d has a real screen to put it on, and it is deleted
 * with `NDS_P2_UI_KIT_DEMO` the moment 1d lands (board row P2-1c).
 *
 * It draws on whichever scene the bounded boot parks in -- on the `normal`
 * scene harness this target builds with, that is the Title park -- because
 * the parked main loop presents once a VBlank with the scene thread stopped,
 * which is exactly the 60 Hz menu-screen cadence the phase's exit criteria
 * budget ~560K ARM9 ticks a frame for.
 *
 * The cursor moves EVERY frame on purpose. A menu that holds still skips the
 * whole commit (gNdsUiKitCommitIdleCount), so a still screen would measure
 * the cheapest case; this measures the most expensive one the kit can produce
 * without a content change. */

#include "nds_build_config.h"

#if NDS_P2_UI_KIT_DEMO

#if !NDS_P2_UI_KIT
#error "NDS_P2_UI_KIT_DEMO needs the kit it demonstrates (NDS_P2_UI_KIT=1)"
#endif

#include <nds.h>

#include <nds/nds_ui_kit.h>
#include <nds/nds_ui_kit_demo.h>
#include <nds/nds_platform.h>
#include <nds/nds_scene.h>

#include "generated/mn_ui_kit.generated.inc"

/* One VBlank is 560,190 ARM9 ticks. Sixteen buckets put the whole 60 Hz
 * budget on the axis at 35,012 ticks a bucket, so a percentile read off this
 * histogram carries a stated 6.25%-of-budget resolution instead of a
 * single mean that hides its own distribution. */
#define NDS_UI_KIT_DEMO_TICK_BUCKET 35012u
#define NDS_UI_KIT_DEMO_TICK_BUCKETS 16u
#define NDS_UI_KIT_DEMO_VBLANK_BUCKETS 4u

#define NDS_UI_KIT_DEMO_PUBLISHED __attribute__((used))

NDS_UI_KIT_DEMO_PUBLISHED volatile u32 gNdsUiKitDemoEntered;
NDS_UI_KIT_DEMO_PUBLISHED volatile u32 gNdsUiKitDemoSceneKind = 0xffffffffu;
NDS_UI_KIT_DEMO_PUBLISHED volatile u32 gNdsUiKitDemoFrames;
NDS_UI_KIT_DEMO_PUBLISHED volatile u32 gNdsUiKitDemoEnterFrameTicks;
NDS_UI_KIT_DEMO_PUBLISHED volatile u32 gNdsUiKitDemoWorkTicksLast;
NDS_UI_KIT_DEMO_PUBLISHED volatile u32 gNdsUiKitDemoWorkTicksMax;
NDS_UI_KIT_DEMO_PUBLISHED volatile u32
    gNdsUiKitDemoWorkHist[NDS_UI_KIT_DEMO_TICK_BUCKETS];
NDS_UI_KIT_DEMO_PUBLISHED volatile u32
    gNdsUiKitDemoVBlankHist[NDS_UI_KIT_DEMO_VBLANK_BUCKETS];
NDS_UI_KIT_DEMO_PUBLISHED volatile u32 gNdsUiKitDemoVBlankMax;

static u32 sNdsUiKitDemoLastPresentTicks;
static u32 sNdsUiKitDemoLastVBlank;
static u32 sNdsUiKitDemoArmed;
static u32 sNdsUiKitDemoPhase;

/* The source's own menu colours: white titles, the P1 red and the P3 green
 * the CSS puts under each portrait (mnplayersvs.c uses the port slot colours
 * for the name plates). */
#define NDS_UI_KIT_DEMO_WHITE 0x00ffffffu
#define NDS_UI_KIT_DEMO_RED 0x00fe3c3cu
#define NDS_UI_KIT_DEMO_GREEN 0x0028dc50u
#define NDS_UI_KIT_DEMO_YELLOW 0x00ffd23cu

static void ndsUiKitDemoPopulate(void)
{
    /* x is (256 - ndsUiKitTextWidth(s)) / 2 for the centred rows, and the
     * portrait centres for the two name plates: the source's own advance and
     * kerning give 80/36/26/16/53/53 px for these strings. */
    ndsUiKitSetText(0u, "SUPER SMASH BROS", NDS_UI_KIT_DEMO_WHITE);
    ndsUiKitMoveText(0u, 88, 14);
    ndsUiKitSetText(1u, "VS MODE", NDS_UI_KIT_DEMO_YELLOW);
    ndsUiKitMoveText(1u, 110, 28);
    ndsUiKitSetText(2u, "MARIO", NDS_UI_KIT_DEMO_RED);
    ndsUiKitMoveText(2u, 39, 112);
    ndsUiKitSetText(3u, "FOX", NDS_UI_KIT_DEMO_GREEN);
    ndsUiKitMoveText(3u, 170, 112);
    ndsUiKitSetText(4u, "DREAM LAND", NDS_UI_KIT_DEMO_WHITE);
    ndsUiKitMoveText(4u, 101, 142);
    ndsUiKitSetText(5u, "PRESS START", NDS_UI_KIT_DEMO_WHITE);
    ndsUiKitMoveText(5u, 101, 162);

    ndsUiKitSetSprite(0u, NDS_MN_UI_KIT_IMAGE_PORTRAIT_MARIO, 30, 56);
    ndsUiKitSetSprite(1u, NDS_MN_UI_KIT_IMAGE_PORTRAIT_FOX, 156, 56);
    ndsUiKitSetSprite(2u, NDS_MN_UI_KIT_IMAGE_CURSOR_HAND_POINT, 100, 62);
}

void ndsUiKitDemoUpdate(void)
{
    u32 now = cpuGetTiming();

    if (gNdsUiKitDemoEntered == 0u)
    {
        if (gNdsSceneBoundaryResult != NDS_SCENE_BOUNDARY_PASS)
        {
            return;
        }
        if (ndsUiKitEnter(NDS_UI_KIT_ENGINE_MAIN) == FALSE)
        {
            /* Counted in gNdsUiKitEnterRejectCount; retrying every frame
             * would hide a hard refusal behind a busy loop. */
            gNdsUiKitDemoEntered = 0xffffffffu;
            return;
        }
        gNdsUiKitDemoSceneKind = gNdsSceneBoundaryKind;
        ndsUiKitDemoPopulate();
        gNdsUiKitDemoEntered = 1u;
        gNdsUiKitDemoEnterFrameTicks = now - sNdsUiKitDemoLastPresentTicks;
        return;
    }
    if (gNdsUiKitDemoEntered != 1u)
    {
        return;
    }

    /* 128 frames out, 128 frames back, one pixel a frame, between the two
     * portraits. */
    sNdsUiKitDemoPhase = (sNdsUiKitDemoPhase + 1u) & 0xffu;
    ndsUiKitMoveSprite(2u,
                       76 + (s32)(((sNdsUiKitDemoPhase < 128u) ?
                                   sNdsUiKitDemoPhase :
                                   (256u - sNdsUiKitDemoPhase)) / 2u),
                       62);
    if (sNdsUiKitDemoPhase == 0u)
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_CONFIRM);
    }
    else if ((sNdsUiKitDemoPhase & 0x3fu) == 0u)
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_MOVE);
    }
    else if (sNdsUiKitDemoPhase == 0xa0u)
    {
        ndsUiKitSfx(NDS_UI_KIT_SFX_BACK);
    }

    if (sNdsUiKitDemoArmed != 0u)
    {
        u32 work = now - sNdsUiKitDemoLastPresentTicks;
        u32 bucket = work / NDS_UI_KIT_DEMO_TICK_BUCKET;

        if (bucket >= NDS_UI_KIT_DEMO_TICK_BUCKETS)
        {
            bucket = NDS_UI_KIT_DEMO_TICK_BUCKETS - 1u;
        }
        gNdsUiKitDemoWorkHist[bucket]++;
        gNdsUiKitDemoWorkTicksLast = work;
        if (work > gNdsUiKitDemoWorkTicksMax)
        {
            gNdsUiKitDemoWorkTicksMax = work;
        }
        gNdsUiKitDemoFrames++;
    }
}

void ndsUiKitDemoAfterPresent(void)
{
    u32 vblank = ndsPlatformVBlankCount();

    if ((gNdsUiKitDemoEntered == 1u) && (sNdsUiKitDemoArmed != 0u))
    {
        u32 interval = vblank - sNdsUiKitDemoLastVBlank;
        u32 bucket = (interval == 0u) ? 0u : (interval - 1u);

        if (bucket >= NDS_UI_KIT_DEMO_VBLANK_BUCKETS)
        {
            bucket = NDS_UI_KIT_DEMO_VBLANK_BUCKETS - 1u;
        }
        gNdsUiKitDemoVBlankHist[bucket]++;
        if (interval > gNdsUiKitDemoVBlankMax)
        {
            gNdsUiKitDemoVBlankMax = interval;
        }
    }
    if (gNdsUiKitDemoEntered == 1u)
    {
        /* Armed one frame AFTER entry so the scene-load frame that reads the
         * pack out of NitroFS is reported on its own
         * (gNdsUiKitDemoEnterFrameTicks) instead of being the max of a steady
         * distribution. */
        sNdsUiKitDemoArmed = 1u;
    }
    sNdsUiKitDemoLastVBlank = vblank;
    sNdsUiKitDemoLastPresentTicks = cpuGetTiming();
}

#endif /* NDS_P2_UI_KIT_DEMO */
