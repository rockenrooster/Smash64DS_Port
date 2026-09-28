#!/usr/bin/env python3
"""Lane 2: post-load read model of the IFCommon files (VS battle, shipping
config: gNdsIFCommonHUDLowerTextMode = 1, NDS_IF_GAMESTATUS_COMPACT = 1).

Tags (bytes of each O2R payload; sum = payload):
  keep_headers       Sprite headers (68 B): copied into SObjs by source display
                     procs / SObj creation
  dead_bitmap_array  Bitmap arrays (16 B each): sprite.bitmap is copied as a
                     pointer only; nothing dereferences it for these files
  dead_pixels        pixel payloads with no DS reader in a VS battle
  setup_read_pixels  pixels read only while the interface is built (tag/arrow
                     bake, GameStatus letters, GameStatus atlases)
  pad_other          gaps between units
"""
from __future__ import annotations

import json
from pathlib import Path

import lane2_o2r as o2r
import lane2_sprites as sp

HERE = Path(__file__).resolve().parent

# reader facts (verified in source; cited in the report)
PIXEL_TAG = {
    "IFCommonPlayerDamage": "dead_pixels",   # lower HUD: nitro:/menus/battle_hud.bin
    "IFCommonTimer": "dead_pixels",          # same
    "IFCommonDigits": "dead_pixels",         # same
    "IFCommonBattlePause": "dead_pixels",    # no native owner; layered path records a native failure
    "IFCommonAnnounceCommon": "dead_pixels", # Sudden Death text: no native owner
    "IFCommonPlayerTags": "setup_read_pixels",  # ndsIFCommonNativeOamBakePlayerTag at setup
    "IFCommonItem": "setup_read_pixels",        # ndsIFCommonNativeOamBakeItemArrow at itManagerInitItems
}


def main():
    out = {}
    names = ["IFCommonPlayerDamage", "IFCommonTimer", "IFCommonDigits", "IFCommonBattlePause",
             "IFCommonPlayerTags", "IFCommonAnnounceCommon", "IFCommonItem"]
    for n in names:
        f = o2r.by_name(n)
        rows, totals, _ = sp.analyse(f)
        setup = PIXEL_TAG[n] == "setup_read_pixels"
        tags = {"keep_headers": totals["sprite_header"],
                PIXEL_TAG[n]: totals["pixels"],
                "pad_other": totals["pad_or_other"]}
        if setup:
            # the tag/arrow bakers key their cells by the Sprite.bitmap POINTER
            # (nds_ifcommon_oam.c BakePlayerTag: sNdsIFCommonPlayerTags[slot].bitmap ==
            # sprite->bitmap), so the Bitmap[] arrays stay addressable
            tags["keep_bitmap_arrays"] = totals["bitmap_array"]
        else:
            tags["dead_bitmap_array"] = totals["bitmap_array"]
        assert sum(tags.values()) == f.data_size
        out[n] = {"fid": f.file_id, "payload": f.data_size, "sprites": len(rows), "tags": tags}
    gs = sp.gamestatus_compact()
    out["IFCommonGameStatus"] = {
        "fid": 82, "payload": 152288, "compact": gs,
        "tags_resident_image": {
            "keep_headers": gs["sprite_headers"],
            "keep_bitmap_arrays": gs["bitmap_arrays"],
            "setup_read_pixels": gs["kept_pixel_bytes"],
            "pad_other": gs["pad_or_other"]},
        "already_dropped_letter_pixels": gs["dropped_letter_pixel_bytes"],
        # measured, not computed: artifacts/performance/2026-09-23_p2-2p8-if-gamestatus-compact/README.md
        "baked_end_streams_measured": 22104}
    out["IFCommonPlayer"] = {"fid": 166, "payload": 976, "note": "DObjDesc/DL/Vtx/anim + one IA8 image; not classified, kept"}
    (HERE / "lane2_ifcommon.json").write_text(json.dumps(out, indent=1))
    tot = {}
    for n, v in out.items():
        if "tags" in v:
            print(f"{n:24s} payload {v['payload']:6d} {v['tags']}")
    print("GameStatus resident image", out["IFCommonGameStatus"]["tags_resident_image"],
          "dropped", gs["dropped_letter_pixel_bytes"])


if __name__ == "__main__":
    main()
