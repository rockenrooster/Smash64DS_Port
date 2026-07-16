#ifndef SSB64_NDS_FREEZE_DIAGNOSTICS_H
#define SSB64_NDS_FREEZE_DIAGNOSTICS_H

#include <PR/ultratypes.h>

#ifndef NDS_FREEZE_DIAGNOSTICS
#define NDS_FREEZE_DIAGNOSTICS 0
#endif

#if NDS_FREEZE_DIAGNOSTICS

#define NDS_FREEZE_DIAGNOSTICS_BREADCRUMB_COUNT 8u

enum NDSFreezeDiagnosticsBreadcrumb {
    NDS_FREEZE_BREADCRUMB_UPDATE_START = 0x55504454u, /* UPDT */
    NDS_FREEZE_BREADCRUMB_HIT_SEARCH = 0x48495453u,   /* HITS */
    NDS_FREEZE_BREADCRUMB_DAMAGE_ENTER = 0x44414d47u, /* DAMG */
    NDS_FREEZE_BREADCRUMB_EFFECT_SPAWN = 0x45464658u, /* EFFX */
    NDS_FREEZE_BREADCRUMB_FGM_ENTER = 0x46474d49u,    /* FGMI */
    NDS_FREEZE_BREADCRUMB_FGM_RETURN = 0x46474d4fu,   /* FGMO */
    NDS_FREEZE_BREADCRUMB_DRAW_START = 0x44524157u,   /* DRAW */
    NDS_FREEZE_BREADCRUMB_FLUSH = 0x464c5553u,        /* FLUS */
    NDS_FREEZE_BREADCRUMB_VBLANK_WAIT = 0x56424c4bu,  /* VBLK */
    NDS_FREEZE_BREADCRUMB_PRESENT_DONE = 0x50524553u   /* PRES */
};

void ndsFreezeDiagnosticsInit(void);
void ndsFreezeDiagnosticsBreadcrumb(u32 marker);
void ndsFreezeDiagnosticsFgmEnter(u16 fgm_id);
void ndsFreezeDiagnosticsFgmReturn(s32 channel);
void ndsFreezeDiagnosticsFlush(void);
void ndsFreezeDiagnosticsVBlankWait(void);
void ndsFreezeDiagnosticsHeartbeat(void);
void ndsFreezeDiagnosticsStallMarker(void);

extern volatile u32 gNdsFreezeDiagnosticsHeartbeat;
extern volatile u32 gNdsFreezeDiagnosticsLastBreadcrumb;
extern volatile u32 gNdsFreezeDiagnosticsBreadcrumbWriteCount;
extern volatile u32 gNdsFreezeDiagnosticsBreadcrumbs[
    NDS_FREEZE_DIAGNOSTICS_BREADCRUMB_COUNT];
extern volatile u32 gNdsFreezeDiagnosticsInterruptedPC;
extern volatile u32 gNdsFreezeDiagnosticsInterruptedLR;
extern volatile u32 gNdsFreezeDiagnosticsWatchdogTripCount;
extern volatile u32 gNdsFreezeDiagnosticsFgmEnterCount;
extern volatile u32 gNdsFreezeDiagnosticsFgmReturnCount;
extern volatile u32 gNdsFreezeDiagnosticsLastFgmID;
extern volatile s32 gNdsFreezeDiagnosticsLastFgmChannel;
extern volatile u32 gNdsFreezeDiagnosticsSwapPending;
extern volatile u32 gNdsFreezeDiagnosticsForceTrip;
extern volatile u32 gNdsFreezeDiagnosticsReportKind;
extern volatile u32 gNdsFreezeDiagnosticsReportGXStatus;
extern volatile u32 gNdsFreezeDiagnosticsReportPolygonCount;
extern volatile u32 gNdsFreezeDiagnosticsReportVertexCount;
extern volatile u32 gNdsFreezeDiagnosticsReportIpcFifo;
extern volatile u32 gNdsFreezeDiagnosticsReportIpcSync;
extern volatile u32 gNdsFreezeDiagnosticsReportPresentedFrames;
extern volatile u32 gNdsFreezeDiagnosticsReportLogicFrames;

#define NDS_FREEZE_DIAGNOSTICS_INIT() ndsFreezeDiagnosticsInit()
#define NDS_FREEZE_DIAGNOSTICS_MARK(marker) \
    ndsFreezeDiagnosticsBreadcrumb((marker))
#define NDS_FREEZE_DIAGNOSTICS_FGM_ENTER(fgm_id) \
    ndsFreezeDiagnosticsFgmEnter((fgm_id))
#define NDS_FREEZE_DIAGNOSTICS_FGM_RETURN(channel) \
    ndsFreezeDiagnosticsFgmReturn((channel))
#define NDS_FREEZE_DIAGNOSTICS_FLUSH() ndsFreezeDiagnosticsFlush()
#define NDS_FREEZE_DIAGNOSTICS_VBLANK_WAIT() \
    ndsFreezeDiagnosticsVBlankWait()
#define NDS_FREEZE_DIAGNOSTICS_HEARTBEAT() \
    ndsFreezeDiagnosticsHeartbeat()

#else

#define NDS_FREEZE_DIAGNOSTICS_INIT() ((void)0)
#define NDS_FREEZE_DIAGNOSTICS_MARK(marker) ((void)0)
#define NDS_FREEZE_DIAGNOSTICS_FGM_ENTER(fgm_id) ((void)0)
#define NDS_FREEZE_DIAGNOSTICS_FGM_RETURN(channel) ((void)0)
#define NDS_FREEZE_DIAGNOSTICS_FLUSH() ((void)0)
#define NDS_FREEZE_DIAGNOSTICS_VBLANK_WAIT() ((void)0)
#define NDS_FREEZE_DIAGNOSTICS_HEARTBEAT() ((void)0)

#endif

#endif
