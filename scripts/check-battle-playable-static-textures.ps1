[CmdletBinding()]
param(
    [string]$ArmCC = 'C:\devkitPro\devkitARM\bin\arm-none-eabi-gcc.exe',
    [string]$HostCC = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$generator = Join-Path $PSScriptRoot 'generate_battle_playable_static_textures.py'

if (-not (Test-Path -LiteralPath $generator -PathType Leaf)) {
    throw "Static texture generator is absent: $generator"
}

$python = Get-Command python -ErrorAction Stop
$fixtureJson = (& $python.Source -B $generator --repo-root $repoRoot `
    --check --fixture-json | Out-String)
if ($LASTEXITCODE -ne 0) {
    throw "Battle Playable static texture generation check failed with exit $LASTEXITCODE."
}
$fixture = $fixtureJson | ConvertFrom-Json
$payload = Join-Path $repoRoot `
    'assets\renderer\battle_playable_static_textures.rgb5a1.bin'
if (-not (Test-Path -LiteralPath $payload -PathType Leaf)) {
    throw "Static texture payload is absent: $payload"
}
$payloadFile = Get-Item -LiteralPath $payload
$payloadHash = (Get-FileHash -LiteralPath $payload -Algorithm SHA256).Hash.ToLowerInvariant()
if ($fixture.key_count -ne 24 -or $fixture.unique_output_count -ne 23 -or
    $fixture.residency_bytes -ne 136192 -or $fixture.payload_bytes -ne 132096 -or
    $payloadFile.Length -ne 132096 -or $payloadHash -ne $fixture.payload_sha256) {
    throw (
        'Unexpected generated static texture corpus: ' +
        "keys=$($fixture.key_count) outputs=$($fixture.unique_output_count) " +
        "prepared=$($fixture.residency_bytes) payload=$($payloadFile.Length) " +
        "sha256=$payloadHash"
    )
}
Write-Output (
    'BATTLE_PLAYABLE_STATIC_TEXTURE_PAYLOAD_OK ' +
    "keys=$($fixture.key_count) outputs=$($fixture.unique_output_count) " +
    "prepared_bytes=$($fixture.residency_bytes) payload_bytes=$($payloadFile.Length) " +
    "sha256=$payloadHash"
)

if (-not (Test-Path -LiteralPath $ArmCC -PathType Leaf)) {
    throw "ARM9 compiler is absent: $ArmCC"
}

if ([string]::IsNullOrWhiteSpace($HostCC)) {
    $hostCommand = Get-Command gcc.exe -ErrorAction SilentlyContinue
    if ($null -eq $hostCommand) {
        $hostCommand = Get-Command clang.exe -ErrorAction SilentlyContinue
    }
    if ($null -eq $hostCommand) {
        throw 'No host C compiler (gcc.exe or clang.exe) is available.'
    }
    $HostCC = $hostCommand.Source
}

$module = Join-Path $repoRoot 'src\nds\battle_playable_static_textures.c'
$header = Join-Path $repoRoot 'include\nds\battle_playable_static_textures.h'
if (-not (Test-Path -LiteralPath $module -PathType Leaf)) {
    throw "Static texture lookup module is absent: $module"
}
if (-not (Test-Path -LiteralPath $header -PathType Leaf)) {
    throw "Static texture lookup header is absent: $header"
}

$includeRoot = Join-Path $repoRoot 'include'
$battleShipInclude = Join-Path $repoRoot 'decomp\BattleShip-main\decomp\include'
$tempBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$tempDir = [System.IO.Path]::GetFullPath(
    (Join-Path $tempBase ('smash64ds-static-texture-lookup-' + [guid]::NewGuid()))
)
if (-not $tempDir.StartsWith($tempBase, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing temporary output outside $tempBase"
}
[void](New-Item -ItemType Directory -Path $tempDir)
$armObject = Join-Path $tempDir 'battle_playable_static_textures.o'
$hostExe = Join-Path $tempDir 'battle_playable_static_textures_host.exe'

$hostFixture = @'
#include "nds/battle_playable_static_textures.h"

#include <stdio.h>

static void bytes_fill(void *destination, u8 value, u32 byte_count)
{
    u8 *bytes = (u8 *)destination;
    u32 index;

    for (index = 0u; index < byte_count; index++)
    {
        bytes[index] = value;
    }
}

static void bytes_copy(void *destination, const void *source, u32 byte_count)
{
    u8 *destination_bytes = (u8 *)destination;
    const u8 *source_bytes = (const u8 *)source;
    u32 index;

    for (index = 0u; index < byte_count; index++)
    {
        destination_bytes[index] = source_bytes[index];
    }
}

static int view_is_clear(const NDSBattlePlayableStaticTextureView *view)
{
    return view->payload_offset == 0u && view->bytes == 0u &&
        view->record_index == 0u && view->owner_mask == 0u &&
        view->logical_width == 0u && view->logical_height == 0u &&
        view->upload_width == 0u && view->upload_height == 0u;
}

static void seed_view(NDSBattlePlayableStaticTextureView *view)
{
    bytes_fill(view, 0xA5u, (u32)sizeof(*view));
}

static void make_key(
    u32 index,
    u32 words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT],
    NDSBattlePlayableStaticTextureLookupKey *key)
{
    const NDSBattlePlayableStaticTextureRecord *record =
        ndsBattlePlayableStaticTextureRecordAt(index);
    u32 image_address = 0x02100000u + record->image_offset;
    u32 tlut_address = 0x02200000u + record->tlut_offset;
    u32 texel1_offset = record->key_words[
        NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD];
    u32 texel1_address = (texel1_offset != 0u) ?
        0x02100000u + texel1_offset : 0u;

    bytes_copy(words, record->key_words, (u32)sizeof(record->key_words));
    words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD] = image_address;
    words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD] = tlut_address;
    words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD] = texel1_address;
    bytes_fill(key, 0u, (u32)sizeof(*key));
    key->words = words;
    key->word_count = NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT;
    key->image.runtime_address = image_address;
    key->image.asset_id = record->image_asset_id;
    key->image.asset_offset = record->image_offset;
    key->tlut.runtime_address = tlut_address;
    key->tlut.asset_id = record->tlut_asset_id;
    key->tlut.asset_offset = record->tlut_offset;
    key->texel1.runtime_address = texel1_address;
    key->texel1.asset_id = (texel1_offset != 0u) ? record->image_asset_id : 0u;
    key->texel1.asset_offset = texel1_offset;
}

