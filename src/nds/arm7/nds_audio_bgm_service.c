#include <calico.h>
#include <string.h>
#include <nds/nds_audio_storage.h>
#include <nds/nds_bgm_ipc.h>

_Static_assert(PxiChannel_User1 == NDS_BGM_IPC_CHANNEL, "BGM PXI channel ABI");
#define BGM_REFILL_EVENT 0x80000000u
#define BGM_CHANNEL NDS_BGM_HW_CHANNEL

static Thread sBgmThread;
/* Main RAM (.sbss): ARM7's image must fit its private WRAM below the DLDI
 * shelter now that ARM9 owns shared WRAM (linker/nds_arm7_ds7_arm9wram.ld).
 * The worker runs once per refill; the storage thread's stack, which runs
 * under every read, stays in WRAM. */
static uint8_t sBgmStack[3072]
    __attribute__((aligned(8), section(".sbss.sBgmStack")));
static Mailbox sBgmMailbox;
static uint32_t sBgmMessages[16];
/* Calico clears a one-shot's callback after invoking it. Alternate two tasks
 * so a callback never rearms the same object that the scheduler is retiring. */
static TickTask sBgmTimers[2];
static uint32_t sTimerQueued[2];
static const NdsBgmSpec *sSpecs;
static uint32_t sSpecCount;
static volatile NdsBgmReport *sReport;
static NdsBgmReport sStats;
static NdsBgmStream sStream;
static NdsBgmPacket sPackets[2];
static uint32_t sPeriods[2];
static uint8_t sBuffers[2][NDS_AUDIO_BGM_PACKET_BYTES] __attribute__((aligned(4)));
static volatile uint32_t sState, sGeneration, sCurrent, sReady, sPending;
static uint64_t sPlayedSamples;
static uint64_t sStartTick;
static uint32_t sVolume = 0x7800u;
/* VOLUME is a doorbell: the PXI handler keeps the latest value and lets at
 * most one VOLUME message wait in the mailbox, so a run of volume changes
 * can never fill it (a full mailbox fails the stream). */
static volatile uint32_t sVolumeLatest, sVolumeQueued;

static void ndsBgmPublish(void)
{
    if (!sReport) return;
    IrqState lock = irqLock();
    uint32_t sequence = sReport->sequence;
    sReport->sequence = sequence + 1u;
    sStats.control = ndsBgmControl(sGeneration, sState);
    sStats.current_buffer = sCurrent;
    sStats.current_samples = sPackets[sCurrent].samples;
    sStats.current_bytes = sPackets[sCurrent].bytes;
    sStats.played_samples_lo = (uint32_t)sPlayedSamples;
    sStats.played_samples_hi = (uint32_t)(sPlayedSamples >> 32);
    sStats.loaded_samples_lo = (uint32_t)sStream.source_samples_loaded;
    sStats.loaded_samples_hi = (uint32_t)(sStream.source_samples_loaded >> 32);
    if (sState == NDS_BGM_PLAYING || sState == NDS_BGM_NATURAL)
    {
        uint64_t elapsed = (tickGetCount() - sStartTick) * 64u;
        sStats.elapsed_ticks_lo = (uint32_t)elapsed;
        sStats.elapsed_ticks_hi = (uint32_t)(elapsed >> 32);
    }
    sStats.volume = sVolume;
    const uint32_t *in = (const uint32_t *)&sStats;
    volatile uint32_t *out = (volatile uint32_t *)sReport;
    for (unsigned i = 2; i < sizeof(sStats)/4; ++i) out[i] = in[i];
    sReport->control = sStats.control;
    sReport->sequence = sequence + 2u;
    irqUnlock(lock);
}

static uint32_t ndsBgmHardwareVolume(void)
{
    return sVolume >= 0x7800u ? 127u : sVolume * 127u / 0x7800u;
}

static int ndsBgmRead(uint32_t offset, void *out, uint32_t bytes)
{
    if (!ndsAudioStorageReadRom(offset, out, bytes)) return 0;
    sStats.reads++; sStats.read_bytes += bytes;
    return 1;
}

