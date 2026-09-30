#!/usr/bin/env python3
"""Host validation for raw EXTRA relocation decoding and Meta Knight inventory.

Run the pinned donor checks with --source-dir PATH or META_KNIGHT_SOURCE_DIR.
The synthetic checks never require donor assets or a ROM build.
"""

from __future__ import annotations

import argparse
from contextlib import contextmanager
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest

from extra_resource_adapter import (
    AdapterError,
    build_meta_knight_inventory,
    decode_resource,
    parse_requests,
)


END = 0xFFFF * 4
SOURCE_DIR: Path | None = None


def payload(*words: int) -> bytes:
    return struct.pack(">" + "I" * len(words), *words)


def reloc(next_slot: int, target_offset: int) -> int:
    """Encode a source relocation descriptor using byte offsets for callers."""
    if next_slot % 4 or target_offset % 4:
        raise ValueError("synthetic relocation offsets must be word aligned")
    return ((next_slot // 4) << 16) | (target_offset // 4)


class RequestParsingTests(unittest.TestCase):
    def test_empty_request_list(self) -> None:
        self.assertEqual(parse_requests("\n\r\n"), ())

    def test_dependency_identity_and_order_survive_decoding(self) -> None:
        requests = parse_requests("${CHARACTER}\r\n\n00E8 Jigglypuff Move Set\n")
        raw = decode_resource(
            "main",
            payload(reloc(8, 0), 0, reloc(END, 4)),
            END,
            0,
            requests,
        )
        self.assertEqual(
            raw.inventory()["external"],
            [
                {"slot": 0, "offset": 0,
                 "dependency": {"kind": "symbol", "name": "CHARACTER"}},
                {"slot": 8, "offset": 4,
                 "dependency": {"kind": "donor_file", "file_id": 0xE8,
                                "label": "Jigglypuff Move Set"}},
            ],
        )

    def test_repeated_dependency_rows_are_not_deduplicated(self) -> None:
        requests = parse_requests("${CHARACTER}\n${CHARACTER}\n")
        raw = decode_resource(
            "main", payload(reloc(4, 0), reloc(END, 4)), END, 0, requests,
        )
        self.assertEqual(len(raw.inventory()["external"]), 2)

    def test_malformed_requests_fail(self) -> None:
        for text in (
            "CHARACTER", "${CHARACTER", "${}",
            "00GG Jigglypuff Move Set", "10000 Overflow", "00E8${CHARACTER}",
            "END OF REQUEST LIST\n${CHARACTER}",
        ):
            with self.subTest(text=text), self.assertRaises(AdapterError):
                parse_requests(text)

    def test_compiled_request_label_is_optional(self) -> None:
        raw = decode_resource(
            "main", payload(reloc(END, 0)), END, 0, parse_requests("00E8"),
        )
        self.assertEqual(
            raw.inventory()["external"][0]["dependency"],
            {"kind": "donor_file", "file_id": 0xE8, "label": ""},
        )


class RelocationDecoderTests(unittest.TestCase):
    def test_empty_chains_preserve_source_identity(self) -> None:
        source = payload(0, 0x12345678)
        raw = decode_resource("empty", source, END, END)
        inventory = raw.inventory()
        self.assertEqual(inventory["name"], "empty")
        self.assertEqual(inventory["bytes"], len(source))
        self.assertEqual(inventory["sha256"], hashlib.sha256(source).hexdigest())
        self.assertEqual(inventory["internal_head"], END)
        self.assertEqual(inventory["external_head"], END)
        self.assertEqual(inventory["internal"], [])
        self.assertEqual(inventory["external"], [])
        self.assertIsNone(raw.pointer(0))

    def test_internal_chain_uses_link_order_and_word_scaled_targets(self) -> None:
        source = payload(reloc(8, 12), 0, reloc(END, 0), 0)
        raw = decode_resource("character", source, 0, END)
        self.assertEqual(
            raw.inventory()["internal"],
            [{"slot": 0, "offset": 12, "resource": "character"},
             {"slot": 8, "offset": 0, "resource": "character"}],
        )
        self.assertEqual(raw.pointer(0).offset, 12)
        self.assertEqual(raw.pointer(8).offset, 0)
        self.assertIsNone(raw.pointer(4))
        self.assertEqual(raw.inventory()["sha256"], hashlib.sha256(source).hexdigest())

    def test_external_target_zero_is_a_live_pointer(self) -> None:
        raw = decode_resource(
            "main", payload(reloc(END, 0)), END, 0,
            parse_requests("${CHARACTER}"),
        )
        self.assertIsNotNone(raw.pointer(0))
        self.assertEqual(raw.pointer(0).offset, 0)
        self.assertEqual(raw.inventory()["external"][0]["offset"], 0)

    def test_processed_local_dependency_preserves_identity_and_zero_target(self) -> None:
        raw = decode_resource(
            "MAIN", payload(reloc(END, 0)), END, 0,
            parse_requests("1550 Meta Knight model"),
            local_dependency_ids={5456: "CHARACTER"},
        )
        self.assertIsNotNone(raw.pointer(0))
        self.assertEqual(raw.pointer(0).offset, 0)
        self.assertEqual(raw.inventory()["external"], [{
            "slot": 0, "offset": 0, "resource": "CHARACTER",
            "dependency": {"kind": "donor_file", "file_id": 5456, "label": "Meta Knight model"},
        }])

    def test_local_dependency_ids_require_explicit_u16_integer_keys(self) -> None:
        for mapping in ({True: "CHARACTER"}, {-1: "CHARACTER"}, {65535: "CHARACTER"},
                        {"5456": "CHARACTER"}, {5456: False}):
            with self.subTest(mapping=mapping), self.assertRaises(AdapterError):
                decode_resource(
                    "MAIN", payload(reloc(END, 0)), END, 0,
                    parse_requests("1550 Meta Knight model"), local_dependency_ids=mapping,
                )

    def test_external_target_bounds_belong_to_dependency(self) -> None:
        raw = decode_resource(
            "main", payload(reloc(END, 0x200)), END, 0,
            parse_requests("00E8 Jigglypuff Move Set"),
        )
        self.assertEqual(raw.pointer(0).offset, 0x200)

    def test_external_dependency_order_follows_links_not_sorted_slots(self) -> None:
        raw = decode_resource(
            "main", payload(reloc(END, 12), 0, reloc(0, 0), 0), END, 8,
            parse_requests("${CHARACTER}\n00E8 Jigglypuff Move Set"),
        )
        by_slot = {row["slot"]: row for row in raw.inventory()["external"]}
        self.assertEqual(by_slot[8]["dependency"], {"kind": "symbol", "name": "CHARACTER"})
        self.assertEqual(by_slot[0]["dependency"]["file_id"], 0xE8)

    def test_nonzero_unrelocated_word_is_not_a_pointer(self) -> None:
        raw = decode_resource("raw", payload(0x12345678), END, END)
        with self.assertRaises(AdapterError):
            raw.pointer(0)

    def test_unaligned_or_out_of_bounds_heads_fail(self) -> None:
        for head in (-4, 1, 2, 3, 16, END + 4):
            for chain in ("internal", "external"):
                with self.subTest(head=head, chain=chain), self.assertRaises(AdapterError):
                    decode_resource(
                        "bad", payload(0, 0, 0, 0),
                        head if chain == "internal" else END,
                        head if chain == "external" else END,
                    )

    def test_truncated_descriptor_fails(self) -> None:
        with self.assertRaises(AdapterError):
            decode_resource("truncated", b"\xff\xff\x00", 0, END)

    def test_empty_resource_cannot_have_a_live_head(self) -> None:
        with self.assertRaises(AdapterError):
            decode_resource("empty", b"", 0, END)

    def test_internal_target_at_end_or_beyond_fails(self) -> None:
        for target in (4, 8, 0xFFFF * 4):
            with self.subTest(target=target), self.assertRaises(AdapterError):
                decode_resource("bad", payload(reloc(END, target)), 0, END)

    def test_next_slot_outside_source_fails(self) -> None:
        with self.assertRaises(AdapterError):
            decode_resource("bad", payload(reloc(8, 0), 0), 0, END)

    def test_internal_cycles_fail(self) -> None:
        for source in (payload(reloc(0, 0)), payload(reloc(4, 0), reloc(0, 0))):
            with self.subTest(source=source.hex()), self.assertRaises(AdapterError):
                decode_resource("cycle", source, 0, END)

    def test_external_cycles_fail(self) -> None:
        with self.assertRaises(AdapterError):
            decode_resource(
                "cycle", payload(reloc(0, 0)), END, 0,
                parse_requests("${CHARACTER}\n${CHARACTER}"),
            )

    def test_chains_cannot_share_a_slot(self) -> None:
        with self.assertRaises(AdapterError):
            decode_resource(
                "overlap", payload(reloc(END, 0)), 0, 0,
                parse_requests("${CHARACTER}"),
            )

    def test_missing_request_rows_fail(self) -> None:
        for requests in ((), parse_requests("${CHARACTER}")):
            with self.subTest(request_count=len(requests)), self.assertRaises(AdapterError):
                decode_resource(
                    "missing", payload(reloc(4, 0), reloc(END, 0)),
                    END, 0, requests,
                )

    def test_excess_request_rows_fail(self) -> None:
        for external_head, source in ((END, payload(0)), (0, payload(reloc(END, 0)))):
            with self.subTest(external_head=external_head), self.assertRaises(AdapterError):
                decode_resource(
                    "excess", source, END, external_head,
                    parse_requests("${CHARACTER}\n00E8 Jigglypuff Move Set"),
                )


class NativeResourceBridgeTests(unittest.TestCase):
    @staticmethod
    def fake_native_module():
        # Capture the native input boundary without importing a build producer.
        return SimpleNamespace(
            PointerRef=lambda asset_id, offset: SimpleNamespace(asset_id=asset_id, offset=offset),
            InputSpec=lambda *args: args,
            O2RResource=lambda spec, source, data, file_id, internal, external: SimpleNamespace(
                spec=spec, source=source, data=data, file_id=file_id,
                internal=internal, external=external,
            ),
        )

    def test_explicit_ids_preserve_payload_and_zero_target(self) -> None:
        requests = parse_requests("${CHARACTER}")
        source = payload(reloc(END, 0), reloc(END, 0))
        raw = decode_resource("main", source, 4, 0, requests)
        native = raw.as_native_resource(100, {requests[0]: 200}, self.fake_native_module())
        self.assertEqual(native.file_id, 100)
        self.assertEqual(native.source, source)
        self.assertEqual(native.data, source)
        self.assertEqual((native.internal[4].asset_id, native.internal[4].offset), (100, 0))
        self.assertEqual((native.external[0].asset_id, native.external[0].offset), (200, 0))

    def test_unresolved_dependencies_fail(self) -> None:
        raw = decode_resource(
            "main", payload(reloc(END, 0)), END, 0, parse_requests("${CHARACTER}"),
        )
        with self.assertRaises(AdapterError):
            raw.as_native_resource(100, {}, self.fake_native_module())

    def test_invalid_resource_ids_fail(self) -> None:
        raw = decode_resource("main", payload(0), END, END)
        for asset_id in (-1, 0xFFFF, 0x10000, "100", 1.5, True, False):
            with self.subTest(asset_id=asset_id), self.assertRaises(AdapterError):
                raw.as_native_resource(asset_id, {}, self.fake_native_module())

    def test_invalid_dependency_ids_fail(self) -> None:
        requests = parse_requests("${CHARACTER}")
        raw = decode_resource("main", payload(reloc(END, 0)), END, 0, requests)
        for asset_id in (-1, 0xFFFF, 0x10000, "100", 1.5, True, False):
            with self.subTest(asset_id=asset_id), self.assertRaises(AdapterError):
                raw.as_native_resource(100, {requests[0]: asset_id}, self.fake_native_module())

class PinnedMetaKnightTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        configured = SOURCE_DIR
        if configured is None and os.environ.get("META_KNIGHT_SOURCE_DIR"):
            configured = Path(os.environ["META_KNIGHT_SOURCE_DIR"])
        if configured is None:
            raise unittest.SkipTest("set META_KNIGHT_SOURCE_DIR or --source-dir for pinned donor checks")
        cls.source_dir = configured.resolve()
        cls.inventory = build_meta_knight_inventory(cls.source_dir)

    def resource(self, name: str) -> dict:
        matches = [row for row in self.inventory["resources"] if row["name"] == name.upper()]
        self.assertEqual(len(matches), 1, f"expected exactly one {name} resource")
        return matches[0]

    @contextmanager
    def processed_request_fixture(self):
        with tempfile.TemporaryDirectory(prefix="meta_knight_processed_requests_test_") as directory:
            clone = Path(directory)
            names = ["main.bin", "character.bin", "config.yaml", "main_reqlist.txt"]
            if (self.source_dir / "character_reqlist.txt").exists():
                names.append("character_reqlist.txt")
            for name in names:
                shutil.copyfile(self.source_dir / name, clone / name)
            requests_path = clone / "main_reqlist.txt"
            original = requests_path.read_text(encoding="utf-8")
            self.assertIn("${CHARACTER}", original)
            requests_path.write_text(original.replace("${CHARACTER}", "1550 Meta Knight model"),
                                     encoding="utf-8")
            yield clone

    @staticmethod
    def model_address_semantics(value):
        """Keep typed model values while separating resolved donor provenance."""
        if isinstance(value, list):
            return [PinnedMetaKnightTests.model_address_semantics(item) for item in value]
        if isinstance(value, dict):
            return {
                key: PinnedMetaKnightTests.model_address_semantics(item)
                for key, item in value.items()
                if not (key == "dependency" and value.get("resource") in ("MAIN", "CHARACTER"))
            }
        return value

    def test_processed_requests_preserve_attributes_trees_and_stock_addresses(self) -> None:
        with self.processed_request_fixture() as clone:
            processed = build_meta_knight_inventory(
                clone, source_resource_ids={"MAIN": 5455, "CHARACTER": 5456},
            )
            self.assertEqual(processed["source_resource_ids"], {"MAIN": 5455, "CHARACTER": 5456})
            self.assertEqual(self.model_address_semantics(processed["model"]),
                             self.model_address_semantics(self.inventory["model"]))
            self.assertEqual(processed["source_inputs"]["config.yaml"],
                             self.inventory["source_inputs"]["config.yaml"])
            self.assertNotEqual(processed["source_inputs"]["main_reqlist.txt"],
                                self.inventory["source_inputs"]["main_reqlist.txt"])
            for actual, original in zip(processed["resources"], self.inventory["resources"]):
                self.assertEqual(actual["sha256"], original["sha256"])
                self.assertEqual(actual["bytes"], original["bytes"])
                self.assertEqual(len(actual["internal"]), len(original["internal"]))
                self.assertEqual(len(actual["external"]), len(original["external"]))
            resolved = [row for row in processed["resources"][0]["external"]
                        if row.get("resource") == "CHARACTER"]
            self.assertTrue(resolved)
            self.assertTrue(all(row["dependency"]["file_id"] == 5456 for row in resolved))

    def test_processed_requests_without_explicit_ids_fail_unresolved_model(self) -> None:
        with self.processed_request_fixture() as clone:
            with self.assertRaisesRegex(AdapterError, "unresolved"):
                build_meta_knight_inventory(clone)

    def test_source_resource_ids_reject_duplicates_booleans_and_invalid_domains(self) -> None:
        mappings = (
            {"MAIN": 5455, "CHARACTER": 5455},
            {"MAIN": True, "CHARACTER": 5456},
            {"MAIN": 5455, "CHARACTER": False},
            {"MAIN": 5455}, {"CHARACTER": 5456},
            {"MAIN": -1, "CHARACTER": 5456},
            {"MAIN": 5455, "CHARACTER": 65535},
            {"MAIN": 5455, "CHARACTER": "5456"},
        )
        for mapping in mappings:
            with self.subTest(mapping=mapping), self.assertRaises(AdapterError):
                build_meta_knight_inventory(self.source_dir, source_resource_ids=mapping)

    def test_raw_resource_identity_and_relocation_counts(self) -> None:
        for name, byte_count, internal_count, external_count, digest in (
            ("main", 2416, 15, 62,
             "9b51e7065e9b44a4a276ab96f18c69ba794ea1ca0e52adcaf9f5cae1422c4f07"),
            ("character", 75296, 582, 0,
             "59cefdb861812208253763953d6bdbff25590ddf14b381b14b933063733eaf86"),
        ):
            with self.subTest(resource=name):
                row = self.resource(name)
                self.assertEqual(row["bytes"], byte_count)
                self.assertEqual(len(row["internal"]), internal_count)
                self.assertEqual(len(row["external"]), external_count)
                self.assertEqual(row["sha256"], digest)
                self.assertEqual(
                    row["sha256"],
                    hashlib.sha256((self.source_dir / f"{name}.bin").read_bytes()).hexdigest(),
                )

    def test_model_preserves_distinct_setup_and_canonical_spaces(self) -> None:
        model = self.inventory["model"]
        self.assertEqual(model["attributes_offset"], 0x624)
        self.assertEqual(model["setup_parts"], [0xEF7CFFC0, 0])
        self.assertEqual(len(model["selected_descriptor_indices"]), 22)
        self.assertEqual(model["canonical_live_nodes"], 23)
        self.assertTrue(model["conversion_gap"])

    def test_both_detail_trees_include_sentinel_and_geometry(self) -> None:
        for detail, offset in (("high", 0x3CB0), ("low", 0x8438)):
            with self.subTest(detail=detail):
                tree = self.inventory["model"]["details"][detail]
                self.assertEqual(tree["tree_offset"], offset)
                self.assertEqual(len(tree["descriptors"]), 32)
                self.assertEqual(len(tree["canonical_roots"]), 13)
                self.assertEqual(tree["direct_triangles"], 291)
                self.assertEqual(
                    {row["joint_id"] for row in tree["hidden_roots"]},
                    {30, 31, 32, 33},
                )

    def test_source_modelpart_alternatives_preserve_joint_domains(self) -> None:
        modelparts = self.inventory["model"]["modelparts"]
        self.assertEqual(len(modelparts["records"]), 10)
        for detail in ("high", "low"):
            with self.subTest(detail=detail):
                roots = modelparts["roots"][detail]
                self.assertEqual(len(roots), 5)
                self.assertEqual({row["joint_id"] for row in roots}, {12, 34})

    def test_electric_skeletons_preserve_source_selectors_rows_roots_and_nofog(self) -> None:
        skeletons = self.inventory["model"]["skeletons"]
        self.assertEqual(skeletons["selector_resource"], "MAIN")
        self.assertEqual(skeletons["selector_offset"], 0x618)
        self.assertEqual(skeletons["gate_joint"], 10)
        self.assertEqual(set(skeletons["variants"]), {"1", "2"})
        targets = {
            "1": [46096, 47152, 47408, 47768, 48040, 48416, 48712],
            "2": [50032, 50896, 51120, 51624, 52040, 52368, 52648],
        }
        for sid, table in (("1", 0x428), ("2", 0x520)):
            with self.subTest(variant=sid):
                variant = skeletons["variants"][sid]
                self.assertEqual(variant["table_resource"], "MAIN")
                self.assertEqual(variant["table_offset"], table)
                self.assertEqual(len(variant["rows"]), 31)
                self.assertEqual([row["source_joint"] for row in variant["rows"]], list(range(4, 35)))
                self.assertEqual([row["joint_id"] for row in variant["roots"]], [6, 10, 11, 14, 15, 23, 28])
                self.assertEqual([row["offset"] for row in variant["roots"]], targets[sid])
                self.assertTrue(all(row["resource"] == "CHARACTER" for row in variant["roots"]))
                flags = [row["flags"] for row in variant["rows"]]
                self.assertEqual(flags[2], 0x40 if sid == "2" else 0)
                self.assertTrue(all(flag == 0 for index, flag in enumerate(flags) if index != 2))

    def test_skeleton_material_callbacks_preserve_live_segment_e_inheritance(self) -> None:
        for sid, variant in self.inventory["model"]["skeletons"]["variants"].items():
            for detail in ("high", "low"):
                with self.subTest(skeleton=sid, detail=detail):
                    inherited = []
                    required = []
                    for root in variant["roots"]:
                        slots = sorted({row["material_slot"] for row in root["branches"]
                                        if "material_slot" in row})
                        self.assertEqual(root["required_material_slots"], slots)
                        binding = root["material_binding_by_detail"][detail]
                        required.append(binding["required_count"])
                        inherited.append(int(binding["kind"] == "inherited"))
                        self.assertEqual(binding["required_count"], max(slots, default=-1) + 1)
                        self.assertIn("gcDrawMObjForDObj", binding["provenance"])
                        self.assertIn("segment-E", binding["provenance"])
                        materials = root["materials_by_detail"][detail]
                        if binding["kind"] == "inherited":
                            self.assertIn(root["joint_id"], (14, 23, 28))
                            self.assertEqual(materials, [])
                            self.assertIsNone(binding["source_binder_joint"])
                        elif materials:
                            self.assertEqual(binding["kind"], "own")
                            self.assertEqual(binding["source_binder_joint"], root["joint_id"])
                        else:
                            self.assertEqual(binding["kind"], "none")
                            self.assertIsNone(binding["source_binder_joint"])
                    self.assertEqual(inherited, [0] * 7 if sid == "1" else [0, 0, 0, 1, 0, 1, 1])
                    self.assertEqual(required, [0] * 7 if sid == "1" else [0, 1, 1, 1, 1, 1, 1])

    def test_stock_sprite_retains_source_pixels_and_costume_palettes(self) -> None:
        stock = self.inventory["model"]["stock"]
        self.assertEqual(stock["offset"], 0x41C)
        sprite = stock["stock_sprite"]
        self.assertEqual(sprite["offset"], 0x123F8)
        self.assertEqual((sprite["width"], sprite["height"]), (8, 10))
        self.assertEqual(len(sprite["bitmaps"]), 1)
        bitmap = sprite["bitmaps"][0]
        self.assertEqual(bitmap["offset"], 0x123E8)
        self.assertEqual(bitmap["pixels"], {"resource": "CHARACTER", "offset": 0x122A8, "bytes": 80})
        self.assertEqual(
            [row["offset"] for row in stock["stock_luts"]["palettes"]],
            [0x122F8, 0x12320, 0x12348, 0x12370, 0x12398, 0x123C0],
        )
        self.assertEqual(stock["emblem"], {"resource": "CHARACTER", "offset": 0x125D8})

    def test_independent_inventory_runs_serialize_identically(self) -> None:
        repeated = build_meta_knight_inventory(self.source_dir)
        self.assertEqual(
            json.dumps(self.inventory, sort_keys=True, allow_nan=False),
            json.dumps(repeated, sort_keys=True, allow_nan=False),
        )

    def assert_corrupt_input_fails(self, filename: str, mutate) -> None:
        with tempfile.TemporaryDirectory(prefix="meta_knight_adapter_test_") as directory:
            clone = Path(directory)
            names = ["main.bin", "character.bin", "config.yaml", "main_reqlist.txt"]
            if (self.source_dir / "character_reqlist.txt").exists():
                names.append("character_reqlist.txt")
            for name in names:
                shutil.copyfile(self.source_dir / name, clone / name)
            target = clone / filename
            changed = bytearray(target.read_bytes())
            mutate(changed)
            target.write_bytes(changed)
            with self.assertRaises(AdapterError):
                build_meta_knight_inventory(clone)

    def test_unsupported_tree_depth_fails(self) -> None:
        self.assert_corrupt_input_fails(
            "character.bin", lambda data: struct.pack_into(">I", data, 0x3CB0, 18),
        )

    def test_missing_tree_sentinel_fails(self) -> None:
        self.assert_corrupt_input_fails(
            "character.bin", lambda data: struct.pack_into(">I", data, 0x3CB0 + 31 * 44, 17),
        )

    def test_nonfinite_tree_transform_fails(self) -> None:
        self.assert_corrupt_input_fails(
            "character.bin", lambda data: struct.pack_into(">f", data, 0x3CB0 + 8, float("nan")),
        )

    def test_external_character_target_outside_payload_fails(self) -> None:
        self.assert_corrupt_input_fails(
            "main.bin", lambda data: struct.pack_into(">H", data, 0x234 + 2, 0xFFFF),
        )

    def test_missing_attribute_marker_fails(self) -> None:
        def remove_marker(data: bytearray) -> None:
            offset = self.inventory["model"]["attributes_offset"] + 0xE4
            self.assertEqual(data[offset:offset + 4], b"\x00\x64\x00\x64")
            data[offset:offset + 4] = b"\x00\x00\x00\x00"

        self.assert_corrupt_input_fails("main.bin", remove_marker)

    def test_unaligned_config_head_fails(self) -> None:
        def unalign_head(data: bytearray) -> None:
            before = b'main: ["01C0", "0000"]'
            self.assertIn(before, data)
            data[:] = data.replace(before, b'main: ["01C1", "0000"]')

        self.assert_corrupt_input_fails("config.yaml", unalign_head)

    def test_skeleton_selector_pointer_outside_main_payload_fails(self) -> None:
        self.assert_corrupt_input_fails(
            "main.bin", lambda data: struct.pack_into(">H", data, 0x61C + 2, 0xFFFF),
        )

    def test_skeleton_root_pointer_outside_character_payload_fails(self) -> None:
        self.assert_corrupt_input_fails(
            "main.bin", lambda data: struct.pack_into(">H", data, 0x438 + 2, 0xFFFF),
        )

    def test_skeleton_null_row_cannot_hide_an_unrelocated_pointer(self) -> None:
        self.assert_corrupt_input_fails(
            "main.bin", lambda data: struct.pack_into(">I", data, 0x428, 0x100),
        )

    def test_skeleton_unsupported_pair_flag_fails(self) -> None:
        self.assert_corrupt_input_fails(
            "main.bin", lambda data: data.__setitem__(0x43C, 0x01),
        )

    def test_skeleton_selector_gate_must_name_a_selected_source_joint(self) -> None:
        self.assert_corrupt_input_fails(
            "main.bin", lambda data: struct.pack_into(">I", data, 0x618, 255),
        )

    def test_skeleton_display_list_requires_its_end_sentinel(self) -> None:
        def remove_ends(data: bytearray) -> None:
            # Remove all possible ENDs in the root decoder's bounded window.
            # A neighboring root's END must not accidentally rescue this fixture.
            for offset in range(46096, 46096 + 256 * 8, 8):
                word = struct.unpack_from(">I", data, offset)[0]
                if word >> 24 == 0xDF:
                    struct.pack_into(">I", data, offset, 0)

        self.assert_corrupt_input_fails("character.bin", remove_ends)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--source-dir", type=Path)
    options, remaining = parser.parse_known_args()
    SOURCE_DIR = options.source_dir
    unittest.main(argv=[sys.argv[0], *remaining])
