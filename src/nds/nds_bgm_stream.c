#include <nds/nds_bgm_stream.h>
#include <string.h>

static uint32_t ndsBgmLe16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}
static uint32_t ndsBgmLe32(const uint8_t *p)
{
    return ndsBgmLe16(p) | (ndsBgmLe16(p + 2) << 16);
}

int ndsBgmStreamOpen(NdsBgmStream *s, const NdsBgmSpec *spec, NdsBgmRead read)
{
    uint8_t header[NDS_AUDIO_BGM_CONTAINER_HEADER_BYTES];
    if (!s || !spec || !read) return 0;
    memset(s, 0, sizeof(*s));
    s->spec = *spec;
    s->error = NDS_BGM_ERROR_HEADER;
    if (spec->file_bytes < sizeof(header) ||
        spec->file_bytes > UINT32_MAX - spec->rom_offset ||
        !spec->source_samples || !spec->packet_count ||
        (spec->loop_sample != NDS_BGM_NO_LOOP &&
         (spec->loop_sample >= spec->source_samples || spec->loop_packet >= spec->packet_count ||
          spec->loop_record < sizeof(header) || spec->loop_record >= spec->file_bytes))) return 0;
    if (!read(spec->rom_offset, header, sizeof(header)))
    {
        s->error = NDS_BGM_ERROR_READ;
        return 0;
    }
    if (ndsBgmLe32(header) != NDS_AUDIO_BGM_CONTAINER_MAGIC ||
        ndsBgmLe16(header + 4) != NDS_AUDIO_BGM_CONTAINER_VERSION ||
        ndsBgmLe16(header + 6) != sizeof(header) ||
        ndsBgmLe32(header + 8) != NDS_AUDIO_BGM_SAMPLE_RATE ||
        ndsBgmLe32(header + 12) != spec->source_samples ||
        ndsBgmLe32(header + 16) != spec->loop_sample ||
        ndsBgmLe32(header + 20) != NDS_AUDIO_BGM_PACKET_SAMPLES ||
        ndsBgmLe32(header + 24) != spec->packet_count ||
        ndsBgmLe32(header + 28) != spec->loop_packet ||
        ndsBgmLe32(header + 32) != spec->loop_record ||
        ndsBgmLe32(header + 36) != (uint32_t)(spec->loop_sample != NDS_BGM_NO_LOOP)) return 0;
    s->offset = sizeof(header);
    s->error = NDS_BGM_ERROR_NONE;
    return 1;
}

int ndsBgmStreamRead(NdsBgmStream *s, NdsBgmPacket *packet,
                     uint8_t data[NDS_AUDIO_BGM_PACKET_BYTES], NdsBgmRead read)
{
    uint8_t header[NDS_AUDIO_BGM_PACKET_HEADER_BYTES];
    uint32_t cursor, index, looped = 0, samples, bytes;
    if (!s || !packet || !data || !read || s->error) return -1;
    cursor = s->offset; index = s->next_packet;
    if (index == s->spec.packet_count)
    {
        if (s->spec.loop_sample == NDS_BGM_NO_LOOP) return 0;
        cursor = s->spec.loop_record; index = s->spec.loop_packet; looped = 1;
    }
    s->error = NDS_BGM_ERROR_PACKET;
    if (index >= s->spec.packet_count || cursor > s->spec.file_bytes ||
        sizeof(header) > s->spec.file_bytes - cursor) return -1;
    if (!read(s->spec.rom_offset + cursor, header, sizeof(header)))
    {
        s->error = NDS_BGM_ERROR_READ;
        return -1;
    }
    samples = ndsBgmLe32(header); bytes = ndsBgmLe32(header + 4);
    cursor += sizeof(header);
    if (!samples || samples > NDS_AUDIO_BGM_PACKET_SAMPLES ||
        bytes != 4u + ((samples + 7u) / 8u) * 4u ||
        bytes > NDS_AUDIO_BGM_PACKET_BYTES || bytes > s->spec.file_bytes - cursor ||
        (index + 1u == s->spec.packet_count && bytes != s->spec.file_bytes - cursor)) return -1;
    uint32_t cycle_samples = looped ? 0u : s->cycle_samples;
    uint32_t expected_samples = s->loop_count || looped ?
        s->spec.source_samples - s->spec.loop_sample : s->spec.source_samples;
    if (cycle_samples > expected_samples || samples > expected_samples - cycle_samples ||
        (index + 1u == s->spec.packet_count && cycle_samples + samples != expected_samples)) return -1;
    if (!read(s->spec.rom_offset + cursor, data, bytes))
    {
        s->error = NDS_BGM_ERROR_READ;
        return -1;
    }
    if (data[2] > 88u || data[3] != 0u) return -1;
    *packet = (NdsBgmPacket){ samples, bytes, looped,
        index + 1u == s->spec.packet_count && s->spec.loop_sample == NDS_BGM_NO_LOOP };
    s->offset = cursor + bytes; s->next_packet = index + 1u;
    s->cycle_samples = cycle_samples + samples;
    s->source_samples_loaded += samples; s->loop_count += looped;
    s->error = NDS_BGM_ERROR_NONE;
    return 1;
}