static int expect_result(
    s32 expected,
    const NDSBattlePlayableStaticTextureLookupKey *key,
    NDSBattlePlayableStaticTextureView *view)
{
    seed_view(view);
    if (ndsBattlePlayableStaticTextureLookup(key, view) != expected)
    {
        return 0;
    }
    if (expected != NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_HIT &&
        !view_is_clear(view))
    {
        return 0;
    }
    return 1;
}

int main(void)
{
    u32 index;
    u32 hits = 0u;
    u32 field_misses = 0u;
    u32 explicit_misses = 0u;
    u32 invalids = 0u;
    u32 prepared_bytes = 0u;
    u32 output_count = 0u;
    u32 output_offsets[24];
    u32 output_bytes[24];

    if (ndsBattlePlayableStaticTextureKeyCount() != 24u ||
        ndsBattlePlayableStaticTexturePayloadBytes() != 132096u ||
        ndsBattlePlayableStaticTexturePreparedBytes() != 136192u)
    {
        return 10;
    }
    if (ndsBattlePlayableStaticTextureRecordAt(24u) != NULL)
    {
        return 11;
    }
    for (index = 0u; index < ndsBattlePlayableStaticTextureKeyCount(); index++)
    {
        const NDSBattlePlayableStaticTextureRecord *record =
            ndsBattlePlayableStaticTextureRecordAt(index);
        u32 words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT];
        NDSBattlePlayableStaticTextureLookupKey key;
        NDSBattlePlayableStaticTextureView view;
        u32 word;

        make_key(index, words, &key);
        if (!expect_result(
                NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_HIT,
                &key, &view))
        {
            return 20 + (int)index;
        }
        if (record == NULL || record->reserved != 0u ||
            view.record_index != index ||
            view.owner_mask != record->owner_mask ||
            view.bytes != record->payload_bytes ||
            view.payload_offset != record->payload_offset ||
            view.logical_width != record->logical_width ||
            view.logical_height != record->logical_height ||
            view.upload_width != record->upload_width ||
            view.upload_height != record->upload_height ||
            record->payload_offset > 132096u ||
            record->payload_bytes > 132096u - record->payload_offset)
        {
            return 50 + (int)index;
        }
        hits++;
        prepared_bytes += view.bytes;
        {
            u32 output;
            for (output = 0u; output < output_count; output++)
            {
                if (output_offsets[output] == record->payload_offset &&
                    output_bytes[output] == record->payload_bytes)
                {
                    break;
                }
            }
            if (output == output_count)
            {
                output_offsets[output_count] = record->payload_offset;
                output_bytes[output_count] = record->payload_bytes;
                output_count++;
            }
        }

        for (word = 0u;
             word < NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT;
             word++)
        {
            if (word == NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD ||
                word == NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD ||
                word == NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD)
            {
                continue;
            }
            words[word] ^= 0x80000000u;
            if (!expect_result(
                    NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_MISS,
                    &key, &view))
            {
                return 80 + (int)word;
            }
            words[word] ^= 0x80000000u;
            field_misses++;
        }
    }

    {
        u32 words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_KEY_WORD_COUNT];
        NDSBattlePlayableStaticTextureLookupKey key;
        NDSBattlePlayableStaticTextureView view;

        make_key(0u, words, &key);
        words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_IMAGE_WORD] ^= 4u;
        if (!expect_result(
                NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_INVALID,
                &key, &view))
        {
            return 150;
        }
        invalids++;

        make_key(0u, words, &key);
        words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TLUT_WORD] ^= 4u;
        if (!expect_result(
                NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_INVALID,
                &key, &view))
        {
            return 151;
        }
        invalids++;

        make_key(0u, words, &key);
        key.image.reserved = 1u;
        if (!expect_result(
                NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_INVALID,
                &key, &view))
        {
            return 152;
        }
        invalids++;

        make_key(0u, words, &key);
        key.word_count--;
        if (!expect_result(
                NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_INVALID,
                &key, &view))
        {
            return 153;
        }
        invalids++;

        make_key(0u, words, &key);
        key.words = NULL;
        if (!expect_result(
                NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_INVALID,
                &key, &view))
        {
            return 154;
        }
        invalids++;

        make_key(0u, words, &key);
        if (ndsBattlePlayableStaticTextureLookup(&key, NULL) !=
            NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_INVALID)
        {
            return 155;
        }
        invalids++;

        make_key(0u, words, &key);
        key.image.asset_id++;
        if (!expect_result(
                NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_MISS,
                &key, &view))
        {
            return 160;
        }
        explicit_misses++;

        make_key(0u, words, &key);
        key.image.asset_offset += 4u;
        if (!expect_result(
                NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_MISS,
                &key, &view))
        {
            return 161;
        }
        explicit_misses++;

        make_key(0u, words, &key);
        words[NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_TEXEL1_WORD] = 0x02300000u;
        key.texel1.runtime_address = 0x02300000u;
        key.texel1.asset_id = 103u;
        key.texel1.asset_offset = 0x1234u;
        if (!expect_result(
                NDS_BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_MISS,
                &key, &view))
        {
            return 162;
        }
        explicit_misses++;
    }

    if (hits != 24u || output_count != 23u || field_misses != 1344u ||
        explicit_misses != 3u || invalids != 6u ||
        prepared_bytes != 136192u)
    {
        return 170;
    }
    printf(
        "BATTLE_PLAYABLE_STATIC_TEXTURE_LOOKUP_HOST_OK "
        "hits=%lu outputs=%lu field_misses=%lu explicit_misses=%lu invalids=%lu "
        "prepared_bytes=%lu payload_bytes=%lu\n",
        (unsigned long)hits,
        (unsigned long)output_count,
        (unsigned long)field_misses,
        (unsigned long)explicit_misses,
        (unsigned long)invalids,
        (unsigned long)prepared_bytes,
        (unsigned long)ndsBattlePlayableStaticTexturePayloadBytes());
    return 0;
}
'@

