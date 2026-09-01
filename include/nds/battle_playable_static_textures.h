#ifndef SSB64_NDS_BATTLE_PLAYABLE_STATIC_TEXTURES_H
#define SSB64_NDS_BATTLE_PLAYABLE_STATIC_TEXTURES_H

#include <PR/ultratypes.h>

#define NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT 59u
#define NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD 0u
#define NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD 4u
#define NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD 32u
#define NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_PAYLOAD_PATH \
    "nitro:/renderer/battle_playable_static_textures.rgb5a1.bin"

enum NDSBattlePlayableStaticTextureLookupResult {
    NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_INVALID = -1,
    NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_MISS = 0,
    NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_HIT = 1
};

/* The adapter must derive this provenance from its validated reloc asset
 * mapping. runtime_address must exactly equal the corresponding raw pointer
 * word in NDSRendererHardwareTextureKey. reserved must remain zero. */
typedef struct NDSBattlePlayableStaticTexturePointerProvenance {
    u32 runtime_address;
    u16 asset_id;
    u16 reserved;
    u32 asset_offset;
} NDSBattlePlayableStaticTexturePointerProvenance;

/* words points at the renderer's complete canonical 59-u32 texture key.
 * Pointer words remain runtime addresses; provenance supplies stable
 * BattleShip asset identities and offsets for those three fields. */
typedef struct NDSBattlePlayableStaticTextureLookupKey {
    const u32 *words;
    u32 word_count;
    NDSBattlePlayableStaticTexturePointerProvenance image;
    NDSBattlePlayableStaticTexturePointerProvenance tlut;
    NDSBattlePlayableStaticTexturePointerProvenance texel1;
} NDSBattlePlayableStaticTextureLookupKey;

/* Exact generated metadata for one renderer key. Pixels are not linked into
 * ARM9 memory; payload_offset/payload_bytes address the NitroFS payload. */
/* libnds GL_TEXTURE_TYPE_ENUM values, restated so this header does not drag in
 * videoGL.h and so the generator's two encodings are named where they are
 * validated. GL_RGB16 is the DS's sixteen-colour paletted format at four bits a
 * texel; GL_RGBA is RGB555 plus one alpha bit at sixteen. */
/* Upper bound on the payload's palette block, so the renderer can hold it in
 * one static buffer without seeing the generated header. The generated value is
 * asserted against this in battle_playable_static_textures.c -- 26 records at
 * 16 entries is 832 bytes even with no dedupe at all. */
#define NDS_BATTLE_STATIC_TEXTURE_PALETTE_BLOCK_MAX_BYTES 1024u

#define NDS_BATTLE_STATIC_TEXTURE_FORMAT_PAL16 3u
#define NDS_BATTLE_STATIC_TEXTURE_FORMAT_RGBA 8u

typedef struct NDSBattlePlayableStaticTextureRecord {
    u16 owner_mask;
    u16 image_asset_id;
    u16 tlut_asset_id;
    u16 reserved;
    u32 image_offset;
    u32 tlut_offset;
    u32 payload_offset;
    u32 payload_bytes;
    u16 logical_width;
    u16 logical_height;
    u16 upload_width;
    u16 upload_height;
    /* THE ENCODING, BECAUSE 16 BITS A TEXEL WAS NEVER THE SOURCE'S IDEA.
     *
     * Every one of these textures comes from an N64 CI4 or IA8 tile -- four or
     * eight bits a texel -- and this corpus stored all of them expanded to
     * RGB555+A1 at two bytes. Measured over the shipping payload, 22 of the 24
     * carry sixteen distinct colours or fewer, so the expansion was buying
     * nothing and costing 74,496 bytes of the 262,144 the DS has for textures.
     * That is the budget the VFX atlas needed and could not get: three separate
     * attempts to grow it past 8,192 bytes starved the stage's texture resolve
     * (generate_nds_particle_banks.py has the measurements).
     *
     * `ds_format` is the libnds GL_TEXTURE_TYPE_ENUM the payload span is encoded
     * in -- GL_RGB16 for the paletted ones, GL_RGBA for the two that genuinely
     * need more than sixteen colours. Paletted records carry their palette in
     * the same payload at `palette_offset`; it uploads to VRAM F/G, which is a
     * different bank from the texels and was never the constrained one. */
    u16 ds_format;
    u16 palette_entries;
    u32 palette_offset;
    u32 key_words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT];
} NDSBattlePlayableStaticTextureRecord;

/* A hit is an immutable file span plus its upload geometry. */
typedef struct NDSBattlePlayableStaticTextureView {
    u32 payload_offset;
    u32 bytes;
    u32 record_index;
    u16 owner_mask;
    u16 logical_width;
    u16 logical_height;
    u16 upload_width;
    u16 upload_height;
    /* See the record's note: the view has to carry the encoding too, because
     * the uploader binds a palette for a paletted span and must not for a
     * direct-colour one. */
    u16 ds_format;
    u16 palette_entries;
    u32 palette_offset;
} NDSBattlePlayableStaticTextureView;

/* Where the palettes start in the payload, and how many bytes they occupy.
 * Accessors rather than macros because the generated header is private to
 * battle_playable_static_textures.c. */
u32 ndsBattlePlayableStaticTexturePaletteBlockOffset(void);
u32 ndsBattlePlayableStaticTexturePaletteBlockBytes(void);

s32 ndsBattlePlayableStaticTextureLookup(
    const NDSBattlePlayableStaticTextureLookupKey *key,
    NDSBattlePlayableStaticTextureView *out_view);
const NDSBattlePlayableStaticTextureRecord *
ndsBattlePlayableStaticTextureRecordAt(u32 index);
u32 ndsBattlePlayableStaticTextureKeyCount(void);
u32 ndsBattlePlayableStaticTexturePayloadBytes(void);
u32 ndsBattlePlayableStaticTexturePreparedBytes(void);

#endif
