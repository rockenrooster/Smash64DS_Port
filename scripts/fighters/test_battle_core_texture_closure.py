"""Source image bytes survive battle FPC geometry removal."""
from types import SimpleNamespace

import pytest

import generate_battle_core_packs as battle


def test_yoshi_battle_keeps_pair_tables_and_relocates_their_display_roots(tmp_path):
    """Source case-1 DObjs read Gfx* pairs, not an ENDDL identity cell."""
    import struct
    battle.generate(tmp_path, ["yoshi"])
    packed = battle.fpc.decode_pack((tmp_path / "06.fpc").read_bytes())
    model = packed["sections"][1]
    spans = packed["spans"][model[4]:model[4] + model[5]]
    fixups = dict(packed["fixups"])
    types = battle.est.TypeTable()
    types.load_dirs(battle.est.HEADER_DIRS)
    index, _ = battle.est.index_closure("Yoshi", types)
    source = next(f for f in index.files if f.file_id == model[0])

    def address(offset):
        matches = [s for s in spans if s[0] <= offset < s[0] + s[2]]
        assert len(matches) == 1, f"missing structural pair/descriptor span {offset:#x}"
        old, new, _size = matches[0]
        return model[1] + new + offset - old

    checked = 0
    for tree in source.objects:
        if tree.type_name != "DObjDesc":
            continue
        for slot in range(tree.offset + 4, tree.offset + tree.size, 44):
            target = source.source["pointers"].get(slot)
            if target is None or target[0] != model[0]:
                continue
            pair = battle._owner(source.objects, target[1])
            if pair.type_name != "Gfx" or pair.pointer_depth != 1:
                continue
            assert fixups[address(slot)] == address(target[1])
            for pair_slot in (target[1], target[1] + 4):
                display = source.source["pointers"].get(pair_slot)
                if display is None:
                    assert struct.unpack_from(">I", packed["data"], address(pair_slot))[0] == 0
                else:
                    assert display[0] == model[0]
                    cell = fixups[address(pair_slot)]
                    assert struct.unpack_from(">II", packed["data"], cell) == (
                        battle.fpc.ENDDL, display[1])
            checked += 1
    assert checked >= 19  # HIGH's 19 pairs plus any LOW/variant pair users.


def row(offset, size, kind, pointers=0):
    return SimpleNamespace(offset=offset, size=size, type_name=kind,
                           pointer_depth=pointers, symbol=f"{kind}_{offset:x}")

def fixture():
    payload = bytearray(96)
    payload[0] = 0xDE                 # Source child display-list call.
    payload[16] = payload[24] = 0xFD  # Palette and image (interior alias).
    payload[32] = 0x01               # Vertex data is owned by native geometry.
    payload[40] = 0xDC               # Light payload is compiled natively too.
    return SimpleNamespace(objects=[row(0, 8, "Gfx"), row(16, 32, "Gfx"),
                                    row(48, 8, "u16"), row(56, 16, "u8"),
                                    row(72, 16, "Vtx")],
                           source={"payload": bytes(payload), "pointers": {
                               4: (320, 16), 20: (320, 48), 28: (320, 60),
                               36: (320, 72), 44: (299, 0)}})


def test_children_keep_exact_texel_and_palette_declarations_not_geometry():
    rows, offsets = battle._gfx_texture_closure(fixture(), 320, {0, 16})
    assert [(r.offset, r.size) for r in rows.values()] == [(48, 8), (56, 16)]
    assert offsets == ((320, 48), (320, 60))


def test_foreign_image_must_not_alias_the_owner_at_the_same_offset():
    source = fixture()
    source.source["pointers"][28] = (338, 60)
    with pytest.raises(battle.BattlePackError, match="escapes.*asset 338"):
        battle._gfx_texture_closure(source, 320, {0})


def test_foreign_bank_preserves_asset_identity_interior_aliases_and_source_bytes():
    source = fixture()
    donor = fixture()
    donor.source["payload"] = bytes(range(96))
    source.source["pointers"][28] = (338, 60)
    files = {320: source, 338: donor}
    images, offsets = battle._gfx_texture_closure(source, 320, {0}, files)
    records, data = battle._foreign_texture_bank(images, files, 320)
    assert offsets == ((320, 48), (338, 60))
    assert records == [(338, 0, 56, 0, 16)]
    assert data == donor.source["payload"][56:72]
    assert data != source.source["payload"][56:72]


