#ifndef SSB64_NDS_SYS_VIDEO_H
#define SSB64_NDS_SYS_VIDEO_H

#include <sys/scheduler.h>

#define SYVIDEO_FLAG_NONE           0x0
#define SYVIDEO_FLAG_ANTIALIAS      0x1
#define SYVIDEO_FLAG_NOANTIALIAS    0x2
#define SYVIDEO_FLAG_SERRATE        0x4
#define SYVIDEO_FLAG_NOSERRATE      0x8
#define SYVIDEO_FLAG_COLORDEPTH16   0x10
#define SYVIDEO_FLAG_COLORDEPTH32   0x20
#define SYVIDEO_FLAG_GAMMA          0x40
#define SYVIDEO_FLAG_NOGAMMA        0x80
#define SYVIDEO_FLAG_BLACKOUT       0x100
#define SYVIDEO_FLAG_NOBLACKOUT     0x200
#define SYVIDEO_FLAG_GAMMADITHER    0x1000
#define SYVIDEO_FLAG_NOGAMMADITHER  0x2000
#define SYVIDEO_FLAG_DITHERFILTER   0x4000
#define SYVIDEO_FLAG_NODITHERFILTER 0x8000
#define SYVIDEO_FLAG_DIVOT          0x10000
#define SYVIDEO_FLAG_NODIVOT        0x20000

#define SYVIDEO_BORDER_SIZE(dimension, pixels, type) \
    ((dimension) * (pixels) * sizeof(type))

#include <ssb_types.h>

typedef struct SYVideoSetup {
    void *framebuffers[3];
    u16 *zbuffer;
    u32 width;
    u32 height;
    u32 flags;
} SYVideoSetup;

#define SYVIDEO_SETUP_DEFAULT() { \
    { &gSYFramebufferSets[0], &gSYFramebufferSets[0], &gSYFramebufferSets[0] }, \
    NULL, \
    320, \
    240, \
    SYVIDEO_FLAG_DIVOT | SYVIDEO_FLAG_DITHERFILTER | \
        SYVIDEO_FLAG_NOGAMMADITHER | 0x800 | \
        SYVIDEO_FLAG_NOBLACKOUT | SYVIDEO_FLAG_NOGAMMA | \
        SYVIDEO_FLAG_COLORDEPTH16 | SYVIDEO_FLAG_NOSERRATE | \
        SYVIDEO_FLAG_ANTIALIAS \
}

#define SYVIDEO_ZBUFFER_START(width, height, w_border, h_border, type) \
    ((u16*)((uintptr_t)gSYZBuffer - \
        ((((width) * (height)) - (((width) - (w_border)) * ((height) - (h_border)))) * \
        sizeof(type))))

/* Reduced from the N64 extent -- (320*240) - (((320*240) - (320*230)) *
 * sizeof(u16)) = 70,400 halfwords -- to the 320x10 border the
 * SYVIDEO_ZBUFFER_START arithmetic above actually names. The DS has a hardware
 * depth buffer in VRAM; cycle 84 measured this storage still untouched .bss at
 * frame 607 against a control that moved in the same run. The definition in
 * src/import/battleship_sys_zbuffer.c carries the full proof and MUST be kept
 * in step with this extent. Nothing takes sizeof(gSYZBuffer). */
