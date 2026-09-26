#!/usr/bin/env python3
"""Focused tests for MFP2 main-user closure, mirror masks, and memory pricing."""

from __future__ import annotations

import struct
import unittest
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import mf_residency as mr  # noqa: E402
import mf_emit  # noqa: E402


def bit(kind):
    return 1 << mr.mc.KINDS.index(kind)


def entry(asset_id, kind, cls=mr.CLS_ALWAYS, need=0, users=0, mf_bytes=4):
    return {
        "asset_id": asset_id,
        "kind": kind,
        "cls": cls,
        "need_mask": need,
        "main_user_mask": users,
        "mf_bytes": mf_bytes,
        "decoded_bytes": 11568,
    }


class MfResidencyTest(unittest.TestCase):
    def test_main_users_come_from_motion_table_references(self):
        shared_mario = 0x100
        shared_kirby = 0x200
        corpus = {
            "tables": {
                "mario": [("mario_a", shared_mario)],
                "luigi": [("luigi_a", shared_mario)],
                "kirby": [("kirby_a", shared_kirby)],
                "purin": [("purin_a", shared_kirby)],
                "ness": [(None, None)],
            }
        }
        users = mr.main_user_masks(corpus)
        self.assertEqual(users[shared_mario], bit("mario") | bit("luigi"))
        self.assertEqual(users[shared_kirby], bit("kirby") | bit("purin"))
        self.assertNotIn("FAMILY_OF", users)

    def test_unmapped_nonnull_motion_fails_admission(self):
        with self.assertRaisesRegex(ValueError, "unmapped required motion"):
            mr.main_user_masks({"tables": {"ness": [("llFTMissingFileID", None)]}})

    def test_opponent_masks_preserve_same_kind_mirrors(self):
        slots = ("mario", "mario", "fox", "purin")
        masks = mr.opponent_masks_for_slots(slots)
        self.assertEqual(masks[0], bit("mario") | bit("fox") | bit("purin"))
        self.assertEqual(masks[1], masks[0])
        self.assertEqual(masks[2], bit("mario") | bit("purin"))
        self.assertEqual(masks[3], bit("mario") | bit("fox"))
        self.assertEqual(mr.opponent_masks_for_slots(("mario",)), (0,))

    def test_selection_handles_borrowed_users_optional_masks_and_mirrors(self):
        rows = [
            entry(0x100, "mario"),
            entry(0x101, "mario", mr.CLS_VICTIM, bit("luigi")),
            entry(0x102, "mario", mr.CLS_VICTIM, bit("mario")),
            entry(0x103, "mario", mr.CLS_VICTIM, bit("fox"), bit("luigi")),
            entry(0x200, "kirby", users=bit("purin")),
            entry(0x201, "kirby", mr.CLS_COPY, bit("purin")),
            entry(0x202, "purin"),
        ]

        def ids(slots):
            return {row["asset_id"] for row in mr.select_compressed_entries(rows, slots)}

        self.assertEqual(ids(("mario",)), {0x100})
        self.assertEqual(ids(("mario", "luigi")), {0x100, 0x101, 0x103})
        self.assertEqual(ids(("mario", "mario")), {0x100, 0x102})
        self.assertEqual(ids(("kirby", "purin")), {0x200, 0x201, 0x202})
        self.assertEqual(ids(("kirby", "kirby")), {0x200})

    def test_raw_exceptions_are_selected_and_priced_separately(self):
        rows = [entry(0x100, "mario", users=bit("luigi"))]
        raw = [{
            "asset_id": 0x300,
            "kind": "samus",
            "main_user_mask": 0,
            "payload_bytes": 113,
            "source_bytes": 129,
        }]
        plan = mr.resident_plan(
            rows, raw, ("luigi", "samus"), tables_blob_bytes=80,
            expanded_tables_bytes=40, tables_object_bytes=24,
        )
        self.assertEqual(plan["compressed_ids"], [0x100])
        self.assertEqual(plan["raw_ids"], [0x300])
        self.assertEqual(plan["resident_header_bytes"], 48)
        self.assertEqual(plan["resident_entry_bytes"], 28)
        self.assertEqual(plan["resident_raw_entry_bytes"], 20)
        self.assertEqual(plan["resident_stream_bytes"], 4)
        self.assertEqual(plan["raw_payload_bytes"], 113)
        self.assertEqual(plan["raw_allocation_bytes"], 128)
        self.assertEqual(plan["raw_asset_rounding_padding"], 15)
        self.assertEqual(plan["pre_raw_a7_bytes"], 244)
        self.assertEqual(plan["raw_alignment_padding"], 12)
        self.assertEqual(plan["raw_source_bytes"], 129)
        self.assertEqual(plan["required_a7_bytes"], 244 + 12 + 128)
        self.assertEqual(plan["decode_scratch_bytes"], 11568)
        self.assertEqual(plan["peak_a7_with_one_decode_buffer_bytes"],
                         244 + 12 + 128 + 11568)

    def test_full_mfp2_directory_overhead_is_explicit(self):
        self.assertEqual(mf_emit.HEADER.size, 48)
        self.assertEqual(mf_emit.ENTRY.size, 28)
        self.assertEqual(mf_emit.RAW_ENTRY.size, 20)
        self.assertEqual(len(mf_emit.HEADER.pack(
            mf_emit.PACK_MAGIC, mf_emit.PACK_VERSION, len(mr.mc.KINDS),
            *([0] * 10),
        )), 48)
        self.assertEqual(len(mf_emit.ENTRY.pack(*([0] * 10))), 28)
        self.assertEqual(len(mf_emit.RAW_ENTRY.pack(*([0] * 7))), 20)
        self.assertEqual(
            mr.full_metadata_bytes(12, 16928, 1570, 29),
            64848,
        )

    def test_arm_table_storage_formula_uses_mft1_blob_shape(self):
        table_header = struct.pack("<BBHH12H", 0, 0, 1, 0, 1, *([0] * 11))
        blob = (
            struct.pack("<IHHHH", mr.MFT1_MAGIC, 1, 0, 1, 0)
            + bytes(2 * mr.MFT1_RECIP_COUNT)
            + struct.pack("<H", 0)
            + table_header
            + struct.pack("<h", 0)
        )
        self.assertEqual(mr.mft1_arm_storage_bytes(blob), 224)
        with self.assertRaises(ValueError):
            mr.mft1_arm_storage_bytes(blob[:-1])


if __name__ == "__main__":
    unittest.main()
