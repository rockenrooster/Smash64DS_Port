#!/usr/bin/env python3
"""Lane 2: byte classification of the IFCommon sprite files.

Every IFCommon file except IFCommonPlayer is a "sprite file": a run of
  [pixel payloads][Bitmap[n]][Sprite]  (+ optional LUT)
units.  Sprite headers are found from the internal fixup table (the slot at
Sprite+52 is Sprite.bitmap; slot at +32 is Sprite.LUT) and validated against
their own fields.  Classes reported (bytes of the O2R payload):

  sprite_header   68 B per Sprite (decomp include/PR/sp.h `struct sprite`)
  bitmap_array    16 B per Bitmap (`struct bitmap`)
  pixels          sum of width_img * actualHeight * bpp per Bitmap piece
  palette         nTLUT * 2 (Sprite.LUT target)
  pad_or_other    every payload byte not claimed above (inter-piece gaps,
                  alignment, anything the parser did not reach)

Nothing here uses the runtime; it only reads decomp/BattleShip-main/BattleShip_o2r.
"""
from __future__ import annotations

import json
import sys
from collections import OrderedDict

import lane2_o2r as o2r

SPRITE_BYTES = 68
BITMAP_BYTES = 16

BPP_BITS = {0: 4, 1: 8, 2: 16, 3: 32}  # G_IM_SIZ_4b/8b/16b/32b
FMT_NAMES = {0: "RGBA", 1: "YUV", 2: "CI", 3: "IA", 4: "I"}


def find_sprites(f: o2r.O2R):
    """Return list of dicts, one per Sprite header, sorted by offset."""
    slots = set(f.internal)
    sprites = []
    for slot, target in sorted(f.internal.items()):
        p = slot - 52
        if p < 0 or p + SPRITE_BYTES > f.data_size:
            continue
        nbitmaps = f.s16(p + 40)
        bmfmt = f.payload[p + 48]
        bmsiz = f.payload[p + 49]
        if not (1 <= nbitmaps <= 64) or bmsiz > 3 or bmfmt > 4:
            continue
        # Sprite.bitmap must point at a run of nbitmaps Bitmaps ending at the
        # Sprite (this extractor always lays them out contiguously).
        contiguous = (target + nbitmaps * BITMAP_BYTES == p)
        if not contiguous:
            continue  # slot inside a Bitmap array, not a Sprite.bitmap slot
        sprites.append({
            "offset": p,
            "bitmap_off": target,
            "contiguous_bitmaps": contiguous,
            "width": f.s16(p + 4),
            "height": f.s16(p + 6),
            "attr": f.be16(p + 20),
            "nTLUT": f.s16(p + 30),
            "lut_off": f.internal.get(p + 32),
            "nbitmaps": nbitmaps,
            "bmfmt": bmfmt,
            "bmsiz": bmsiz,
        })
    # de-duplicate (a slot near another sprite can alias): keep first per offset
    out = OrderedDict()
    for s in sprites:
        out.setdefault(s["offset"], s)
    return list(out.values())


def analyse(f: o2r.O2R):
    sprites = find_sprites(f)
    claimed = bytearray(f.data_size)  # 0 unclaimed, else class id
    CLS = {1: "sprite_header", 2: "bitmap_array", 3: "pixels", 4: "palette"}

    def claim(a, n, c):
        for i in range(a, min(a + n, f.data_size)):
            claimed[i] = c

    rows = []
    for s in sprites:
        p = s["offset"]
        claim(p, SPRITE_BYTES, 1)
        claim(s["bitmap_off"], s["nbitmaps"] * BITMAP_BYTES, 2)
        pix_total = 0
        pieces = []
        for b in range(s["nbitmaps"]):
            bo = s["bitmap_off"] + b * BITMAP_BYTES
            w_img = f.s16(bo + 2)
            hgt = f.s16(bo + 12)
            buf = f.internal.get(bo + 8)
            bits = BPP_BITS[s["bmsiz"]]
            nbytes = (w_img * hgt * bits + 7) // 8
            if buf is not None and nbytes > 0:
                claim(buf, nbytes, 3)
                pix_total += nbytes
                pieces.append((buf, nbytes, w_img, hgt))
        pal = 0
        if s["lut_off"] is not None and s["nTLUT"] > 0:
            pal = s["nTLUT"] * 2
            claim(s["lut_off"], pal, 4)
        rows.append({**s, "pixel_bytes": pix_total, "palette_bytes": pal,
                     "pieces": pieces,
                     "fmt": FMT_NAMES.get(s["bmfmt"], str(s["bmfmt"])),
                     "bpp": BPP_BITS[s["bmsiz"]]})
    totals = {"sprite_header": 0, "bitmap_array": 0, "pixels": 0,
              "palette": 0, "pad_or_other": 0}
    for c in claimed:
        totals[CLS.get(c, "pad_or_other")] += 1
    return rows, totals, claimed


