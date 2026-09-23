#ifndef NDS_MOTION_MF_H
#define NDS_MOTION_MF_H

/* P2-2p8 Phase 3 (docs/p2/FOUR_FIGHTER_30FPS_ARCHITECTURE.md A2, owner ruling
 * D6): the compact motion format "MF1".
 *
 * A BPS1 clip (an AObjEvent16 script set, generate_battlepack_anim.py) is
 * stored as one MSB-first bitstream of the symbols the pose parser consumes --
 * slot table, command words as ranks in the previous word's successor list,
 * payloads, per-track value deltas and rates, run tails -- each class coded
 * with a static canonical Huffman table shared by every kind. Decoding
 * reproduces the clip's BPS1 bytes exactly (scripts/motion/check_mf_pack.py
 * proves it for every clip with THIS file compiled for the host), so every
 * consumer after the decode sees today's bytes.
 *
 * Layouts (little-endian) are written by scripts/motion/mf_emit.py and
 * documented in artifacts/performance/2026-09-23_p2-2p8-mf-host/README.md.
 *
 * Not linked into any ROM yet: Phase 3 wires it into the match-load bank. */

#if defined(NDS_MF_HOST)
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
#else
#include <nds/ndstypes.h>
#endif

#define NDS_MF_TABLES_MAGIC 0x3154464Du /* "MFT1" */
#define NDS_MF_PACK_MAGIC 0x3150464Du   /* "MFP1" */
#define NDS_MF_PACK_VERSION 1u

#define NDS_MF_MAX_CODE_BITS 12
#define NDS_MF_LUT_BITS 6
#define NDS_MF_CLASS_COUNT 9
#define NDS_MF_CTX_COUNT 8

/* Symbol classes, in the order mf_emit.py numbers them. */
enum
{
    nNdsMfClassNSlot = 0,
    nNdsMfClassSlot = 1,
    nNdsMfClassCRank = 2,
    nNdsMfClassCRank0 = 3,
    nNdsMfClassPay = 4,
    nNdsMfClassVD = 5,
    nNdsMfClassRT = 6,
    nNdsMfClassRT6 = 7,
    nNdsMfClassTail = 8
};

/* Expanded symbols: a literal value, or NDS_MF_ESC | category. */
#define NDS_MF_ESC 0x40000000

typedef struct NdsMfTable
{
    const s32 *syms;                          /* canonical order */
    u16 first_code[NDS_MF_MAX_CODE_BITS + 1]; /* per code length */
    u16 first_index[NDS_MF_MAX_CODE_BITS + 1];
    u16 count[NDS_MF_MAX_CODE_BITS + 1];
    u16 lut[1 << NDS_MF_LUT_BITS];            /* (index << 4) | length, 0 = longer code */
} NdsMfTable;

typedef struct NdsMfTables
{
    const u16 *words;       /* command words by order-0 index (blob) */
    const u16 *recip15;     /* rate predictor reciprocals (blob) */
    const u16 *succ_words;  /* successor lists, word indices (blob) */
    const u16 *succ_off;    /* [nwords + 1] start in succ_words, by context */
    const u16 *succ_len;    /* [nwords + 1] 0 = context has no successor list */
    const NdsMfTable *table[NDS_MF_CLASS_COUNT][NDS_MF_CTX_COUNT];
    u16 nwords;
    u16 ntables;
} NdsMfTables;

/* Pack header and rows (MFP1). */
typedef struct NdsMfPackHeader
{
    u32 magic;
    u16 version;
    u16 kind_count;
    u32 tables_off;
    u32 tables_bytes;
    u32 dir_off;
    u32 dir_count;
    u32 kind_off;
    u32 by_id_off;          /* u16[dir_count]: entry indices sorted by asset id */
    u32 data_off;
    u32 data_bytes;
    u32 reserved[2];
} NdsMfPackHeader;

typedef struct NdsMfPackKind
{
    char name[8];
    u16 dir_first;          /* the kind's entries are contiguous ... */
    u16 dir_count;          /* ... and so is its data, in entry order */
    u32 reserved;
} NdsMfPackKind;

typedef struct NdsMfPackEntry
{
    u32 asset_id;
    u32 data_rel;           /* stream start, from header.data_off (4-aligned) */
    u32 stream_bits;
    u32 decoded_bytes;      /* the BPS1 clip size */
    u32 crc32;              /* of the decoded BPS1 bytes */
    u8 kind;                /* index into the kind table */
    u8 cls;                 /* 0 always resident, 1 victim, 2 Kirby copy */
    u16 need_mask;          /* cls 1/2: opponent kinds (bit = kind index) that need it */
} NdsMfPackEntry;

/* Bytes of storage ndsMfExpandTables needs for this blob (0 = malformed). */
u32 ndsMfTablesStorageBytes(const u8 *blob, u32 blob_bytes);

/* Expand the serialised tables. The blob must stay resident and 2-aligned:
 * words, reciprocals and successor lists are used in place. Returns 0 or a
 * negative error. */
s32 ndsMfExpandTables(const u8 *blob, u32 blob_bytes, void *storage,
                      u32 storage_bytes, NdsMfTables *out);

/* Decode one clip. Returns the bytes written (the BPS1 clip size) or a
 * negative error; *bits_used (optional) receives the bits consumed. */
s32 ndsMfDecodeClip(const NdsMfTables *tables, const u8 *stream,
                    u32 stream_bytes, u8 *out, u32 out_cap, u32 *bits_used);

#endif
