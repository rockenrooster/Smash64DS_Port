"""Host execution tests for src/nds/nds_native_wallpaper.c.

Compiles the REAL runtime TU on host against small shadows
(scripts/stages/native_wallpaper_host/) of its DS-only imports: the
platform overlay/commit/affine-queue trio, the one-open reloc stream,
and dmaFillHalfWords writing into a page-guarded 256x192 VRAM stand-in
(a clean harness exit proves no store escaped the layer). The descriptor
table fixture is generated through the REAL generator's render_header
with synthetic payloads, so the schema stays locked to
generate_native_wallpapers.py.

Covers: every output row for RGB555 battle and Results intensity
expansion (backward, in place), cached reuse without any stream reads,
reload on epoch change / palette change / invalidation, rejection of
unknown assets, wrong offsets, bad formats/sizes, palette-presence
mismatch, missing files, short reads and corrupt texels (with the
partial-row damage boundary checked), and the affine contract: queued
pa/pd/dx/dy plus first/last sampled texels at the Results full-screen
crop and at real battle transforms derived from the source pan/zoom law
(grwallpaper.c 1.004..2.0 scale, pos clamps) through the port's 9/8
presentation stretch. Invalid transforms are refused before any I/O.

Run:  python -m pytest scripts/stages/test_native_wallpaper_runtime.py -q
  or: python scripts/stages/test_native_wallpaper_runtime.py
"""

from __future__ import annotations

import hashlib
import itertools
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

_p = Path(__file__).resolve().parent
while _p.name != "scripts":
    _p = _p.parent
sys.path.insert(0, str(_p))
import _paths  # noqa: F401,E402

import generate_native_wallpapers as gen  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = Path(__file__).resolve().parent / "native_wallpaper_host"
RUNTIME = ROOT / "src" / "nds" / "nds_native_wallpaper.c"
INCLUDE = ROOT / "include"

W, H, PITCH, SCREEN_H = 240, 176, 256, 192
VRAM_BYTES = PITCH * SCREEN_H * 2
FILL = 0x8000

BATTLE_ID, BATTLE_OFF = 0x58, 0x26C88
NOALPHA_ID = 0x59
SHORT_ID = 0x5B
ABSENT_ID = 0x5D
WIDE_ID = 0x5F
BADINT_ID = 0x5E
FMT2_ID = 0x63
RESULTS_ID, RESULTS_OFF = 0x22, 0xD5C8

PALETTE_A = [0x8000 | (i << 10) | (i << 5) | i for i in range(16)]
PALETTE_B = [0x8000 | ((15 - i) << 10) | (((i * 2) & 31) << 5) | (15 - i)
             for i in range(16)]

# Battle transforms. FLOOR is grWallpaperMakeCommon's rest pose (pos 10,10,
# scale 1.004 -> q16 65798) after the port's 9/8 presentation stretch
# (sprite_preview_backend.c): origin 160+mul9div8(10-160)=-8,
# 120+mul9div8(10-120)=-3, scale 65798+((65798+4)>>3)=74023. IDENTITY, MAX
# and CENTER are the law's other ends (scale 1.0, 2.0, centred origin).
IDENTITY = (0, 0, 65536, 65536)
FLOOR = (-8, -3, 74023, 74023)
UNSTRETCHED_FLOOR = (10, 10, 65798, 65798)
MAX_ZOOM = (160, 120, 131072, 131072)
BATTLE_TRANSFORMS = {
    "identity": (IDENTITY, (256, 256, 128, 128), (0, 0), (255, 191)),
    "floor": (FLOOR, (227, 227, 1565, 657), (6, 2), (232, 171)),
    "unstretched_floor": (UNSTRETCHED_FLOOR, (255, 255, -1913, -1913),
                          (-8, -8), (246, 182)),
    "max_zoom": (MAX_ZOOM, (128, 128, -16320, -12224), (-64, -48), (63, 47)),
}
RESULTS_AFFINE = (240, 235, 120, 117)


def battle_payload() -> bytes:
    out = bytearray()
    for y in range(H):
        for x in range(W):
            out += struct.pack(
                "<H", 0x8000 | (((x + 2 * y) & 31) << 10) |
                (((x * 3 + y) & 31) << 5) | ((x ^ y) & 31))
    return bytes(out)


def results_payload() -> bytes:
    return bytes((x * 5 + y * 11) % 16 * 17
                 for y in range(H) for x in range(W))


