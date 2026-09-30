#ifndef NDS_META_RECORD_CODEC_H
#define NDS_META_RECORD_CODEC_H

#include <ssb_types.h>
#include <string.h>

/* One Meta aggregate (92 source-compatible bytes), one Meta-vs-Meta cell,
 * and twelve legacy-vs-Meta cells. Counter cells are KO/tally/played u16.
 * Version 1 is little-endian ARM32; there are no pointers or padding in the
 * serialized payload. Source aggregate extent is asserted by the consumer. */
#define NDS_META_RECORD_PAYLOAD_BYTES (92u + 6u + (12u * 6u))
#define NDS_META_RECORD_BYTES 188u

static inline u32 ndsMetaRecordChecksum(const u8 *record)
{
    u32 value = 2166136261u;
    u32 i;
    for (i = 0; i < NDS_META_RECORD_BYTES; i++)
    {
        if ((i >= 12u) && (i < 16u)) continue;
        value = (value ^ record[i]) * 16777619u;
    }
    return value;
}

static inline u32 ndsMetaRecordReadU32(const u8 *bytes)
{
    return (u32)bytes[0] | ((u32)bytes[1] << 8) | ((u32)bytes[2] << 16) | ((u32)bytes[3] << 24);
}

static inline void ndsMetaRecordWriteU32(u8 *bytes, u32 value)
{
    bytes[0] = (u8)value;
    bytes[1] = (u8)(value >> 8);
    bytes[2] = (u8)(value >> 16);
    bytes[3] = (u8)(value >> 24);
}

static inline void ndsMetaRecordEncode(u8 *record, const u8 *payload)
{
    memset(record, 0, NDS_META_RECORD_BYTES);
    memcpy(record, "S64MKVS1", 8);
    record[8] = 1; /* format version, u16 little-endian */
    record[10] = (u8)NDS_META_RECORD_PAYLOAD_BYTES;
    record[11] = (u8)(NDS_META_RECORD_PAYLOAD_BYTES >> 8);
    memcpy(record + 16, payload, NDS_META_RECORD_PAYLOAD_BYTES);
    ndsMetaRecordWriteU32(record + 12, ndsMetaRecordChecksum(record));
}

static inline sb32 ndsMetaRecordDecode(const u8 *record, u8 *payload)
{
    if (memcmp(record, "S64MKVS1", 8) || record[8] != 1 || record[9] != 0 ||
        record[10] != (u8)NDS_META_RECORD_PAYLOAD_BYTES ||
        record[11] != (u8)(NDS_META_RECORD_PAYLOAD_BYTES >> 8) ||
        record[186] != 0 || record[187] != 0 ||
        ndsMetaRecordReadU32(record + 12) != ndsMetaRecordChecksum(record))
    {
        return FALSE;
    }
    memcpy(payload, record + 16, NDS_META_RECORD_PAYLOAD_BYTES);
    return TRUE;
}

#endif
