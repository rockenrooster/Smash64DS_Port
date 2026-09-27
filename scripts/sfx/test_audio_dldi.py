"""Run the actual public DLDI adapter with two host callers and driver export."""
from pathlib import Path
from test_audio_storage import run_c

ROOT = Path(__file__).resolve().parents[2]


def test_serialized_driver_chunks_failures_reentry_and_unchanged_export():
    source = (ROOT / "src/nds/arm7/nds_audio_dldi.c").read_text()
    source = "\n".join(line for line in source.splitlines() if not line.startswith("#include"))
    source = source.replace("#define NDS_AUDIO_DLDI_ADDRESS 0x0380b000u",
                            "#define NDS_AUDIO_DLDI_ADDRESS ((uintptr_t)driver)")
    run_c(r'''
#define _GNU_SOURCE
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <sched.h>
typedef uint32_t sec_t;
typedef struct { pthread_mutex_t mutex; } RMutex;
static void rmutexLock(RMutex *m) { assert(!pthread_mutex_lock(&m->mutex)); }
static void rmutexUnlock(RMutex *m) { assert(!pthread_mutex_unlock(&m->mutex)); }
typedef struct DISC_INTERFACE {
    uint32_t ioType, features;
    bool (*startup)(void), (*isInserted)(void);
    bool (*readSectors)(sec_t,sec_t,void*);
    bool (*writeSectors)(sec_t,sec_t,const void*);
    bool (*clearStatus)(void), (*shutdown)(void);
} DISC_INTERFACE;
typedef struct DldiHeader {
    uint32_t magic_num; char magic_str[8];
    uint8_t version_num,driver_sz_log2,fix_flags,alloc_sz_log2;
    char iface_name[48];
    uintptr_t dldi_start,dldi_end,glue_start,glue_end,got_start,got_end,bss_start,bss_end;
    DISC_INTERFACE disc;
} DldiHeader;
#define DLDI_MAGIC_VAL 0xbf8da5ed
#define DLDI_MAGIC_STRING " Chishm"
#define DLDI_MAGIC_STRING_LEN 8
#define DLDI_FEATURE_CAN_READ 1
#define DLDI_FEATURE_CAN_WRITE 2
#define DLDI_SIZE_MAX 14
static uint8_t driver[4096] __attribute__((aligned(32)));
static struct { uint32_t dldi_features,dldi_io_type; } env;
#define g_envExtraInfo (&env)
static _Atomic unsigned active, overlap, callback_count, fail_at;
static bool simple(void) { return true; }
static bool read_driver(sec_t sector,sec_t count,void *out) {
    assert(count <= 16);
    if (atomic_fetch_add(&active,1)) atomic_fetch_add(&overlap,1);
    for (unsigned i=0;i<8;++i) sched_yield();
    memset(out, (int)sector, count*512);
    unsigned n=atomic_fetch_add(&callback_count,1)+1;
    atomic_fetch_sub(&active,1);
    return n!=fail_at;
}
static bool write_driver(sec_t sector,sec_t count,const void *in) {
    (void)in; uint8_t scratch[16*512]; return read_driver(sector,count,scratch);
}
void __real_armCopyMem32(void *dst,const void *src,size_t size) { memcpy(dst,src,size); }
''' + source + r'''
static void *reader(void *arg) {
    uint8_t buffer[65*512]; (void)arg;
    for (unsigned i=0;i<40;++i) assert(((DldiHeader*)driver)->disc.readSectors(7,65,buffer));
    return NULL;
}
static void *writer(void *arg) {
    uint8_t buffer[65*512]={0}; (void)arg;
    for (unsigned i=0;i<40;++i) assert(((DldiHeader*)driver)->disc.writeSectors(9,65,buffer));
    return NULL;
}
int main(void) {
    pthread_mutexattr_t attr;
    assert(!pthread_mutexattr_init(&attr));
    assert(!pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE));
    assert(!pthread_mutex_init(&sDldiMutex.mutex,&attr));
    assert(ndsAudioDldiInstall() && !gNdsArm7DldiSerialized); // no device
    memset(driver,0xa5,sizeof(driver));
    DldiHeader *h=(DldiHeader*)driver;
    env.dldi_features=3;env.dldi_io_type=0x524f4d;
    h->magic_num=DLDI_MAGIC_VAL;memcpy(h->magic_str,DLDI_MAGIC_STRING,8);
    h->dldi_start=(uintptr_t)driver;h->driver_sz_log2=12;
    h->disc=(DISC_INTERFACE){env.dldi_io_type,3,simple,simple,read_driver,write_driver,simple,simple};
    uint8_t before[sizeof(driver)], exported[sizeof(driver)];
    memcpy(before,driver,sizeof(driver));
    h->magic_num=0;assert(!ndsAudioDldiInstall());h->magic_num=DLDI_MAGIC_VAL;
    assert(ndsAudioDldiInstall() && gNdsArm7DldiSerialized);
    assert(ndsAudioDldiInstall() && sDldiOriginal.readSectors==read_driver);
    pthread_t a,b;assert(!pthread_create(&a,NULL,reader,NULL));assert(!pthread_create(&b,NULL,writer,NULL));
    assert(!pthread_join(a,NULL));assert(!pthread_join(b,NULL));
    assert(!overlap && callback_count==400 && gNdsArm7DldiReads==200 && gNdsArm7DldiWrites==200);
    uint8_t buf[33*512];unsigned previous=callback_count;
    fail_at=previous+2;assert(!h->disc.readSectors(0,33,buf));assert(callback_count==previous+2);
    fail_at=0;
    assert(!h->disc.readSectors(UINT32_MAX,2,buf));
    assert(h->disc.startup() && h->disc.isInserted() && h->disc.clearStatus() && h->disc.shutdown());
    __wrap_armCopyMem32(exported,driver,sizeof(driver));
    assert(!memcmp(before,exported,sizeof(driver))); // chainloading gets the original driver
    __wrap_armCopyMem32(driver,driver,sizeof(driver));
    assert(h->disc.readSectors!=read_driver);
    assert(!pthread_mutex_destroy(&sDldiMutex.mutex));
    return 0;
}
''', extra_flags=("-pthread",))
