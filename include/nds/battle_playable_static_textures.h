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
} NDSBattlePlayableStaticTextureView;

s32 ndsBattlePlayableStaticTextureLookup(
    const NDSBattlePlayableStaticTextureLookupKey *key,
    NDSBattlePlayableStaticTextureView *out_view);
const NDSBattlePlayableStaticTextureRecord *
ndsBattlePlayableStaticTextureRecordAt(u32 index);
u32 ndsBattlePlayableStaticTextureKeyCount(void);
u32 ndsBattlePlayableStaticTexturePayloadBytes(void);
u32 ndsBattlePlayableStaticTexturePreparedBytes(void);

#endif
