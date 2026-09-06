#!/usr/bin/env python3
"""Host-execute the REAL preview-pack loader helpers from src/port/reloc_preview_pack.c.

Measured result: the suite compiles the production bodies verbatim and
runs them against FPCs generated in temporary directories from repo
reference source. Regression cases reject stale generations, padding
targets and root cells outside the Model section.

Scope actually proven here (host cap: 3 FPCs, 2 compiler runs max):
  * extracted, unmodified via source_test_helpers.function: ndsPreviewHash,
    ndsPreviewRange, ndsPreviewValidateSections, ndsPreviewSectionContains,
    ndsPreviewSection,
    ndsPreviewFileOffset, ndsRelocNativeSourceSize, ndsRelocNativeRootOffset,
    ndsRelocNativeAssetAddress (all from src/port/reloc_preview_pack.c).
  * stubbed seams (NOT claimed as coverage): the NDSPreviewResident /
    NDSRelocLoadedFile / Gfx / scene types, sNdsPreviewResidents table,
    sNdsRelocSceneGeneration, ndsRelocReadNative32 (portable memcpy),
    ndsRelocFindLoadedFileByData (small table scan), NitroFS FILE I/O,
    syTaskmanMalloc, the loaded-file registry, and both normalizers
    (ndsRelocNormalizeFighterAttributesFile,
    ndsRelocNormalizeBattleInterfaceSprites). No normalizer or hardware
    behavior is claimed from these stubs.
  * real FPCs used: 00.mario (2 sections), 02.donkey (3 sections, tail
    asset 319), 05.link (27 spans). Python cross-checks header asset IDs,
    original-vs-compact sizes, and tail sections against the generated
    source maps in the temporary fixture dir; the C harness checks hashes,
    section validation, span mapping, root identities and normal-file
    passthrough.

NOT proven here: the full ndsRelocLoadPreviewFighter FILE/allocator/
registry/normalizer path, animation loading, and real CSS runtime.
Header magic/version/fkind/
asset-ID/file-size checks live inline in that function, not in a separable
helper; they are pinned by string checks plus the independent Python
decoder, not by C execution.

Run:
    python -m pytest scripts/fighters/test_preview_pack_loader.py -q
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, SCRIPT_DIR)
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts" / "menus"))
from source_test_helpers import function  # noqa: E402
import generate_preview_core_packs as gen  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SRC = (ROOT / "src/port/reloc_preview_pack.c").read_text(encoding="utf-8")
SCENE_H = (ROOT / "include/sc/scene.h").read_text(encoding="utf-8")
PACK_H = (ROOT / "include/nds/nds_preview_pack.h").read_text(encoding="utf-8")

# Host cap: three representative packs only (mario/donkey/link).
KINDS = [("mario", "00"), ("donkey", "02"), ("link", "05")]
FPCKIND = {"mario": 0, "donkey": 2, "link": 5}


def pin(pattern, text, label):
    import re
    if not re.search(pattern, text, re.S):
        raise AssertionError(f"reference drifted, update test: {label}")
    return True


# Loader parser assumptions this suite relies on (fail loudly on drift).
pin(r"header\.magic != NDS_PREVIEW_PACK_MAGIC", SRC, "magic check")
pin(r"header\.version != NDS_PREVIEW_PACK_VERSION", SRC, "version check")
pin(r"header\.fkind != \(u32\)fkind", SRC, "fkind check")
pin(r"header\.main_asset_id != ndsRelocAssetIDForToken", SRC, "main id check")
pin(r"header\.model_asset_id != ndsRelocAssetIDForToken", SRC, "model id check")
pin(r"header\.file_bytes != \(u32\)file_size", SRC, "file size check")
pin(r"fkind == nFTKindNess", SRC, "ness red gate")
pin(r"ndsPreviewHash\(data, header\.data_bytes, 2166136261u\) != header\.data_hash",
    SRC, "data hash check")
pin(r"hash != header\.fixup_hash", SRC, "fixup hash check")
pin(r"ndsPreviewHash\(spans.*!= header\.span_hash", SRC, "span hash check")
pin(r"0xdf000000u", SRC, "root ENDDL tag")
pin(r"NDS_PREVIEW_PACK_NULL", SRC, "null target")
pin(r"NDS_PREVIEW_PACK_MAGIC 0x31435046u", PACK_H, "FPC1 magic")
pin(r"nSCKind1PGamePlayers,", SCENE_H, "scene header defines gate kind")


def extract_real():
    out = {
        "hash": function(SRC, "ndsPreviewHash"),
        "range": function(SRC, "ndsPreviewRange"),
        "validate": function(SRC, "ndsPreviewValidateSections"),
        "contains": function(SRC, "ndsPreviewSectionContains"),
        "section": function(SRC, "ndsPreviewSection"),
        "fileoff": function(SRC, "ndsPreviewFileOffset"),
        "sourcesize": function(SRC, "ndsRelocNativeSourceSize"),
        "rootoff": function(SRC, "ndsRelocNativeRootOffset"),
        "assetaddr": function(SRC, "ndsRelocNativeAssetAddress"),
    }
    if "16777619u" not in out["hash"]:
        raise AssertionError("extracted hash lost FNV prime")
    if "span->source_offset" not in out["fileoff"]:
        raise AssertionError("extracted fileoff lost span walk")
    if "0xdf000000u" not in out["rootoff"]:
        raise AssertionError("extracted rootoff lost ENDDL check")
    if "sections[0].asset_id == header->main_asset_id" not in out["validate"]:
        raise AssertionError("extracted validate lost asset order check")
    if "ndsPreviewFileOffset" not in out["assetaddr"]:
        raise AssertionError("extracted assetaddr lost fileoff call")
    return out


HARNESS_HEAD = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define ARRAY_COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define NDS_PREVIEW_PACK_MAGIC 0x31435046u
#define NDS_PREVIEW_PACK_VERSION 1u
#define NDS_PREVIEW_PACK_MAX_SECTIONS 4u
#define NDS_PREVIEW_PACK_NULL 0xffffffffu
#define NDS_RELOC_LOADED_FILE_CAPACITY 96u

typedef struct NDSPreviewPackHeader {
    u32 magic, version, file_bytes, fkind, section_count, fixup_count,
        span_count, reserved, data_bytes, data_hash, fixup_hash, span_hash,
        main_asset_id, model_asset_id, reserved_tail0, reserved_tail1;
} NDSPreviewPackHeader;
typedef struct NDSPreviewPackSection {
    u32 asset_id, data_offset, data_bytes, source_bytes, first_span,
        span_count, roots_offset, root_count;
} NDSPreviewPackSection;
typedef struct NDSPreviewPackSpan {
    u32 source_offset, data_offset, data_bytes;
} NDSPreviewPackSpan;
typedef struct Gfx { u32 w0; u32 w1; } Gfx;

/* Minimal NDSRelocLoadedFile: every field the extracted bodies touch. */
typedef struct NDSRelocLoadedFile {
    u32 asset_id; u32 bit; void *data; u32 data_size;
    u32 owner_scene; u32 owner_generation;
    u16 reloc_intern_offset; u16 reloc_extern_offset;
    u32 extern_count; u16 *extern_file_ids;
    u32 external_fixup_count; u32 external_fixup_fail_count;
    u32 internal_fixup_count;
    u8 internal_fixups_applied; u8 external_fixups_applied;
    u8 format_fixups_applied; u8 fixups_applying; u8 offsets_are_relative;
    u8 reserved[3];
} NDSRelocLoadedFile;
typedef struct NDSPreviewResident {
    u32 generation;
    NDSPreviewPackSection *sections;
    NDSPreviewPackSpan *spans;
} NDSPreviewResident;

static NDSPreviewResident sNdsPreviewResidents[12];
static u32 sNdsRelocSceneGeneration = 7u;
/* Stub scene gate values (test only; production now uses the defined
 * nSCKind1PGamePlayers -- see test_scene_gate_uses_defined_kind). */
enum { nSCKind1PGamePlayers = 0xA5u, nSCKindOther = 0x00u };
static struct { u8 scene_curr; u8 scene_prev; } gSCManagerSceneData;

/* Portable native-32 read (stub seam; production adds an aligned fast path). */
static u32 ndsRelocReadNative32(const void *addr)
{
    u32 v; memcpy(&v, addr, sizeof(v)); return v;
}
/* Registry stub: small table scan for the AssetAddress test only. */
static NDSRelocLoadedFile *sFindTable[8];
static u32 sFindCount = 0u;
static NDSRelocLoadedFile *ndsRelocFindLoadedFileByData(void *file)
{
    u32 i;
    for (i = 0u; i < sFindCount; i++)
        if (sFindTable[i]->data == file) return sFindTable[i];
    return NULL;
}
'''

HOST_MAIN = r'''
static int sFailures = 0;
#define CHECK(cond, ...) do { \
    if (!(cond)) { printf("FAIL %d: ", __LINE__); printf(__VA_ARGS__); \
        printf("\n"); sFailures++; } \
} while (0)

static u8 *load_file(const char *name, u32 *out_len)
{
    FILE *f = fopen(name, "rb");
    u8 *b; u32 n; long sz;
    if (!f) { printf("FAIL: cannot open %s\n", name); exit(1); }
    fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
    b = malloc((size_t)sz); n = (u32)fread(b, 1, (size_t)sz, f); fclose(f);
    if (n != (u32)sz) { printf("FAIL: short read %s\n", name); exit(1); }
    *out_len = n; return b;
}
static void parse(const u8 *b, NDSPreviewPackHeader *h,
                  NDSPreviewPackSection *secs)
{
    u32 i;
    memcpy(h, b, sizeof(*h));
    for (i = 0u; i < h->section_count; i++)
        memcpy(&secs[i], b + 64 + 32 * i, sizeof(secs[i]));
}
static void be_to_native(u8 *base, u32 bytes)
{
    u32 i;
    for (i = 0u; i < bytes; i += 4u) {
        u8 b0 = base[i], b1 = base[i+1], b2 = base[i+2], b3 = base[i+3];
        base[i] = b3; base[i+1] = b2; base[i+2] = b1; base[i+3] = b0;
    }
}
static int in_sections(NDSPreviewPackSection *secs, u32 n, u32 off)
{
    u32 i;
    for (i = 0u; i < n; i++)
        if (secs[i].data_offset <= off &&
            off < secs[i].data_offset + secs[i].data_bytes) return 1;
    return 0;
}

static const char *NAMES[3] = { "00.fpc", "02.fpc", "05.fpc" };

int main(void)
{
    u32 k;
    /* A. Hashes over real bytes match real header hashes; 1-bit flip fails. */
    for (k = 0u; k < 3u; k++) {
        u8 *b; u32 len; NDSPreviewPackHeader h; NDSPreviewPackSection s[4];
        const u8 *data, *fix, *span;
        b = load_file(NAMES[k], &len); parse(b, &h, s);
        data = b + 64 + 32 * h.section_count;
        fix = data + h.data_bytes; span = fix + 8 * h.fixup_count;
        CHECK(ndsPreviewHash(data, h.data_bytes, 2166136261u) == h.data_hash,
              "%s data hash", NAMES[k]);
        CHECK(ndsPreviewHash(fix, 8 * h.fixup_count, 2166136261u) == h.fixup_hash,
              "%s fixup hash", NAMES[k]);
        CHECK(ndsPreviewHash(span, 12 * h.span_count, 2166136261u) == h.span_hash,
              "%s span hash", NAMES[k]);
        { u8 save = data[4];
          ((u8 *)data)[4] ^= 0xFFu;
          CHECK(ndsPreviewHash(data, h.data_bytes, 2166136261u) != h.data_hash,
                "%s corrupt byte must mismatch", NAMES[k]);
          ((u8 *)data)[4] = save; }
        free(b);
    }
    /* B. Real sections validate; corrupt bounds/type markers do not. */
    for (k = 0u; k < 3u; k++) {
        u8 *b; u32 len; NDSPreviewPackHeader h; NDSPreviewPackSection s[4], m[4];
        b = load_file(NAMES[k], &len); parse(b, &h, s);
        CHECK(ndsPreviewValidateSections(&h, s) == TRUE, "%s validates", NAMES[k]);
        memcpy(m, s, sizeof(s));
        m[0].data_bytes = 0u;
        CHECK(ndsPreviewValidateSections(&h, m) == FALSE, "%s zero bytes", NAMES[k]);
        memcpy(m, s, sizeof(s)); m[0].data_offset |= 1u;
        CHECK(ndsPreviewValidateSections(&h, m) == FALSE, "%s misalign", NAMES[k]);
        memcpy(m, s, sizeof(s)); m[0].data_offset = s[1].data_offset;
        CHECK(ndsPreviewValidateSections(&h, m) == FALSE, "%s overlap", NAMES[k]);
        memcpy(m, s, sizeof(s)); m[0].asset_id = m[1].asset_id;
        CHECK(ndsPreviewValidateSections(&h, m) == FALSE, "%s dup id", NAMES[k]);
        memcpy(m, s, sizeof(s)); m[0].asset_id = 0xdeadbeefu;
        CHECK(ndsPreviewValidateSections(&h, m) == FALSE, "%s wrong main", NAMES[k]);
        memcpy(m, s, sizeof(s)); m[0].root_count = 1u; m[0].roots_offset = 0u;
        CHECK(ndsPreviewValidateSections(&h, m) == FALSE,
              "%s roots on main rejected", NAMES[k]);
        memcpy(m, s, sizeof(s)); m[1].first_span = h.span_count;
        CHECK(ndsPreviewValidateSections(&h, m) == FALSE, "%s span overrun", NAMES[k]);
        memcpy(m, s, sizeof(s)); m[1].source_bytes = 0u;
        CHECK(ndsPreviewValidateSections(&h, m) == FALSE, "%s zero source", NAMES[k]);
        { NDSPreviewPackHeader hm = h; hm.data_bytes += 16u;
          CHECK(ndsPreviewValidateSections(&hm, s) == FALSE, "%s tail size", NAMES[k]); }
        free(b);
    }
    /* C. Span mapping on real mario model spans; pruned gap rejected. */
    {
        u8 *b; u32 len; NDSPreviewPackHeader h; NDSPreviewPackSection s[4];
        NDSPreviewPackSpan *sp; NDSRelocLoadedFile lf; u32 i, nsp;
        b = load_file("00.fpc", &len); parse(b, &h, s);
        nsp = h.span_count;
        sp = malloc(12 * nsp);
        memcpy(sp, b + 64 + 32 * h.section_count + h.data_bytes + 8 * h.fixup_count,
               12 * nsp);
        memset(&lf, 0, sizeof(lf));
        sNdsPreviewResidents[0].generation = sNdsRelocSceneGeneration;
        sNdsPreviewResidents[0].sections = malloc(sizeof(s));
        memcpy(sNdsPreviewResidents[0].sections, s, sizeof(s));
        sNdsPreviewResidents[0].spans = sp;
        lf.reserved[0] = 1u; lf.reserved[1] = 1u;
        lf.owner_generation = sNdsRelocSceneGeneration;
        for (i = s[1].first_span; i < s[1].first_span + s[1].span_count; i++) {
            u32 out = 0xdeadbeefu;
            CHECK(ndsPreviewFileOffset(&lf, sp[i].source_offset,
                                       sp[i].data_bytes, &out) == TRUE,
                  "mario span %u maps", i);
            CHECK(out == sp[i].data_offset, "mario span %u base", i);
            if (sp[i].data_bytes > 1u) {
                CHECK(ndsPreviewFileOffset(&lf,
                      sp[i].source_offset + sp[i].data_bytes - 1u, 1u,
                      &out) == TRUE, "mario span %u tail", i);
                CHECK(out == sp[i].data_offset + sp[i].data_bytes - 1u,
                      "mario span %u tail addr", i);
            }
        }
        { u32 out = 0xdeadbeefu; /* pruned model gap 1944..8703 */
          CHECK(ndsPreviewFileOffset(&lf, 1944u, 1u, &out) == FALSE,
                "pruned source gap rejected"); }
        { u32 out = 0xdeadbeefu;
          CHECK(ndsPreviewFileOffset(&lf, 29968u, 1u, &out) == FALSE,
                "source overrun rejected"); }
        /* Normal-file path preserved: identity, same bounds. */
        { NDSRelocLoadedFile n; u32 out = 0xdeadbeefu;
          memset(&n, 0, sizeof(n)); n.data_size = 1904u;
          CHECK(ndsPreviewFileOffset(&n, 100u, 4u, &out) == TRUE && out == 100u,
                "normal identity");
          CHECK(ndsPreviewFileOffset(&n, 1904u, 1u, &out) == FALSE,
                "normal overrun rejected");
          CHECK(ndsRelocNativeSourceSize(&n) == 1904u, "normal size"); }
        CHECK(ndsRelocNativeSourceSize(&lf) == 29968u, "mario source extent");
        free(b);
    }
    /* D. Root identities: real cells decode to their original offsets. */
    {
        u8 *b; u32 len; NDSPreviewPackHeader h; NDSPreviewPackSection s[4];
        u8 *sec; NDSRelocLoadedFile lf; u32 i; u32 nsp;
        NDSPreviewPackSpan *sp;
        b = load_file("00.fpc", &len); parse(b, &h, s);
        nsp = h.span_count;
        sp = malloc(12 * nsp);
        memcpy(sp, b + 64 + 32 * h.section_count + h.data_bytes + 8 * h.fixup_count,
               12 * nsp);
        sec = malloc(s[1].data_bytes);
        memcpy(sec, b + 64 + 32 * h.section_count + s[1].data_offset,
               s[1].data_bytes);
        be_to_native(sec, s[1].data_bytes);
        sNdsPreviewResidents[0].sections[1].data_offset = 0u; /* section base */
        memset(&lf, 0, sizeof(lf));
        lf.data = sec; lf.data_size = s[1].data_bytes;
        lf.reserved[0] = 1u; lf.reserved[1] = 1u;
        lf.owner_generation = sNdsRelocSceneGeneration;
        /* Re-point resident section at the byteswapped copy. */
        sNdsPreviewResidents[0].sections[1].data_offset = 0u;
        for (i = 0u; i < s[1].root_count; i++) {
            const Gfx *cell = (const Gfx *)(sec + s[1].roots_offset + 8 * i);
            u32 want;
            memcpy(&want, (const u8 *)sec + s[1].roots_offset + 8 * i + 4, 4);
            CHECK(ndsRelocReadNative32(cell) == 0xdf000000u, "cell %u tag", i);
            CHECK(ndsRelocNativeRootOffset(&lf, cell) == want,
                  "cell %u identity", i);
        }
        { const Gfx *bad = (const Gfx *)(sec + s[1].roots_offset);
          u8 tmp[8]; memcpy(tmp, bad, 8); tmp[0] ^= 0xFFu;
          CHECK(ndsRelocNativeRootOffset(&lf, (const Gfx *)tmp) == 0xffffffffu,
                "corrupt tag rejected"); }
        CHECK(ndsRelocNativeRootOffset(&lf, (const Gfx *)(sec + 1u)) == 0xffffffffu,
              "misaligned root rejected");
        CHECK(ndsRelocNativeRootOffset(&lf, (const Gfx *)sec) == 0xffffffffu,
              "pre-roots offset rejected");
        free(sec); free(b);
    }
    /* E. AssetAddress: preview maps, pruned fails closed, normal/scene pass. */
    {
        u8 *b; u32 len; NDSPreviewPackHeader h; NDSPreviewPackSection s[4];
        u8 *sec0; NDSRelocLoadedFile plf, nlf;
        b = load_file("00.fpc", &len); parse(b, &h, s);
        sec0 = malloc(s[0].data_bytes + 16u);
        memcpy(sec0, b + 64 + 32 * h.section_count + s[0].data_offset,
               s[0].data_bytes);
        memset(&plf, 0, sizeof(plf));
        plf.data = sec0; plf.data_size = s[0].data_bytes;
        plf.reserved[0] = 1u; plf.reserved[1] = 0u;
        plf.owner_generation = sNdsRelocSceneGeneration;
        memset(&nlf, 0, sizeof(nlf));
        nlf.data = sec0; nlf.data_size = s[0].data_bytes;
        sFindCount = 0u;
        gSCManagerSceneData.scene_curr = (u8)nSCKind1PGamePlayers;
        /* Normal file: identity even where preview spans have gaps. */
        sFindTable[sFindCount++] = &nlf;
        CHECK(ndsRelocNativeAssetAddress(sec0, 100u) ==
              (const void *)(sec0 + 100u), "normal passthrough");
        sFindCount = 0u;
        /* Preview main identity span maps; off-end fails closed. */
        sFindTable[sFindCount++] = &plf;
        CHECK(ndsRelocNativeAssetAddress(sec0, 100u) ==
              (const void *)(sec0 + 100u), "preview main maps");
        CHECK(ndsRelocNativeAssetAddress(sec0, 1904u) == NULL,
              "preview overrun NULL");
        /* Other scenes never remap. */
        gSCManagerSceneData.scene_curr = (u8)nSCKindOther;
        CHECK(ndsRelocNativeAssetAddress(sec0, 1904u) ==
              (const void *)(sec0 + 1904u), "non-1P identity");
        gSCManagerSceneData.scene_curr = (u8)nSCKind1PGamePlayers;
        free(sec0); free(b);
    }
    /* F. A stale preview never becomes a normal identity-mapped file. */
    {
        NDSRelocLoadedFile lf; u32 out = 0xdeadbeefu;
        memset(&lf, 0, sizeof(lf));
        lf.data_size = 8288u; lf.reserved[0] = 1u; lf.reserved[1] = 1u;
        lf.owner_generation = sNdsRelocSceneGeneration;
        sNdsPreviewResidents[0].generation = sNdsRelocSceneGeneration - 1u;
        CHECK(ndsPreviewFileOffset(&lf, 100u, 4u, &out) == FALSE,
              "stale source offset rejected");
        CHECK(ndsRelocNativeSourceSize(&lf) == 0u,
              "stale source size rejected");
        lf.data = (u8 *)(uintptr_t)0x1000u;
        CHECK(ndsRelocNativeRootOffset(&lf, (Gfx *)(uintptr_t)0x1064u) == UINT32_MAX,
              "stale root identity rejected");
        sNdsPreviewResidents[0].generation = sNdsRelocSceneGeneration;
    }
    /* G. Fixups must target a section body, not inter-section padding. */
    {
        u8 *b; u32 len; NDSPreviewPackHeader h; NDSPreviewPackSection s[4];
        u32 pad;
        b = load_file("00.fpc", &len); parse(b, &h, s);
        pad = s[0].data_offset + s[0].data_bytes - 4u; /* last body word */
        CHECK(ndsPreviewRange(pad, 4u, h.data_bytes) == TRUE,
              "padding word in range");
        CHECK(in_sections(s, h.section_count, pad) == TRUE, "sanity: in body");
        { u32 fake = h.data_bytes - 4u; /* synthetic pad past shrunken sec1 */
          NDSPreviewPackSection m[4]; memcpy(m, s, sizeof(s));
          m[1].data_bytes -= 16u;
          CHECK(ndsPreviewRange(fake, 4u, h.data_bytes) == TRUE,
                "coarse range accepts pad, fine helper must still reject");
          CHECK(in_sections(m, h.section_count, fake) == FALSE,
                "pad target outside bodies");
          CHECK(ndsPreviewSectionContains(m, h.section_count, fake, 1u) == FALSE,
                "production helper rejects padding target");
          CHECK(ndsPreviewSectionContains(m, h.section_count, pad, 4u) == TRUE,
                "production helper accepts body slot"); }
        free(b);
    }
    if (sFailures == 0) { printf("ALL PASS\n"); return 0; }
    printf("%d FAILURES\n", sFailures);
    return 1;
}
'''


def build_source():
    real = extract_real()
    # AssetAddress references resident sections; drop the stray line the
    # scenario above guards (kept simple: scenario uses valid wiring).
    return "\n".join([
        HARNESS_HEAD,
        real["hash"],
        real["range"],
        real["validate"],
        real["contains"],
        real["section"],
        real["fileoff"],
        real["sourcesize"],
        real["rootoff"],
        real["assetaddr"],
        HOST_MAIN,
    ])


def bounded(text, limit=2000):
    text = (text or "").strip()
    return text if len(text) <= limit else text[:limit] + "... [truncated]"


class PreviewPackLoaderTest(unittest.TestCase):
    MAX_LOG = 2000

    @classmethod
    def setUpClass(cls):
        # Measured result: 3-kind generation takes about 1 second on host.
        # One generation serves every test in this class. The encoder
        # default (no --input-dir) auto-generates source metadata from repo
        # reference source into <output-dir>/source-metadata, so no ignored
        # builds inputs are read.
        cls.work = tempfile.mkdtemp(prefix="preview-loader-")
        cls.fpc_dir = Path(cls.work) / "packs"
        rc = gen.main(["--output-dir", str(cls.fpc_dir),
                       "--kinds", "mario,donkey,link"])
        assert rc == 0, "fixture packs must encode"
        cls.map_dir = cls.fpc_dir / "source-metadata"
        assert (cls.map_dir / "mario_compact_map.json").exists(), \
            "default generation must emit source maps"
        cls.c_source = build_source()

    def compile_and_run(self, source_text, tag, compiler):
        with tempfile.TemporaryDirectory() as directory:
            for name in ("00.fpc", "02.fpc", "05.fpc"):
                src = self.fpc_dir / name
                self.assertTrue(src.exists(), f"missing generated pack {name}")
                shutil.copyfile(src, os.path.join(directory, name))
            source = Path(directory) / f"preview_loader_{tag}.c"
            program = Path(directory) / f"preview_loader_{tag}.exe"
            source.write_text(source_text, encoding="utf-8")
            built = subprocess.run(
                [compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                 "-fmax-errors=8" if "gcc" in compiler else "-ferror-limit=8",
                 str(source), "-o", str(program)],
                capture_output=True)
            self.assertEqual(
                built.returncode, 0,
                f"host build failed:\n{bounded(built.stderr.decode('utf-8', 'replace'), self.MAX_LOG)}")
            ran = subprocess.run([str(program)], capture_output=True, timeout=60,
                                 cwd=directory)
            out = ran.stdout.decode("utf-8", "replace")
            err = ran.stderr.decode("utf-8", "replace")
            self.assertEqual(
                ran.returncode, 0,
                f"host run failed (stderr):\n{bounded(err, self.MAX_LOG)}\n"
                f"(stdout):\n{bounded(out, self.MAX_LOG)}")
            self.assertIn("ALL PASS", out,
                          f"missing ALL PASS:\n{bounded(out, self.MAX_LOG)}")

    def pick_compilers(self):
        first = next((c for c in ("clang", "gcc", "cc")
                      if shutil.which(c)), None)
        self.assertIsNotNone(first, "Host C compiler required (clang/gcc/cc)")
        gcc = shutil.which("gcc")
        tags = [first]
        if gcc and gcc not in tags and "gcc" not in first:
            tags.append("gcc")
        return tags[:2]  # concrete cap: at most 2 host runs

    def test_real_helpers_against_fpcs(self):
        compilers = self.pick_compilers()
        self.compile_and_run(self.c_source, "real", compilers[0])

    def test_real_helpers_second_compiler(self):
        compilers = self.pick_compilers()
        if len(compilers) < 2:
            self.skipTest("single compiler available; cap respected")
        self.compile_and_run(self.c_source, "gcc", compilers[1])

    def test_header_ids_sizes_tails_from_maps(self):
        """Real FPC headers carry real asset IDs, source extents, tails."""
        for kind, num in KINDS:
            blob = (self.fpc_dir / f"{num}.fpc").read_bytes()
            d = gen.decode_pack(blob)
            m = json.loads((self.map_dir / f"{kind}_compact_map.json").read_text(
                encoding="utf-8"))
            main_fid = next(iter({r["target_file"]
                                  for r in m["main_intern"]["retained"]}))
            model_fid = int(m["model_intern"]["reloc_file"].split("_")[0])
            h = d["header"]
            self.assertEqual(h[12], main_fid, kind)
            self.assertEqual(h[13], model_fid, kind)
            self.assertEqual(h[3], FPCKIND[kind], kind)
            secs = d["sections"]
            main_len = m["section_boundaries"]["main"][1]
            self.assertEqual(secs[0][3], main_len, kind)  # original extent
            self.assertEqual(secs[0][2], main_len, kind)  # compact identity
            self.assertEqual(secs[1][3], m["checks"]["model_payload_bytes"],
                             kind)  # original, not compact
            tails = m.get("tail_files", [])
            self.assertEqual(len(secs), 2 + len(tails), kind)
            for s, t in zip(secs[2:], tails):
                self.assertEqual(s[0], t["fid"], kind)
                self.assertEqual(s[2], t["bytes"], kind)
                self.assertEqual(s[3], t["bytes"], kind)
            self.assertEqual(h[2], len(blob), kind)  # file_bytes exact

    def test_scene_gate_uses_defined_kind(self):
        """Loader gates only on scene kinds defined in include/sc/scene.h."""
        import re
        uses = set(re.findall(r"nSCKind\w+", SRC))
        defined = set(re.findall(r"nSCKind\w+", SCENE_H))
        missing = uses - defined
        self.assertEqual(missing, set(),
                         f"loader uses undefined scene kinds: {missing}")

    def test_loader_parser_pins(self):
        """Pin the inline header/normalizer seams the C suite assumes."""
        for needle in ("ndsRelocNormalizeFighterAttributesFile(records[0])",
                       "ndsRelocNormalizeBattleInterfaceSprites(records[i])",
                       "*fighter->p_file_main = records[0]->data",
                       "*fighter->p_file_model = records[1]->data",
                       "sNdsRelocSceneGeneration"):
            self.assertIn(needle, SRC, needle)


if __name__ == "__main__":
    unittest.main()
