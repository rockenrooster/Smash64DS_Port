#ifndef NDS_BGM_STREAM_H
#define NDS_BGM_STREAM_H
#include <stdint.h>

#define NDS_AUDIO_BGM_SAMPLE_RATE 22050u
#define NDS_AUDIO_BGM_CONTAINER_MAGIC 0x31414742u
#define NDS_AUDIO_BGM_CONTAINER_VERSION 1u
#define NDS_AUDIO_BGM_CONTAINER_HEADER_BYTES 40u
#define NDS_AUDIO_BGM_PACKET_HEADER_BYTES 8u
#define NDS_AUDIO_BGM_PACKET_SAMPLES 16384u
#define NDS_AUDIO_BGM_PACKET_BYTES 8196u
#define NDS_AUDIO_BGM_BUFFER_COUNT 2u
#define NDS_AUDIO_BGM_RESIDENT_BYTES (NDS_AUDIO_BGM_BUFFER_COUNT * NDS_AUDIO_BGM_PACKET_BYTES)
#define NDS_BGM_NO_LOOP UINT32_MAX

/* Immutable source/producer contract, admitted on ARM9 before playback. */
typedef struct NdsBgmSpec {
    uint32_t rom_offset, file_bytes, source_samples, loop_sample;
    uint32_t packet_count, loop_packet, loop_record, track_id;
} NdsBgmSpec;
_Static_assert(sizeof(NdsBgmSpec) == 32u, "BGM specification ABI");

enum { NDS_BGM_ERROR_NONE, NDS_BGM_ERROR_HEADER, NDS_BGM_ERROR_PACKET,
       NDS_BGM_ERROR_READ, NDS_BGM_ERROR_UNDERRUN, NDS_BGM_ERROR_QUEUE,
       NDS_BGM_ERROR_COMMAND };

typedef int (*NdsBgmRead)(uint32_t offset, void *data, uint32_t bytes);
typedef struct NdsBgmStream {
    NdsBgmSpec spec;
    uint32_t offset, next_packet, error, loop_count, cycle_samples;
    uint64_t source_samples_loaded;
} NdsBgmStream;
typedef struct NdsBgmPacket {
    uint32_t samples, bytes, loop_restart, final;
} NdsBgmPacket;

int ndsBgmStreamOpen(NdsBgmStream *stream, const NdsBgmSpec *spec, NdsBgmRead read);
/* 1 = complete packet, 0 = finite EOF, -1 = failed input; no partial publish. */
int ndsBgmStreamRead(NdsBgmStream *stream, NdsBgmPacket *packet,
                     uint8_t data[NDS_AUDIO_BGM_PACKET_BYTES], NdsBgmRead read);
#endif
