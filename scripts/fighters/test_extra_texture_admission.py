#!/usr/bin/env python3
"""Focused host checks for the typed Meta admission producer and legacy ABI."""
import copy
from pathlib import Path
import struct
import tempfile
import unittest
from unittest import mock

import generate_nds_fighter_admission as admission
from extra_resource_adapter import AdapterError
from extra_texture_admission import MetaTextureClosure, MAIN_ID


class AdmissionAbiTests(unittest.TestCase):
    def test_default_legacy_directory_stays_twelve_ordinals(self):
        with mock.patch.object(admission.est, 'TypeTable'), \
                mock.patch.object(admission.est, 'parse_native_image_census'), \
                mock.patch.object(admission.est, 'build_fighter_ledgers', return_value={}):
            _, index, payload = admission.build(kinds_only=[])
        self.assertEqual(payload, struct.pack('<4I', admission.MAGIC, 1, 12, 28) + bytes(24 * 8))
        self.assertEqual(index, [(0, 0)] * 24)
        self.assertIn('NDS_FIGHTER_ADMISSION_KINDS 12u', admission.render_header(index, payload))
        self.assertNotIn('METAKNIGHT_INDEX', admission.render_header(index, payload))

    def test_thirteenth_directory_names_compact_and_runtime_ids(self):
        header = admission.render_header([(0, 0)] * 26, bytes(224))
        self.assertIn('NDS_FIGHTER_ADMISSION_KINDS 13u', header)
        self.assertIn('NDS_FIGHTER_ADMISSION_METAKNIGHT_INDEX 12u', header)
        self.assertIn('NDS_FIGHTER_ADMISSION_METAKNIGHT_RUNTIME_KIND 29u', header)
        self.assertIn('MetaKnight HIGH', header)
        with self.assertRaisesRegex(ValueError, 'roster mapping'):
            admission.render_header([(0, 0)] * 25, bytes(216))


class FrozenMetaSourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.native_dir = Path(__file__).resolve().parents[2] / 'builds/p4/meta-knight-native'
        if not (cls.native_dir / 'native-runtime-bindings.json').is_file():
            raise unittest.SkipTest('qualified frozen Meta resource fixture is absent')
        cls.closure = MetaTextureClosure(cls.native_dir, admission)
        cls.tables = admission.enumerate_kind('MetaKnight', MAIN_ID, cls.closure)
        cls.closure.validate_records(cls.tables)
        cls.report = cls.closure.report(cls.tables)

    def test_complete_roots_costumes_and_record_size(self):
        self.assertEqual([len(self.tables[d]) for d in (0, 1)], [90, 90])
        self.assertEqual([len(self.closure.native_part_rows(d)) for d in (0, 1)], [45, 45])
        self.assertEqual([len({row[0] for row in self.closure.native_part_rows(d)})
                          for d in (0, 1)], [37, 37])
        self.assertEqual(self.report['material_count'], 26)
        self.assertEqual(self.report['material_stream_count'], 46)
        for detail in (0, 1):
            for costume in range(6):
                self.assertTrue(any(record['costume'] & (1 << costume)
                                    for record in self.tables[detail].values()))
            self.assertEqual(sum(len(admission.pack_record(record, 12))
                                 for record in self.tables[detail].values()), 10080)

    def test_source_alias_and_physical_union_are_preserved(self):
        spans = self.report['source_texture_spans']
        aliases = [entry for entry in spans if entry['offset'] == 54400]
        self.assertEqual({entry['role'] for entry in aliases}, {'image', 'palette'})
        self.assertEqual(self.report['source_texture_bytes_sum'], 19616)
        self.assertEqual(self.report['source_texture_union_bytes'], 19408)
        self.assertEqual(self.report['resource_union'], [5456])
        face = [entry for entry in self.report['material_table_extents']
                if entry['offset'] in (512, 18824)]
        self.assertEqual(len(face), 2)
        self.assertTrue(all(entry['texture_id_max'] == 6 and entry['palette_id_max'] == 5
                            for entry in face))

    def test_alpha_only_view_covers_full_ia8_source_and_all_alpha_values(self):
        views = self.report['alpha_only_views']
        self.assertEqual([entry['offset'] for entry in views], [63608, 67712])
        for entry in views:
            self.assertEqual(entry['bytes'], 4096)
            self.assertEqual(entry['source_alpha_nibbles'], list(range(16)))
            self.assertEqual(entry['view_key_bit'], 28)
        for detail in (0, 1):
            records = [entry for entry in self.tables[detail].values()
                       if entry['combine'] == (0xFC321803, 0xFF17FFFF)]
            packed = struct.unpack('<28I', admission.pack_record(records[0], 12))
            self.assertEqual(packed[20:22], (0xFC321803, 0xFF17FFFF))
            self.assertEqual(packed[22] & (3 << 20), 1 << 20)

    def test_wrong_format_and_costume_domain_fail_closed(self):
        records = copy.deepcopy(self.tables)
        alpha = next(entry for entry in records[0].values()
                     if entry['combine'] == (0xFC321803, 0xFF17FFFF))
        w0, w1 = alpha['render_set']
        alpha['render_set'] = ((w0 & ~(7 << 21)) | (2 << 21), w1)
        with self.assertRaisesRegex(AdapterError, 'requires source IA8'):
            self.closure.validate_records(records)
        records = copy.deepcopy(self.tables)
        next(iter(records[0].values()))['costume'] |= 1 << 6
        with self.assertRaisesRegex(AdapterError, 'nonexistent costume'):
            self.closure.validate_records(records)

    def test_qualified_append_preserves_legacy_records_and_ordinals(self):
        index = [(0, 1), (1, 1)] + [(2, 0)] * 22
        records = bytes(range(112)) * 2
        legacy = struct.pack('<4I', admission.MAGIC, 1, 12, 28)
        legacy += b''.join(struct.pack('<2I', *row) for row in index) + records
        with tempfile.TemporaryDirectory() as directory:
            payload_path = Path(directory) / 'admission.bin'
            header_path = Path(directory) / 'header.h'
            payload_path.write_bytes(legacy)
            header_path.write_text(
                f'#define NDS_FIGHTER_ADMISSION_PAYLOAD_BYTES {len(legacy)}u\n'
                f'#define NDS_FIGHTER_ADMISSION_PAYLOAD_FNV 0x{admission.fnv1a32(legacy):08X}u\n'
                '#define NDS_FIGHTER_ADMISSION_KINDS 12u\n'
                '#define NDS_FIGHTER_ADMISSION_ROOT_MAX 100u\n')
            with mock.patch.object(admission, 'HEADER_PATH', str(header_path)):
                _, new_index, new_payload = admission.build_from_legacy_payload(payload_path, self.native_dir)
                self.assertEqual(new_index[:24], index)
                self.assertEqual(new_index[24:], [(2, 90), (92, 90)])
                self.assertEqual(struct.unpack_from('<4I', new_payload), (admission.MAGIC, 1, 13, 28))
                self.assertEqual(new_payload[224:224 + len(records)], records)
                self.assertEqual(admission.build.root_max, 100)
                header_path.write_text(admission.render_header(new_index, new_payload))
                _, repeated_index, repeated = admission.build_from_legacy_payload(payload_path, self.native_dir)
                self.assertEqual(repeated_index, new_index)
                self.assertEqual(repeated, new_payload)
                payload_path.write_bytes(legacy[:-1] + bytes([legacy[-1] ^ 1]))
                with self.assertRaisesRegex(ValueError, 'qualified tracked header'):
                    admission.build_from_legacy_payload(payload_path, self.native_dir)


if __name__ == '__main__':
    unittest.main()
