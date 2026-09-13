"""Host execution tests for src/nds/nds_native_stage_blob.c.

Compiles the REAL loader TU on host against small shadows
(scripts/stages/native_stage_blob_host/) of its DS-only imports and drives
it with REAL generated blobs: cold load, same-scene reuse, heap-generation
invalidation, FNV/hash, truncation, header/offset bounds, binding-table
initialization, and the caller-lifetime rules the resolver relies on
(re-fetch per use, gkind match, NULL after rewind or failed load).

No ROM build, no emulator. Failures are one short line per case.
"""
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

import generate_nds_native_stage as generator

FIXTURE = Path(__file__).resolve().parent / "native_stage_blob_host"
LOADER = Path(__file__).resolve().parents[2] / "src" / "nds" / "nds_native_stage_blob.c"
INCLUDE = Path(__file__).resolve().parents[2] / "include"

# Header field offsets (generate_nds_native_stage header comment).
_OFF_MAGIC, _OFF_ABI, _OFF_HLEN = 0, 4, 6
_OFF_SLAB, _OFF_BODY, _OFF_FNV = 8, 12, 16
_OFF_COUNTS = 20
_OFF_RIGID, _OFF_CAMERA = 70, 78
_OFF_SEG0, _OFF_GKIND, _OFF_DLMASK = 86, 87, 90
_OFF_OFFSETS = 94
_ABSENT = 0xFFFFFFFF

# Stages under test: one layer packet (no binding tables) and two DLLink
# packets (binding_dobjs/heads resident). gkinds follow blob_gkind().
STAGES = ("yoster", "sector", "bonus1_mario")


def _header_u16(blob, index):
    return struct.unpack_from("<H", blob, _OFF_COUNTS + 2 * index)[0]


def _header_u32(blob, offset):
    return struct.unpack_from("<I", blob, offset)[0]


def _offsets(blob):
    return list(struct.unpack_from("<16I", blob, _OFF_OFFSETS))


class LoaderHarness:
    def __init__(self, directory):
        compiler = shutil.which("gcc") or shutil.which("clang")
        if compiler is None:
            raise unittest.SkipTest("host C compiler required")
        self.binary = Path(directory) / "blob_harness"
        result = subprocess.run(
            [compiler, "-std=c11", "-Wall", "-Wextra", "-O1",
             "-I", str(FIXTURE), "-I", str(INCLUDE),
             "-o", str(self.binary),
             str(FIXTURE / "harness.c"), str(LOADER)],
            capture_output=True, text=True, timeout=120)
        if result.returncode != 0:
            raise AssertionError(
                "harness compile failed: " + (result.stdout + result.stderr)[-2000:])

    def run(self, root, command, gkind):
        result = subprocess.run(
            [str(self.binary), str(root), command, str(gkind)],
            capture_output=True, text=True, timeout=60)
        if result.returncode != 0:
            raise AssertionError(
                f"harness {command}/{gkind} exited {result.returncode}: "
                + result.stderr[-500:])
        blocks = {}
        for line in result.stdout.splitlines():
            key, _, value = line.partition("=")
            prefix, _, field = key.partition(".")
            blocks.setdefault(prefix, {})[field] = value
        return blocks


class StageBlobLoaderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.tmp = tempfile.TemporaryDirectory(prefix="smash64ds-blob-loader-")
        cls.harness = LoaderHarness(cls.tmp.name)
        cls.packets = {name: generator.generate(cls.root, name) for name in STAGES}
        cls.blobs = {name: generator.build_stage_blob(packet, name)
                     for name, packet in cls.packets.items()}
        cls.gkinds = {name: generator.blob_gkind(name) for name in STAGES}

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def stage_root(self, blobs):
        root = Path(tempfile.mkdtemp(dir=self.tmp.name))
        staged = root / "stages"
        staged.mkdir()
        for name, blob in blobs.items():
            (staged / f"native_stage_{name}.bin").write_bytes(blob)
        return root

    def check_packet_block(self, block, name, blob):
        """Every loader-published scalar and fixup against generator truth."""
        header, _ = generator.parse_stage_blob(blob)
        body_len = len(blob) - generator.NDS_STAGE_BLOB_HEADER_LEN
        packet = self.packets[name]
        self.assertEqual(block["result"], "1", f"{name} load failed")
        self.assertEqual(block["null"], "0", f"{name} packet NULL after TRUE")
        self.assertEqual(block["bytes"], str(160 + body_len), f"{name} byte count")
        self.assertEqual(block["active"], str(self.gkinds[name]), f"{name} active kind")
        self.assertEqual(block["packet_gkind"], str(self.gkinds[name]), f"{name} gkind tag")
        # ab3a8f083e4 (2026-09-07): a blob stage never claims a generated
        # segment-0 program, whatever header[86] advertises, because the
        # segment-0 tables are linked symbols (Dream Land only) and a blob
        # stage that claimed one was validated against the wrong certificate.
        self.assertEqual(block["has_seg0"], "0", f"{name} seg0 flag")
        self.assertEqual(block["rigid"], str(header["rigid_mask"]), f"{name} rigid mask")
        self.assertEqual(block["camera"], str(header["camera_mask"]), f"{name} camera mask")
        for index, key in enumerate(generator._BLOB_COUNT_KEYS):
            self.assertEqual(block[f"c{index}"], str(header["counts"][key]),
                             f"{name} count {key}")
        slab_base = int(block["slab"].split(":")[0])
        self.assertEqual(block["slab"].split(":")[1], str(body_len), f"{name} slab size")
        for index in range(16):
            if index == 13:
                # Baked-world table is bounds-checked by the loader but has
                # no published pointer at NDS_TASK51_STAGE_NATIVE=0.
                self.assertEqual(block.get(f"t{index}"), "null",
                                 f"{name} baked table leaked a pointer")
                continue
            offset = _offsets(blob)[index]
            tag = block.get(f"t{index}")
            if offset == _ABSENT:
                self.assertEqual(tag, "null", f"{name} table {index} should be NULL")
                self.assertNotIn(f"peek{index}", block, f"{name} table {index} peeked NULL")
            else:
                self.assertEqual(int(tag) - slab_base, offset, f"{name} table {index} fixup")
                want = min(16, body_len - offset)
                self.assertEqual(block[f"peek{index}"],
                                 blob[160 + offset:160 + offset + want].hex(),
                                 f"{name} table {index} bytes")
        if header["dl_link_owner_mask"]:
            self.assertEqual(block["dobjs"], ",".join(map(str, packet.binding_dobjs)),
                             f"{name} binding dobjs")
            self.assertEqual(block["heads"], ",".join(map(str, packet.binding_heads)),
                             f"{name} binding heads")
            for dobj in packet.binding_dobjs:
                self.assertLess(dobj, len(packet.dobjs), f"{name} dobj bound")
            for head in packet.binding_heads:
                self.assertLessEqual(head, 3, f"{name} head bound")
        else:
            self.assertNotIn("dobjs", block, f"{name} unexpected binding dobjs")
            self.assertNotIn("heads", block, f"{name} unexpected binding heads")

    def check_failed_block(self, block, gkind, bucket):
        self.assertEqual(block["result"], "0", "corrupt blob loaded")
        self.assertEqual(block["null"], "1", "stale packet after failed load")
        self.assertEqual(block["load_count"], "1", "load not counted")
        self.assertEqual(block["active"], str(gkind), "active kind not latched")
        self.assertEqual(block[bucket], "1", f"wrong failure bucket, want {bucket}")
        other = "read" if bucket == "hash" else "hash"
        self.assertEqual(block[other], "0", f"failure double-counted in {other}")

    def mutate(self, blob, **fields):
        data = bytearray(blob)
        for key, value in fields.items():
            if key.startswith("off"):
                struct.pack_into("<I", data, _OFF_OFFSETS + 4 * int(key[3:]), value)
            elif key.startswith("cnt"):
                struct.pack_into("<H", data, _OFF_COUNTS + 2 * int(key[3:]), value)
            elif key.startswith("u32@"):
                struct.pack_into("<I", data, int(key[4:]), value)
            elif key.startswith("u8@"):
                data[int(key[3:])] = value & 0xFF
            else:
                raise AssertionError(f"bad mutation {key}")
        return bytes(data)

    def test_cold_load_matches_generated_blob(self):
        for name in STAGES:
            with self.subTest(stage=name):
                root = self.stage_root({name: self.blobs[name]})
                blocks = self.harness.run(root, "once", self.gkinds[name])
                self.check_packet_block(blocks["a"], name, self.blobs[name])

    def test_same_scene_reload_replaces_slab_in_static_mirror(self):
        name = "sector"
        root = self.stage_root({name: self.blobs[name]})
        blocks = self.harness.run(root, "twice", self.gkinds[name])
        self.check_packet_block(blocks["first"], name, self.blobs[name])
        self.check_packet_block(blocks["second"], name, self.blobs[name])
        # Caller lifetime: one static mirror, re-fetched per load; the reload
        # strands exactly one arena body (mallocs == 2, new slab base).
        self.assertEqual(blocks["first"]["mirror"], blocks["second"]["mirror"],
                         "mirror moved between reloads")
        self.assertNotEqual(blocks["first"]["slab"], blocks["second"]["slab"],
                            "reload reused a stale slab")
        self.assertEqual(blocks["second"]["mallocs"], "2", "reload malloc count")

    def test_generation_rewind_invalidates_and_reload_recovers(self):
        name = "yoster"
        root = self.stage_root({name: self.blobs[name]})
        blocks = self.harness.run(root, "regen", self.gkinds[name])
        self.check_packet_block(blocks["cold"], name, self.blobs[name])
        self.assertNotEqual(blocks["regen"].get("cached_mirror"), "-1",
                            "no packet to rewind")
        self.assertEqual(blocks["regen"]["after_rewind_null"], "1",
                         "rewound slab still live")
        self.check_packet_block(blocks["reloaded"], name, self.blobs[name])
        self.assertEqual(int(blocks["reloaded"]["generation"]),
                         int(blocks["cold"]["generation"]) + 1, "generation not keyed")

    def test_dreamland_stays_linked_without_a_blob(self):
        blocks = self.harness.run(self.stage_root({}), "once", 6)
        self.assertEqual(blocks["a"]["result"], "1", "dreamland no-op failed")
        self.assertEqual(blocks["a"]["null"], "1", "dreamland exposed a blob packet")
        self.assertEqual(blocks["a"]["mallocs"], "0", "dreamland touched the arena")

    def test_bad_kind_and_missing_file_decline_to_read_fail(self):
        name = "yoster"
        root = self.stage_root({name: self.blobs[name]})
        for gkind in (41, 10):
            with self.subTest(gkind=gkind):
                self.check_failed_block(
                    self.harness.run(root, "once", gkind)["a"], gkind, "read")
        with self.subTest(gkind="missing"):
            self.check_failed_block(
                self.harness.run(self.stage_root({}), "once",
                                 self.gkinds[name])["a"], self.gkinds[name], "read")

    def test_flipped_body_byte_fails_hash(self):
        name = "yoster"
        blob = self.blobs[name]
        bad = bytearray(blob)
        bad[generator.NDS_STAGE_BLOB_HEADER_LEN + 7] ^= 0xFF
        root = self.stage_root({name: bytes(bad)})
        self.check_failed_block(
            self.harness.run(root, "once", self.gkinds[name])["a"],
            self.gkinds[name], "hash")

    def test_truncated_blob_fails_read(self):
        name = "sector"
        blob = self.blobs[name]
        for label, cut in (("last-byte", blob[:-1]),
                           ("header-only", blob[:160]),
                           ("short-header", blob[:100])):
            with self.subTest(case=label):
                root = self.stage_root({name: cut})
                self.check_failed_block(
                    self.harness.run(root, "once", self.gkinds[name])["a"],
                    self.gkinds[name], "read")

    def test_bad_magic_abi_and_length_fail_hash(self):
        name = "yoster"
        blob = self.blobs[name]
        cases = {
            "magic": self.mutate(blob, **{"u32@0": 0xDEADBEEF}),
            "abi": self.mutate(blob, **{"u8@4": 0xFF}),
            "header_len": self.mutate(blob, **{"u8@6": 0x9F}),
        }
        for label, bad in cases.items():
            with self.subTest(case=label):
                root = self.stage_root({name: bad})
                self.check_failed_block(
                    self.harness.run(root, "once", self.gkinds[name])["a"],
                    self.gkinds[name], "hash")

    def test_offset_bounds_fail_hash(self):
        name = "sector"
        blob = self.blobs[name]
        body_len = len(blob) - generator.NDS_STAGE_BLOB_HEADER_LEN
        offsets = _offsets(blob)
        cases = {
            # Bindings table pushed to overrun the body.
            "overrun": self.mutate(blob, off3=body_len - 4),
            # Assets table loses its 4-byte alignment.
            "misaligned": self.mutate(blob, off0=offsets[0] + 1),
            # Assets counted present but marked absent.
            "absent-counted": self.mutate(blob, off0=_ABSENT),
            # Asset count grown past the published slab sum.
            "slab-count": self.mutate(blob, cnt0=_header_u16(blob, 0) + 1),
            # Slab byte count padded without changing the tables.
            "slab-bytes": self.mutate(blob, **{f"u32@{_OFF_SLAB}":
                                               _header_u32(blob, _OFF_SLAB) + 16}),
            # Unpublished baked-world table pushed to overrun the body:
            # still range-checked even with no pointer to fill.
            "baked-overrun": self.mutate(blob, off13=body_len - 4),
        }
        for label, bad in cases.items():
            with self.subTest(case=label):
                root = self.stage_root({name: bad})
                self.check_failed_block(
                    self.harness.run(root, "once", self.gkinds[name])["a"],
                    self.gkinds[name], "hash")

    def test_wrong_embedded_kind_and_zeroed_dlmask_fail_hash(self):
        name = "sector"
        blob = self.blobs[name]
        gkind = self.gkinds[name]
        cases = {
            "kind": self.mutate(blob, **{f"u8@{_OFF_GKIND}": (gkind + 1) % 41}),
            "dlmask": self.mutate(blob, **{f"u32@{_OFF_DLMASK}": 0}),
        }
        for label, bad in cases.items():
            with self.subTest(case=label):
                root = self.stage_root({name: bad})
                self.check_failed_block(
                    self.harness.run(root, "once", gkind)["a"], gkind, "hash")


if __name__ == "__main__":
    unittest.main()
