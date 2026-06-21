#include <stdint.h>

#include <PR/os.h>
#include <nds/nds_controller.h>
#include <nds/nds_os.h>
#include <sys/malloc.h>

typedef struct QueueThreadTest {
    OSMesgQueue input;
    OSMesg input_buffer[1];
    OSMesgQueue output;
    OSMesg output_buffer[1];
} QueueThreadTest;

static void ndsOsTestConsumer(void *arg)
{
    QueueThreadTest *test = arg;
    OSMesg message = NULL;

    if (osRecvMesg(&test->input, &message, OS_MESG_BLOCK) == 0) {
        osSendMesg(&test->output,
                   (OSMesg)((uintptr_t)message + 1u), OS_MESG_BLOCK);
    }
}

int ndsOsSelfTest(void)
{
    OSMesgQueue queue;
    OSMesg buffer[3];
    OSMesg message;
    QueueThreadTest thread_test;
    OSThread consumer;
    int controller_test;
    SYMallocRegion arena;
    u8 arena_data[64];
    void *allocation;

    controller_test = ndsControllerBackendSelfTest();
    if (controller_test != 0) return 20 + controller_test;

    syMallocInit(&arena, 1, arena_data, sizeof(arena_data));
    allocation = syMallocSet(&arena, 5, 8);
    if (((uintptr_t)allocation & 7u) != 0) return 30;
    if ((u8 *)arena.ptr != (u8 *)allocation + 5) return 31;
    syMallocReset(&arena);
    if (arena.ptr != arena.start) return 32;

    osCreateMesgQueue(&queue, buffer, 3);
    if (osRecvMesg(&queue, &message, OS_MESG_NOBLOCK) != -1) return 1;
    if (osSendMesg(&queue, (OSMesg)1, OS_MESG_NOBLOCK) != 0) return 2;
    if (osSendMesg(&queue, (OSMesg)2, OS_MESG_NOBLOCK) != 0) return 3;
    if (osJamMesg(&queue, (OSMesg)0, OS_MESG_NOBLOCK) != 0) return 4;
    if (osSendMesg(&queue, (OSMesg)3, OS_MESG_NOBLOCK) != -1) return 5;
    if (osRecvMesg(&queue, &message, OS_MESG_NOBLOCK) != 0 ||
        (uintptr_t)message != 0u) return 6;
    if (osRecvMesg(&queue, &message, OS_MESG_NOBLOCK) != 0 ||
        (uintptr_t)message != 1u) return 7;
    if (osRecvMesg(&queue, &message, OS_MESG_NOBLOCK) != 0 ||
        (uintptr_t)message != 2u) return 8;

    osCreateMesgQueue(&thread_test.input, thread_test.input_buffer, 1);
    osCreateMesgQueue(&thread_test.output, thread_test.output_buffer, 1);
    osCreateThread(&consumer, 90, ndsOsTestConsumer, &thread_test, NULL,
                   50);
    osStartThread(&consumer);
    if (consumer.state != OS_STATE_WAITING) return 9;
    if (osSendMesg(&thread_test.input, (OSMesg)0x1234,
                   OS_MESG_NOBLOCK) != 0) return 10;

    ndsOsRunThreads();
    if (osRecvMesg(&thread_test.output, &message,
                   OS_MESG_NOBLOCK) != 0) return 11;
    if ((uintptr_t)message != 0x1235u) return 12;
    if (consumer.state != OS_STATE_STOPPED) return 13;

    osDestroyThread(&consumer);
    return 0;
}
