"""Mixed-width/source-domain negatives; no generated assets or ROM builds."""
from __future__ import annotations

import copy
import json
from pathlib import Path
import struct
import tempfile
import unittest

import generate_meta_attribute_metadata as meta
from extra_native_asset_adapter import encode_o2r


class MixedMetadataTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = Path("C:/devkitPro/devkitARM/bin/arm-none-eabi-gcc.exe")
        if not compiler.is_file():
            raise unittest.SkipTest("devkitARM layout oracle unavailable")
        cls.layout, cls.oracle = meta.compiler_layout(
            compiler, meta.ROOT / "decomp/BattleShip-main/decomp", Path("C:/devkitPro/libnds/include"))

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="meta-mixed-metadata-")
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def test_oracle_consumes_project_headers_and_target_scalar_widths(self):
        self.assertEqual(self.layout["POINTER_SIZE"], 4)
        self.assertEqual(self.layout["U16_SIZE"], 2)
        headers = {row["path"] for row in self.oracle["headers"]}
        self.assertIn(str(meta.ROOT / "include/ft/fighter.h"), headers)
        self.assertIn(str(meta.ROOT / "include/PR/sp.h"), headers)

    def test_layout_parser_rejects_host_pointer_width_and_missing_words(self):
        with self.assertRaisesRegex(meta.MetadataError, "not the ARM32"):
            meta.parse_layout_assembly("nds_meta_layout:\n.word 8\n.word 2\n", ["POINTER_SIZE", "U16_SIZE"])
        with self.assertRaisesRegex(meta.MetadataError, "layout count"):
            meta.parse_layout_assembly("nds_meta_layout:\n.word 4\n", ["POINTER_SIZE", "U16_SIZE"])

    def attribute_payload(self):
        payload = bytearray(self.layout["FTATTR_SIZE"] + 4)
        for index, (name, _) in enumerate(meta.ATTR_FIELDS):
            struct.pack_into(">H", payload, self.layout["ATTR_" + name + "_OFFSET"], 0x1200 + index)
        return payload

    def test_mixed_values_follow_source_big_endian_lanes(self):
        payload = self.attribute_payload()
        fields = meta.attribute_fields(payload, 0, self.layout)
        self.assertEqual([row["expected_native_value"] for row in fields], list(range(0x1200, 0x120A)))
        # A word-swap changes the halfword order; expected values must remain
        # source values so runtime lane-restore validation detects omission.
        swapped = b"".join(payload[i:i + 4][::-1] for i in range(0, len(payload), 4))
        bad = meta.attribute_fields(swapped, 0, self.layout)
        self.assertNotEqual(fields[0]["expected_native_value"], bad[0]["expected_native_value"])

    def test_bad_attribute_span_and_changed_field_width_are_rejected(self):
        payload = self.attribute_payload()
        for at, data in ((1, payload), (0, payload[:-5])):
            with self.subTest(at=at), self.assertRaises(meta.MetadataError):
                meta.attribute_fields(data, at, self.layout)
        wrong = dict(self.layout)
        wrong["ATTR_DEAD_FGM_0_SIZE"] = 4
        with self.assertRaisesRegex(meta.MetadataError, "unsupported mixed field layout"):
            meta.attribute_fields(payload, 0, wrong)

    def sprite_resources(self, count=1, siz=0, missing_pixels=False):
        layout = self.layout
        sprite_at = 0
        bitmap_at = (layout["SPRITE_SIZE"] + 3) & ~3
        pixels_at = bitmap_at + layout["BITMAP_SIZE"]
        payload = bytearray(pixels_at + 128)
        for member, value in (("width", 16), ("height", 16), ("nbitmaps", count), ("ndisplist", 36)):
            struct.pack_into(">h", payload, layout["SPRITE_" + member.upper() + "_OFFSET"], value)
        payload[layout["SPRITE_BMFMT_OFFSET"]] = 2
        payload[layout["SPRITE_BMSIZ_OFFSET"]] = siz
        for member, value in (("width", 16), ("width_img", 16), ("actualHeight", 16)):
            struct.pack_into(">h", payload, bitmap_at + layout["BITMAP_" + member.upper() + "_OFFSET"], value)
        intern = {layout["SPRITE_BITMAP_OFFSET"]: bitmap_at}
        if not missing_pixels:
            intern[bitmap_at + layout["BITMAP_BUF_OFFSET"]] = pixels_at
        container = encode_o2r(1234, bytes(payload), intern, {})
        path = self.root / "SpriteFixture"
        path.write_bytes(container)
        bindings = {"assets": [{"native_file_id": 1234, "path": path.name,
                                "size": len(payload), "container_sha256": meta.sha(container)}]}
        resources = meta.VerifiedResources(bindings, self.root)
        return resources, resources.get(1234), sprite_at

    def test_sprite_proves_bitmap_and_pixel_owner_spans(self):
        resources, owner, at = self.sprite_resources()
        row = meta.sprite_metadata(resources, owner, at, self.layout, "STOCK")
        self.assertEqual(row["bitmap_asset_id"], 1234)
        self.assertEqual(row["fields"]["nbitmaps"], 1)
        self.assertEqual(row["bitmaps"][0]["pixels_bytes"], 128)

    def test_invalid_sprite_count_format_and_missing_pixel_pointer_fail(self):
        for args in ({"count": 0}, {"siz": 4}, {"missing_pixels": True}):
            with self.subTest(args=args), self.assertRaises(meta.MetadataError):
                resources, owner, at = self.sprite_resources(**args)
                meta.sprite_metadata(resources, owner, at, self.layout, "STOCK")

    def test_mutated_container_and_unknown_resource_identity_fail(self):
        resources, _, _ = self.sprite_resources()
        fresh = meta.VerifiedResources({"assets": list(resources.rows.values())}, self.root)
        path = self.root / "SpriteFixture"
        path.write_bytes(path.read_bytes()[:-1] + b"!")
        with self.assertRaisesRegex(meta.MetadataError, "hash changed"):
            fresh.get(1234)
        with self.assertRaisesRegex(meta.MetadataError, "missing required native resource"):
            fresh.get(9999)

    def test_wrong_pin_and_wrong_donor_output_identity_fail(self):
        lock = json.loads((meta.ROOT / "docs/P4/source-lock.json").read_text())
        pins = {row["path"]: row["commit"] for row in lock["submodules"]}
        donor = {"phase": "resolved", "character": "MetaKnight", "sources": {
            "extra": {"commit": pins["decomp/smashremix-plus-extra"], "clean": True},
            "remix": {"commit": pins["decomp/smashremix"], "clean": True}},
            "output_roles": {"review_tables": "review", "assets": "assets"},
            "outputs": [{"path": "review", "sha256": "a" * 64}, {"path": "assets", "sha256": "b" * 64}]}
        bindings = {"schema": "smash64ds.p4-native-runtime-bindings.v1", "character": "MetaKnight",
                    "source_rom_sha256": "a" * 64, "source_files_rom_sha256": "b" * 64}
        meta.validate_provenance(bindings, donor, lock)
        wrong = copy.deepcopy(donor)
        wrong["sources"]["extra"]["commit"] = "0" * 40
        with self.assertRaisesRegex(meta.MetadataError, "wrong or unclean"):
            meta.validate_provenance(bindings, wrong, lock)
        wrong = dict(bindings, source_rom_sha256="c" * 64)
        with self.assertRaisesRegex(meta.MetadataError, "frozen donor identity differs"):
            meta.validate_provenance(wrong, donor, lock)


if __name__ == "__main__":
    unittest.main()