def build_vram(rows):
    """rows: up to 176 lists of texel halfwords; missing/padding -> FILL."""
    out = []
    for y in range(SCREEN_H):
        row = rows[y] if y < len(rows) else None
        if not row:
            out.extend([FILL] * PITCH)
        else:
            out.extend(row)
            out.extend([FILL] * (PITCH - len(row)))
    return struct.pack(f"<{PITCH * SCREEN_H}H", *out)


def battle_rows(payload, count=H):
    tex = struct.unpack(f"<{len(payload) // 2}H", payload)
    return [list(tex[y * W:(y + 1) * W]) for y in range(count)]


def results_rows(payload, palette, count=H):
    return [[palette[b // 17] for b in payload[y * W:(y + 1) * W]]
            for y in range(count)]


def c_div(a, b):
    """C integer division: truncation toward zero."""
    q = abs(a) // abs(b)
    return q if (a < 0) == (b < 0) else -q


def battle_affine(origin_x, origin_y, scale_x, scale_y):
    """Python mirror of ndsNativeWallpaperAffine's format-0 path."""
    if (scale_x == 0 or scale_y == 0 or
            not -32768 <= origin_x <= 32767 or
            not -32768 <= origin_y <= 32767):
        return None
    pa = ((1 << 24) + scale_x // 2) // scale_x
    pd = ((1 << 24) + scale_y // 2) // scale_y
    if not (0 < pa <= 32767 and 0 < pd <= 32767):
        return None
    x = c_div(pa, 2) - c_div(origin_x * 4 * pa, 5)
    y = c_div(pd, 2) - c_div(origin_y * 4 * pd, 5)
    if not (-134217728 <= x <= 134217727
            and -134217728 <= y <= 134217727):
        return None
    return pa, pd, x, y


def sampled_texel(affine, screen_x, screen_y):
    """First/last hardware sample: floor((d + screen*step) / 256)."""
    pa, pd, dx, dy = affine
    return ((dx + screen_x * pa) >> 8, (dy + screen_y * pd) >> 8)


class NativeWallpaperRuntimeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("gcc") or shutil.which("clang")
        assert compiler, "host C compiler required"
        cls.tmp = tempfile.TemporaryDirectory(
            prefix="smash64ds-wallpaper-runtime-")
        tmp = Path(cls.tmp.name)
        cls.nitro_root = tmp / "root" / "wallpapers"
        cls.nitro_root.mkdir(parents=True)
        cls.battle = battle_payload()
        cls.results = results_payload()
        noalpha = bytearray(cls.battle)
        bad = 4 * W + 9
        (clear,) = struct.unpack_from("<H", noalpha, bad * 2)
        struct.pack_into("<H", noalpha, bad * 2, clear & 0x7FFF)
        cls.noalpha = bytes(noalpha)
        badint = bytearray(cls.results)
        badint[5 * W + 7] = 18
        cls.badint = bytes(badint)
        cls.short = cls.battle[:90 * W * 2]

        def asset(key, file_id, offset, fmt, payload, native_w=W):
            source = gen.WallpaperSource(key, "", file_id, offset, True)
            return gen.ConvertedAsset(
                source, 300, 220, native_w, H, fmt,
                f"native_wallpaper_{key}.bin", payload, offset)

        assets = [
            asset("battle", BATTLE_ID, BATTLE_OFF,
                  gen.FORMAT_BATTLE_RGB555, cls.battle),
            asset("noalpha", NOALPHA_ID, BATTLE_OFF,
                  gen.FORMAT_BATTLE_RGB555, cls.noalpha),
            asset("short", SHORT_ID, BATTLE_OFF,
                  gen.FORMAT_BATTLE_RGB555, cls.short),
            asset("absent", ABSENT_ID, BATTLE_OFF,
                  gen.FORMAT_BATTLE_RGB555, cls.battle),
            asset("wide", WIDE_ID, BATTLE_OFF,
                  gen.FORMAT_BATTLE_RGB555, b"", native_w=320),
            asset("fmt2", FMT2_ID, BATTLE_OFF, 2, b""),
            asset("results", RESULTS_ID, RESULTS_OFF,
                  gen.FORMAT_RESULTS_I8, cls.results),
            asset("badint", BADINT_ID, RESULTS_OFF,
                  gen.FORMAT_RESULTS_I8, cls.badint),
        ]
        generated = tmp / "generated"
        generated.mkdir()
        (generated / "native_wallpapers.generated.inc").write_text(
            gen.render_header(assets), encoding="utf-8")
        for name, payload in (("battle", cls.battle),
                              ("noalpha", cls.noalpha),
                              ("short", cls.short),
                              ("results", cls.results),
                              ("badint", cls.badint)):
            (cls.nitro_root / f"native_wallpaper_{name}.bin").write_bytes(
                payload)

        cls.binary = tmp / "wallpaper_harness"
        # The runtime's generated-table include is source-relative, and
        # src/nds/generated/ holds Main's build output, which would shadow
        # any -I fixture. Compile a hash-verified byte copy from the temp
        # tree so "generated/..." resolves to this test's fixture.
        cls.runtime_copy = tmp / "nds_native_wallpaper.c"
        shutil.copyfile(RUNTIME, cls.runtime_copy)
        if (hashlib.sha256(cls.runtime_copy.read_bytes()).hexdigest() !=
                hashlib.sha256(RUNTIME.read_bytes()).hexdigest()):
            raise AssertionError("runtime copy is not byte-identical")
        result = subprocess.run(
            [compiler, "-std=c11", "-Wall", "-Wextra", "-O1",
             "-I", str(FIXTURE), "-I", str(INCLUDE), "-I", str(tmp),
             "-o", str(cls.binary),
             str(FIXTURE / "harness.c"), str(cls.runtime_copy)],
            capture_output=True, text=True, timeout=120)
        if result.returncode != 0:
            raise AssertionError(
                "harness compile failed: "
                + (result.stdout + result.stderr)[-2000:])
        cls._dumps = itertools.count()

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_harness(self, command, asset_id, offset, origin_x, origin_y,
                    scale_x, scale_y, palette=0):
        dump = Path(self.tmp.name) / f"vram_{next(self._dumps)}.bin"
        result = subprocess.run(
            [str(self.binary), str(self.nitro_root.parent), command,
             str(dump), hex(asset_id), hex(offset), str(origin_x),
             str(origin_y), str(scale_x), str(scale_y), str(palette)],
            capture_output=True, text=True, timeout=60)
        if result.returncode != 0:
            raise AssertionError(
                f"harness {command}/{asset_id:#x} exited "
                f"{result.returncode}: {result.stderr[-500:]}")
        blocks = {}
        for line in result.stdout.splitlines():
            key, _, value = line.partition("=")
            prefix, _, field = key.partition(".")
            blocks.setdefault(prefix, {})[field] = value
        return blocks, dump.read_bytes()

    def check_load(self, block, *, reads, result="1"):
        self.assertEqual(block["result"], result, "draw result")
        self.assertEqual(block["load"], "1", "load count")
        self.assertEqual(block["reuse"], "0", "reuse count")
        self.assertEqual(block["fail"], "0", "read-failure count")
        self.assertEqual(block["opens"], "1", "stream opens")
        self.assertEqual(block["reads"], str(reads), "stream reads")

    def check_reject(self, block, *, opens, fail, dump,
                     message="draw should have been refused"):
        self.assertEqual(block["result"], "0", message)
        self.assertEqual(block["load"], "0", "load count")
        self.assertEqual(block["reuse"], "0", "reuse count")
        self.assertEqual(block["fail"], str(fail), "read-failure count")
        self.assertEqual(block["opens"], str(opens), "stream opens")
        self.assertEqual(block["queued"], "0", "nothing queued")
        self.assertEqual(len(dump), VRAM_BYTES, "layer dump size")

    def test_battle_rgb555_writes_every_output_row(self):
        origin_x, origin_y, sx, sy = FLOOR
        blocks, dump = self.run_harness("draw", BATTLE_ID, BATTLE_OFF,
                                        origin_x, origin_y, sx, sy)
        self.check_load(blocks["d1"], reads=H)
        self.assertEqual(
            (int(blocks["d1"]["qpa"]), int(blocks["d1"]["qpd"]),
             int(blocks["d1"]["qdx"]), int(blocks["d1"]["qdy"])),
            battle_affine(origin_x, origin_y, sx, sy), "queued affine")
        self.assertEqual(dump, build_vram(battle_rows(self.battle)),
                         "all 192 output rows")

    def test_results_intensity_expands_every_output_row(self):
        blocks, dump = self.run_harness("draw", RESULTS_ID, RESULTS_OFF,
                                        *IDENTITY, palette=1)
        self.check_load(blocks["d1"], reads=H)
        # Backward in-place expansion: a forward loop would trample the
        # unread index bytes, so row-exact equality proves direction.
        self.assertEqual(dump,
                         build_vram(results_rows(self.results, PALETTE_A)),
                         "all 192 output rows")
        self.assertEqual(
            (int(blocks["d1"]["qpa"]), int(blocks["d1"]["qpd"]),
             int(blocks["d1"]["qdx"]), int(blocks["d1"]["qdy"])),
            RESULTS_AFFINE, "Results affine")

    def test_cached_reuse_does_not_read(self):
        blocks, dump = self.run_harness("draw_twice", BATTLE_ID, BATTLE_OFF,
                                        *IDENTITY)
        self.check_load(blocks["d1"], reads=H)
        second = blocks["d2"]
        self.assertEqual(second["result"], "1", "reuse draw result")
        self.assertEqual(second["load"], "1", "no reload")
        self.assertEqual(second["reuse"], "1", "reuse count")
        self.assertEqual(second["fail"], "0", "read-failure count")
        self.assertEqual(second["opens"], "1", "no second open")
        self.assertEqual(second["reads"], str(H), "no second read")
        self.assertEqual(dump, build_vram(battle_rows(self.battle)),
                         "layer unchanged by reuse")

    def test_epoch_change_reloads(self):
        blocks, _ = self.run_harness("epoch_cycle", BATTLE_ID, BATTLE_OFF,
                                     *IDENTITY)
        self.assertEqual(blocks["d1"]["load"], "1")
        self.assertEqual(blocks["d2"]["result"], "1")
        self.assertEqual(blocks["d2"]["load"], "2", "epoch change reloads")
        self.assertEqual(blocks["d2"]["reuse"], "0")
        self.assertEqual(blocks["d2"]["opens"], "2")
        self.assertEqual(blocks["d2"]["reads"], str(2 * H))

    def test_palette_change_reloads(self):
        blocks, dump = self.run_harness("palette_cycle", RESULTS_ID,
                                        RESULTS_OFF, *IDENTITY)
        self.assertEqual(blocks["d1"]["load"], "1")
        self.assertEqual(blocks["d2"]["reuse"], "1", "same palette reuses")
        self.assertEqual(blocks["d3"]["result"], "1")
        self.assertEqual(blocks["d3"]["load"], "2", "palette change reloads")
        self.assertEqual(blocks["d3"]["reuse"], "1")
        self.assertEqual(blocks["d3"]["opens"], "2")
        self.assertEqual(dump,
                         build_vram(results_rows(self.results, PALETTE_B)),
                         "reload expanded with the new palette")

    def test_invalidation_reloads_and_queues_identity(self):
        blocks, _ = self.run_harness("invalidate_cycle", BATTLE_ID,
                                     BATTLE_OFF, *IDENTITY)
        self.assertEqual(blocks["d1"]["load"], "1")
        self.assertEqual(
            (int(blocks["inv"]["qpa"]), int(blocks["inv"]["qpd"]),
             int(blocks["inv"]["qdx"]), int(blocks["inv"]["qdy"])),
            (256, 256, 0, 0), "invalidate queues the identity affine")
        self.assertEqual(blocks["inv"]["queued"], "1")
        self.assertEqual(blocks["d2"]["result"], "1")
        self.assertEqual(blocks["d2"]["load"], "2", "invalidation reloads")
        self.assertEqual(blocks["d2"]["reuse"], "0")

    def test_unknown_asset_and_offset_reject(self):
        for label, asset_id, offset in (
                ("unknown id", 0x777, BATTLE_OFF),
                ("unknown offset", BATTLE_ID, 0x1)):
            with self.subTest(case=label):
                blocks, dump = self.run_harness("draw", asset_id, offset,
                                                *IDENTITY)
                self.check_reject(blocks["d1"], opens=0, fail=0, dump=dump)
                self.assertEqual(dump, b"\x00" * VRAM_BYTES,
                                 "layer untouched")

    def test_bad_format_and_dimensions_reject(self):
        for label, asset_id, palette in (("format 2", FMT2_ID, 0),
                                         ("320 wide", WIDE_ID, 0)):
            with self.subTest(case=label):
                blocks, dump = self.run_harness("draw", asset_id, BATTLE_OFF,
                                                *IDENTITY, palette=palette)
                self.check_reject(blocks["d1"], opens=0, fail=0, dump=dump)

    def test_palette_presence_mismatch_reject(self):
        for label, asset_id, offset, palette in (
                ("Results without palette", RESULTS_ID, RESULTS_OFF, 0),
                ("battle with palette", BATTLE_ID, BATTLE_OFF, 1)):
            with self.subTest(case=label):
                blocks, dump = self.run_harness("draw", asset_id, offset,
                                                *IDENTITY, palette=palette)
                self.check_reject(blocks["d1"], opens=0, fail=0, dump=dump)

    def test_missing_file_reject(self):
        blocks, dump = self.run_harness("draw", ABSENT_ID, BATTLE_OFF,
                                        *IDENTITY)
        self.check_reject(blocks["d1"], opens=1, fail=1, dump=dump)
        self.assertEqual(blocks["d1"]["reads"], "0")
        # The open fails before dmaFill, so the layer keeps its zeros.
        self.assertEqual(dump, b"\x00" * VRAM_BYTES, "no clear on open fail")

    def test_short_read_rejects_with_partial_rows(self):
        blocks, dump = self.run_harness("draw", SHORT_ID, BATTLE_OFF,
                                        *IDENTITY)
        self.check_reject(blocks["d1"], opens=1, fail=1, dump=dump)
        self.assertEqual(blocks["d1"]["reads"], "90", "rows read before cut")
        rows = battle_rows(self.short, count=90)
        self.assertEqual(dump, build_vram(rows),
                         "90 loaded rows then the dmaFill clear")

    def test_non_opaque_battle_texel_rejects_at_that_row(self):
        blocks, dump = self.run_harness("draw", NOALPHA_ID, BATTLE_OFF,
                                        *IDENTITY)
        self.check_reject(blocks["d1"], opens=1, fail=1, dump=dump)
        self.assertEqual(blocks["d1"]["reads"], "5", "stops at corrupt row 4")
        rows = battle_rows(self.noalpha, count=4)
        rows.append(list(
            struct.unpack_from(f"<{9}H", self.noalpha, 4 * W * 2)))
        self.assertEqual(dump, build_vram(rows),
                         "rows before the bad texel plus 9 pixels of row 4")

    def test_bad_intensity_rejects_before_writing_that_row(self):
        blocks, dump = self.run_harness("draw", BADINT_ID, RESULTS_OFF,
                                        *IDENTITY, palette=1)
        self.check_reject(blocks["d1"], opens=1, fail=1, dump=dump)
        self.assertEqual(blocks["d1"]["reads"], "6", "stops at corrupt row 5")
        rows = results_rows(self.badint, PALETTE_A, count=5)
        self.assertEqual(dump, build_vram(rows),
                         "5 expanded rows; row 5 never reaches VRAM")

    def test_affine_results_full_screen_first_and_last_texels(self):
        blocks, _ = self.run_harness("draw", RESULTS_ID, RESULTS_OFF,
                                     *IDENTITY, palette=1)
        affine = (int(blocks["d1"]["qpa"]), int(blocks["d1"]["qpd"]),
                  int(blocks["d1"]["qdx"]), int(blocks["d1"]["qdy"]))
        self.assertEqual(sampled_texel(affine, 0, 0), (0, 0),
                         "first sampled texel")
        self.assertEqual(sampled_texel(affine, 255, 191), (239, 175),
                         "last sampled texel covers the full image")

    def test_affine_battle_transforms_first_and_last_texels(self):
        for label, (transform, affine, first, last) in \
                BATTLE_TRANSFORMS.items():
            with self.subTest(case=label):
                blocks, _ = self.run_harness("draw", BATTLE_ID, BATTLE_OFF,
                                             *transform)
                got = (int(blocks["d1"]["qpa"]), int(blocks["d1"]["qpd"]),
                       int(blocks["d1"]["qdx"]), int(blocks["d1"]["qdy"]))
                self.assertEqual(got, battle_affine(*transform),
                                 "queued affine vs oracle")
                self.assertEqual(got, affine, "queued affine vs source law")
                self.assertEqual(sampled_texel(got, 0, 0), first,
                                 "first sampled texel")
                self.assertEqual(sampled_texel(got, 255, 191), last,
                                 "last sampled texel")

    def test_invalid_transforms_reject_before_io(self):
        cases = {
            "zero scale x": (0, 0, 0, 65536),
            "zero scale y": (0, 0, 65536, 0),
            "origin x high": (32768, 0, 65536, 65536),
            "origin x low": (-32769, 0, 65536, 65536),
            "origin y high": (0, 32768, 65536, 65536),
            "pa underflow": (0, 0, 1 << 26, 1 << 26),
            "pa overflow": (0, 0, 400, 400),
            "dx overflow": (-32768, 0, 513, 513),
        }
        for label, transform in cases.items():
            with self.subTest(case=label):
                self.assertIsNone(battle_affine(*transform),
                                  "oracle expects rejection")
                blocks, dump = self.run_harness("draw", BATTLE_ID,
                                                BATTLE_OFF, *transform)
                self.check_reject(blocks["d1"], opens=0, fail=0, dump=dump)
                self.assertEqual(dump, b"\x00" * VRAM_BYTES,
                                 "layer untouched")


if __name__ == "__main__":
    unittest.main()