#if defined(NDS_RENDERER_HW_TRIANGLES) && NDS_RENDERER_HW_TRIANGLES
extern u16 gSYZBuffer[1]; /* Address identity; hardware GX owns depth. */
#else
extern u16 gSYZBuffer[320 * 10];
#endif
/* [3][230][320] -> [2] -> [1][231][320]: 441,600 -> 294,400 -> 147,840 bytes.
 * The second step frees 146,560 more.
 *
 * The DS never rasterises into these (GX renders to VRAM; we present from
 * sFramebuffers[] in nds_platform.c), but the array is NOT dead -- the VS
 * Results photo wipe reads it. Sizing it is therefore arithmetic on that read,
 * not a judgement call.
 *
 * THE SPAN, RE-DERIVED FROM THE COMPILED READER, NOT FROM THIS COMMENT
 * (ndsBaseLBTransitionSetupTransition, mnVSResultsFuncStart's only caller;
 * disassembly in artifacts/performance/2026-08-15_framebuffer-collapse/):
 *
 *   ldr r1, [gSYSchedulerCurrentFramebuffer]   ; base
 *   adds r1, r1, #0x00023f14                   ; + 147,220  = BORDER(320,10)
 *                                              ;            + BORDER(320,220)
 *                                              ;            + BORDER(1,10)
 *   inner: ldr r2,[r1,r3] for r3 = 0..596 step 4   ; 600 bytes read per row
 *   outer: adds r1, r1, #0xfffffd80            ; -640 = one row BACKWARD
 *          220 iterations (heap advance 0x203a0 = 300*220*2, 600 B per row)
 *
 * highest byte read = 147,220 + 599            = 147,819
 * lowest  byte read = 147,220 - 640*219        =   7,060
 *
 * So the wipe touches base+7,060 .. base+147,819 INCLUSIVE. The object must
 * therefore span offsets 0..147,819 = 147,820 bytes, which is 230.97 rows;
 * rounded up to whole 640-byte rows that is 231 rows = 147,840 bytes. The
 * loads are 32-bit and 147,220 % 4 == 0, so the base must stay 4-aligned --
 * hence the explicit attribute rather than relying on a linker default.
 *
 * READER SET ESTABLISHED FROM THE LINKED ELF, NOT FROM GREP (2026-08-15,
 * smash64ds-battle-playable-hwtri.elf; every word-aligned literal in every
 * SHF_ALLOC section whose value lands inside the object):
 *   - ndsBaseLBTransitionSetupTransition  READS, through
 *     gSYSchedulerCurrentFramebuffer (which has exactly four references: two
 *     scheduler ASSIGNS, this read, and lbTransitionSetupTransition's NULL
 *     fallback in src/import/battleship_lbtransition.c).
 *   - ndsBaseSCManagerRunLoop             WRITES the clear, bounded by
 *     sizeof(gSYFramebufferSets), so it shrinks with this extent.
 *   - ndsBaseSCVSBattleStartScene         address arithmetic only
 *     (arena_size = &gSYFramebufferSets - &ovl4_BSS_END), and the port
 *     overwrites that field with ndsTaskmanArenaSize().
 *   - the SYVideoSetup initializers       data only, all slots = [0].
 *   - sixteen *StartScene z-buffer stores of SYVIDEO_ZBUFFER_START(...) =
 *     gSYZBuffer-6,400. gSYZBuffer still immediately follows this object, so
 *     that pointer lands at base+141,440 -- which the collapse moved from the
 *     dead tail of old buffer [2] INTO the wipe's read span. It is NEVER
 *     dereferenced (cycle 84 measured the whole 6,400 still holding the
 *     scmanager clear value mid-battle, against a control that moved in the
 *     same run; src/import/battleship_sys_zbuffer.c carries the proof), so this
 *     is inert -- but if anything ever DOES rasterise through that pointer it
 *     now corrupts the Results photo instead of nothing. Restore both extents
 *     together if that day comes.
 * No other reader exists. Nothing else takes sizeof().
 *
 * All three SYVIDEO_SETUP_DEFAULT slots alias buffer 0 so the wipe always reads
 * from the one buffer whose 147,820-byte span is in range. Safe because
 * sys/scheduler.c only ASSIGNS gSYSchedulerCurrentFramebuffer (709/712/724/730/
 * 1204) and never compares the buffers against each other -- its only pointer
 * test is the SYSCHEDULER_BUFFER_NULL sentinel, which a non-NULL alias passes.
 * mvopeningroom.c does compare them, but those lines are in the non-NDS arm.
 *
 * Every decomp TU sees THIS header, not decomp's: INCLUDES puts `include`
 * before $(BATTLESHIP_DECOMP)/src, verified by preprocessing
 * src/import/battleship_scmanager.c. That matters because scmanager.c's clear
 * bounds itself with sizeof(gSYFramebufferSets), so the clear shrinks with this
 * extent instead of overrunning it.
 *
 * The outer [1] is kept rather than dropping to [231][320] on purpose: it keeps
 * &gSYFramebufferSets[0] a pointer to a whole BUFFER, so a future [1]/[2] would
 * be out of range rather than silently overlapping row 1 and row 2 of the live
 * one. mntitle.c is the only such upstream user; battleship_mntitle.c aliases
 * all three setup pointers to [0] at the import boundary before syVideoInit. */
