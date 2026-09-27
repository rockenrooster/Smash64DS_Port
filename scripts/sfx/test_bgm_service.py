"""Execute the ARM7 BGM service with virtual IRQ, media and sound hardware."""
from pathlib import Path
from test_audio_storage import run_c

ROOT = Path(__file__).resolve().parents[2]


def test_service_commands_eof_loop_reentry_underrun_and_queue_failure():
    source = (ROOT / "src/nds/arm7/nds_audio_bgm_service.c").read_text()
    source = "\n".join(x for x in source.splitlines() if not x.startswith("#include"))
    fixture = r'''
#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include <nds/nds_audio_storage.h>
#include <nds/nds_bgm_ipc.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#define SYSTEM_CLOCK 33513982u
enum { PxiChannel_User1=24, SoundVolDiv_1=0, SoundMode_OneShot=2, SoundFmt_ImaAdpcm=2 };
typedef unsigned PxiChannel;
typedef unsigned IrqState;
typedef struct { int dummy; } Thread;
typedef struct { uint32_t data[16]; unsigned head,count; } Mailbox;
typedef struct TickTask { void (*fn)(struct TickTask*); uint32_t delay,queued,executing; } TickTask;
static jmp_buf done;
static unsigned irq_depth, replies, last_reply, fail_read, reads, sleep_calls, wait_channel;
static uint64_t now;
static bool active[2];
static const uint8_t *hardware[2];
static unsigned hardware_bytes[2], hardware_volume[2];
static uint8_t media[1024];
static IrqState irqLock(void) { return irq_depth++; }
static void irqUnlock(IrqState state) { assert(irq_depth==state+1); --irq_depth; }
static uint64_t tickGetCount(void) { return now; }
static void tickTaskStop(TickTask *t) { assert(!t->executing);t->fn=NULL;t->queued=0; }
static void tickTaskStart(TickTask *t,void (*fn)(TickTask*),uint32_t delay,uint32_t period) {
    assert(delay && !period && !t->queued && !t->executing);t->fn=fn;t->delay=delay;t->queued=1;
}
static unsigned soundTimerFromHz(unsigned hz) { return (SYSTEM_CLOCK/2+hz/2)/hz; }
static void soundChStart(unsigned ch) { assert(ch==14||ch==15);active[ch-14]=true; }
static void soundChStop(unsigned ch) { assert(ch==14||ch==15);active[ch-14]=false; }
static bool soundChIsActive(unsigned ch) { wait_channel=ch-14;return active[ch-14]; }
static void threadSleepTicks(unsigned ticks) { assert(ticks==1);sleep_calls++;active[wait_channel]=false; }
static void soundChSetVolume(unsigned ch,unsigned vol,unsigned div) {
    assert(ch>=14&&ch<=15&&vol<=127&&!div);hardware_volume[ch-14]=vol;
}
static void soundChPreparePcm(unsigned ch,unsigned vol,unsigned div,unsigned pan,unsigned timer,
                              unsigned mode,unsigned format,const void *data,unsigned pnt,unsigned len) {
    assert(ch>=14&&ch<=15&&!active[ch-14]&&!div&&pan==64&&timer==760&&mode==2&&format==2&&!pnt);
    hardware[ch-14]=data;hardware_bytes[ch-14]=len*4;hardware_volume[ch-14]=vol;
}
static void mailboxPrepare(Mailbox *m,uint32_t *buf,unsigned size) { (void)buf;assert(size==16);memset(m,0,sizeof(*m)); }
static bool mailboxTrySend(Mailbox *m,uint32_t value) {
    if(m->count==16)return false;m->data[(m->head+m->count++)%16]=value;return true;
}
static uint32_t mailboxRecv(Mailbox *m) {
    if(!m->count)longjmp(done,1);uint32_t value=m->data[m->head++%16];m->count--;return value;
}
static void pxiReply(PxiChannel ch,uint32_t value) { assert(ch==24);replies++;last_reply=value; }
static void pxiSetHandler(PxiChannel ch,void (*fn)(void*,uint32_t),void *p) { assert(ch==24&&fn&&!p); }
static void threadPrepare(Thread *t,int (*fn)(void*),void *arg,void *stack,unsigned priority) {
    (void)t;assert(fn&&!arg&&stack&&priority==10);
}
static void threadStart(Thread *t) { (void)t; }
uint32_t ndsAudioStorageRomSize(void) { return sizeof(media); }
int ndsAudioStorageReadRom(uint32_t offset,void *out,uint32_t bytes) {
    assert(offset<=sizeof(media)&&bytes<=sizeof(media)-offset);
    for(unsigned i=0;i<2;++i)
        if(active[i]) assert((uintptr_t)out+bytes <= (uintptr_t)hardware[i] ||
                              (uintptr_t)out >= (uintptr_t)hardware[i]+hardware_bytes[i]);
    reads++;if(fail_read)return 0;memcpy(out,media+offset,bytes);return 1;
}
''' + f'\n#include "{(ROOT / "src/nds/nds_bgm_stream.c").as_posix()}"\n' + source + r'''
static void pump(void) { if(!setjmp(done))ndsBgmWorker(NULL); }
static void command(unsigned op,unsigned arg) { ndsBgmCommandHandler(NULL,ndsBgmCommand(op,arg));pump(); }
static void fire(unsigned hardware_done) {
    TickTask *task=&sBgmTimers[sCurrent];
    void (*fn)(TickTask*)=task->fn;assert(fn);now+=task->delay;
    assert(task->queued);task->queued=0;task->executing=1;
    if(hardware_done)active[sCurrent]=false;fn(task);task->fn=NULL;task->executing=0;
}
static void word(uint8_t *p,uint32_t n) { for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(n>>(8*i)); }
static NdsBgmSpec asset(unsigned base,unsigned packets,bool loop,unsigned id) {
    uint8_t *p=media+base;memset(p,0,40+packets*16);
    word(p,0x31414742);p[4]=1;p[6]=40;word(p+8,22050);word(p+12,packets*8);
    word(p+16,loop?0:UINT32_MAX);word(p+20,16384);word(p+24,packets);
    word(p+28,loop?0:UINT32_MAX);word(p+32,loop?40:0);word(p+36,loop);
    for(unsigned i=0;i<packets;++i){word(p+40+i*16,8);word(p+44+i*16,8);}
    return (NdsBgmSpec){base,40+packets*16,packets*8,loop?0:UINT32_MAX,
                        packets,loop?0:UINT32_MAX,loop?40:0,id};
}
int main(void) {
    void *memory;
#ifdef _WIN32
    memory=VirtualAlloc((void*)0x02010000,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
#else
    memory=mmap((void*)0x02010000,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
#endif
    assert(memory==(void*)0x02010000);
    NdsBgmSpec *spec=memory;NdsBgmReport *report=(void*)((uint8_t*)memory+128);
    NdsBgmInit *init=(void*)((uint8_t*)memory+320);
    spec[0]=asset(64,2,false,12);spec[1]=asset(256,1,true,0);
    *init=(NdsBgmInit){NDS_BGM_IPC_ABI,(uint32_t)(uintptr_t)spec,2,(uint32_t)(uintptr_t)report,0x7800,{0}};
    ndsArm7BgmStartService();
    unsigned setup=(uint32_t)(uintptr_t)init>>5;
    init->report=init->specs;command(NDS_BGM_INIT,setup);assert(last_reply==NDS_BGM_REPLY_ERROR);
    init->report=(uint32_t)(uintptr_t)report;command(NDS_BGM_INIT,setup);
    assert(last_reply==ndsBgmCommand(NDS_BGM_INIT,setup));
    unsigned ack=replies;command(NDS_BGM_PLAY,1<<6);
    assert(replies==ack&&sState==NDS_BGM_PLAYING&&active[0]&&sReady==2&&report->chunks==1);
    assert(hardware_volume[0]==127&&report->track_id==12);
    fire(1);assert(sCurrent==1&&sPending==1);
    // Natural completion must not depend on servicing an empty refill first.
    fire(1);assert(sState==NDS_BGM_NATURAL&&report->natural_stops==1&&!active[0]&&!active[1]);pump();
    command(NDS_BGM_PLAY,(2<<6)|1);fire(0);pump();
    assert(sleep_calls&&report->loops_played&&sState==NDS_BGM_PLAYING&&sReady);
    command(NDS_BGM_VOLUME,0x3c00);assert(hardware_volume[0]==63&&hardware_volume[1]==63);
    // New play can precede an old timer message. The old generation cannot refill it.
    ndsBgmCommandHandler(NULL,ndsBgmCommand(NDS_BGM_PLAY,3<<6));fire(1);
    unsigned before=reads;pump();assert(sGeneration==3&&reads==before+5);
    command(NDS_BGM_STOP,4);assert(last_reply==ndsBgmCommand(NDS_BGM_STOP,4)&&!active[0]&&!active[1]);
    command(NDS_BGM_PLAY,(5<<6)|1);fire(1);fire(1);
    assert(sState==NDS_BGM_FAILED&&report->seam_misses==1&&report->error==NDS_BGM_ERROR_UNDERRUN);pump();
    command(NDS_BGM_RESET,6);assert(!report->error_stops&&!report->reads);
    fail_read=1;command(NDS_BGM_PLAY,7<<6);assert(sState==NDS_BGM_FAILED&&report->read_errors==1);fail_read=0;
    command(NDS_BGM_RESET,8);
    for(unsigned i=0;i<16;++i)ndsBgmCommandHandler(NULL,ndsBgmCommand(NDS_BGM_VOLUME,i));
    ndsBgmCommandHandler(NULL,ndsBgmCommand(NDS_BGM_STOP,9));
    assert(report->event_drops==1&&last_reply==NDS_BGM_REPLY_ERROR&&sState==NDS_BGM_FAILED);
    assert(!irq_depth);
#ifdef _WIN32
    assert(VirtualFree(memory,0,MEM_RELEASE));
#else
    assert(!munmap(memory,4096));
#endif
    return 0;
}
'''
    run_c(fixture, extra_flags=("-Wno-misleading-indentation",))