static void ndsBgmStopHardware(uint32_t state)
{
    IrqState lock = irqLock();
    for (uint32_t i = 0; i < 2; ++i)
    {
        if (sTimerQueued[i]) tickTaskStop(&sBgmTimers[i]);
        sTimerQueued[i] = 0u;
    }
    soundChStop(BGM_CHANNEL); soundChStop(BGM_CHANNEL + 1u);
    sReady = sPending = 0u;
    sState = state;
    irqUnlock(lock);
}

static void ndsBgmFail(uint32_t error)
{
    ndsBgmStopHardware(NDS_BGM_FAILED);
    sStats.error = error; sStats.error_stops++;
    if (error == NDS_BGM_ERROR_HEADER) sStats.header_errors++;
    if (error == NDS_BGM_ERROR_PACKET) sStats.packet_errors++;
    if (error == NDS_BGM_ERROR_READ) sStats.read_errors++;
    ndsBgmPublish();
}

static void ndsBgmTimerCallback(TickTask *task);
static uint32_t ndsBgmTimerPeriod(uint32_t samples)
{
    /* Preserve the original /1024 timer's rounded deadline, expressed in the
     * SDK's /64 tick units. Calico's timer resources remain shared with mic. */
    uint32_t period = (uint32_t)(((uint64_t)samples * (SYSTEM_CLOCK >> 10) +
                                 NDS_AUDIO_BGM_SAMPLE_RATE/2u) / NDS_AUDIO_BGM_SAMPLE_RATE);
    if (!period) period = 1u;
    return period * 16u;
}

static void ndsBgmStartBuffer(uint32_t buffer)
{
    soundChStart(BGM_CHANNEL + buffer);
    sCurrent = buffer;
    sReady &= ~(1u << buffer);
    sStats.chunks++;
    sStats.loops_played += sPackets[buffer].loop_restart;
    sTimerQueued[buffer] = 1u;
    tickTaskStart(&sBgmTimers[buffer], ndsBgmTimerCallback, sPeriods[buffer], 0u);
}

static void ndsBgmTimerCallback(TickTask *task)
{
    uint32_t fired = task == &sBgmTimers[1] ? 1u : 0u;
    sTimerQueued[fired] = 0u;
    if (sState != NDS_BGM_PLAYING) return;
    uint32_t old = sCurrent, next = old ^ 1u;
    sPlayedSamples += sPackets[old].samples;
    if (!(sReady & (1u << next)))
    {
        if (sPackets[old].final)
        {
            ndsBgmStopHardware(NDS_BGM_NATURAL);
            sStats.natural_stops++; sStats.last_natural_track = sStats.track_id;
            ndsBgmPublish();
        }
        else
        {
            sStats.seam_misses++;
            ndsBgmFail(NDS_BGM_ERROR_UNDERRUN);
        }
        return;
    }
    ndsBgmStartBuffer(next);
    sStats.seams++;
    sPending |= 1u << old;
    if (!mailboxTrySend(&sBgmMailbox, BGM_REFILL_EVENT | sGeneration))
    {
        sStats.event_drops++;
        ndsBgmFail(NDS_BGM_ERROR_QUEUE);
    }
    else ndsBgmPublish();
}

static int ndsBgmFill(uint32_t buffer)
{
    /* One-shot hardware may still be draining the padded final word. Its
     * enable bit owns the buffer until completion; never overwrite that DMA. */
    while (soundChIsActive(BGM_CHANNEL + buffer)) threadSleepTicks(1u);
    uint32_t before_loops = sStream.loop_count;
    uint32_t start = (uint32_t)tickGetCount();
    NdsBgmPacket packet;
    int result = ndsBgmStreamRead(&sStream, &packet, sBuffers[buffer], ndsBgmRead);
    uint32_t elapsed = ((uint32_t)tickGetCount() - start) * 64u;
    sStats.refill_ticks_last = elapsed;
    if (elapsed > sStats.refill_ticks_max) sStats.refill_ticks_max = elapsed;
    if (result == 0) return 0;
    if (result < 0) return result;
    uint32_t period = ndsBgmTimerPeriod(packet.samples);
    IrqState lock = irqLock();
    if (sState != NDS_BGM_STARTING && sState != NDS_BGM_PLAYING)
    {
        irqUnlock(lock);
        return -1;
    }
    sPackets[buffer] = packet;
    sPeriods[buffer] = period;
    soundChPreparePcm(BGM_CHANNEL + buffer, ndsBgmHardwareVolume(), SoundVolDiv_1, 64u,
                     soundTimerFromHz(NDS_AUDIO_BGM_SAMPLE_RATE), SoundMode_OneShot,
                     SoundFmt_ImaAdpcm, sBuffers[buffer], 0u, packet.bytes/4u);
    sReady |= 1u << buffer;
    sStats.prepared++; sStats.loops_loaded += sStream.loop_count - before_loops;
    irqUnlock(lock);
    return 1;
}

