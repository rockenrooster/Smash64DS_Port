#!/usr/bin/env python3
"""Host test for variant native owner image residency (Metal Mario + polygons).

Contract: the twelve variant owners (mmario, nmario, nfox, ndonkey, nsamus,
nlink, nyoshi, ncaptain, nkirby, npikachu, npurin, nness) wire into the same
scene-generation image lifecycle as the base owners -- path/size/bind blocks,
IMAGE-flag-gated runtime tables, VERIFY equivalence rows -- and the campaign
scene-start preload loop makes every wave-reachable image resident (both
details) before battle begins, because Zako waves replace fighters mid-battle
via ftManagerMakeFighter. No in-battle I/O, no base-fighter substitutions:
each variant resolves to its own owner-specific tables.

Strategy: source-contract asserts over the owned files plus a host gcc
measurement of the real generated header under campaign build flags for
exact image byte sizes. No ROM, no emulator, no generated writes. Logs
bounded.
"""
from __future__ import annotations

import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent


def _repo() -> Path:
    for parent in (HERE, *HERE.parents):
        if (parent / "src" / "nds" / "nds_renderer_assets.c").is_file():
            return parent
    raise AssertionError("repo root not found above scripts/fighters")


REPO = _repo()
ASSETS_C = REPO / "src" / "nds" / "nds_renderer_assets.c"
FTMANAGER_C = REPO / "src" / "import" / "battleship_ftmanager.c"
MAKEFILE = REPO / "Makefile"
HEADER_H = (
    REPO / "include" / "nds" / "generated"
    / "nds_native_fighter_image.generated.h"
)
TABLES_H = REPO / "include" / "nds" / "nds_native_fighter_tables.h"
LOG_CAP = 8000

# (flag stem, Title, image-slot macro suffix). NLUIGI reuses NMARIO and has
# no image of its own, so it is covered by the reuse asserts, not this list.
VARIANTS = [
    ("MMARIO", "MMario"),
    ("NMARIO", "NMario"),
    ("NFOX", "NFox"),
    ("NDONKEY", "NDonkey"),
    ("NSAMUS", "NSamus"),
    ("NLINK", "NLink"),
    ("NYOSHI", "NYoshi"),
    ("NCAPTAIN", "NCaptain"),
    ("NKIRBY", "NKirby"),
    ("NPIKACHU", "NPikachu"),
    ("NPURIN", "NPurin"),
    ("NNESS", "NNess"),
]
# Zako-wave-reachable image slots (NLUIGI rides NMARIO's packet).
ZAKO_SLOTS = [
    "NMARIO", "NFOX", "NDONKEY", "NSAMUS", "NLINK", "NYOSHI", "NCAPTAIN",
    "NKIRBY", "NPIKACHU", "NPURIN", "NNESS",
]


def _read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def _function_body(src: str, start_marker: str) -> str:
    start = src.index(start_marker)
    brace = src.index("{", start)
    depth = 0
    for end in range(brace, len(src)):
        if src[end] == "{":
            depth += 1
        elif src[end] == "}":
            depth -= 1
            if depth == 0:
                return src[brace:end + 1]
    raise AssertionError(f"unbalanced braces after {start_marker!r}")


def _gcc():
    cc = shutil.which("gcc") or shutil.which("cc") or shutil.which("clang")
    assert cc, "no host C compiler found"
    return cc


