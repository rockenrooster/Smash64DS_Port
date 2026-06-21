#ifndef SSB64_NDS_OS_H
#define SSB64_NDS_OS_H

#include <PR/ultratypes.h>

typedef s32 OSPri;
typedef s32 OSId;
typedef u32 OSEvent;
typedef u32 OSIntMask;
typedef u64 OSTime;
typedef void *OSMesg;

typedef struct OSPiHandle_s {
    u32 type;
} OSPiHandle;

typedef struct {
    u16 type;
    u8 status;
    u8 errno;
} OSContStatus;

typedef struct {
    u16 button;
    s8 stick_x;
    s8 stick_y;
    u8 errno;
} OSContPad;

typedef struct {
    int status;
    struct OSMesgQueue_s *queue;
    int channel;
} OSPfs;

typedef struct {
    u32 ctrl, width, burst, vSync, hSync, leap, hStart, xScale, vCurrent;
} OSViCommonRegs;

typedef struct {
    u32 origin, yScale, vStart, vBurst, vIntr;
} OSViFieldRegs;

typedef struct {
    u8 type;
    OSViCommonRegs comRegs;
    OSViFieldRegs fldRegs[2];
} OSViMode;

typedef union {
    struct { f32 f_odd, f_even; } f;
    f64 d;
} __OSfp;

/* Preserve the fields inspected by original debug and object code. */
typedef struct {
    u64 at, v0, v1, a0, a1, a2, a3;
    u64 t0, t1, t2, t3, t4, t5, t6, t7;
    u64 s0, s1, s2, s3, s4, s5, s6, s7;
    u64 t8, t9, gp, sp, s8, ra, lo, hi;
    u32 sr, pc, cause, badvaddr, rcp, fpcsr;
    __OSfp fp0, fp2, fp4, fp6, fp8, fp10, fp12, fp14;
    __OSfp fp16, fp18, fp20, fp22, fp24, fp26, fp28, fp30;
} __OSThreadContext;

typedef struct OSThread_s {
    struct OSThread_s *next;
    OSPri priority;
    struct OSThread_s **queue;
    struct OSThread_s *tlnext;
    u16 state;
    u16 flags;
    OSId id;
    int fp;
    __OSThreadContext context;

    /* DS backend state. Original game code does not access these fields. */
    void (*port_entry)(void *);
    void *port_arg;
    void *port_coroutine;
    u8 port_registered;
} OSThread;

typedef struct OSMesgQueue_s {
    OSThread *mtqueue;
    OSThread *fullqueue;
    s32 validCount;
    s32 first;
    s32 msgCount;
    OSMesg *msg;
} OSMesgQueue;

#define OS_STATE_STOPPED  1
#define OS_STATE_RUNNABLE 2
#define OS_STATE_RUNNING  4
#define OS_STATE_WAITING  8

#define OS_MESG_NOBLOCK 0
#define OS_MESG_BLOCK   1

#define OS_EVENT_SP     4
#define OS_EVENT_SI     5
#define OS_EVENT_VI     7
#define OS_EVENT_DP     9
#define OS_EVENT_PRENMI 14

#define OS_TV_PAL  0
#define OS_TV_NTSC 1
#define OS_TV_MPAL 2

#define MAXCONTROLLERS 4
#define CONT_NO_RESPONSE_ERROR 0x8
#define CONT_CARD_ON 0x01
#define CONT_TYPE_NORMAL 0x0005

#define A_BUTTON     0x8000
#define B_BUTTON     0x4000
#define Z_TRIG       0x2000
#define START_BUTTON 0x1000
#define U_JPAD       0x0800
#define D_JPAD       0x0400
#define L_JPAD       0x0200
#define R_JPAD       0x0100
#define L_TRIG       0x0020
#define R_TRIG       0x0010
#define U_CBUTTONS   0x0008
#define D_CBUTTONS   0x0004
#define L_CBUTTONS   0x0002
#define R_CBUTTONS   0x0001

#define OS_PRIORITY_MAX    255
#define OS_PRIORITY_VIMGR  254
#define OS_PRIORITY_PIMGR  150
#define OS_PRIORITY_APPMAX 127
#define OS_PRIORITY_IDLE   0

#define MQ_GET_COUNT(mq) ((mq)->validCount)
#define MQ_IS_EMPTY(mq) (MQ_GET_COUNT(mq) == 0)
#define MQ_IS_FULL(mq) (MQ_GET_COUNT(mq) >= (mq)->msgCount)
#define OS_DCACHE_ROUNDUP_SIZE(x) ((u32)((((u32)(x) + 0xfu) / 0x10u) * 0x10u))
#define OS_DCACHE_ROUNDUP_ADDR(x) ((void*)((((u32)(uintptr_t)(x)) + 0xfu) & ~0xfu))

extern s32 osTvType;
extern OSViMode osViModeNtscLan1;
extern OSViMode osViModeMpalLan1;

void osCreateThread(OSThread *thread, OSId id, void (*entry)(void *),
                    void *arg, void *sp, OSPri priority);
void osDestroyThread(OSThread *thread);
void osYieldThread(void);
void osStartThread(OSThread *thread);
void osStopThread(OSThread *thread);
OSId osGetThreadId(OSThread *thread);
void osSetThreadPri(OSThread *thread, OSPri priority);
OSPri osGetThreadPri(OSThread *thread);

void osCreateMesgQueue(OSMesgQueue *queue, OSMesg *buffer, s32 count);
s32 osSendMesg(OSMesgQueue *queue, OSMesg message, s32 flag);
s32 osJamMesg(OSMesgQueue *queue, OSMesg message, s32 flag);
s32 osRecvMesg(OSMesgQueue *queue, OSMesg *message, s32 flag);

OSTime osGetTime(void);
void osInitialize(void);
void osCreateViManager(OSPri priority);
OSPiHandle *osCartRomInit(void);
void osCreatePiManager(OSPri priority, OSMesgQueue *queue,
                       OSMesg *buffer, s32 count);
void osSetEventMesg(OSEvent event, OSMesgQueue *queue, OSMesg message);
u32 osGetCount(void);
void osInvalDCache(void *address, s32 size);
void osWritebackDCache(void *address, s32 size);
void osWritebackDCacheAll(void);
void *osViGetCurrentFramebuffer(void);
void *osViGetNextFramebuffer(void);
void osViSetYScale(f32 scale);
void osViSetMode(OSViMode *mode);
void osViSetEvent(OSMesgQueue *queue, OSMesg message, u32 retrace_count);
void osViSwapBuffer(void *framebuffer);
void osViBlack(u8 active);
s32 osDpSetNextBuffer(void *buffer, u64 size);
s32 osContInit(OSMesgQueue *queue, u8 *controller_bits,
               OSContStatus *status);
s32 osContStartQuery(OSMesgQueue *queue);
void osContGetQuery(OSContStatus *status);
s32 osContStartReadData(OSMesgQueue *queue);
void osContGetReadData(OSContPad *pad);
s32 osMotorInit(OSMesgQueue *queue, OSPfs *pfs, int channel);
s32 osMotorStart(OSPfs *pfs);
s32 osMotorStop(OSPfs *pfs);

#endif
