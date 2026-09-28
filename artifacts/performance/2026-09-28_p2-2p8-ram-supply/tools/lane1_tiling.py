#!/usr/bin/env python3
"""Lane 1: class tiling of every resident stage file.

Sources per file kind
  typed geometry / images / actors banks : typed-source tiling (lane1_typed) -- exact,
      validated (total == payload, relocation slots only inside pointer-bearing fields)
  GR*Map files : MPGroundData@0x14 (0xA8), item weights (20 B), attribute area (rest)
  wallpaper containers (Stage*/MVOpeningRoomWallpaper) : Sprite + Bitmap table + pixel strips
"""
from __future__ import annotations

import re
from collections import defaultdict
from dataclasses import dataclass

import lane1_o2r as o
import lane1_typed as ty

TCLASSES = ["gfx", "vtx", "tex", "pal", "dobjdesc", "dltab", "mobj", "anim", "coll", "hdr", "attr",
            "sprite", "pixels", "other16", "pad"]


@dataclass
class Block:
    off: int
    size: int
    cls: str
    name: str
    ctype: str

    @property
    def end(self) -> int:
        return self.off + self.size


def _lut_refs(src: str) -> set:
    return set(re.findall(r"lut=([A-Za-z0-9_]+)", src))


def _tex_names(src: str) -> set:
    """Declarations annotated /* @tex ... */ immediately before them."""
    out = set()
    for m in re.finditer(r"/\*\s*@tex[^*]*\*/\s*(?:static\s+)?(?:u8|u16)\s+([A-Za-z0-9_]+)", src):
        out.add(m.group(1))
    return out


def classify_decl(d: ty.Decl, tex_names: set, lut_names: set) -> str:
    t = d.ctype
    if t in ("PAD", "ALIGN"):
        return "pad"
    if t == "Gfx":
        return "gfx"
    if t == "Vtx":
        return "vtx"
    if t == "DObjDesc":
        return "dobjdesc"
    if t == "DObjDLLink":
        return "dltab"
    if t in ("MObjSub", "MObjSub*", "MObjSub**", "MObjSub***"):
        return "mobj"
    if t in ("MPVertexData", "MPVertexLinks", "MPLineInfo", "MPMapObjData", "MPGeometryData"):
        return "coll"
    if t in ("MPGroundData", "MPItemWeights"):
        return "hdr"
    if t == "u16" and "MPVertexArray" in d.name:
        return "coll"
    if t == "u8":
        if d.name in tex_names or re.search(r"Tex_|_Tex", d.name):
            return "tex"
        if "_pad_" in d.name:
            return "pad"
        return "tex" if d.name in tex_names else "anim"     # remaining u8: none in stage files (checked)
    if t == "u16":
        if d.name in lut_names or re.search(r"palette|Lut|LUT", d.name) or d.size in (32, 512):
            return "pal"
        return "other16"
    if t == "u8*":
        return "mobj"          # sprite pointer arrays (MObjSub.sprites)
    if t in ("AObjEvent32*", "AObjEvent32**", "AObjEvent32***"):
        return "anim"
    if t in ("u32", "f32", "Vec3f", "SYInterpDesc", "GRSectorDesc", "void*", "void**", "s32"):
        return "anim"
    return "anim"


_CACHE = {}


def tile_typed(fid: int):
    if fid in _CACHE:
        return _CACHE[fid]
    decls, probs = ty.parse_file(fid)
    if decls is None:
        return None
    import glob
    g = glob.glob(str(ty.RELOC_SRC / f"{fid}_*.c"))
    src = open(g[0], encoding="utf-8", errors="replace").read()
    tex_names = _tex_names(src)
    lut_names = _lut_refs(src)
    blocks = [Block(d.offset, d.size, classify_decl(d, tex_names, lut_names), d.name, d.ctype) for d in decls]
    size = o.index()[fid].data_size
    end = blocks[-1].end if blocks else 0
    if end < size:
        blocks.append(Block(end, size - end, "pad", "tail-align", "ALIGN"))
    _CACHE[fid] = (blocks, probs)
    return _CACHE[fid]


def tile_map(fid: int, header_off: int = 0x14):
    f = o.index()[fid]
    n = f.data_size
    blocks = []
    hdr_end = header_off + 0xA8
    # item weights: internal slot at header+0x84
    iw = f.internal.get(header_off + 0x84)
    iw_off = iw.offset if iw else None
    used = []
    used.append((header_off, hdr_end, "hdr", "MPGroundData"))
    if iw_off is not None:
        used.append((iw_off, iw_off + 20, "hdr", "item_weights"))
    used.sort()
    cur = 0
    for s, e, c, nm in used:
        if s > cur:
            blocks.append(Block(cur, s - cur, "attr", "map-attr", "?"))
        blocks.append(Block(s, e - s, c, nm, "?"))
        cur = max(cur, e)
    if cur < n:
        # everything after the header: item / weapon attribute structs and alignment tail
        tail = f.payload[cur:n]
        if not any(tail):
            blocks.append(Block(cur, n - cur, "pad", "tail-align", "ALIGN"))
        else:
            blocks.append(Block(cur, n - cur, "attr", "map-attr", "?"))
    return blocks


def tile_wallpaper(fid: int):
    f = o.index()[fid]
    n = f.data_size
    SPR = 0x26C88
    BMT = 0x269C8
    blocks = []
    # pixel strips: [0, BMT): strips + 8-byte inter-strip padding
    blocks.append(Block(0, BMT, "pixels", "strips", "?"))
    blocks.append(Block(BMT, SPR - BMT, "sprite", "Bitmap[44]", "Bitmap"))
    blocks.append(Block(SPR, n - SPR, "sprite", "Sprite", "Sprite"))
    return blocks


def tile(fid: int, role: str):
    if role == "wallpaper":
        return tile_wallpaper(fid), []
    if role == "map":
        return tile_map(fid), []
    return tile_typed(fid)


def class_bytes(blocks) -> dict:
    d = defaultdict(int)
    for b in blocks:
        d[b.cls] += b.size
    return dict(d)


if __name__ == "__main__":
    import sys
    for fid in [int(x) for x in sys.argv[1:]] or [104]:
        blocks, probs = tile_typed(fid)
        print(fid, class_bytes(blocks), probs[:2])
