#!/usr/bin/env python3
"""Host test for scene-resident native fighter PreparedDense arrays.

Contract: the exact generated initial PreparedDense bytes ship inside the
existing NitroFS native owner image payloads, the runtime tables bind
`prepared_dense` to that aligned resident member, and no image-backed owner
keeps a static initialized array in builds that load it from an image.
Mario/Fox keep their frozen static path (no image, no bind block).

Strategy: exercise the real producer (`generate_nds_native_owner_images`)
on small synthetic owner contexts (no BattleShip inputs, no ROM, no
emulator), host-compile the rendered header with gcc to prove the layout
(size/alignment/member placement) under both PreparedDense shapes, and
assert the runtime bind contract by reading `src/nds/nds_renderer_assets.c`
source. All build artifacts live in a temp dir. Logs bounded.
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
sys.path.insert(0, str(HERE))


def _repo() -> Path:
    for parent in (HERE, *HERE.parents):
        if (parent / "src" / "nds" / "nds_renderer_assets.c").is_file():
            return parent
    raise AssertionError("repo root not found above scripts/fighters")


REPO = _repo()
LOADER_C = REPO / "src" / "nds" / "nds_renderer_assets.c"
OWNERS_PY = HERE / "generate_nds_native_owners.py"
LOG_CAP = 8000

import generate_nds_native_owner_images as images  # noqa: E402
import generate_nds_native_owners as owners  # noqa: E402
from native_owner_image_arrays import (  # noqa: E402
    NATIVE_OWNER_IMAGE_ARRAYS,
    NATIVE_OWNER_RESIDENT_ARRAYS,
)

PREPARED_GUARD = "NDS_RENDERER_PROFILE_LEVEL < 2"
NDS_NATIVE_OWNER_IMAGE_ABI_TAG = 0x344F444E


def _synthetic_context(owner_name="luigi", detail="high", dense_count=3):
    """Minimal context with the keys _member_values reads."""
    dense_vertices = [
        (10 + i, 20 + i, -5 - i, 100 * i, -100 * i, 1, 2 + i,
         0x80402010 + i)
        for i in range(dense_count)
    ]
    gx_positions = [(x * 16, y * 16, z * 16)
                    for x, y, z, *_ in dense_vertices]
    streams = {
        mode: ([0], [1], [0], [0], [3], [0x0001, 0x0002, 0x0003])
        for mode in (1, 2)
    }
    return {
        "owner_name": owner_name,
        "detail": detail,
        "state": [(0x11111111, 0x22222222, 3)],
        "sequence": [0, 1],
        "vertex": [(1, 2, 3, 4, 0x1000, 5, 6)],
        "triangles": [0],
        "runs": [(0, 1, 0, 0xFFFFFFFF)],
        "epochs": [(0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0)],
        "dense_vertices": dense_vertices,
        "gx_positions": gx_positions,
        "dense_color_sources": [7] * dense_count,
        "action_dense_spans": [0x0401],
        "packed_corners": [0x0002, 0x0801, 0x0000],
        "run_first_corner": [0],
        "run_first_unique": [0],
        "run_unique_count": [2],
        "run_unique_dense": [0, 1],
        "direct_epoch_policies": [0x01],
        "primitive_streams": streams,
    }


def _member_dict(context):
    return {name: (ctype, values, guard)
            for ctype, name, values, guard in images._member_values(context)}


def _decode_row_xy_z(row: str) -> tuple[int, int, int]:
    m = re.fullmatch(
        r"\{ \.gx_xy = (0x[0-9a-fA-F]+)u, \.gx_z = (0x[0-9a-fA-F]+)u \},?",
        row.strip().rstrip(","),
    )
    assert m, f"prepared_dense row format changed: {row!r}"
    xy, z = int(m.group(1), 16), int(m.group(2), 16)
    x = xy & 0xFFFF
    y = (xy >> 16) & 0xFFFF
    return x, y, z


def _sign16(value: int) -> int:
    return value - 0x10000 if value >= 0x8000 else value


def _gcc():
    cc = shutil.which("gcc") or shutil.which("cc") or shutil.which("clang")
    assert cc, "no host C compiler found"
    return cc


def _compile_and_run(c_path: Path, extra=()) -> str:
    exe = c_path.with_suffix(".exe" if sys.platform == "win32" else ".out")
    p = subprocess.run([_gcc(), "-std=c99", "-O1", "-w", str(c_path),
                        "-o", str(exe), *extra],
                       capture_output=True, text=True, timeout=120)
    assert p.returncode == 0, f"host compile failed: {(p.stderr or '')[:2000]}"
    q = subprocess.run([str(exe)], capture_output=True, text=True, timeout=60)
    out = (q.stdout or "")[:LOG_CAP]
    assert q.returncode == 0, f"host program failed:\n{out}"
    return out


class PreparedDenseResidencyTests(unittest.TestCase):
    def test_shared_list_contract(self):
        self.assertIn("PreparedDense", NATIVE_OWNER_IMAGE_ARRAYS)
        self.assertNotIn("PreparedDense", NATIVE_OWNER_RESIDENT_ARRAYS)
        # The residents that remain: owner roots and palette slots.
        self.assertEqual(set(NATIVE_OWNER_RESIDENT_ARRAYS),
                         {"Roots", "CrossPaletteSlots"})

    def test_member_values_shape(self):
        context = _synthetic_context()
        members = images._member_values(context)
        names = [name for _ctype, name, _values, _guard in members]
        # Placed immediately after dense_normals: a u32 member, so the new
        # member always starts 4-aligned with zero padding in both layouts.
        self.assertEqual(names[names.index("dense_normals") + 1],
                         "prepared_dense")
        by_name = _member_dict(context)
        ctype, values, guard = by_name["prepared_dense"]
        self.assertEqual(ctype, "NDSNativePreparedDenseVertex")
        self.assertEqual(guard, PREPARED_GUARD)
        self.assertEqual(len(values), len(context["dense_vertices"]))
        # Same stems the owners choke point guards out of the binary; this
        # mirrors the agreement assert in the producer main().
        described = {images._array_stem(name) for name in names}
        self.assertEqual(described, set(NATIVE_OWNER_IMAGE_ARRAYS))

    def test_member_rows_are_exact_pack(self):
        context = _synthetic_context(dense_count=3)
        by_name = _member_dict(context)
        _ctype, values, _guard = by_name["prepared_dense"]
        for row, (x, y, z, _s, _t, _b, _c, _r) in zip(
                values, context["dense_vertices"]):
            # The producer must call the owners pack helper (source chain),
            # and the emitted words must decode to the scaled positions.
            decoded = _decode_row_xy_z(row)
            self.assertEqual(decoded, (x * 16 & 0xFFFF,
                                       y * 16 & 0xFFFF,
                                       z * 16 & 0xFFFF))
            (_sx, _sy, _sz) = (_sign16(decoded[0]), _sign16(decoded[1]),
                               _sign16(decoded[2]))
            self.assertEqual((_sx, _sy, _sz), (x * 16, y * 16, z * 16))
        producer_src = Path(images.__file__).read_text(encoding="utf-8")
        self.assertIn("owners.pack_fifo_vertex16_scaled", producer_src)

    def test_missing_inputs_fail_closed(self):
        context = _synthetic_context()
        del context["gx_positions"]
        with self.assertRaises(KeyError):
            images._member_values(context)
        short = _synthetic_context(dense_count=3)
        short["gx_positions"] = short["gx_positions"][:1]
        with self.assertRaises(IndexError):
            images._member_values(short)

    def test_header_names_prepared_dense(self):
        contexts = {
            ("luigi", "high"): _synthetic_context("luigi", "high", 3),
            ("luigi", "low"): _synthetic_context("luigi", "low", 2),
        }
        header = images.render_header(contexts)
        # Fallback element type for standalone image TUs, exactly once, and
        # skipped when the renderer provides its own identical definition.
        self.assertEqual(
            header.count("typedef struct NDSNativePreparedDenseVertex"), 1)
        self.assertIn("#ifndef NDS_NATIVE_PREPARED_DENSE_DEFINED_BY_RENDERER",
                      header)
        self.assertIn("} __attribute__((packed, aligned(2))) "
                      "NDSNativePreparedDenseVertex;", header)
        # Member declared under the same PROFILE guard as the runtime field.
        decl = "    NDSNativePreparedDenseVertex prepared_dense[3];"
        self.assertIn(f"#if {PREPARED_GUARD}\n{decl}\n#endif", header)
        self.assertIn("    NDSNativePreparedDenseVertex prepared_dense[2];",
                      header)
        # Count macro and equivalence row name the static array the image
        # replaces, so the VERIFY build byte-compares the same bytes.
        self.assertIn("#define NDS_NATIVE_IMAGE_LUIGI_HIGH_PREPARED_DENSE_COUNT"
                      " 3u", header)
        self.assertIn("X(NDSNativeLuigiHighImage, prepared_dense, "
                      "sNdsNativeLuigiFighterPreparedDense)", header)
        self.assertIn("X(NDSNativeLuigiLowImage, prepared_dense, "
                      "sNdsNativeLuigiFighterPreparedDenseLow)", header)
        # abi_tag stays the first member of every image struct.
        for type_name in ("NDSNativeLuigiHighImage", "NDSNativeLuigiLowImage"):
            body = header.split(f"typedef struct {type_name}")[1].split(
                f"}} {type_name};")[0]
            first = [ln.strip() for ln in body.splitlines()
                     if ln.strip() and not ln.strip().startswith("#")
                     and ln.strip() != "{"][0]
            self.assertEqual(first, "u32 abi_tag[1];")

    def test_header_layout_host_compiled(self):
        """Prove size/alignment/placement of the new member with gcc.

        Both PreparedDense shapes: HW-light 10 B and CPU-light 16 B. The
        member must start 4-aligned (element 0 word loads stay aligned) with
        member bytes exactly N * sizeof, i.e. zero padding and an image byte
        increase exactly equal to the static bytes removed.
        """
        contexts = {("luigi", "high"): _synthetic_context("luigi", "high", 3)}
        header = images.render_header(contexts)
        tables_h = (REPO / "include" / "nds" / "nds_native_fighter_tables.h"
                    ).read_text(encoding="utf-8")
        with tempfile.TemporaryDirectory(prefix="pd_residency_") as tmp:
            tmp_path = Path(tmp)
            for hw, size in ((1, 10), (0, 16)):
                inc = tmp_path / f"inc{hw}"
                (inc / "nds").mkdir(parents=True)
                (inc / "nds_build_config.h").write_text(
                    "#pragma once\n"
                    f"#define NDS_RENDERER_PROFILE_LEVEL 0\n"
                    f"#define NDS_R2_FIGHTER_HW_LIGHT {hw}\n"
                    "#define NDS_RENDERER_M2_DETAILED_LEDGER 0\n"
                    "#define NDS_TASK56_FIGHTER_PRIMITIVES 1\n",
                    encoding="utf-8")
                (inc / "nds" / "nds_native_fighter_tables.h").write_text(
                    tables_h, encoding="utf-8")
                (inc / "gen.h").write_text(header, encoding="utf-8")
                c_path = tmp_path / f"layout{hw}.c"
                c_path.write_text(
                    "#include <stdio.h>\n"
                    "#include <stddef.h>\n"
                    "typedef unsigned char u8; typedef unsigned short u16;\n"
                    "typedef unsigned int u32; typedef short s16;\n"
                    "typedef int s32;\n"
                    '#include "gen.h"\n'
                    "int main(void) {\n"
                    "    printf(\"pd_size=%u\\n\", (unsigned)sizeof("
                    "NDSNativePreparedDenseVertex));\n"
                    "    printf(\"off=%u\\n\", (unsigned)offsetof("
                    "NDSNativeLuigiHighImage, prepared_dense));\n"
                    "    printf(\"memb=%u\\n\", (unsigned)sizeof("
                    "((NDSNativeLuigiHighImage*)0)->prepared_dense));\n"
                    "    printf(\"tagoff=%u\\n\", (unsigned)offsetof("
                    "NDSNativeLuigiHighImage, abi_tag));\n"
                    "    return 0;\n"
                    "}\n",
                    encoding="utf-8")
                out = _compile_and_run(
                    c_path, extra=(f"-I{inc}", f"-I{tmp_path}"))
                got = dict(line.split("=") for line in out.split() if "=" in line)
                self.assertEqual(int(got["pd_size"]), size)
                self.assertEqual(int(got["tagoff"]), 0)
                self.assertEqual(int(got["memb"]), 3 * size)
                self.assertEqual(int(got["off"]) % 4, 0,
                                 f"prepared_dense not 4-aligned (hw={hw})")
                print(f"hw={hw} pd_size={got['pd_size']} off={got['off']} "
                      f"memb={got['memb']} tagoff={got['tagoff']}")

    def test_renderer_bind_contract(self):
        src = LOADER_C.read_text(encoding="utf-8")
        # The renderer provides its own identical element type; the generated
        # header skips its fallback copy here.
        define = "#define NDS_NATIVE_PREPARED_DENSE_DEFINED_BY_RENDERER 1"
        self.assertIn(define, src)
        self.assertLess(src.index(define),
                        src.index("nds_native_fighter_image.generated.h"))
        # The bind macro takes no static array anymore and binds the tables
        # pointer to the image's own resident member (arena-writable buffer).
        m = re.search(r"#define NDS_IMG_BIND\(([^)]*)\)", src)
        self.assertIsNotNone(m)
        self.assertNotIn("prepared_", m.group(1))
        self.assertIn("(NDSNativePreparedDenseVertex *)img_->prepared_dense",
                      src)
        # All 46 image-backed call sites pass exactly (tables, type, base,
        # prefix): 3 commas, and no static PreparedDense 5th argument.
        # (The #define line and the _COLOR/_PRIMITIVES helpers are excluded:
        # call sites are the only indented NDS_IMG_BIND invocations.)
        calls = re.findall(r"(?m)^\s+NDS_IMG_BIND\((.*?)\);", src, re.DOTALL)
        self.assertEqual(len(calls), 46)
        for call in calls:
            self.assertEqual(call.count(","), 3,
                             f"bind call arg count changed: {call[:80]}")
            self.assertNotIn("PreparedDense", call)
        # No static PreparedDense symbol survives inside the bind function.
        fn_start = src.index("static void ndsRendererNativeBindOwnerImage")
        brace = src.index("{", fn_start)
        depth = 0
        for end in range(brace, len(src)):
            if src[end] == "{":
                depth += 1
            elif src[end] == "}":
                depth -= 1
                if depth == 0:
                    break
        self.assertIn("}", src[brace:end + 1])
        self.assertNotIn("PreparedDense", src[brace:end + 1])
        # ABI checks preserved: both layout sizes still asserted.
        self.assertIn(
            "_Static_assert(sizeof(NDSNativePreparedDenseVertex) == 10u", src)
        self.assertIn(
            "_Static_assert(sizeof(NDSNativePreparedDenseVertex) == 16u", src)
        # Mario/Fox frozen static path untouched (no image, still static).
        self.assertIn("sNdsNativeFighterPreparedDense,", src)
        self.assertIn("sNdsNativeFighterPreparedDenseLow,", src)
        self.assertNotIn("NDS_NATIVE_OWNER_IMAGE_MARIO", src)

    def test_verification_after_reference_scratch_was_drawn(self):
        sys.path.insert(0, str(REPO / 'scripts/menus'))
        from source_test_helpers import function
        source = LOADER_C.read_text(encoding='utf8')
        start = source.index('typedef struct NDSNativePreparedDenseVertex')
        end = source.index('typedef struct NDSNativeRoot', start)
        record = source[start:end].rsplit('#endif', 1)[0]
        helper = function(source, 'ndsRendererNativeVerifyPreparedMember')
        start = source.index('#define NDS_IMG_VERIFY(')
        dispatch = source[start:source.index('#else', start)]
        with tempfile.TemporaryDirectory() as directory:
            for hw in (0, 1):
                program = f'''#include <assert.h>
#include <stdint.h>
typedef uint32_t u32; typedef uint16_t u16; typedef int16_t s16;
#define NDS_R2_FIGHTER_HW_LIGHT {hw}
static u32 gNdsNativeOwnerImageMatchCount, gNdsNativeOwnerImageMismatchCount;
void ndsRendererNativeVerifyMember(const void*,u32,const void*,u32);
''' + record + '\n' + helper + '\n' + dispatch + r'''
int main(void) {
    NDSNativePreparedDenseVertex reference[2] = {{0}};
    struct { NDSNativePreparedDenseVertex rows[2]; } image = {{{0}}}, *img_ = &image;
    reference[0].gx_xy = image.rows[0].gx_xy = 0xabcdef01;
    reference[1].gx_z = image.rows[1].gx_z = 0x8000;
    reference[0].s = 37; reference[1].t = -49;
#if !NDS_R2_FIGHTER_HW_LIGHT
    reference[0].shaded_rgba = 0x11223344; reference[1].packed_color = 0x1234;
#endif
    NDS_IMG_VERIFY(ignored, rows, reference)
    assert(gNdsNativeOwnerImageMatchCount == 1);
    image.rows[0].gx_xy ^= 1;
    NDS_IMG_VERIFY(ignored, rows, reference)
    image.rows[0].gx_xy ^= 1;
    image.rows[1].s = 1;
    NDS_IMG_VERIFY(ignored, rows, reference)
    ndsRendererNativeVerifyPreparedMember(image.rows, sizeof(image.rows)-1,
                                          reference, sizeof(reference));
    assert(gNdsNativeOwnerImageMismatchCount == 3);
    return 0;
}
'''
                c = Path(directory) / ('verify%d.c' % hw)
                exe = c.with_suffix('.exe')
                c.write_text(program)
                result = subprocess.run([shutil.which('gcc'), '-std=c11', str(c),
                                         '-o', str(exe)], capture_output=True)
                self.assertEqual(result.returncode, 0, result.stderr.decode())
                subprocess.run([str(exe)], check=True, capture_output=True)

    def test_owners_chokepoint_and_typedef_shape(self):
        owners_src = OWNERS_PY.read_text(encoding="utf-8")
        # The P2 PreparedDense emission routes through the guarded local
        # emit_rows choke point exactly once (guarded out when imaged).
        self.assertEqual(
            owners_src.count(
                '"NDSNativePreparedDenseVertex", f"{stem}PreparedDense{suffix}"'),
            1)
        self.assertIn("if base in NATIVE_OWNER_IMAGE_ARRAYS:", owners_src)
        # The renderer typedef the storage test extracts keeps its shape:
        # both HW branches and the packed 10-byte layout.
        renderer_src = LOADER_C.read_text(encoding="utf-8")
        a = renderer_src.index("typedef struct NDSNativePreparedDenseVertex")
        b = renderer_src.index("typedef struct NDSNativeRoot", a)
        record = renderer_src[a:b].rsplit("#endif", 1)[0]
        self.assertIn("#if !NDS_R2_FIGHTER_HW_LIGHT", record)
        self.assertIn("} __attribute__((packed, aligned(2))) "
                      "NDSNativePreparedDenseVertex;", record)


if __name__ == "__main__":
    unittest.main(verbosity=2)
