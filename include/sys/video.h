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
    { &gSYFramebufferSets[0], &gSYFramebufferSets[1], &gSYFramebufferSets[2] }, \
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

extern u16 gSYZBuffer[(320 * 240) - (((320 * 240) - (320 * 230)) * sizeof(u16))];
extern u16 gSYFramebufferSets[3][230][320];
extern u16 *gSYVideoZBuffer;
extern u32 gSYVideoColorDepth;
extern s32 gSYVideoResWidth;
extern s32 gSYVideoResHeight;

void syVideoInit(SYVideoSetup *video_setup);
void syVideoApplySettingsNoBlock(SYTaskVi *vi);
u32 syVideoGetFillColor(u32 color);

#endif
