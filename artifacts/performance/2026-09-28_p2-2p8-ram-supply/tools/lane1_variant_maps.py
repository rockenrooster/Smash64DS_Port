#!/usr/bin/env python3
"""Lane 1: proof of concept for the build-time compact ground map (T1, option B).

For each of the nine VS stages, rewrite GR<Stage>Map (O2R) so that its `wallpaper` pointer no longer
crosses to the 158,928-byte wallpaper container: the container's Bitmap[44] table and Sprite record
(776 B, the only bytes the DS reads) are appended to the map file itself, both `wallpaper` and
`Sprite.bitmap` become INTERNAL fixups, every Bitmap.buf becomes 0 (nothing reads pixels), and the
wallpaper file id is dropped from the extern table.  The variants are written to ../variants/ and
re-parsed with the stage generator's own parser (generate_nds_native_stage.load_o2r) to prove:

  * the container parses; external fixups drop by exactly one, internal fixups rise by exactly two
    (wallpaper slot -> stub Sprite, stub Sprite.bitmap -> stub Bitmap table)
  * the extern tree of the variant no longer contains the wallpaper file
  * lbRelocGetFileSize's arithmetic (16-byte aligned sum) shrinks by 158,144 B per stage

Nothing here touches the repo: output stays under the lane folder.
"""
from __future__ import annotations

import hashlib
import json
import struct
import sys
from pathlib import Path

import lane1_o2r as o
import lane1_readers as rd

OUT = Path(__file__).resolve().parents[1] / "variants"
HEADER_KEEP = 0x40


def align16(n):
    return (n + 15) & ~15


def parse_raw(raw: bytes):
    file_id, ih, eh, ec = struct.unpack_from("<IHHI", raw, 0x40)
    ids = list(struct.unpack_from(f"<{ec}H", raw, 0x4C)) if ec else []
    dsz = struct.unpack_from("<I", raw, 0x4C + 2 * ec)[0]
    payload = bytearray(raw[0x4C + 2 * ec + 4:])
    assert len(payload) == dsz
    return raw[:0x40], file_id, ih, eh, ids, payload


def chain_slots(payload: bytearray, head: int):
    """[(slot_byte_offset, target_byte_offset)] in chain order."""
    out = []
    cur = head
    while cur != 0xFFFF:
        slot = cur * 4
        w = struct.unpack_from(">I", payload, slot)[0]
        out.append((slot, (w & 0xFFFF) * 4))
        cur = w >> 16
    return out