extern u16 gSYFramebufferSets[1][231][320] __attribute__((aligned(4)));
extern u16 *gSYVideoZBuffer;
extern u32 gSYVideoColorDepth;
extern s32 gSYVideoResWidth;
extern s32 gSYVideoResHeight;

void syVideoInit(SYVideoSetup *video_setup);
void syVideoSetFlags(u32 flags);

/* DS PLATFORM VIDEO SEAM (Congra/Credits endings, 1P build).
 *
 * Source mn/mncommon/mncongra.c:407-430 and sc/sccommon/scstaffroll.c:2326-
 * 2338 bracket syVideoInit/syTaskmanStartTask with N64 framebuffer clear loops
 * that run to 0x80400000. Those loops are NEVER executed on DS: the Congra /
 * Credits wrappers replace only the platform start (fighter selection
 * preserved) with the DS arena/video setup, reusing ndsTaskmanArenaStart/Size
 * and the original FuncStart/FuncDraw. Mapping the address macro to a DS
 * buffer and running the loops would overwrite RAM, so the macro below exists
 * ONLY to keep the inert setup tables compiling --
 * dMNCongraVideoSetup / dSCStaffrollVideoSetup initializers -- and the
 * wrappers overwrite all three slots with &gSYFramebufferSets[0] at entry
 * (like mnTitleStartScene) before syVideoInit. User-facing rendering stays
 * source-derived; no bitmap is invented here.
 *
 * BLACKOUT contract: source mnCongraFuncDraw :377 / scStaffrollFuncDraw :2245
 * latch SYVIDEO_FLAG_BLACKOUT once on exit (5-frame wait to Title /
 * RollEndWait to Startup/OpeningRoom). The N64 VI honors that flag; on DS the
 * shared video seam (src/import/battleship_sys_video.c) mirrors it onto the
 * DS brightness latch (ndsVideoSetBlackout) in syVideoSetFlags and mirrors
 * the setup flags in syVideoInit (every setup carries NOBLACKOUT), so every
 * next scene recovers with no per-scene or per-frame reset work anywhere.
 * Source applies VI flags as scheduler tasks at a frame boundary
 * (scheduler.c:373-379, :693-702, :1038-1056), never at the setFlags call
 * site; the DS side matches: ndsVideoSetBlackout only latches software state
 * and marks it dirty, and ndsVideoBlackoutCommit performs the master-
 * brightness writes from the post-VBlank window of ndsPlatformEndFrame. The
 * seam touches only the master-brightness latches (no VRAM bank changes;
 * banks stay as ndsPlatformInit left them, per sm64-nds inspection). */
#ifndef SYVIDEO_DEFINE_FRAMEBUFFER_ADDR
#define SYVIDEO_DEFINE_FRAMEBUFFER_ADDR(width, height, w_border, h_border, type, id) \
    ((void*)&gSYFramebufferSets[0])
#endif

void ndsVideoSetBlackout(sb32 black);
sb32 ndsVideoGetBlackout(void);
/* Frame-boundary apply of the blackout latch; call after VBlank (see
 * src/port/video_blackout.c). Cheap no-op when the latch is clean. */
void ndsVideoBlackoutCommit(void);

/* The N64 screen-centre offsets (decomp sys/video.c:33-42, :110). The Screen
 * Adjust scene (battleship_mnscreenadjust.c) reads and writes them and the
 * save carries them; on DS they change nothing on screen (an LCD has no
 * overscan), so the setter only records them -- the accepted no-op delta
 * battleship_lbbackup.c documents. Defined by the Screen Adjust TU. */
extern s16 gSYVideoOffsetLeft;
extern s16 gSYVideoOffsetTop;
void syVideoSetCenterOffsets(s16 left, s16 right, s16 top, s16 bottom);
void syVideoApplySettingsNoBlock(SYTaskVi *vi);
u32 syVideoGetFillColor(u32 color);

#endif
