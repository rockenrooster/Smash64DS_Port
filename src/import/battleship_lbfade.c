/* DS owner of BattleShip's screen-fade actor:
 * decomp/BattleShip-main/decomp/src/lb/lbfade.c:34-90.
 *
 * Source behavior, preserved exactly:
 * - lbFadeMakeActor wires a real Transition GObj: display proc at the
 *   caller's link priority on all cameras (func_80009F74, ~0) plus a Func-kind
 *   update proc. Every caller passes link nGCCommonLinkIDTransition (13) and
 *   priority 10; colors are black in practice ({0,0,0,0x00} fade-from,
 *   {0,0,0,0xFF} fade-to) but the API carries arbitrary RGBA.
 * - Update (lbfade.c:34-56): alpha counter saturates 0..fade_length over
 *   fade_length ticks; the length counter runs fade_length+2, so the proceed
 *   callback (*is_proceed_scene=TRUE) and the GObj eject fire 2 ticks after
 *   the ramp saturates. Globals are single-instance: a later MakeActor
 *   overwrites an earlier fade, same as source.
 * - Display alpha (lbfade.c:61-66): (current/max)*255 in float, inverted when
 *   base a==0. The max<=0 arm is a host-safe guard (AVOID_UB, no caller
 *   passes 0; min is 10): a zero-length fade reports its end state.
 *
 * DS presentation (single final hardware application):
 * - This TU runs the actual timing and publishes the per-frame color+alpha;
 *   it emits NO RDP words, so the opaque MenuFILL sink never sees a fade word
 *   to misdraw. ndsPlatformEndFrame calls ndsLBFadePushHardwareFrame() once
 *   after all draws: the published frame becomes a MASTER_BRIGHT fade-down
 *   level latched into src/port/video_blackout.c (sole register owner) and
 *   written post-VBlank. One application covers 3D, both staging layers, and
 *   fade-only frames; there is no staging blend (it would miss pre-display
 *   commits, never reach 3D-only pixels, and double-fade beside hardware).
 * - Black-only scope (see <lb/lbfade_ds.h>): all actual source callers pass
 *   {0,0,0,...} (pinned by scripts/menus/test_source_fade.py). A published
 *   non-black frame yields no hardware level rather than a wrong hue.
 * - The hardware-blackout seam (src/port/video_blackout.c) keeps sole
 *   ownership of REG_MASTER_BRIGHT/SUB; blackout (full black) wins over any
 *   fade level in that TU's resolve.
 *
 * Callers (all fade-from black except where noted): mnStartup start(16)/end
 * fade-to(10), scVSBattle start(12)/sudden-death(12), mvUnknownMario(12),
 * scAutoDemo(30), scExplain(12), sc1PGame(12), sc1PBonusStage(12),
 * sc1PTrainingMode(12), mnCongra fade-to(90, +proceed ptr, +BLACKOUT latch). */

#include <ssb_types.h>
#include <sys/obj.h>
#include <sys/objman.h>
#include <lb/lbfade_ds.h>
#include <nds/nds_video.h>

volatile u32 gNdsLBFadeCreateCount;
volatile u32 gNdsLBFadeDisplayCount;

/* Source globals (lbfade.c:9-26), same names for direct comparability. */
static s32 sLBFadeAlphaMax;
static s32 sLBFadeAlphaCurrent;
static s32 sLBFadeLength;
static SYColorRGBA sLBFadeColor;
static sb32 *sLBFadeIsProceedScene;
static sb32 sLBFadeIsEjectGObj;

/* Latest published display frame. Set every time the display proc runs;
 * discarded by ndsSObjPreviewBeginFrame before the next frame's display
 * procs, so exactly one EndFrame push can observe each published frame. */
static u32 sLBFadeFrameActive = FALSE;
static SYColorRGBA sLBFadeFrameColor;
static u8 sLBFadeFrameAlpha;

s32 ndsLBFadeComputeAlpha(s32 current, s32 max, u8 base_alpha)
{
    s32 alpha;

    if (max <= 0)
    {
        return (base_alpha == 0) ? 0 : 0xFF;
    }
    alpha = (s32)(((f32)current / (f32)max) * 255.0F);
    if (base_alpha == 0)
    {
        alpha = 0xFF - alpha;
    }
    return alpha;
}

u32 ndsLBFadeAlphaToHardwareLevel(u8 alpha)
{
    return ((u32)alpha * NDS_LBFADE_HW_LEVEL_MAX + 127u) / 255u;
}

