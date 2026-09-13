"""Foreign IMAGE identities survive native deduplication and image packing."""
import struct
from pathlib import Path

import pytest

import generate_nds_native_owners as native


ROOT = Path(__file__).resolve().parents[2]


def test_image_dedup_distinguishes_owner_local_and_foreign_same_offset():
    commands = [(0, 0xfd, 0xfd100000, 0x12340008)]
    local_asset = native.P2_O2R_ASSETS["kirby"][1]
    local = native._decode_control("kirby", 0, commands, {
        0: native.stage_manifest.PointerRef(local_asset, 32)})[0]
    foreign = native._decode_control("kirby", 0, commands, {
        0: native.stage_manifest.PointerRef(338, 32)})[0]
    assert local[0][:3] == foreign[0][:3]
    assert local[0][3] == 0 and foreign[0][3] == 339
    states, lookup, sequence = [], {}, []
    native._append_state_span(local + foreign + local, states, lookup, sequence)
    assert len(states) == 2
    assert sequence == [0, 1, 0]
    local_bytes = native._pack_rows(native.STATE_DELTA_FORMAT, local)
    assert local_bytes == struct.pack("<IIB3x", *local[0][:3])
    foreign_bytes = native._pack_rows(native.STATE_DELTA_FORMAT, foreign)
    assert foreign_bytes[:9] == local_bytes[:9]
    assert foreign_bytes[9:] == bytes((0x53, 0x01, 0))
    assert "{ 83u, 1u, 0u }" in native.render_state_delta(foreign[0])


@pytest.mark.parametrize("detail", ("high", "low"))
def test_kirby_auxiliary_root_images_keep_yoshi_source_asset(detail):
    context = native.build_p2_owner_runtime_context(ROOT, "kirby", detail)
    root = next(row for row in context["roots"] if row[0] == 0x17850)
    indices = []
    for epoch in context["epochs"][root[1]:root[1] + root[4]]:
        for first, count in ((epoch[0], epoch[4]), (epoch[1], epoch[5])):
            indices.extend(context["sequence"][first:first + count] if count else [])
    images = [context["state"][index] for index in indices
              if context["state"][index][2] == 6]
    assert {(row[1], row[3]) for row in images} == {(0x9ec8, 339), (0x9ef0, 339)}
    assert all(row[3] == 339 for row in images)
    # All local rows retain their original 12-byte representation, including
    # recovered light deltas and source-state commands unrelated to IMAGE.
    for row in context["state"]:
        if row[3] == 0:
            assert native._pack_rows(native.STATE_DELTA_FORMAT, [row]) == struct.pack("<IIB3x", *row[:3])


@pytest.mark.parametrize("owner", ("yoshi", "nyoshi", "boss"))
def test_welded_pair_streams_keep_original_image_slots(owner):
    # Synthetic pair streams insert parent VTX commands before the post DL;
    # they must resolve fixups from original source PCs after that insertion.
    for detail in ("high", "low"):
        context = native.build_p2_owner_runtime_context(ROOT, owner, detail)
        foreign = {(row[1], row[3]) for row in context["state"] if row[3]}
        # NYoshi's source references the shared polygon texture asset 302.
        assert foreign == ({(0x8, 303), (0x30, 303)} if owner == "nyoshi" else set())


@pytest.mark.parametrize("detail", ("high", "low"))
def test_link_ci_root_keeps_palette_load_ownership(detail):
    """Link root 0x2C88 must replay each CI palette before its texel image."""
    context = native.build_p2_owner_runtime_context(ROOT, "link", detail)
    root = next(row for row in context["roots"] if row[0] == 0x2C88)
    sequence = []
    for epoch in context["epochs"][root[1]:root[1] + root[4]]:
        for first, count in ((epoch[0], epoch[4]), (epoch[1], epoch[5])):
            if count:
                sequence.extend(context["sequence"][first:first + count])
    if root[5]:
        sequence.extend(context["sequence"][root[2]:root[2] + root[5]])

    rows = [context["state"][index] for index in sequence]
    image_effect = native.SOURCE_STATE_EFFECTS[0xFD]
    tlut_effect = native.SOURCE_STATE_EFFECTS[0xF0]
    for palette_offset in (0xDD58, 0xDE88):
        image_index = next(
            index for index, row in enumerate(rows)
            if row[2] == image_effect and row[1] == palette_offset)
        tlut = rows[image_index + 1]
        assert tlut[2] == tlut_effect
        assert ((tlut[1] >> 14) & 0x3FF) + 1 == 16
