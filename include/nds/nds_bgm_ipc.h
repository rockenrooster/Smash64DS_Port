#ifndef NDS_BGM_IPC_H
#define NDS_BGM_IPC_H
#include <nds/nds_bgm_stream.h>

#define NDS_BGM_IPC_ABI 0x42474d32u
#define NDS_BGM_IPC_CHANNEL 24u
/* The two hardware channels the ARM7 stream alternates its one-shot buffers
 * on (nds_audio_bgm_service.c). The waiting buffer's channel is prepared but
 * not started, so a scan of active channels reads it as free; the ARM9 FGM
 * picker excludes the pair (nds_audio_fgm.c). */
#define NDS_BGM_HW_CHANNEL 14u
#define NDS_BGM_HW_CHANNEL_MASK (3u << NDS_BGM_HW_CHANNEL)
#define NDS_BGM_MAX_TRACKS 64u
#define NDS_BGM_GENERATION_MASK 0x1ffffu
#define NDS_BGM_REPLY_ERROR 0x03ffffffu
enum { NDS_BGM_INIT, NDS_BGM_PLAY, NDS_BGM_STOP, NDS_BGM_VOLUME, NDS_BGM_RESET };
enum { NDS_BGM_STOPPED, NDS_BGM_STARTING, NDS_BGM_PLAYING, NDS_BGM_NATURAL, NDS_BGM_FAILED };

/* Only ARM7 writes this report after INIT; ARM9 invalidates its cache before
 * reading. control is the atomic source-visible state. sequence protects the
 * wider diagnostic snapshot; it is not a second playback state machine. */
typedef struct NdsBgmReport {
    uint32_t control, sequence, track_id, error;
    uint32_t chunks, refills, reads, read_bytes;
    uint32_t loops_loaded, loops_played, prepared, seams;
    uint32_t seam_misses, event_drops, natural_stops, last_natural_track;
    uint32_t error_stops, current_buffer, current_samples, current_bytes;
    uint32_t played_samples_lo, played_samples_hi, loaded_samples_lo, loaded_samples_hi;
    uint32_t play_commands, stop_commands, volume, refill_ticks_last;
    uint32_t refill_ticks_max, bad_commands, header_errors, packet_errors;
    uint32_t read_errors, elapsed_ticks_lo, elapsed_ticks_hi, initial_loaded_samples;
    uint32_t reserved[4];
} __attribute__((aligned(32))) NdsBgmReport;

typedef struct NdsBgmInit {
    uint32_t abi, specs, spec_count, report;
    uint32_t volume, reserved[3];
} __attribute__((aligned(32))) NdsBgmInit;

_Static_assert(sizeof(NdsBgmReport) == 160u, "BGM report ABI");
_Static_assert(sizeof(NdsBgmInit) == 32u, "BGM init must own one cache line");
static inline uint32_t ndsBgmCommand(uint32_t operation, uint32_t argument)
{
    return (operation << 23) | (argument & 0x7fffffu);
}
static inline uint32_t ndsBgmControl(uint32_t generation, uint32_t state)
{
    return (generation & NDS_BGM_GENERATION_MASK) | (state << 17);
}
#endif
