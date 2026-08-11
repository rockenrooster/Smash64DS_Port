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
extern u16 gSYZBuffer[320 * 10];
/* Reduced from [3] to [2]: 441,600 -> 294,400 bytes, freeing 147,200.
 *
 * The DS never rasterises into these (GX renders to VRAM; we present from
 * sFramebuffers[] in nds_platform.c), but the array is NOT dead -- the VS
 * Results photo wipe reads it. Sizing it is therefore arithmetic on that read,
 * not a judgement call. lbtransition.c:226-241 starts at
 *   base + BORDER(320,10) + BORDER(320,220) + BORDER(1,10) = base + 147,220
 * -- already 20 bytes past buffer 0 -- and walks BACKWARD 640 bytes per row for
 * 220 rows, so it touches base+7,060 .. base+147,819. That is 620 bytes into
 * buffer 1, which is why [1] is NOT sufficient and [2] is.
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
 * KNOWN GAP, out of P1 scope: mntitle.c:126-127 hardcodes &gSYFramebufferSets[1]
 * and [2]. [1] stays valid and [2] becomes a one-past-the-end pointer, legal to
 * form and never dereferenced in P1 because the title scene never runs. If the
 * title scene is ever brought into a shipping configuration, patch it under
 * scripts/decomp-patches/battleship/ first. */
extern u16 gSYFramebufferSets[2][230][320];
extern u16 *gSYVideoZBuffer;
extern u32 gSYVideoColorDepth;
extern s32 gSYVideoResWidth;
extern s32 gSYVideoResHeight;

void syVideoInit(SYVideoSetup *video_setup);
void syVideoApplySettingsNoBlock(SYTaskVi *vi);
u32 syVideoGetFillColor(u32 color);

#endif