class VariantImageResidencyTests(unittest.TestCase):
    def test_bind_blocks_for_all_variants(self):
        src = _read(ASSETS_C)
        body = _function_body(src, "static void ndsRendererNativeBindOwnerImage")
        for flag, title in VARIANTS:
            self.assertIn(f"#if NDS_NATIVE_OWNER_IMAGE_{flag}", body)
            self.assertIn(f"owner_slot == NDS_NATIVE_IMAGE_SLOT_{flag}", body)
            for detail, suffix in (("Low", "LOW"), ("High", "HIGH")):
                self.assertIn(
                    f"sNdsNative{title}Fighter{detail}Tables", body)
                self.assertIn(f"NDSNative{title}{detail}Image", body)
                self.assertIn(
                    f"NDS_NATIVE_IMAGE_{flag}_{suffix}", body)
        # Every variant slot is bound exactly once per detail (no doubles,
        # no base-fighter aliasing inside the bind function).
        for flag, _title in VARIANTS:
            self.assertEqual(
                body.count(f"NDS_NATIVE_IMAGE_SLOT_{flag}"), 1)

    def test_path_size_verify_blocks_for_all_variants(self):
        src = _read(ASSETS_C)
        for flag, title in VARIANTS:
            self.assertIn(f'"nitro:/fighters/{flag.lower()}_low.bin"', src)
            self.assertIn(f'"nitro:/fighters/{flag.lower()}_high.bin"', src)
            self.assertIn(f"sizeof(NDSNative{title}LowImage)", src)
            self.assertIn(f"sizeof(NDSNative{title}HighImage)", src)
            self.assertIn(f"NDS_NATIVE_IMAGE_{flag}_LOW_MEMBERS", src)
            self.assertIn(f"NDS_NATIVE_IMAGE_{flag}_HIGH_MEMBERS", src)

    def test_runtime_tables_conditional_macros(self):
        src = _read(ASSETS_C)
        for flag, title in VARIANTS:
            # Empty load-bound tables under the image flag, const static
            # tables otherwise -- the same macro the base owners use.
            self.assertIn(f"#if NDS_NATIVE_OWNER_IMAGE_{flag}\n"
                          f"static NDSNativeFighterRuntimeTables "
                          f"sNdsNative{title}FighterHighTables;", src)
            self.assertIn(f"#if NDS_NATIVE_OWNER_IMAGE_{flag}\n"
                          f"static NDSNativeFighterRuntimeTables "
                          f"sNdsNative{title}FighterLowTables;", src)
            # Owner runtimes point at their own tables (no substitution).
            self.assertIn(f"&sNdsNative{title}FighterHighTables", src)
            self.assertIn(f"&sNdsNative{title}FighterLowTables", src)

    def test_owner_resolution_uses_variant_owners(self):
        src = _read(ASSETS_C)
        canonical = _function_body(
            src, "ndsRendererNativeFighterCanonicalOwnerForDetail")
        for _flag, title in VARIANTS:
            self.assertIn(f"&sNdsNative{title}LowOwner", canonical)
            self.assertIn(f"&sNdsNative{title}HighOwner", canonical)
        # Variant slots never resolve to the Mario/Fox combined owners.
        for line in canonical.splitlines():
            if "sNdsNativeMMario" in line or "sNdsNativeN" in line:
                self.assertNotIn("sNdsNativeMarioHighOwner", line)
                self.assertNotIn("sNdsNativeFoxHighOwner", line)

        # The public detail resolver must preserve program-specific owners
        # (Samus morph, Link specials, Kirby variants) before falling back to
        # the canonical per-slot owner above.
        detail = _function_body(src, "ndsRendererNativeFighterOwnerForDetail")
        self.assertIn("ndsRendererNativeFighterOwnerForProgramDetail", detail)
        self.assertIn("ndsRendererNativeFighterCanonicalOwnerForDetail", detail)

    def test_preload_seam_at_scene_start(self):
        src = _read(FTMANAGER_C)
        self.assertIn("ndsFTManagerPreloadVariantOwnerImages", src)
        # Hooked into the source preload loop's own function: the Zako
        # branch of sc1PGameFuncStart calls ftManagerSetupFilesAllKind for
        # every N kind, so this fires before battle begins.
        setup = _function_body(src, "void ftManagerSetupFilesAllKind")
        self.assertIn("ndsFTManagerPreloadVariantOwnerImages();", setup)
        helper = _function_body(
            src, "static void ndsFTManagerPreloadVariantOwnerImages")
        # Stage gating mirrors the source: Zako waves need every polygon
        # kind, Metal Mario's stage needs its own owner, nothing else pays.
        self.assertIn("nSC1PGameStageZako", helper)
        self.assertIn("nSC1PGameStageMMario", helper)
        for slot in ZAKO_SLOTS + ["MMARIO"]:
            self.assertIn(f"NDS_NATIVE_IMAGE_SLOT_{slot}, 0u", helper)
            self.assertIn(f"NDS_NATIVE_IMAGE_SLOT_{slot}, 1u", helper)
            # Image and verification builds preload; pure static controls do not.
            self.assertIn(f"#if NDS_NATIVE_OWNER_IMAGE_{slot}", helper)
        # NLuigi reuses the NMario packet: no slot of its own in the
        # header, preloaded through NMARIO here and in MakeFighter.
        header = _read(HEADER_H)
        self.assertNotIn("NDS_NATIVE_IMAGE_SLOT_NLUIGI", header)
        self.assertIn("NDS_NATIVE_IMAGE_SLOT_NMARIO", helper)
        ftmanager = src
        self.assertIn("nFTKindNLuigi", ftmanager)
        self.assertIn("NDS_NATIVE_IMAGE_SLOT_NMARIO", ftmanager)

    def test_preload_executes_only_in_campaign_battle(self):
        sys.path.insert(0, str(REPO / 'scripts/menus'))
        from source_test_helpers import function
        helper = function(_read(FTMANAGER_C), 'ndsFTManagerPreloadVariantOwnerImages')
        for imaged, verify in ((1, 0), (0, 1), (0, 0)):
            code = f'''#include <assert.h>
typedef unsigned u32; typedef unsigned char u8;
#define NDS_NATIVE_OWNER_IMAGE_NMARIO {imaged}
#define NDS_NATIVE_OWNER_IMAGE_MMARIO {imaged}
#define NDS_NATIVE_OWNER_IMAGE_VERIFY {verify}
#define NDS_P2_NMARIO 1
#define NDS_P2_MMARIO 1
#define NDS_NATIVE_IMAGE_SLOT_NMARIO 13u
#define NDS_NATIVE_IMAGE_SLOT_MMARIO 12u
#define nSCKind1PGame 52
#define nSC1PGameStageZako 13
#define nSC1PGameStageMMario 8
static struct {{ int scene_curr; u8 spgame_stage; }} gSCManagerSceneData;
static unsigned calls, verifies, mask;
static int ndsRendererNativeEnsureOwnerImage(u32 slot,u32 detail) {{
    assert(detail < 2); calls++; mask |= 1u << slot; return 1;
}}
static int ndsRendererNativeVerifyOwnerImage(u32 slot,u32 detail) {{
    assert(detail < 2); verifies++; return 1;
}}
''' + helper + f'''
int main(void) {{
    gSCManagerSceneData.spgame_stage=nSC1PGameStageZako;
    gSCManagerSceneData.scene_curr=17;
    ndsFTManagerPreloadVariantOwnerImages(); assert(calls==0);
    gSCManagerSceneData.scene_curr=6;
    ndsFTManagerPreloadVariantOwnerImages(); assert(calls==0);
    gSCManagerSceneData.scene_curr=52;
    ndsFTManagerPreloadVariantOwnerImages();
    assert(calls=={2 if imaged or verify else 0});
    assert(mask=={1 << 13 if imaged or verify else 0});
    gSCManagerSceneData.spgame_stage=nSC1PGameStageMMario;
    ndsFTManagerPreloadVariantOwnerImages();
    assert(calls=={4 if imaged or verify else 0});
    assert(verifies=={4 if verify else 0});
    return 0;
}}
'''
            with tempfile.TemporaryDirectory() as directory:
                c = Path(directory) / 'preload.c'
                exe = c.with_suffix('.exe')
                c.write_text(code)
                result = subprocess.run([shutil.which('gcc'), '-std=c11', str(c),
                                         '-o', str(exe)], capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr.decode())
                subprocess.run([str(exe)], check=True, capture_output=True)

    def test_makefile_flags_and_staging(self):
        mk = _read(MAKEFILE)
        for flag, _title in VARIANTS:
            # Flag derives from the matching P2 build flag (enabled only
            # when the owner is built).
            self.assertIn(
                f"NDS_NATIVE_OWNER_IMAGE_{flag} = "
                f"$(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),"
                f"$(NDS_P2_{flag}),0)", mk)
            # NitroFS staging membership under the same P2 guard.
            self.assertIn(f"ifeq ($(NDS_P2_{flag}),1)", mk)
        staged = [line for line in mk.splitlines()
                  if "NDS_NATIVE_IMAGE_OWNERS += " in line]
        for flag, _title in VARIANTS:
            self.assertIn(f"NDS_NATIVE_IMAGE_OWNERS += {flag.lower()}",
                          staged)
        # The image generator set covers every variant owner.
        all_owners = next(
            line for line in mk.splitlines()
            if line.startswith("NDS_NATIVE_IMAGE_ALL_OWNERS :="))
        for flag, _title in VARIANTS:
            self.assertIn(flag.lower(), all_owners)

    def test_abi_tag_consistency_ndo5(self):
        header = _read(HEADER_H)
        src = _read(ASSETS_C)
        for text in (header, src):
            m = re.search(
                r"#define\s+NDS_NATIVE_OWNER_IMAGE_ABI_TAG\s+(0x[0-9a-fA-F]+)u?",
                text)
            self.assertIsNotNone(m)
            self.assertEqual(int(m.group(1), 16), 0x354F444E)

    def test_image_sizes_host_measured(self):
        """Exact active-image bytes per variant owner under campaign flags.

        Compiles the real generated header with the campaign-walk build
        config (PROFILE 0, HW-light, no ledger, Task56 mode 2) and reports
        sizeof per image. Prints stay for the residency report; asserts pin
        the layout contract (tag first, prepared_dense contiguous with zero
        padding after dense_normals).
        """
        header = _read(HEADER_H)
        tables = _read(TABLES_H)
        dense_counts = {}
        for flag, title in VARIANTS:
            for detail in ("High", "Low"):
                m = re.search(
                    rf"#define NDS_NATIVE_IMAGE_{flag}_{detail.upper()}"
                    r"_DENSE_VERTICES_COUNT (\d+)u", header)
                self.assertIsNotNone(m)
                dense_counts[(flag, detail)] = int(m.group(1))
        with tempfile.TemporaryDirectory(prefix="variant_image_") as tmp:
            tmp_path = Path(tmp)
            (tmp_path / "nds").mkdir()
            (tmp_path / "nds_build_config.h").write_text(
                "#pragma once\n"
                "#define NDS_RENDERER_PROFILE_LEVEL 0\n"
                "#define NDS_R2_FIGHTER_HW_LIGHT 1\n"
                "#define NDS_RENDERER_M2_DETAILED_LEDGER 0\n"
                "#define NDS_TASK56_FIGHTER_PRIMITIVES 2\n",
                encoding="utf-8")
            (tmp_path / "nds" / "nds_native_fighter_tables.h").write_text(
                tables, encoding="utf-8")
            (tmp_path / "gen.h").write_text(header, encoding="utf-8")
            probe_lines = [
                "#include <stdio.h>",
                "#include <stddef.h>",
                "typedef unsigned char u8; typedef unsigned short u16;",
                "typedef unsigned int u32; typedef short s16;",
                "typedef int s32;",
                '#include "gen.h"',
                "int main(void){",
            ]
            for flag, title in VARIANTS:
                for detail in ("High", "Low"):
                    t = f"NDSNative{title}{detail}Image"
                    probe_lines.append(
                        f'printf("{flag}_{detail} size=%u tagoff=%u '
                        f'pdoff=%u adsoff=%u\\n",'
                        f"(unsigned)sizeof({t}),"
                        f"(unsigned)offsetof({t}, abi_tag),"
                        f"(unsigned)offsetof({t}, prepared_dense),"
                        f"(unsigned)offsetof({t}, action_dense_spans));")
            probe_lines.append("return 0;}")
            c_path = tmp_path / "probe.c"
            c_path.write_text("\n".join(probe_lines), encoding="utf-8")
            exe = c_path.with_suffix(".exe")
            p = subprocess.run(
                [_gcc(), "-std=c99", "-O1", "-w", str(c_path),
                 "-o", str(exe), "-I", str(tmp_path)],
                capture_output=True, text=True, timeout=120)
            self.assertEqual(p.returncode, 0,
                             f"host compile failed: {(p.stderr or '')[:2000]}")
            q = subprocess.run([str(exe)], capture_output=True, text=True,
                               timeout=60)
            out = (q.stdout or "")[:LOG_CAP]
            self.assertEqual(q.returncode, 0, f"host probe failed:\n{out}")
            print(out)
            for line in out.splitlines():
                m = re.fullmatch(
                    r"(\w+)_(High|Low) size=(\d+) tagoff=(\d+) "
                    r"pdoff=(\d+) adsoff=(\d+)", line.strip())
                self.assertIsNotNone(m, f"probe line changed: {line!r}")
                flag, detail = m.group(1), m.group(2)
                size, tagoff, pdoff, adsoff = (
                    int(m.group(3)), int(m.group(4)),
                    int(m.group(5)), int(m.group(6)))
                self.assertEqual(tagoff, 0)
                self.assertEqual(pdoff % 4, 0)
                # Zero padding between prepared_dense and the next member:
                # HW-light stride is 10 B, always even, so the u16 that
                # follows starts exactly at end of the array.
                self.assertEqual(
                    adsoff - pdoff,
                    dense_counts[(flag, detail)] * 10)


if __name__ == "__main__":
    unittest.main(verbosity=2)
