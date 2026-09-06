/* DS video blackout + source-fade seam for the 1P endings (Congra / Credits)
 * and BattleShip's lbFade screen fades.
 *
 * Owns the platform side of source SYVIDEO_FLAG_BLACKOUT. The shared video
 * seam (src/import/battleship_sys_video.c) mirrors that flag onto this latch
 * -- syVideoSetFlags asserts on BLACKOUT (the endings' FuncDraw sources latch
 * it once: mncongra.c:377 for the 5-frame wait to Title, scstaffroll.c:2245
 * for -1 -> -2 to Startup/OpeningRoom) and syVideoInit mirrors the setup
 * flags (every setup carries NOBLACKOUT), so every next scene recovers with
 * no per-scene or per-frame reset work. Documented in include/sys/video.h.
 *
 * SOURCE FADE (BattleShip lb/lbfade.c, all callers black): the fade TU
 * (src/import/battleship_lbfade.c) publishes one color+alpha frame per
 * display-proc run; ndsPlatformEndFrame calls ndsLBFadePushHardwareFrame()
 * once after all draws, which lands here via ndsVideoSetSourceFade as a
 * fade-down level 0..16. Blackout (full black) takes precedence: the resolve
 * is max(blackout ? 16 : 0, fade). One register write per frame covers 3D,
 * both staging layers, and fade-only frames with no staging commit; no
 * software staging blend exists (it would miss pre-display commits, never
 * reach 3D-only pixels, and double-fade beside this pass).
 *
 * DEFERRED COMMIT, matching source VI scheduling. The original never applies
 * a BLACKOUT/NOBLACKOUT at the syVideoSetFlags call site: video.c:80-93 only
 * ORs into sSYVideoFlags, and the change reaches the VI as a scheduler task
 * applied at a frame boundary (scheduler.c:373-379 osViBlack from
 * sySchedulerApplyViMode, driven by the retrace/task loop at scheduler.c:693-
 *   702 and :1038-1056). So on DS the latches are software state only; the two
 * master-brightness registers are written by ndsVideoBlackoutCommit() from
 * the post-VBlank window of ndsPlatformEndFrame -- the same boundary that
 * already flips videoSetMode and commits OAM/affine. A MASTER_BRIGHT write
 * issued mid-frame from FuncDraw/StartScene latches into the live scanout and
 * can split the frame on hardware (top half old brightness, bottom new).
 *
 * This TU is the ONLY writer of REG_MASTER_BRIGHT / REG_MASTER_BRIGHT_SUB in
 * the port, and the dirty gate means a commit costs one branch when nothing
 * changed, so the latch never fights a per-frame owner that may share those
 * registers later. Effect is the two master-brightness latches only (fade-
 * down mode, level 16 = full black); VRAM banks are never touched -- they
 * stay as ndsPlatformInit left them. HW pokes compile only on ARM9 so the
 * latch state stays host-testable; rendering for the endings themselves is
 * still owed (source-derived plates/glyphs, no invented bitmap). */

#include <sys/video.h>

#ifdef ARM9
#include <nds/arm9/video.h>
#endif

static sb32 sNdsVideoBlackout = FALSE;
static u32 sNdsVideoSourceFadeLevel;
static sb32 sNdsVideoBrightnessDirty = FALSE;

void ndsVideoSetBlackout(sb32 black)
{
    sNdsVideoBlackout = (black != FALSE) ? TRUE : FALSE;
    sNdsVideoBrightnessDirty = TRUE;
}

sb32 ndsVideoGetBlackout(void)
{
    return sNdsVideoBlackout;
}

void ndsVideoSetSourceFade(u32 level)
{
    if (level > 16u)
    {
        level = 16u;
    }
    if (level != sNdsVideoSourceFadeLevel)
    {
        sNdsVideoSourceFadeLevel = level;
        sNdsVideoBrightnessDirty = TRUE;
    }
}

u32 ndsVideoGetSourceFade(void)
{
    return sNdsVideoSourceFadeLevel;
}

u16 ndsVideoResolveBrightnessValue(u32 blackout, u32 fade_level)
{
    u32 level;

    if (fade_level > 16u)
    {
        fade_level = 16u;
    }
    level = ((blackout != 0u) ? 16u : 0u);
    if (fade_level > level)
    {
        level = fade_level;
    }
    if (level == 0u)
    {
        return (u16)0u;
    }
    /* MASTER_BRIGHT bits 14-15 select the effect (1 = up, 2 = down) and
     * bits 0-4 the level (0-16); fade-down at level 16 is full black. */
    return (u16)((2u << 14) | level);
}

void ndsVideoBlackoutCommit(void)
{
    if (sNdsVideoBrightnessDirty == FALSE)
    {
        return;
    }
    sNdsVideoBrightnessDirty = FALSE;
#ifdef ARM9
    {
        const u16 value = ndsVideoResolveBrightnessValue(
            (u32)sNdsVideoBlackout, sNdsVideoSourceFadeLevel);

        REG_MASTER_BRIGHT = value;
        REG_MASTER_BRIGHT_SUB = value;
    }
#endif
}