static void ndsBgmPlay(uint32_t argument)
{
    uint32_t index = argument & 63u;
    uint32_t generation = argument >> 6;
    ndsBgmStopHardware(NDS_BGM_STOPPED);
    sGeneration = generation;
    sStats.play_commands++;
    if (!sSpecs || index >= sSpecCount || !generation)
    {
        ndsBgmFail(NDS_BGM_ERROR_COMMAND);
        return;
    }
    sStats.track_id = sSpecs[index].track_id;
    sCurrent = 0u; sPlayedSamples = 0u;
    memset(sPackets, 0, sizeof(sPackets));
    sState = NDS_BGM_STARTING;
    ndsBgmPublish();
    if (!ndsBgmStreamOpen(&sStream, sSpecs + index, ndsBgmRead) ||
        ndsBgmFill(0) != 1 || ndsBgmFill(1) < 0)
    {
        ndsBgmFail(sStream.error ? sStream.error : NDS_BGM_ERROR_PACKET);
        return;
    }
    IrqState lock = irqLock();
    sState = NDS_BGM_PLAYING;
    sStats.initial_loaded_samples = (uint32_t)sStream.source_samples_loaded;
    sStartTick = tickGetCount();
    ndsBgmStartBuffer(0);
    irqUnlock(lock);
    ndsBgmPublish();
}

static int ndsBgmInit(uint32_t address)
{
    if ((address & 31u) || !ndsAudioStorageMainRange(address, sizeof(NdsBgmInit))) return 0;
    const NdsBgmInit *init = (const NdsBgmInit *)(uintptr_t)address;
    if (init->abi != NDS_BGM_IPC_ABI || !init->spec_count || init->spec_count > NDS_BGM_MAX_TRACKS ||
        (init->specs & 31u) || (init->report & 31u) ||
        !ndsAudioStorageMainRange(init->specs, init->spec_count * sizeof(NdsBgmSpec)) ||
        !ndsAudioStorageMainRange(init->report, sizeof(NdsBgmReport)) ||
        init->reserved[0] || init->reserved[1] || init->reserved[2] || init->volume > 0x7800u)
        return 0;
    uint32_t spec_end = init->specs + init->spec_count * sizeof(NdsBgmSpec);
    uint32_t report_end = init->report + sizeof(NdsBgmReport);
    if ((init->specs < report_end && init->report < spec_end) ||
        (address < report_end && init->report < address + sizeof(*init))) return 0;
    const NdsBgmSpec *specs = (const NdsBgmSpec *)(uintptr_t)init->specs;
    uint32_t media_size = ndsAudioStorageRomSize();
    for (uint32_t i = 0; i < init->spec_count; ++i)
        if (specs[i].rom_offset > media_size || specs[i].file_bytes > media_size - specs[i].rom_offset)
            return 0;
    ndsBgmStopHardware(NDS_BGM_STOPPED);
    sSpecs = specs; sSpecCount = init->spec_count;
    sReport = (volatile NdsBgmReport *)(uintptr_t)init->report;
    sVolume = init->volume;
    memset(&sStats, 0, sizeof(sStats)); sStats.last_natural_track = UINT32_MAX;
    ndsBgmPublish();
    return 1;
}

