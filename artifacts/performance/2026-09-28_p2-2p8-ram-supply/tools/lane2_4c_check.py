#!/usr/bin/env python3
"""Lane 2: does any sprite in the IFCommon files use G_IM_SIZ_4c (bmsiz == 4)?

lbCommonMakeSObjForGObj (src/port/sprite_preview_backend.c:133-139) runs
lbCommonDecodeSpriteBitmapsSiz4b when sprite->bmsiz == G_IM_SIZ_4c: that reads
and rewrites the sprite's pixel bytes in place when an SObj is made.  If any
file below had such sprites, its pixels would NOT be dead after load.  Scans
every internal-fixup slot with the sprite layout (bitmap ptr at +52, nbitmaps
s16 at +40, bmfmt/bmsiz bytes at +48/+49) WITHOUT the bmsiz<=3 filter that
lane2_sprites.py applies, requiring the contiguous Bitmap[] layout.
"""
from __future__ import annotations

import json
from collections import Counter

import lane2_o2r as o2r

SPRITE_BYTES, BITMAP_BYTES = 68, 16
NAMES = ["IFCommonPlayerDamage", "IFCommonTimer", "IFCommonDigits", "IFCommonBattlePause",
         "IFCommonPlayerTags", "IFCommonAnnounceCommon", "IFCommonGameStatus", "IFCommonItem"]


def main():
    out = {}
    for n in NAMES:
        f = o2r.by_name(n)
        dist = Counter()
        seen = set()
        for slot, target in sorted(f.internal.items()):
            p = slot - 52
            if p < 0 or p + SPRITE_BYTES > f.data_size or p in seen:
                continue
            nb = f.s16(p + 40)
            fmt, siz = f.payload[p + 48], f.payload[p + 49]
            if not (1 <= nb <= 64) or fmt > 4 or siz > 7:
                continue
            if target + nb * BITMAP_BYTES != p:
                continue
            seen.add(p)
            dist[siz] += 1
        out[n] = {"sprites": len(seen), "bmsiz_histogram": dict(sorted(dist.items())),
                  "sprites_4c": dist.get(4, 0)}
        print(f"{n:24s} sprites={len(seen):3d} bmsiz histogram={dict(sorted(dist.items()))} 4c={dist.get(4, 0)}")
    json.dump(out, open("lane2_4c_check.json", "w"), indent=1)


if __name__ == "__main__":
    main()
