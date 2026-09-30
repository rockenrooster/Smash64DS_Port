"""Source-qualified Meta FPC2/BEX2 compaction and lifetime checks."""
import copy
from pathlib import Path
import struct
import unittest

import extra_core_packs as packs
import extra_resource_adapter as raw
import generate_battle_core_packs as battle
import generate_preview_core_packs as preview


class MetaCorePackTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[2]
        cls.native = root / 'builds/p4/meta-knight-native'
        source = root / 'builds/p4/meta-knight-donor/extra/build/extra_characters/MetaKnight'
        if not (cls.native / 'native-runtime-bindings.json').is_file() or not source.is_dir():
            raise unittest.SkipTest('qualified frozen Meta donor/native fixtures are absent')
        # In-memory host fixture; no generator outputs or source files written.
        cls.ir = raw.build_meta_knight_model_ir(source, 5456, 1572,
                                               source_resource_ids={'MAIN': 5455, 'CHARACTER': 5456})
        cls.blob, cls.ext, cls.report = packs.build_meta_pack(cls.native, cls.ir)
        cls.decoded = preview.decode_pack(cls.blob)

    def test_identity_and_css_residency_fit(self):
        header = self.decoded['header']
        self.assertEqual(header[3], 29)
        self.assertEqual(header[12:15], (5455, 5456, 75296))
        self.assertEqual(self.report['file_bytes'], 40076)
        self.assertEqual(self.report['resident_allocation'], 36332)
        self.assertEqual(self.report['css_pack_and_high_image_peak'], 62160)
        self.assertLessEqual(self.report['css_pack_and_high_image_peak'], 80 * 1024)
        self.assertEqual(self.report['core_and_both_detail_images'], 87988)
        self.assertEqual(self.report['css_scene_peak_separate_lifetimes'], 132280)

    def test_every_structural_byte_is_retained_and_all_removed_geometry_is_qualified(self):
        _, source = packs.assets.load_o2r(self.native / 'reloc_extra/MetaKnightModel')
        section = self.decoded['sections'][1]
        spans = self.decoded['spans'][section[4]:section[4] + section[5]]
        kept_bytes = set()
        for old, new, length in spans:
            body = self.decoded['data'][section[1] + new:section[1] + new + length]
            self.assertEqual(body, source.payload[old:old + length])
            kept_bytes.update(range(old, old + length))
        removed = set()
        for first, end in self.report['geometry_replacement_ranges']:
            removed.update(range(first, end))
        self.assertEqual(len(kept_bytes), 33200)
        self.assertEqual(len(removed), 42096)
        self.assertFalse(removed & kept_bytes)
        self.assertEqual(removed | kept_bytes, set(range(75296)))
        self.assertEqual(self.report['root_cells'], 59)
        self.assertEqual(self.report['fixups'], 460)

    def test_main_hurtboxes_attributes_and_all_main_dependency_rows_survive(self):
        _, source = packs.assets.load_o2r(self.native / 'reloc_extra/MetaKnightMain')
        section = self.decoded['sections'][0]
        self.assertEqual(self.decoded['data'][section[1]:section[1] + section[2]], source.payload)
        header = struct.unpack_from(battle.EXTERN_HEADER_FMT, self.ext)
        self.assertEqual(header[:3], (battle.EXTERN_MAGIC, battle.EXTERN_VERSION, 11))
        rows = list(struct.iter_unpack(battle.EXTERN_ROW_FMT,
                                      self.ext[struct.calcsize(battle.EXTERN_HEADER_FMT):]))
        self.assertEqual(rows, self.report['external_patches'])
        self.assertEqual(sum(dep == 331 for slot, dep, target in rows), 9)
        self.assertIn((0, 232, 52), rows)
        self.assertIn((4, 351, 8496), rows)
        self.assertEqual(self.report['external_status_allocations'],
                         {232: 6016, 201: 2096, 331: 17936, 351: 12112})

    def test_exact_selected_win4_clip_uses_existing_animation_lifetime(self):
        self.assertEqual(self.report['required_menu_clips'],
                         [{'menu_row': 0, 'native_file_id': 5497, 'bytes': 7888},
                          {'menu_row': 4, 'native_file_id': 5551, 'bytes': 18904}])
        self.assertFalse(self.report['menu_sections'])
        self.assertEqual(self.report['motion_status_allocation'], 13056)
        self.assertEqual(self.report['battle_figatree_bytes'], 65264)

    def test_missing_electric_ir_and_changed_native_ir_fail_closed(self):
        missing = copy.deepcopy(self.ir)
        missing.pop('skeleton_ir')
        with self.assertRaisesRegex(preview.PackError, 'electric skeleton native IR coverage'):
            packs.build_meta_pack(self.native, missing)
        changed = copy.deepcopy(self.ir)
        changed['model_ir']['high']['ir']['triangles'][0] ^= 1
        with self.assertRaisesRegex(preview.PackError, 'native IR hash mismatch'):
            packs.build_meta_pack(self.native, changed)

    def test_meta_is_only_an_explicit_roster_opt_in(self):
        self.assertEqual(preview.parse_kinds(None), preview.KIND_ORDER)
        self.assertEqual(len(preview.KIND_ORDER), 12)
        with self.assertRaises(preview.PackError):
            preview.parse_kinds('metaknight')
        self.assertEqual(preview.parse_kinds('metaknight', allow_meta=True), ['metaknight'])
        self.assertEqual(preview.parse_kinds('29', allow_meta=True), ['metaknight'])


if __name__ == '__main__':
    unittest.main()