def write_chain(payload: bytearray, slots):
    """slots: [(slot_off, target_off)] -> returns head index; rewrites the chain words."""
    if not slots:
        return 0xFFFF
    for i, (slot, target) in enumerate(slots):
        nxt = (slots[i + 1][0] // 4) if i + 1 < len(slots) else 0xFFFF
        assert target % 4 == 0 and target // 4 <= 0xFFFF and nxt <= 0xFFFF
        struct.pack_into(">I", payload, slot, (nxt << 16) | (target // 4))
    return slots[0][0] // 4


def build_variant(map_fid: int, wall_fid: int):
    idx = o.index()
    mp = idx[map_fid]
    wp = idx[wall_fid]
    raw = mp.path.read_bytes()
    hdr, fid, ih, eh, ids, payload = parse_raw(raw)
    assert fid == map_fid
    internal = chain_slots(payload, ih)
    external = chain_slots(payload, eh)
    assert len(external) == len(ids), (len(external), len(ids))
    # locate the wallpaper slot: MPGroundData at 0x14, wallpaper at +0x48
    wall_slot = 0x14 + 0x48
    k = [i for i, (s, t) in enumerate(external) if s == wall_slot]
    assert len(k) == 1 and ids[k[0]] == wall_fid, (k, ids)
    k = k[0]
    tgt_sprite_src = external[k][1]                      # 0x26C88
    new_external = [e for i, e in enumerate(external) if i != k]
    new_ids = [x for i, x in enumerate(ids) if i != k]
    # stub = Bitmap[44] + Sprite of the container (raw big-endian bytes)
    bm_off, spr_off = 0x269C8, tgt_sprite_src
    assert spr_off == 0x26C88 and wp.data_size == 0x26CD0
    stub_bm = bytearray(wp.payload[bm_off:spr_off])      # 704 B
    stub_spr = bytearray(wp.payload[spr_off:wp.data_size])  # 72 B
    base = len(payload)
    assert base % 16 == 0
    bm_new = base
    spr_new = base + len(stub_bm)
    # Bitmap.buf slots (+8 of each 16-byte entry) held relocation words: no pixels remain, so 0
    for i in range(44):
        struct.pack_into(">I", stub_bm, 16 * i + 8, 0)
    # Sprite.bitmap slot (+0x34) becomes an internal relocation to the stub Bitmap table
    sprite_bitmap_slot = spr_new + 0x34
    new_len = align16(base + len(stub_bm) + len(stub_spr))
    payload.extend(stub_bm)
    payload.extend(stub_spr)
    payload.extend(bytes(new_len - len(payload)))
    new_internal = internal + [(wall_slot, spr_new), (sprite_bitmap_slot, bm_new)]
    ih2 = write_chain(payload, new_internal)
    eh2 = write_chain(payload, new_external)
    # original wallpaper slot word held an EXTERNAL chain word; write_chain rewrote it as internal
    out = bytearray(hdr)
    out += struct.pack("<IHHI", fid, ih2, eh2, len(new_ids))
    out += struct.pack(f"<{len(new_ids)}H", *new_ids)
    out += struct.pack("<I", len(payload))
    out += payload
    return bytes(out), dict(bm_off=bm_new, spr_off=spr_new, size=len(payload), dropped=wp.data_size)


def main():
    OUT.mkdir(exist_ok=True)
    idx = o.index()
    report = []
    for label, key, gname, gkind in rd.STAGES:
        mid = rd.map_id(key)
        tree = o.tree(mid)
        wall = [f for f in tree if rd.role_of(f, mid) == "wallpaper"][0]
        blob, info = build_variant(mid, wall.file_id)
        p = OUT / idx[mid].path.name
        p.write_bytes(blob)
        # re-parse with the generator's own parser
        import generate_nds_native_stage as g
        spec = g.InputSpec(str(p.relative_to(o.REPO)).replace("\\", "/") if p.is_relative_to(o.REPO) else str(p),
                           hashlib.sha256(blob).hexdigest())
        res = g.load_o2r(Path("/"), spec) if False else g.load_o2r(o.REPO, g.InputSpec(
            str(p.relative_to(o.REPO)).replace("\\", "/"), hashlib.sha256(blob).hexdigest()))
        orig = idx[mid]
        ok_ext = len(res.external) == len(orig.external) - 1
        ok_int = len(res.internal) == len(orig.internal) + 2
        # new extern tree membership and alloc
        ids = [x for x in orig.extern_ids]
        # rebuild tree with the variant's extern ids
        seen = {mid}
        order = [("map", info["size"])]

        def rec(fid):
            if fid in seen:
                return
            seen.add(fid)
            f = idx[fid]
            order.append((f.rel, f.aligned))
            for e in f.extern_ids:
                rec(e)

        raw = blob
        _, _, _, _, vids, _ = parse_raw(raw)
        for e in vids:
            rec(e)
        alloc_new = 0
        for i, (nm, sz) in enumerate(order):
            alloc_new = sz if i == 0 else align16(alloc_new) + sz
        alloc_old = o.tree_alloc(tree)
        # the wallpaper slot now resolves inside the file, to the stub Sprite
        wpslot = 0x14 + 0x48
        tgt = res.internal.get(wpslot)
        sprite_ok = tgt is not None and tgt.offset == info["spr_off"]
        bm_ptr = res.internal.get(info["spr_off"] + 0x34)
        bm_ok = bm_ptr is not None and bm_ptr.offset == info["bm_off"]
        report.append({
            "stage": label, "map_file": orig.path.name, "orig_size": orig.data_size, "variant_size": info["size"],
            "external_fixups": [len(orig.external), len(res.external)],
            "internal_fixups": [len(orig.internal), len(res.internal)],
            "extern_ids": [orig.extern_ids, vids],
            "wallpaper_in_tree": any(f == wall.file_id for f in vids),
            "alloc_old": alloc_old, "alloc_new": alloc_new, "saved": alloc_old - alloc_new,
            "checks": {"ext_minus_1": ok_ext, "int_plus_2": ok_int, "wallpaper_slot_to_stub_sprite": sprite_ok,
                       "stub_sprite_bitmap_ptr": bm_ok},
            "stub": info,
        })
    (Path(__file__).resolve().parents[1] / "lane1_variant_maps.json").write_text(json.dumps(report, indent=1))
    for r in report:
        ck = all(r["checks"].values()) and not r["wallpaper_in_tree"]
        print(f"{r['stage']:18s} {r['map_file']:14s} {r['orig_size']:4d} -> {r['variant_size']:4d}  ext {r['external_fixups']}  "
              f"int {r['internal_fixups']}  alloc {r['alloc_old']:,} -> {r['alloc_new']:,} (saved {r['saved']:,})  "
              f"{'OK' if ck else 'FAIL'}")


if __name__ == "__main__":
    main()