/* lbfade.c:34-56, character-identical timing/callback/eject. */
void lbFadeProcUpdate(GObj *gobj)
{
    if (sLBFadeAlphaCurrent < sLBFadeAlphaMax)
    {
        sLBFadeAlphaCurrent++;
    }
    if (sLBFadeLength != 0)
    {
        sLBFadeLength--;

        if (sLBFadeLength == 0)
        {
            if (sLBFadeIsProceedScene != NULL)
            {
                *sLBFadeIsProceedScene = TRUE;
            }
            if (sLBFadeIsEjectGObj != FALSE)
            {
                gcEjectGObj(gobj);
            }
        }
    }
}

/* lbfade.c:59-73 with the RDP word emission replaced by frame publication
 * (rationale above). Alpha math is the source formula via
 * ndsLBFadeComputeAlpha. */
void lbFadeProcDisplay(GObj *gobj)
{
    (void)gobj;

    sLBFadeFrameColor = sLBFadeColor;
    sLBFadeFrameAlpha =
        (u8)ndsLBFadeComputeAlpha(sLBFadeAlphaCurrent, sLBFadeAlphaMax,
                                 sLBFadeColor.a);
    sLBFadeFrameActive = TRUE;
    gNdsLBFadeDisplayCount++;
}

u32 ndsLBFadePeekFrame(u8 *out_r, u8 *out_g, u8 *out_b, u8 *out_alpha)
{
    if (sLBFadeFrameActive == FALSE)
    {
        return FALSE;
    }
    if (out_r != NULL)
    {
        *out_r = sLBFadeFrameColor.r;
    }
    if (out_g != NULL)
    {
        *out_g = sLBFadeFrameColor.g;
    }
    if (out_b != NULL)
    {
        *out_b = sLBFadeFrameColor.b;
    }
    if (out_alpha != NULL)
    {
        *out_alpha = sLBFadeFrameAlpha;
    }
    return TRUE;
}

u32 ndsLBFadePeekHardwareLevel(u32 *out_level)
{
    u32 level = 0u;

    if (out_level != NULL)
    {
        *out_level = 0u;
    }
    if (sLBFadeFrameActive == FALSE)
    {
        return FALSE;
    }
    if (((u32)sLBFadeFrameColor.r | (u32)sLBFadeFrameColor.g |
         (u32)sLBFadeFrameColor.b) != 0u)
    {
        /* No caller publishes this (all source fade colors are black); a
         * hue the fade-down register cannot express draws no fade rather
         * than a wrong one. */
        return FALSE;
    }
    level = ndsLBFadeAlphaToHardwareLevel(sLBFadeFrameAlpha);
    if (out_level != NULL)
    {
        *out_level = level;
    }
    return TRUE;
}

void ndsLBFadeDiscardFrame(void)
{
    sLBFadeFrameActive = FALSE;
}

void ndsLBFadePushHardwareFrame(void)
{
    u32 level = 0u;

    (void)ndsLBFadePeekHardwareLevel(&level);
    ndsVideoSetSourceFade(level);
    /* Native shell screens have no SObj BeginFrame. Consume here so a source
     * fade cannot persist into those screens after its scene has ended. */
    ndsLBFadeDiscardFrame();
}

/* lbfade.c:77-90, source-exact wiring and state init. Port gcMakeGObjSPAfter
 * takes u8 link (source s32); all callers pass 13, so the cast is exact. */
void lbFadeMakeActor(u32 id, s32 link, u32 link_priority, SYColorRGBA *color,
                     s32 fade_length, sb32 is_eject_gobj,
                     sb32 *is_proceed_scene)
{
    GObj *gobj = gcMakeGObjSPAfter(id, NULL, (u8)link, GOBJ_PRIORITY_DEFAULT);

    func_80009F74(gobj, lbFadeProcDisplay, link_priority, 0, ~0);
    gcAddGObjProcess(gobj, lbFadeProcUpdate, nGCProcessKindFunc, 0);

    sLBFadeColor = *color;
    sLBFadeAlphaMax = fade_length;
    sLBFadeAlphaCurrent = 0;
    sLBFadeLength = fade_length + 2;
    sLBFadeIsEjectGObj = is_eject_gobj;
    sLBFadeIsProceedScene = is_proceed_scene;
    gNdsLBFadeCreateCount++;
}
