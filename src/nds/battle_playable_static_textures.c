#include "nds/battle_playable_static_textures.h"

#include "generated/battle_playable_static_textures.generated.inc"

_Static_assert(NDS_BATTLE_STATIC_TEXTURE_KEY_WORDS ==
                   NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT,
               "generated/runtime texture key width mismatch");
_Static_assert(NDS_BATTLE_STATIC_TEXTURE_KEY_COUNT == 20u,
               "canonical static texture key count changed");
_Static_assert(NDS_BATTLE_STATIC_TEXTURE_OUTPUT_COUNT == 19u,
               "canonical static texture output count changed");
_Static_assert(NDS_BATTLE_STATIC_TEXTURE_PAYLOAD_BYTES == 90112u,
               "canonical static texture payload size changed");
_Static_assert(NDS_BATTLE_STATIC_TEXTURE_PREPARED_BYTES == 94208u,
               "canonical static texture prepared size changed");

static void ndsBattlePlayableStaticTextureClearView(
    NDSBattlePlayableStaticTextureView *view)
{
    if (view == 0)
    {
        return;
    }
    view->payload_offset = 0u;
    view->bytes = 0u;
    view->record_index = 0u;
    view->owner_mask = 0u;
    view->logical_width = 0u;
    view->logical_height = 0u;
    view->upload_width = 0u;
    view->upload_height = 0u;
}

static s32 ndsBattlePlayableStaticTextureProvenanceValid(
    const NDSBattlePlayableStaticTexturePointerProvenance *provenance,
    u32 raw_word, s32 required)
{
    if ((provenance == 0) || (provenance->reserved != 0u) ||
        (provenance->runtime_address != raw_word))
    {
        return FALSE;
    }
    if (required != FALSE)
    {
        return ((raw_word != 0u) && (provenance->asset_id != 0u)) ?
            TRUE : FALSE;
    }
    if (raw_word == 0u)
    {
        return ((provenance->asset_id == 0u) &&
                (provenance->asset_offset == 0u)) ? TRUE : FALSE;
    }
    return (provenance->asset_id != 0u) ? TRUE : FALSE;
}

static s32 ndsBattlePlayableStaticTextureInputValid(
    const NDSBattlePlayableStaticTextureLookupKey *key)
{
    if ((key == 0) || (key->words == 0) ||
        (key->word_count !=
         NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT))
    {
        return FALSE;
    }
    if (ndsBattlePlayableStaticTextureProvenanceValid(
            &key->image,
            key->words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD],
            TRUE) == FALSE)
    {
        return FALSE;
    }
    if (ndsBattlePlayableStaticTextureProvenanceValid(
            &key->tlut,
            key->words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD],
            TRUE) == FALSE)
    {
        return FALSE;
    }
    return ndsBattlePlayableStaticTextureProvenanceValid(
        &key->texel1,
        key->words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD],
        FALSE);
}

static s32 ndsBattlePlayableStaticTextureRecordImageCompare(
    const NDSBattlePlayableStaticTextureRecord *record,
    const NDSBattlePlayableStaticTextureLookupKey *key)
{
    if (record->image_asset_id < key->image.asset_id)
    {
        return -1;
    }
    if (record->image_asset_id > key->image.asset_id)
    {
        return 1;
    }
    if (record->image_offset < key->image.asset_offset)
    {
        return -1;
    }
    if (record->image_offset > key->image.asset_offset)
    {
        return 1;
    }
    return 0;
}

static s32 ndsBattlePlayableStaticTextureRecordMatches(
    const NDSBattlePlayableStaticTextureRecord *record,
    const NDSBattlePlayableStaticTextureLookupKey *key)
{
    u32 word;

    if ((record->image_asset_id != key->image.asset_id) ||
        (record->image_offset != key->image.asset_offset) ||
        (record->tlut_asset_id != key->tlut.asset_id) ||
        (record->tlut_offset != key->tlut.asset_offset))
    {
        return FALSE;
    }
    /* This first generated cut contains no TEXEL1 composite. A nonzero,
     * internally consistent TEXEL1 provenance is a clean miss, never a
     * partial match against primary-only pixels. */
    if ((key->texel1.runtime_address != 0u) ||
        (key->texel1.asset_id != 0u) ||
        (key->texel1.asset_offset != 0u))
    {
        return FALSE;
    }
    for (word = 0u;
         word < NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT;
         word++)
    {
        if ((word == NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD) ||
            (word == NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD) ||
            (word == NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD))
        {
            continue;
        }
        if (record->key_words[word] != key->words[word])
        {
            return FALSE;
        }
    }
    return TRUE;
}

