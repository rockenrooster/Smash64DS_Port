"""Relocatable NitroFS blob roundtrips and host C layout checks."""
import re
import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

import check_nds_native_stage as check
import generate_nds_native_stage as generator
from native_stage_descriptors import get_descriptor


class StageBlobTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[2]
        cls.packets = {name: generator.generate(cls.root, name)
                       for name in ("dreamland", "yoster", "bonus1_mario")}

    def test_emitted_c_layout_matches_blob_binding_format(self):
        compiler = shutil.which("gcc") or shutil.which("clang")
        if compiler is None:
            self.skipTest("host C compiler required")
        text = generator.render_include(self.packets["dreamland"], "dreamland").decode()
        declarations = re.findall(r"typedef struct NDSNativeStage\w+ \{.*?\} NDSNativeStage\w+;",
                                  text, re.S)
        checks = re.findall(r"_Static_assert\(sizeof\(NDSNativeStage\w+\)[^;]*;", text)
        self.assertTrue(declarations and checks)
        size = struct.calcsize(generator.BINDING_ROW_FORMAT)
        source = ("#include <stdint.h>\n#include <stddef.h>\n"
                  "typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;\n"
                  "typedef int16_t s16; typedef int32_t s32;\n" +
                  "\n".join(declarations + checks) +
                  f'\n_Static_assert(sizeof(NDSNativeStageBinding) == {size}, "blob stride");\n'
                  '_Static_assert(offsetof(NDSNativeStageBinding, source_vertex_count) == 16, "vertex count");\n'
                  '_Static_assert(offsetof(NDSNativeStageBinding, triangle_count) == 18, "triangle count");\n'
                  '_Static_assert(offsetof(NDSNativeStageBinding, binding_pad) == 26, "tail padding");\n')
        with tempfile.TemporaryDirectory(prefix="smash64ds-stage-abi-") as directory:
            unit = Path(directory) / "stage_abi.c"
            unit.write_text(source)
            result = subprocess.run([compiler, "-std=c11", "-fsyntax-only", str(unit)],
                                    capture_output=True, text=True, timeout=60)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_header_layout_is_160_bytes(self):
        packet = self.packets["yoster"]
        blob = generator.build_stage_blob(packet, "yoster")
        self.assertEqual(len(blob[:generator.NDS_STAGE_BLOB_HEADER_LEN]),
                         generator.NDS_STAGE_BLOB_HEADER_LEN)
        magic, abi, header_len = struct.unpack_from("<4sHH", blob, 0)
        self.assertEqual(magic, generator.NDS_STAGE_BLOB_MAGIC)
        self.assertEqual(abi, generator.NDS_STAGE_BLOB_ABI)
        self.assertEqual(header_len, generator.NDS_STAGE_BLOB_HEADER_LEN)
        header, _parsed = generator.parse_stage_blob(blob)
        offsets = [o for o in (
            struct.unpack_from("<16I", blob, 94)) if o != 0xFFFFFFFF]
        self.assertEqual(offsets, sorted(offsets))
        for offset in offsets:
            self.assertEqual(offset % 4, 0)
        self.assertEqual(header["slab_bytes"], packet.slab_bytes())
        self.assertEqual(header["fnv"],
                         generator.fnv1a_bytes(blob[header_len:]))

    def test_roundtrip_every_table(self):
        for name, packet in self.packets.items():
            with self.subTest(stage=name):
                desc = get_descriptor(name)
                size, fnv = check.verify_blob_roundtrip(packet, desc)
                blob = generator.build_stage_blob(packet, desc)
                self.assertEqual(size, len(blob))
                header, _parsed = generator.parse_stage_blob(blob)
                self.assertEqual(fnv, header["fnv"])

    def test_tampered_body_fails_closed(self):
        packet = self.packets["dreamland"]
        blob = bytearray(generator.build_stage_blob(packet, "dreamland"))
        blob[generator.NDS_STAGE_BLOB_HEADER_LEN] ^= 1
        with self.assertRaises(generator.Falsifier):
            generator.parse_stage_blob(bytes(blob))

    def test_dreamland_include_sha_unchanged(self):
        desc = get_descriptor("dreamland")
        packet = self.packets["dreamland"]
        rendered = generator.render_include(packet, desc)
        output = self.root / generator.default_output_for_stage("dreamland")
        self.assertTrue(output.is_file())
        self.assertEqual(output.read_bytes(), rendered)
        self.assertEqual(generator.sha256(rendered), desc.include_sha)
        self.assertEqual(desc.include_sha, generator.EXPECTED_INCLUDE_SHA256)

    def test_blob_gkind_mirrors_emitter(self):
        import emit_native_stage_runtime_rows as emitter

        for name in ("dreamland", "yoster", "bonus1_mario"):
            with self.subTest(stage=name):
                self.assertEqual(generator.blob_gkind(name),
                                 emitter.GKIND[name])


if __name__ == "__main__":
    unittest.main()
