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
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts/menus"))
from source_test_helpers import source as read_split  # noqa: E402
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from makefile_parts import makefile_text  # noqa: E402

HERE = Path(__file__).resolve().parent


def _repo() -> Path:
    for parent in (HERE, *HERE.parents):
        if (parent / "src" / "nds" / "nds_renderer_assets.c").is_file():
            return parent
    raise AssertionError("repo root not found above scripts/fighters")


REPO = _repo()
ASSETS_C = REPO / "src" / "nds" / "nds_renderer_assets.c"
FTMANAGER_C = REPO / "src" / "import" / "battleship_ftmanager.c"
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


def _read(path: Path) -> str:
    return read_split(path)


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

    def test_path_size_rows_for_all_variants(self):
        # Each variant's paths and image types are one generated row; the
        # renderer's path and byte lookups expand the rows (the byte count is
        # the sizeof of the row's image type).
        header = _read(HEADER_H)
        for flag, title in VARIANTS:
            self.assertRegex(
                header,
                rf"X\(NDS_NATIVE_IMAGE_SLOT_{flag},\s*\\\s*"
                rf'NDS_NITRO_FIGHTERS "{flag.lower()}_high\.bin",\s*'
                rf'NDS_NITRO_FIGHTERS "{flag.lower()}_low\.bin",\s*\\\s*'
                rf"NDSNative{title}HighImage,\s*NDSNative{title}LowImage\)")
            self.assertIn(f"NDS_NATIVE_IMAGE_{flag}_LOW_MEMBERS", header)
            self.assertIn(f"NDS_NATIVE_IMAGE_{flag}_HIGH_MEMBERS", header)
        src = _read(ASSETS_C)
        self.assertIn(
            "NDS_NATIVE_OWNER_IMAGE_ROWS(NDS_NATIVE_OWNER_IMAGE_PATH_ROW)", src)
        self.assertIn(
            "NDS_NATIVE_OWNER_IMAGE_ROWS(NDS_NATIVE_OWNER_IMAGE_BYTES_ROW)", src)
        self.assertIn("(u32)sizeof(low_t) : (u32)sizeof(high_t)", src)

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
        # Stage gating mirrors the source: Zako waves arm the polygon image
        # pool (fighter construction fills it, wave by wave), Metal Mario's
        # stage loads its own owner, nothing else pays.
        self.assertIn("nSC1PGameStageZako", helper)
        self.assertIn("ndsRendererNativeArmPolygonImagePool();", helper)
        self.assertIn("nSC1PGameStageMMario", helper)
        self.assertIn("NDS_NATIVE_IMAGE_SLOT_MMARIO, 0u", helper)
        self.assertIn("NDS_NATIVE_IMAGE_SLOT_MMARIO, 1u", helper)
        self.assertIn("#if NDS_NATIVE_OWNER_IMAGE_MMARIO", helper)
        # NLuigi reuses the NMario packet: no slot of its own in the
        # header, loaded through NMARIO at fighter construction.
        header = _read(HEADER_H)
        self.assertNotIn("NDS_NATIVE_IMAGE_SLOT_NLUIGI", header)
        ftmanager = src
        self.assertIn("nFTKindNLuigi", ftmanager)
        self.assertIn("NDS_NATIVE_IMAGE_SLOT_NMARIO", ftmanager)

    def test_preload_executes_only_in_campaign_battle(self):
        sys.path.insert(0, str(REPO / 'scripts/menus'))
        from source_test_helpers import function
        helper = function(_read(FTMANAGER_C), 'ndsFTManagerPreloadVariantOwnerImages')
        for imaged in (1, 0):
            code = f'''#include <assert.h>
typedef unsigned u32; typedef unsigned char u8;
#define NDS_NATIVE_OWNER_IMAGE_MMARIO {imaged}
#define NDS_NATIVE_IMAGE_SLOT_MMARIO 12u
#define nSCKind1PGame 52
#define nSC1PGameStageZako 13
#define nSC1PGameStageMMario 8
static struct {{ int scene_curr; u8 spgame_stage; }} gSCManagerSceneData;
static unsigned calls, arms, mask;
static void ndsRendererNativeArmPolygonImagePool(void) {{ arms++; }}
static int ndsRendererNativeEnsureOwnerImage(u32 slot,u32 detail) {{
    assert(detail < 2); calls++; mask |= 1u << slot; return 1;
}}
''' + helper + f'''
int main(void) {{
    gSCManagerSceneData.spgame_stage=nSC1PGameStageZako;
    gSCManagerSceneData.scene_curr=17;
    ndsFTManagerPreloadVariantOwnerImages(); assert(calls==0 && arms==0);
    gSCManagerSceneData.scene_curr=6;
    ndsFTManagerPreloadVariantOwnerImages(); assert(calls==0 && arms==0);
    gSCManagerSceneData.scene_curr=52;
    ndsFTManagerPreloadVariantOwnerImages();
    assert(calls==0 && arms==1);
    gSCManagerSceneData.spgame_stage=nSC1PGameStageMMario;
    ndsFTManagerPreloadVariantOwnerImages();
    assert(calls=={2 if imaged else 0} && arms==1);
    assert(mask=={1 << 12 if imaged else 0});
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
        mk = makefile_text(REPO)
        for flag, _title in VARIANTS:
            # Flag derives from the matching P2 build flag (enabled only
            # when the owner is built).
            self.assertIn(
                f"NDS_NATIVE_OWNER_IMAGE_{flag} = "
                f"$(if $(filter 1,$(NDS_NATIVE_OWNER_IMAGE)),"
                f"$(NDS_{flag}),0)", mk)
            # NitroFS staging membership under the same P2 guard.
            self.assertIn(f"ifeq ($(NDS_{flag}),1)", mk)
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

    def test_abi_tag_consistency_ndo6(self):
        header = _read(HEADER_H)
        src = _read(ASSETS_C)
        for text in (header, src):
            m = re.search(
                r"#define\s+NDS_NATIVE_OWNER_IMAGE_ABI_TAG\s+(0x[0-9a-fA-F]+)u?",
                text)
            self.assertIsNotNone(m)
            self.assertEqual(int(m.group(1), 16), 0x364F444E)

    def test_image_sizes_host_measured(self):
        """Exact active-image bytes per variant owner under campaign flags.

        Compiles the real generated header with the campaign-walk build
        config (PROFILE 0, HW-light, no ledger, primitive mode 2) and reports
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
                "#define NDS_FIGHTER_PRIMITIVES 2\n",
                encoding="utf-8")
            (tmp_path / "nds" / "nds_native_fighter_tables.h").write_text(
                tables, encoding="utf-8")
            # The generated header includes the lean-live flag header too.
            (tmp_path / "nds" / "nds_ftr_lean_live.h").write_text(
                (REPO / "include" / "nds" / "nds_ftr_lean_live.h").read_text(
                    encoding="utf-8"), encoding="utf-8")
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
