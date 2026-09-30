#!/usr/bin/env python3
"""Execute production preview mapping for legacy, Meta Knight and bad kinds.

Only the loaded-file/scene registry is a fixture. The roster header, section
lookup, range predicate and compact source-offset lookup are production code.
No donor assets, generators, ROMs or emulator sessions are used.
"""
from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/menus"))
from source_test_helpers import function


HARNESS = r"""
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <nds/nds_p4_roster.h>
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define NDS_PREVIEW_PACK_MAX_SECTIONS 4u
typedef struct NDSPreviewPackSection {
    u32 asset_id, data_offset, data_bytes, source_bytes, first_span,
        span_count, roots_offset, root_count;
} NDSPreviewPackSection;
typedef struct NDSPreviewPackSpan {
    u32 source_offset, data_offset, data_bytes;
} NDSPreviewPackSpan;
typedef struct NDSPreviewResident {
    u32 generation, model_source_bytes;
    NDSPreviewPackSection *sections;
    NDSPreviewPackSpan *spans;
} NDSPreviewResident;
typedef struct NDSRelocLoadedFile {
    u32 data_size, owner_generation;
    u8 reserved[3];
} NDSRelocLoadedFile;
static NDSPreviewResident sNdsPreviewResidents[
    NDS_P4_LEGACY_SELECTION_COUNT + (NDS_P4_METAKNIGHT != 0)];
static u32 sNdsRelocSceneGeneration = 7u;
"""

MAIN = r"""
#define CHECK(c) do { if (!(c)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    NDSPreviewPackSection sections[13];
    NDSPreviewPackSpan spans[13];
    NDSRelocLoadedFile loaded;
    const NDSPreviewResident *resident = NULL;
    u32 kind, output;
    memset(sections, 0, sizeof(sections));
    memset(spans, 0, sizeof(spans));
    memset(&loaded, 0, sizeof(loaded));
    loaded.owner_generation = sNdsRelocSceneGeneration;
    loaded.data_size = 512u;
    for (kind = 0; kind < ARRAY_COUNT(sNdsPreviewResidents); kind++) {
        sections[kind].asset_id = 0x1500u + kind;
        sections[kind].span_count = 1u;
        spans[kind].source_offset = 0x4000u + kind * 0x100u;
        spans[kind].data_offset = 0x20u;
        spans[kind].data_bytes = 16u;
        sNdsPreviewResidents[kind].generation = sNdsRelocSceneGeneration;
        sNdsPreviewResidents[kind].sections = &sections[kind];
        sNdsPreviewResidents[kind].spans = &spans[kind];
    }
    /* Preserve every ordinary identity with both feature settings. */
    for (kind = 0; kind < 12u; kind++) {
        CHECK(ndsRosterSelectionIndex(kind) == kind);
        CHECK(ndsRosterRuntimeKind(kind) == kind);
        loaded.reserved[0] = (u8)(kind + 1u);
        CHECK(ndsPreviewSection(&loaded, &resident) == &sections[kind]);
        CHECK(resident == &sNdsPreviewResidents[kind]);
        CHECK(ndsPreviewFileOffset(&loaded, spans[kind].source_offset + 7u,
                                   1u, &output) != FALSE);
        CHECK(output == 0x27u);
    }
    /* Runtime 29 and compact row 12 must never be interchangeable. */
    loaded.reserved[0] = 30u;
#if NDS_P4_METAKNIGHT
    CHECK(ndsRosterSelectionIndex(29u) == 12u);
    CHECK(ndsRosterRuntimeKind(12u) == 29u);
    CHECK(ndsPreviewSection(&loaded, &resident) == &sections[12]);
    CHECK(resident == &sNdsPreviewResidents[12]);
    CHECK(ndsPreviewFileOffset(&loaded, spans[12].source_offset, 16u,
                               &output) != FALSE);
    CHECK(output == 0x20u);
    CHECK(ndsPreviewFileOffset(&loaded, spans[12].source_offset + 15u,
                               2u, &output) == FALSE);
#else
    CHECK(ndsRosterSelectionIndex(29u) == NDS_P4_NO_SELECTION_INDEX);
    CHECK(ndsRosterRuntimeKind(12u) == 28u);
    CHECK(ndsPreviewSection(&loaded, &resident) == NULL);
#endif
    /* Boss, variants, EnumCount, Null, negative and unknown kinds fail closed;
     * compact row 12 does not grant runtime Boss kind 12 a preview. */
    {
        const u32 invalid[] = {12u, 13u, 26u, 27u, 28u, 30u, 254u};
        u32 i;
        for (i = 0; i < ARRAY_COUNT(invalid); i++) {
            CHECK(ndsRosterSelectionIndex(invalid[i]) == NDS_P4_NO_SELECTION_INDEX);
            loaded.reserved[0] = (u8)(invalid[i] + 1u);
            CHECK(ndsPreviewSection(&loaded, &resident) == NULL);
            CHECK(ndsPreviewFileOffset(&loaded, 0u, 1u, &output) == FALSE);
        }
        CHECK(ndsRosterSelectionIndex((u32)-1) == NDS_P4_NO_SELECTION_INDEX);
        CHECK(ndsRosterRuntimeKind(13u) == 28u);
    }
    loaded.reserved[0] = 1u;
    loaded.owner_generation--;
    CHECK(ndsPreviewSection(&loaded, &resident) == NULL);
    loaded.owner_generation++;
    loaded.reserved[1] = 4u;
    CHECK(ndsPreviewSection(&loaded, &resident) == NULL);
    loaded.reserved[1] = 0u;
    sNdsPreviewResidents[0].generation--;
    CHECK(ndsPreviewSection(&loaded, &resident) == NULL);
    /* Ordinary source files still use their unchanged extent. */
    loaded.reserved[0] = 0u;
    CHECK(ndsPreviewFileOffset(&loaded, 511u, 1u, &output) != FALSE);
    CHECK(output == 511u);
    CHECK(ndsPreviewFileOffset(&loaded, 512u, 1u, &output) == FALSE);
    puts("PASS: production compact preview identity");
    return 0;
}
"""


class PreviewIdentityTests(unittest.TestCase):
    def test_production_mapping_enabled_and_disabled(self) -> None:
        compiler = shutil.which("clang") or shutil.which("gcc")
        self.assertIsNotNone(compiler, "host C compiler required")
        source = (ROOT / "src/port/reloc_preview_pack.c").read_text(encoding="utf-8")
        extracted = "\n".join(function(source, name) for name in (
            "ndsPreviewRange", "ndsPreviewSection", "ndsPreviewFileOffset"))
        with tempfile.TemporaryDirectory(prefix="p4-preview-identity-") as temp:
            directory = Path(temp)
            fixture = directory / "identity.c"
            fixture.write_text(HARNESS + extracted + MAIN, encoding="utf-8")
            for enabled in (0, 1):
                with self.subTest(enabled=enabled):
                    binary = directory / f"identity-{enabled}.exe"
                    build = subprocess.run([
                        compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                        f"-DNDS_P4_METAKNIGHT={enabled}", "-I", str(ROOT / "include"),
                        str(fixture), "-o", str(binary),
                    ], capture_output=True, text=True, timeout=60)
                    self.assertEqual(build.returncode, 0, build.stderr[:4000])
                    run = subprocess.run([str(binary)], capture_output=True,
                                         text=True, timeout=60)
                    self.assertEqual(run.returncode, 0, run.stderr[:4000])
                    self.assertIn("PASS: production compact preview identity", run.stdout)


if __name__ == "__main__":
    unittest.main()