static s32 ndsBattlePlayableStaticTextureRecordValid(
    const NDSBattlePlayableStaticTextureRecord *record)
{
    u32 expected_bytes;

    if ((record == 0) || (record->owner_mask == 0u) ||
        (record->reserved != 0u) ||
        (record->image_asset_id == 0u) ||
        (record->tlut_asset_id == 0u) ||
        (record->logical_width == 0u) ||
        (record->logical_height == 0u) ||
        (record->upload_width < record->logical_width) ||
        (record->upload_height < record->logical_height))
    {
        return FALSE;
    }
    expected_bytes = (u32)record->upload_width *
        (u32)record->upload_height * 2u;
    if ((record->payload_bytes != expected_bytes) ||
        (record->payload_offset > NDS_BATTLE_STATIC_TEXTURE_PAYLOAD_BYTES) ||
        (record->payload_bytes >
         NDS_BATTLE_STATIC_TEXTURE_PAYLOAD_BYTES - record->payload_offset))
    {
        return FALSE;
    }
    return TRUE;
}

s32 ndsBattlePlayableStaticTextureLookup(
    const NDSBattlePlayableStaticTextureLookupKey *key,
    NDSBattlePlayableStaticTextureView *out_view)
{
    u32 left;
    u32 right;
    u32 index;

    ndsBattlePlayableStaticTextureClearView(out_view);
    if ((out_view == 0) ||
        (ndsBattlePlayableStaticTextureInputValid(key) == FALSE))
    {
        return NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_INVALID;
    }

    /* Generated records are sorted by asset ID and source offset. Find the
     * first possible image in O(log N), then compare the usually one or two
     * exact render-state variants for that source pointer. */
    left = 0u;
    right = NDS_BATTLE_STATIC_TEXTURE_KEY_COUNT;
    while (left < right)
    {
        u32 middle = left + ((right - left) >> 1);
        s32 compare = ndsBattlePlayableStaticTextureRecordImageCompare(
            &sNdsBattleStaticTextureRecords[middle], key);

        if (compare < 0)
        {
            left = middle + 1u;
        }
        else
        {
            right = middle;
        }
    }
    for (index = left; index < NDS_BATTLE_STATIC_TEXTURE_KEY_COUNT; index++)
    {
        const NDSBattlePlayableStaticTextureRecord *record =
            &sNdsBattleStaticTextureRecords[index];

        if (ndsBattlePlayableStaticTextureRecordImageCompare(record, key) != 0)
        {
            break;
        }
        if (ndsBattlePlayableStaticTextureRecordMatches(record, key) == FALSE)
        {
            continue;
        }
        if (ndsBattlePlayableStaticTextureRecordValid(record) == FALSE)
        {
            return NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_INVALID;
        }
        out_view->payload_offset = record->payload_offset;
        out_view->bytes = record->payload_bytes;
        out_view->record_index = index;
        out_view->owner_mask = record->owner_mask;
        out_view->logical_width = record->logical_width;
        out_view->logical_height = record->logical_height;
        out_view->upload_width = record->upload_width;
        out_view->upload_height = record->upload_height;
        return NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_HIT;
    }
    return NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_MISS;
}

const NDSBattlePlayableStaticTextureRecord *
ndsBattlePlayableStaticTextureRecordAt(u32 index)
{
    return (index < NDS_BATTLE_STATIC_TEXTURE_KEY_COUNT) ?
        &sNdsBattleStaticTextureRecords[index] : 0;
}

u32 ndsBattlePlayableStaticTextureKeyCount(void)
{
    return NDS_BATTLE_STATIC_TEXTURE_KEY_COUNT;
}

u32 ndsBattlePlayableStaticTexturePayloadBytes(void)
{
    return NDS_BATTLE_STATIC_TEXTURE_PAYLOAD_BYTES;
}

u32 ndsBattlePlayableStaticTexturePreparedBytes(void)
{
    return NDS_BATTLE_STATIC_TEXTURE_PREPARED_BYTES;
}
