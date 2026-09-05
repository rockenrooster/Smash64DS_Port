"""Relocatable NitroFS blob roundtrips; no ROM or compiler needed."""
import struct
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