def test_overlapping_foreign_declarations_share_one_preserved_span():
    donor = fixture()
    images = {(338, 48, 16): row(48, 16, "u8"),
              (338, 56, 16): row(56, 16, "u8")}
    records, data = battle._foreign_texture_bank(images, {338: donor}, 320)
    assert records == [(338, 0, 48, 0, 24)]
    assert data == donor.source["payload"][48:72]


def test_image_cannot_be_a_vertex_allocation():
    source = fixture()
    source.source["pointers"][28] = (320, 72)
    with pytest.raises(battle.BattlePackError, match="non-image Vtx"):
        battle._gfx_texture_closure(source, 320, {0})


def test_samus_high_and_low_images_map_to_real_source_payloads(tmp_path):
    battle.generate(tmp_path, ["samus"])
    packed = battle.fpc.decode_pack((tmp_path / "03.fpc").read_bytes())
    model = packed["sections"][1]
    spans = packed["spans"][model[4]:model[4] + model[5]]
    source = battle._source_payload(model[0])
    offsets = set()
    for detail in ("high", "low"):
        context = battle.native.build_p2_owner_runtime_context(
            battle.ROOT, "samus", detail)
        offsets.update(delta[1] for delta in context["state"]
                       if delta[2] == 6 and delta[3] == 0)
    assert 0xCED8 in offsets  # Previously omitted, so the runtime returned NULL.
    for offset in offsets:
        matches = [span for span in spans if span[0] <= offset < span[0] + span[2]]
        assert len(matches) == 1, hex(offset)
        original, compact, size = matches[0]
        assert packed["data"][model[1] + compact:model[1] + compact + size] == \
            source[original:original + size]


def test_kirby_bex2_contains_private_source_yoshi_pixels(tmp_path):
    import struct
    battle.generate(tmp_path, ["kirby"])
    data = (tmp_path / "08.ext").read_bytes()
    magic, version, externs, count, size, expected_hash, reserved = struct.unpack_from(
        battle.EXTERN_HEADER_FMT, data)
    assert (magic, version, count, size, reserved) == (battle.EXTERN_MAGIC, 2, 2, 296, 0)
    first = struct.calcsize(battle.EXTERN_HEADER_FMT) + externs * 6
    assert battle.fpc.fnv1a32(data[first:]) == expected_hash
    records = [struct.unpack_from(battle.FOREIGN_ROW_FMT, data, first + i * 16)
               for i in range(count)]
    assert records == [(338, 0, 0x9EC8, 0, 32), (338, 0, 0x9EF0, 32, 264)]
    pixels = data[first + count * 16:]
    source = battle._source_payload(338)
    for asset, zero, offset, packed, span in records:
        assert pixels[packed:packed + span] == source[offset:offset + span]
    local = battle.fpc.decode_pack((tmp_path / "08.fpc").read_bytes())
    model = local["sections"][1]
    local_spans = local["spans"][model[4]:model[4] + model[5]]
    local_source = battle._source_payload(model[0])
    for detail in ("high", "low"):
        contexts = [battle.native.build_p2_owner_runtime_context(
            battle.ROOT, "kirby", detail)]
        contexts.extend(battle.native.build_p2_kirby_hat_runtime_context(
            battle.ROOT, detail, modelpart) for modelpart in
            battle.native.KIRBY_COPY_HAT_MODEL_PART_IDS)
        for context in contexts:
            for _w0, offset, kind, asset_plus_one in context["state"]:
                if kind != 6:
                    continue
                if asset_plus_one:
                    matches = [r for r in records if r[0] == asset_plus_one - 1
                               and r[2] <= offset < r[2] + r[4]]
                    assert len(matches) == 1, (detail, asset_plus_one, hex(offset))
                else:
                    matches = [s for s in local_spans if s[0] <= offset < s[0] + s[2]]
                    assert len(matches) == 1, (detail, hex(offset))
                    original, compact, size = matches[0]
                    assert local["data"][model[1] + compact:model[1] + compact + size] == \
                        local_source[original:original + size]
