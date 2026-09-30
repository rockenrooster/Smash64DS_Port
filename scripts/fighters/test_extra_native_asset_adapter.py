#!/usr/bin/env python3
"""Host proof of event closure, pointer domains, and RELO emission."""
from pathlib import Path
import struct
import tempfile
import unittest

import extra_native_asset_adapter as adapter
from extra_resource_adapter import Dependency, decode_resource


class Files:
    def __init__(self, resources):
        self.resources = resources

    def load(self, file_id):
        return self.resources[file_id]


def words(*values):
    return struct.pack(f">{len(values)}I", *values)


def read_emitted(blob):
    with tempfile.TemporaryDirectory() as temporary:
        path = Path(temporary) / "resource"
        path.write_bytes(blob)
        return adapter.load_o2r(path)


def descriptor(script=0, animation=5000, flags=0):
    return {"animation_file_id": animation, "script_donor_value": script, "flags": flags}


class NativeEventTests(unittest.TestCase):
    def test_loop_subroutine_and_internal_target_relocate_once(self):
        raw = decode_resource("232", words(0x80000002, 0x04000001, 0x84000000,
                                           0x88000000, 0xFFFF0008, 0,
                                           0x12345678, 0x12345678,
                                           0xCC03FFFF, 0x8C000000),
                              16, 0x3FFFC)
        compiler = adapter.EventCompiler(Files({232: raw}), b"", {})
        blob, report = compiler.emit([descriptor(), descriptor()], {5000: 5000})
        file_id, native = read_emitted(blob)
        self.assertEqual(file_id, adapter.MOTION_ID)
        desc0, desc1 = struct.unpack_from(">III", native.payload), struct.unpack_from(">III", native.payload, 12)
        self.assertEqual(desc0, desc1)
        self.assertEqual(desc0[0], 5000)
        self.assertEqual(desc0[1], 24)
        self.assertEqual(native.pointer(24 + 16).offset, 24 + 24)
        self.assertEqual(len(native.internal), 1)
        self.assertEqual(report["graph_nodes"], 7)
        self.assertNotIn(words(0x12345678), native.payload)

    def test_external_script_is_compiled_into_same_container(self):
        source = decode_resource("232", words(0x88000000, 0xFFFF0000, 0),
                                 0x3FFFC, 4, (Dependency("donor_file", file_id=201),))
        shared = decode_resource("201", words(0x04000003, 0x8C000000),
                                 0x3FFFC, 0x3FFFC)
        compiler = adapter.EventCompiler(Files({232: source, 201: shared}), b"", {})
        blob, _ = compiler.emit([descriptor()], {5000: 5000})
        _, native = read_emitted(blob)
        root = struct.unpack_from(">III", native.payload)[1]
        self.assertEqual(native.pointer(root + 4).offset, 12)
        self.assertEqual(native.payload[12:20], shared.payload)
        self.assertFalse(native.external)

    def test_both_throw_description_rows_survive(self):
        values = tuple(range(14))
        source = decode_resource("232", words(0x30000000, 0xFFFF0003, 0, *values),
                                 4, 0x3FFFC)
        compiler = adapter.EventCompiler(Files({232: source}), b"", {})
        blob, _ = compiler.emit([descriptor()], {5000: 5000})
        _, native = read_emitted(blob)
        root = struct.unpack_from(">III", native.payload)[1]
        target = native.pointer(root + 4).offset
        self.assertEqual(native.payload[target:target + 56], words(*values))

    def test_no_script_sentinel_is_preserved_without_fake_event(self):
        compiler = adapter.EventCompiler(Files({}), b"", {})
        blob, report = compiler.emit([descriptor(adapter.NONE, 0, 0x50000000)], {})
        _, native = read_emitted(blob)
        self.assertEqual(struct.unpack(">III", native.payload), (0, adapter.NONE, 0x50000000))
        self.assertEqual(report["graph_nodes"], 0)
        self.assertFalse(native.internal)

    def test_linked_rom_addresses_become_threaded_offsets(self):
        linked = words(0x88000000, 0x8050000C, 0, 0x04000001, 0x8C000000)
        compiler = adapter.EventCompiler(Files({}), linked, {},
                                          static_regions=((0x80500000, len(linked), 0),))
        blob, _ = compiler.emit([descriptor(0x80500000)], {5000: 5000})
        _, native = read_emitted(blob)
        root = struct.unpack_from(">III", native.payload)[1]
        self.assertEqual(root, 12)
        self.assertEqual(native.pointer(root + 4).offset, root + 12)
        self.assertNotIn(words(0x8050000C), native.payload)

    def test_unclassified_pointer_cannot_enter_native_output(self):
        compiler = adapter.EventCompiler(Files({}), b"", {})
        with self.assertRaisesRegex(adapter.ConversionError, "unclassified address"):
            compiler.emit([descriptor(0x80001000)], {5000: 5000})

    def test_unknown_custom_command_is_not_silently_skipped(self):
        source = decode_resource("232", words(0xFE123456, 0), 0x3FFFC, 0x3FFFC)
        compiler = adapter.EventCompiler(Files({232: source}), b"", {})
        with self.assertRaisesRegex(adapter.ConversionError, "unknown custom command"):
            compiler.emit([descriptor()], {5000: 5000})

    def test_custom_control_transfer_needs_typed_lowering(self):
        source = decode_resource("232", words(0xDB000100, 0), 0x3FFFC, 0x3FFFC)
        compiler = adapter.EventCompiler(Files({232: source}), b"", {})
        with self.assertRaisesRegex(adapter.ConversionError, "typed table/control lowering"):
            compiler.emit([descriptor()], {5000: 5000})

    def test_unmapped_animation_fails(self):
        compiler = adapter.EventCompiler(Files({}), b"", {})
        with self.assertRaisesRegex(adapter.ConversionError, "unmapped animation"):
            compiler.emit([descriptor(adapter.NONE)], {})