def main():
    names = sys.argv[1:] or [
        "IFCommonPlayerDamage", "IFCommonTimer", "IFCommonDigits",
        "IFCommonBattlePause", "IFCommonPlayerTags", "IFCommonAnnounceCommon",
        "IFCommonGameStatus", "IFCommonItem"]
    out = {}
    for n in names:
        f = o2r.by_name(n)
        rows, totals, claimed = analyse(f)
        out[n] = {"fid": f.file_id, "payload": f.data_size,
                  "sprites": len(rows), "totals": totals,
                  "sum": sum(totals.values())}
        print(f"{n} fid={f.file_id} payload={f.data_size} sprites={len(rows)} "
              f"{totals} (sum {sum(totals.values())})")
    print(json.dumps(out, indent=1))


if __name__ == "__main__":
    main()


# ---------------------------------------------------------------------------
# IFCommonGameStatus compact-image accounting (NDS_IF_GAMESTATUS_COMPACT).
# The asset table is parsed from src/nds/nds_ifcommon_oam.c so the set of
# assets that own OBJ tiles (whose pixels are baked and dropped) is the code's,
# not an assumption.
# ---------------------------------------------------------------------------
import re as _re


def gamestatus_compact():
    src = (o2r.REPO / "src/nds/nds_ifcommon_oam.c").read_text(encoding="utf-8", errors="replace")
    m = _re.search(r"sNdsIFCommonAssetSpecs\[\s*NDS_IFCOMMON_ASSET_COUNT\s*\]\s*=\s*\{(.*?)\n\};", src, _re.S)
    body = m.group(1)
    specs = []
    num = r"(0x[0-9a-fA-F]+|\d+)"
    for e in _re.finditer(r"\{\s*(0x[0-9a-fA-F]+)u,\s*" + r",\s*".join([num] * 7) + r",", body):
        specs.append((int(e.group(1), 16), int(e.group(8), 0)))
    f = o2r.by_name("IFCommonGameStatus")
    rows, totals, claimed = analyse(f)
    by_off = {r["offset"]: r for r in rows}
    # two logical assets (ShadowInitial/ShadowGo) share one Sprite: de-duplicate
    uniq = {}
    for off, tiles in specs:
        uniq[off] = max(uniq.get(off, 0), tiles)
    letters = [off for off, tiles in uniq.items() if tiles != 0]
    keepers = [off for off, tiles in uniq.items() if tiles == 0]
    dropped = sum(by_off[off]["pixel_bytes"] for off in letters)
    kept_pixels = sum(by_off[off]["pixel_bytes"] for off in keepers)
    image = f.data_size - dropped
    headers = 68 * len(uniq)
    bitmaps = sum(16 * by_off[off]["nbitmaps"] for off in uniq)
    return {
        "asset_rows": len(specs), "unique_sprites": len(uniq),
        "letter_sprites": len(letters), "keeper_sprites": len(keepers),
        "dropped_letter_pixel_bytes": dropped,
        "resident_image_bytes": image,
        "kept_pixel_bytes": kept_pixels,
        "sprite_headers": headers, "bitmap_arrays": bitmaps,
        "pad_or_other": image - kept_pixels - headers - bitmaps,
    }


def write_counts():
    import json
    from pathlib import Path
    out = {}
    for n in ("IFCommonPlayerDamage", "IFCommonTimer", "IFCommonDigits", "IFCommonBattlePause",
              "IFCommonPlayerTags", "IFCommonAnnounceCommon", "IFCommonGameStatus", "IFCommonItem"):
        rows, totals, _ = analyse(o2r.by_name(n))
        out[n] = len(rows)
    Path(__file__).with_name("lane2_sprites_counts.json").write_text(json.dumps(out, indent=1))
    return out