try {
    & $ArmCC -std=c11 -Os -mcpu=arm946e-s -marm -ffreestanding `
        -fdata-sections -ffunction-sections -Wall -Wextra -Werror `
        -D_LANGUAGE_C -I $includeRoot -I $battleShipInclude `
        -c $module -o $armObject
    if ($LASTEXITCODE -ne 0) {
        throw "Static texture lookup module failed ARM9 compilation."
    }

    $armSize = Join-Path (Split-Path -Parent $ArmCC) 'arm-none-eabi-size.exe'
    if (-not (Test-Path -LiteralPath $armSize -PathType Leaf)) {
        throw "ARM size tool is absent: $armSize"
    }
    $sizeLines = @(& $armSize -A $armObject)
    if ($LASTEXITCODE -ne 0) {
        throw "ARM size inspection failed with exit $LASTEXITCODE."
    }
    $textBytes = 0
    $rodataBytes = 0
    $dataBytes = 0
    $bssBytes = 0
    foreach ($line in $sizeLines) {
        if ($line -match '^\s*(\.\S+)\s+(\d+)\s+') {
            $section = $Matches[1]
            $bytes = [int64]$Matches[2]
            if ($section -like '.text*') { $textBytes += $bytes }
            elseif ($section -like '.rodata*') { $rodataBytes += $bytes }
            elseif ($section -like '.data*') { $dataBytes += $bytes }
            elseif ($section -like '.bss*') { $bssBytes += $bytes }
        }
    }
    if ($textBytes -le 0 -or $rodataBytes -le 0 -or $rodataBytes -ge 8192 -or
        $dataBytes -ne 0 -or $bssBytes -ne 0) {
        throw "Unexpected ARM sections: text=$textBytes rodata=$rodataBytes data=$dataBytes bss=$bssBytes"
    }
    Write-Output (
        'BATTLE_PLAYABLE_STATIC_TEXTURE_METADATA_ARM_OK embedded_payload=NO ' +
        "text_bytes=$textBytes rodata_bytes=$rodataBytes data_bytes=$dataBytes bss_bytes=$bssBytes"
    )

    $hostFixture | & $HostCC -x c -std=c11 -O2 -Wall -Wextra -Werror `
        -D_LANGUAGE_C -I $includeRoot -I $battleShipInclude -I $repoRoot `
        $module - -o $hostExe
    if ($LASTEXITCODE -ne 0) {
        throw "Static texture host lookup fixture failed to compile."
    }
    & $hostExe
    if ($LASTEXITCODE -ne 0) {
        throw "Static texture host lookup fixture failed with exit $LASTEXITCODE."
    }
}
finally {
    $resolvedTempDir = [System.IO.Path]::GetFullPath($tempDir)
    if (-not $resolvedTempDir.StartsWith(
            $tempBase, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing cleanup outside $tempBase"
    }
    if (Test-Path -LiteralPath $resolvedTempDir) {
        Remove-Item -LiteralPath $resolvedTempDir -Recurse -Force
    }
}