static int ndsBgmWorker(void *unused)
{
    (void)unused;
    for (;;)
    {
        uint32_t message = mailboxRecv(&sBgmMailbox);
        if (message & BGM_REFILL_EVENT)
        {
            if ((message & NDS_BGM_GENERATION_MASK) != sGeneration || sState != NDS_BGM_PLAYING)
                continue;
            for (uint32_t buffer = 0; buffer < 2; ++buffer)
            {
                if (!(sPending & (1u << buffer))) continue;
                int result = ndsBgmFill(buffer);
                IrqState lock = irqLock();
                sPending &= ~(1u << buffer);
                irqUnlock(lock);
                if (result < 0)
                {
                    if (sState != NDS_BGM_FAILED) ndsBgmFail(sStream.error);
                    break;
                }
                if (result > 0) sStats.refills++;
            }
            ndsBgmPublish();
            continue;
        }
        uint32_t operation = message >> 23, argument = message & 0x7fffffu;
        uint32_t reply = message;
        switch (operation)
        {
        case NDS_BGM_INIT:
            if (!ndsBgmInit(argument << 5)) reply = NDS_BGM_REPLY_ERROR;
            pxiReply((PxiChannel)NDS_BGM_IPC_CHANNEL, reply);
            break;
        case NDS_BGM_PLAY:
            ndsBgmPlay(argument);
            break;
        case NDS_BGM_STOP:
        case NDS_BGM_RESET:
            ndsBgmStopHardware(NDS_BGM_STOPPED);
            sGeneration = argument & NDS_BGM_GENERATION_MASK;
            if (operation == NDS_BGM_RESET)
            {
                memset(&sStats, 0, sizeof(sStats));
                sStats.last_natural_track = UINT32_MAX;
                memset(&sStream, 0, sizeof(sStream));
                memset(sPackets, 0, sizeof(sPackets)); sPlayedSamples = 0;
            }
            else sStats.stop_commands++;
            ndsBgmPublish();
            pxiReply((PxiChannel)NDS_BGM_IPC_CHANNEL, reply);
            break;
        case NDS_BGM_VOLUME:
        {
            IrqState lock = irqLock();
            argument = sVolumeLatest;
            sVolumeQueued = 0u;
            irqUnlock(lock);
        }
            if (argument > 0x7800u) argument = 0x7800u;
            sVolume = argument;
            soundChSetVolume(BGM_CHANNEL, ndsBgmHardwareVolume(), SoundVolDiv_1);
            soundChSetVolume(BGM_CHANNEL + 1u, ndsBgmHardwareVolume(), SoundVolDiv_1);
            ndsBgmPublish();
            break;
        default:
            sStats.bad_commands++;
            ndsBgmFail(NDS_BGM_ERROR_COMMAND);
            break;
        }
    }
    return 0;
}

static void ndsBgmCommandHandler(void *unused, uint32_t message)
{
    (void)unused;
    if ((message >> 23) == NDS_BGM_VOLUME)
    {
        sVolumeLatest = message & 0x7fffffu;
        if (!sVolumeQueued && mailboxTrySend(&sBgmMailbox, message))
            sVolumeQueued = 1u;
        return;
    }
    if (!mailboxTrySend(&sBgmMailbox, message))
    {
        sStats.event_drops++;
        ndsBgmFail(NDS_BGM_ERROR_QUEUE);
        uint32_t operation = message >> 23;
        if (operation == NDS_BGM_INIT || operation == NDS_BGM_STOP || operation == NDS_BGM_RESET)
            pxiReply((PxiChannel)NDS_BGM_IPC_CHANNEL, NDS_BGM_REPLY_ERROR);
    }
}

void ndsArm7BgmStartService(void)
{
    mailboxPrepare(&sBgmMailbox, sBgmMessages, 16u);
    pxiSetHandler((PxiChannel)NDS_BGM_IPC_CHANNEL, ndsBgmCommandHandler, NULL);
    threadPrepare(&sBgmThread, ndsBgmWorker, NULL, sBgmStack + sizeof(sBgmStack), 10);
    threadStart(&sBgmThread);
}
