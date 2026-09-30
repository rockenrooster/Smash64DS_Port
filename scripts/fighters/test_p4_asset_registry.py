#!/usr/bin/env python3
"""Compile the actual P4 asset consumer predicates against its frozen catalogue.

Host only. Temporary C/executable files stay in a temporary directory; no DS
build, generated output or emulator state is changed. Expected answers come
from the independent native resource binding manifest, not copied C switches.
"""
from __future__ import annotations

import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "src/port/reloc_backend_assets.c"
NATIVE = ROOT / "builds/p4/meta-knight-native"


def function(source: str, name: str) -> str:
    match = re.search(r"^static\s+(?:u32|s32|size_t|void)\s+" + re.escape(name)
                      + r"\s*\([^;]*?\)\s*\{", source, re.MULTILINE)
    if not match:
        raise ValueError(f"missing C function {name}")
    start = source.index("{", match.start())
    depth = 1
    index = start + 1
    while depth:
        depth += (source[index] == "{") - (source[index] == "}")
        index += 1
    return source[match.start():index]


class P4AssetConsumer(unittest.TestCase):
    def test_sprite_lane_repair_is_idempotent_and_rejects_bad_owner_before_writes(self) -> None:
        compiler = shutil.which("gcc") or shutil.which("clang")
        if not compiler:
            self.skipTest("host C compiler is unavailable")
        source = ASSETS.read_text(encoding="utf-8")
        sprite_header = (ROOT / "include/PR/sp.h").read_text(encoding="utf-8")
        # The host uses its ordinary pointer width. Put the fixture records in
        # disjoint synthetic spans; the producer's ARM32 tests separately own
        # the real 68-byte Sprite/16-byte Bitmap physical offsets.
        types = "\n".join(re.search(r"struct " + name + r"\s*\{.*?\};", sprite_header,
                                    re.DOTALL).group() +
                           f"\ntypedef struct {name} {name.capitalize()};"
                           for name in ("bitmap", "sprite"))
        descriptor = re.search(r"typedef struct NDSRelocP4SpriteDesc\s*\{.*?\}\s*NDSRelocP4SpriteDesc;",
                               source, re.DOTALL).group()
        table = re.search(r"static const NDSRelocP4SpriteDesc sNdsRelocP4SpriteDescs\[\]\s*=\s*\{.*?\};",
                          source, re.DOTALL).group()
        functions = "\n".join(function(source, name) for name in (
            "ndsRelocSwapS16Pair", "ndsRelocSwapSpriteAttrZDepth",
            "ndsRelocReverseSpriteColorBytes", "ndsRelocNormalizeSpriteHeaderFields",
            "ndsRelocNormalizeSpriteBitmapTable", "ndsRelocNormalizeP4FighterSprites"))
        unit = r"""
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
typedef float f32;
typedef struct Gfx Gfx;
#define TRUE 1
#define FALSE 0
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
""" + types + r"""
typedef struct bitmap Bitmap;
typedef struct sprite Sprite;
typedef struct NDSRelocLoadedFile { u32 asset_id; void *data; u32 data_size; u8 reserved[3]; } NDSRelocLoadedFile;
static NDSRelocLoadedFile owner;
static int deny_owner;
static u32 missing_map = 0xFFFFFFFFu;
static u32 bad_map = 0xFFFFFFFFu;
static s32 ndsPreviewFileOffset(const NDSRelocLoadedFile *file, u32 source, u32 size, u32 *target) {
    (void)size;
    if (source == missing_map) return FALSE;
    if (source == bad_map) { *target = file->data_size - 1; return TRUE; }
    switch (source) {
    case 512: *target = 768; return TRUE;
    case 1536: *target = 2048; return TRUE;
    case 0: *target = 64; return TRUE;
    case 1024: *target = 1280; return TRUE;
    default: return FALSE;
    }
}
static u32 fixture_offset(u32 source) {
    u32 target = source;
    if (owner.reserved[0]) assert(ndsPreviewFileOffset(&owner, source, 1, &target));
    return target;
}
static NDSRelocLoadedFile *ndsRelocFindLoadedFileByAsset(u32 id) {
    return (!deny_owner && id == owner.asset_id) ? &owner : NULL;
}
static s32 ndsRelocRangeInLoadedFile(const NDSRelocLoadedFile *file, size_t offset, size_t size) {
    return file && offset <= file->data_size && size <= file->data_size - offset;
}
static s32 ndsRelocPointerRangeInLoadedFile(const NDSRelocLoadedFile *file, const void *ptr, size_t size) {
    return file && (uintptr_t)ptr >= (uintptr_t)file->data &&
           ndsRelocRangeInLoadedFile(file, (uintptr_t)ptr - (uintptr_t)file->data, size);
}
#define NDS_META_SPRITES(X) \
    X(5456u, 512u, 8u, 10u, 1u, 2u, 0u, 5456u, 0u) \
    X(5456u, 1536u, 27u, 25u, 1u, 4u, 0u, 5456u, 1024u)
""" + descriptor + "\n" + table + "\n" + functions + r"""
static void reset_raw(void) {
    unsigned i;
    memset(owner.data, 0, owner.data_size);
    deny_owner = 0;
    missing_map = bad_map = 0xFFFFFFFFu;
    for (i = 0; i < ARRAY_COUNT(sNdsRelocP4SpriteDescs); i++) {
        const NDSRelocP4SpriteDesc *desc = &sNdsRelocP4SpriteDescs[i];
        Sprite *sprite = (Sprite *)((u8 *)owner.data + fixture_offset(desc->offset));
        Bitmap *bitmap = (Bitmap *)((u8 *)owner.data + fixture_offset(desc->bitmap_offset));
        sprite->width = desc->height; sprite->height = desc->width;
        sprite->nbitmaps = 36; sprite->ndisplist = 1;
        sprite->red = 0x44; sprite->green = 0x33;
        sprite->blue = 0x22; sprite->alpha = 0x11;
        sprite->bitmap = bitmap;
        bitmap->width = 5; bitmap->width_img = 3;
        bitmap->actualHeight = 9; bitmap->LUToffset = 7;
    }
}
int main(void) {
    u8 before[4096];
    unsigned i, compact;
    owner.asset_id = 5456; owner.data = calloc(1, sizeof(before)); owner.data_size = sizeof(before);
    assert(owner.data != NULL);
    for (compact = 0; compact <= 1; compact++) {
    owner.reserved[0] = compact;
    reset_raw();
    assert(ndsRelocNormalizeP4FighterSprites(&owner));
    for (i = 0; i < ARRAY_COUNT(sNdsRelocP4SpriteDescs); i++) {
        const NDSRelocP4SpriteDesc *desc = &sNdsRelocP4SpriteDescs[i];
        Sprite *sprite = (Sprite *)((u8 *)owner.data + fixture_offset(desc->offset));
        Bitmap *bitmap = sprite->bitmap;
        assert((u32)(u16)sprite->width == desc->width && (u32)(u16)sprite->height == desc->height);
        assert(sprite->nbitmaps == 1 && sprite->ndisplist == 36 && sprite->bmfmt == desc->bmfmt);
        assert(sprite->red == 0x11 && sprite->green == 0x22 && sprite->blue == 0x33 && sprite->alpha == 0x44);
        assert(bitmap->width == 3 && bitmap->width_img == 5 && bitmap->actualHeight == 7 && bitmap->LUToffset == 9);
    }
    memcpy(before, owner.data, sizeof(before));
    assert(ndsRelocNormalizeP4FighterSprites(&owner));
    assert(!memcmp(before, owner.data, sizeof(before)));
    reset_raw(); deny_owner = 1; memcpy(before, owner.data, sizeof(before));
    assert(!ndsRelocNormalizeP4FighterSprites(&owner));
    assert(!memcmp(before, owner.data, sizeof(before)));
    reset_raw();
    ((Sprite *)((u8 *)owner.data + fixture_offset(512)))->bitmap = (Bitmap *)((u8 *)owner.data + 4040);
    memcpy(before, owner.data, sizeof(before));
    assert(!ndsRelocNormalizeP4FighterSprites(&owner));
    assert(!memcmp(before, owner.data, sizeof(before)));
    reset_raw(); ((Sprite *)((u8 *)owner.data + fixture_offset(512)))->ndisplist = 0;
    memcpy(before, owner.data, sizeof(before));
    assert(!ndsRelocNormalizeP4FighterSprites(&owner));
    assert(!memcmp(before, owner.data, sizeof(before)));
    if (compact) {
        reset_raw(); missing_map = 512; memcpy(before, owner.data, sizeof(before));
        assert(!ndsRelocNormalizeP4FighterSprites(&owner));
        assert(!memcmp(before, owner.data, sizeof(before)));
        reset_raw(); missing_map = 0; memcpy(before, owner.data, sizeof(before));
        assert(!ndsRelocNormalizeP4FighterSprites(&owner));
        assert(!memcmp(before, owner.data, sizeof(before)));
        reset_raw(); bad_map = 512; memcpy(before, owner.data, sizeof(before));
        assert(!ndsRelocNormalizeP4FighterSprites(&owner));
        assert(!memcmp(before, owner.data, sizeof(before)));
        reset_raw(); bad_map = 0; memcpy(before, owner.data, sizeof(before));
        assert(!ndsRelocNormalizeP4FighterSprites(&owner));
        assert(!memcmp(before, owner.data, sizeof(before)));
    }
    }
    free(owner.data);
    return 0;
}
"""
        with tempfile.TemporaryDirectory(prefix="smash64ds-p4-sprites-") as directory:
            path = Path(directory)
            (path / "sprites.c").write_text(unit, encoding="utf-8")
            exe = path / "sprites.exe"
            result = subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                                     str(path / "sprites.c"), "-o", str(exe)],
                                    text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            subprocess.run([str(exe)], check=True, text=True, capture_output=True)

    def test_generated_identity_roles_sizes_and_animation_parser_dispatch(self) -> None:
        compiler = shutil.which("gcc") or shutil.which("clang")
        if not compiler:
            self.skipTest("host C compiler is unavailable")
        binding_path = NATIVE / "native-runtime-bindings.json"
        if not binding_path.is_file():
            self.skipTest("frozen native resource bindings are unavailable")
        manifest = json.loads(binding_path.read_text(encoding="utf-8"))
        source = ASSETS.read_text(encoding="utf-8")
        enum = re.search(r"enum NDSRelocP4Role\s*\{[^}]*\};", source).group()
        functions = "\n".join(function(source, name) for name in (
            "ndsRelocP4AssetRole", "ndsRelocP4GeneratedPayloadSize",
            "ndsRelocP4GeneratedAllocSize", "ndsRelocIsFighterAnimID",
            "ndsRelocIsFighterAObj32Asset", "ndsRelocIsFighterAObj16Asset"))
        unit = """
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <nds_metaknight_native_assets.generated.h>
typedef uint32_t u32;
typedef int32_t s32;
#define NDS_P4_METAKNIGHT 1
#define TRUE 1
#define FALSE 0
#define NDS_RELOC_ALIGN(value) (((value) + 15u) & ~15u)
#define NDS_RELOC_ASSET_INVALID 0xFFFFFFFFu
#define NDS_RELOC_ASSET_MARIO_ANIM_APPEAR1 633u
#define NDS_RELOC_ASSET_MARIO_ANIM_APPEAR2 634u
#define NDS_RELOC_ASSET_FOX_ANIM_APPEAR 777u
#define NDS_RELOC_ASSET_FOX_ANIM_ARWING 778u
static s32 ndsRelocIsMarioFoxAnimID(u32 id) { return id == 499u; }
static s32 ndsRelocIsGeneratedP2FighterAObj32Asset(u32 id) { (void)id; return FALSE; }
static u32 ndsRelocP2FighterAnimAssetIDForToken(u32 id) { (void)id; return NDS_RELOC_ASSET_INVALID; }
""" + enum + "\n" + functions + r"""
int main(void) {
    unsigned id;
    while (scanf("%u", &id) == 1)
        printf("%u %zu %zu %d %d\n", ndsRelocP4AssetRole(id),
               ndsRelocP4GeneratedPayloadSize(id), ndsRelocP4GeneratedAllocSize(id),
               ndsRelocIsFighterAObj16Asset(id), ndsRelocIsFighterAObj32Asset(id));
    return 0;
}
"""
        rows = manifest["assets"]
        by_id = {row["native_file_id"]: row for row in rows}
        self.assertEqual(len(by_id), 172)
        unknown = [0, 29, 5454, 5552, 0x8013E700, 0x80596BE4, 0xFFFFFFFF]
        self.assertTrue(all(value not in by_id for value in unknown))
        ids = list(by_id) + unknown + [499]
        with tempfile.TemporaryDirectory(prefix="smash64ds-p4-assets-") as directory:
            path = Path(directory)
            (path / "registry.c").write_text(unit, encoding="utf-8")
            exe = path / "registry.exe"
            compile_result = subprocess.run([
                compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                "-Wno-unused-function", "-I", str(NATIVE), str(path / "registry.c"),
                "-o", str(exe)], text=True, capture_output=True)
            self.assertEqual(compile_result.returncode, 0, compile_result.stderr)
            result = subprocess.run([str(exe)], input="\n".join(map(str, ids)) + "\n",
                                    text=True, capture_output=True, check=True)
        answers = [list(map(int, line.split())) for line in result.stdout.splitlines()]
        self.assertEqual(len(answers), len(ids))
        roles = {"MAIN": 1, "MODEL": 2, "MOTION": 3, "ANIM16": 4,
                 "ANIM32": 5, "SHIELD": 6, "DEPENDENCY": 7}
        for asset_id, actual in zip(ids, answers):
            if asset_id in by_id:
                row = by_id[asset_id]
                expected = [roles[row["role"]], (row["size"] + 15) & ~15,
                            row["allocation_size"], int(row["role"] == "ANIM16"),
                            int(row["role"] == "ANIM32")]
            else:
                expected = [0, 0, 0, int(asset_id == 499), 0]
            self.assertEqual(actual, expected, f"native asset {asset_id}")
        self.assertEqual(answers[ids.index(0x6000)][3:], [0, 0],
                         "FT motion/events must bypass both AObj parsers")


if __name__ == "__main__":
    unittest.main()