class NativeRelocTests(unittest.TestCase):
    def test_deterministic_internal_and_external_chain_round_trip(self):
        payload = bytes(24)
        blob = adapter.encode_o2r(5455, payload, {8: 0, 4: 20}, {12: (5456, 0), 16: (331, 8)})
        file_id, resource = read_emitted(blob)
        self.assertEqual(file_id, 5455)
        self.assertEqual(resource.pointer(8).offset, 0)
        self.assertEqual(resource.pointer(4).offset, 20)
        self.assertEqual(resource.pointer(12).dependency.file_id, 5456)
        self.assertEqual(resource.pointer(16).dependency.file_id, 331)
        self.assertEqual(blob, adapter.encode_o2r(5455, payload, {4: 20, 8: 0},
                                                {16: (331, 8), 12: (5456, 0)}))

    def test_pointer_chain_overlap_and_u16_overflow_are_rejected(self):
        with self.assertRaisesRegex(adapter.ConversionError, "overlap"):
            adapter.encode_o2r(5455, bytes(16), {4: 0}, {4: (5456, 0)})
        with self.assertRaisesRegex(adapter.ConversionError, "outside u16"):
            adapter.encode_o2r(5455, bytes(16), {}, {4: (0x10000, 0)})
        with self.assertRaisesRegex(adapter.ConversionError, "encoding"):
            adapter.encode_o2r(5455, bytes(16), {}, {4: (5456, 0x40000)})

    def test_inherited_source_identity_requires_all_data_and_targets(self):
        source = decode_resource("232", words(0xFFFF0001, 7), 0, 0x3FFFC)
        same = decode_resource("232", source.payload, 0, 0x3FFFC)
        adapter.verified_identity(source, same)
        changed = decode_resource("232", words(0xFFFF0001, 8), 0, 0x3FFFC)
        with self.assertRaisesRegex(adapter.ConversionError, "source identity differs"):
            adapter.verified_identity(source, changed)


if __name__ == "__main__":
    unittest.main()
