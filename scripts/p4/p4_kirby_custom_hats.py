#!/usr/bin/env python3
"""P4: the hats of Kirby's custom powers (p4_kirby.py CUSTOM_POWERS).

The source has none for these contents (EXTRA disables their copy; the owner
asked for Brawl-style copies, 2026-10-12). Each hat is one display list in a
file of its own, drawn at Kirby's joint 6 over his plain head as a P4 sidecar
root (renderer_adapter_fighter.c ndsFighterDrawKirbyHatSidecar), the way the
powers' weapons ride joint 17. Two kinds:

  "spans"  a joint's list from the content's model (the detail named): the
           material spans named are kept, every segment-E material call is
           replaced by the commands its MObj writes for costume 0
           (gcDrawMObjForDObj: the palette image; with the alpha flag also
           the TLUT load and the sprite image), and the vertices are scaled
           and moved onto Kirby's head.
  "band"   a ring around Kirby's head between two heights, with tails at the
           back, generated, drawn with the material of one of the content's
           spans (the band takes texel rows of its texture).
  "part"   a model part of the content (FTModelPartDesc: joint, part,
           detail) whole, its own inline material: Meta Knight's Mach
           Tornado, drawn around Kirby while he spins (BUGS.md 2026-10-12).
           The content generator's render deviations for those bytes come
           along ("vertex_tint", "ia_intensity_max", "combine": its
           OWNER_VERTEX_TINT, OWNER_IA_INTENSITY_MAX and the combine the
           content's native owner aliases).

A file holds one list per spec (build_lists). The vertices, texels and
palettes are copied into the file, so its pointers are internal relocations
only. Build output: derived from the user's ROM.
"""
from __future__ import annotations

import math
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import remix_rom as R  # noqa: E402
import ft_layout  # noqa: E402

DOBJ_DESC_SIZE = 0x2C
MODELPART_SIZE = 20          # FTModelPart: dl, three pointers, flags
JOINT_COMMON_START = 4       # nFTPartsJointCommonStart
NO_RELOC = 0xFFFF
TRIANGLE_OPS = (0x05, 0x06, 0x07)
OP_VTX, OP_DL, OP_ENDDL, OP_SETTIMG = 0x01, 0xDE, 0xDF, 0xFD
OP_LOADBLOCK, OP_LOADTLUT, OP_SETTILESIZE = 0xF3, 0xF0, 0xF2
# The commands that belong to one span's texture (dropped with the span).
TEXTURE_STATE = (OP_SETTIMG, OP_LOADBLOCK, OP_LOADTLUT, 0xF5, OP_SETTILESIZE, 0xD7,
                 0xE6, 0xE8)
SEGMENT_E = 0x0E
MOBJ_PALETTE, MOBJ_ALPHA = 0x4, 0x1
SIZ_BYTES = {0: 0.5, 1: 1, 2: 2, 3: 4}
# G_GEOMETRYMODE clearing G_CULL_FRONT | G_CULL_BACK: the band is thin, both
# faces show.
CLEAR_CULL = (0xD9FFF9FF, 0x00000000)


class HatError(SystemExit):
    pass


class _Files:
    """The content's main and model files: bytes and pointer slots."""

    def __init__(self, rom: R.Rom, ids):
        self.data = {fid: rom.file_bytes(fid) for fid in ids}
        self.slots = {fid: rom.reloc_slots(fid) for fid in ids}

    def ptr(self, fid: int, off: int):
        """(file, offset) a pointer slot names, None for no pointer."""
        s = self.slots[fid].get(off)
        if s is None:
            return None
        if s[0] == "intern":
            return fid, s[1]
        if s[1] not in self.data:
            raise HatError(f"pointer at {fid:#x}+{off:#x} leaves the content's files")
        return s[1], s[2]


def _joint_list(files: _Files, facts: dict, joint: int, detail: int):
    lay = ft_layout.layout()
    main, model = facts["main"], facts["model"]
    cont = files.ptr(main, facts["attributes"] + lay["FTAttributes.commonparts_container"])
    part = cont[1] + detail * lay["sizeof(FTCommonPart)"]
    if files.data[cont[0]][part + lay["FTCommonPart.flags"]] & 1:
        raise HatError("the content's lists are DL pairs (Yoshi's form)")
    tree = files.ptr(cont[0], part + lay["FTCommonPart.dobjdesc"])
    dl = files.ptr(tree[0], tree[1] + joint * DOBJ_DESC_SIZE + 4)
    if dl is None or dl[0] != model:
        raise HatError(f"joint {joint} draws no list of the model file")
    mobjs = []
    table = files.ptr(cont[0], part + lay["FTCommonPart.p_mobjsubs"])
    rows = files.ptr(table[0], table[1] + 4 * joint) if table else None
    k = 0
    while rows is not None:
        sub = files.ptr(rows[0], rows[1] + 4 * k)
        if sub is None:
            break
        sf, so = sub
        data = files.data[sf]
        pal = files.ptr(sf, so + 0x2C)
        spr = files.ptr(sf, so + 0x04)
        mobjs.append({"flags": struct.unpack_from(">H", data, so + 0x30)[0],
                      "fmt": data[so + 2], "siz": data[so + 3],
                      "palette": files.ptr(*pal) if pal else None,
                      "sprite": files.ptr(*spr) if spr else None})
        k += 1
    return dl[1], mobjs


def _part_list(files: _Files, facts: dict, joint: int, part: int, detail: int):
    """A model part's list (FTModelPartContainer -> FTModelPartDesc ->
    modelparts[part][detail]); its material is inline, so no MObjs."""
    lay = ft_layout.layout()
    main, model = facts["main"], facts["model"]
    cont = files.ptr(main, facts["attributes"] + lay["FTAttributes.modelparts_container"])
    if cont is None:
        raise HatError("the content has no model parts")
    desc = files.ptr(cont[0], cont[1] + 4 * (joint - JOINT_COMMON_START))
    if desc is None:
        raise HatError(f"joint {joint} has no model parts")
    row = desc[1] + (part * 2 + detail) * MODELPART_SIZE
    dl = files.ptr(desc[0], row)
    if dl is None or dl[0] != model:
        raise HatError(f"joint {joint} part {part} draws no list of the model file")
    if files.ptr(desc[0], row + 4) is not None:
        raise HatError(f"joint {joint} part {part} has MObjs (only inline materials are lowered)")
    return dl[1], []


def _material(mobj: dict, model: int) -> list:
    """gcDrawMObjForDObj's commands for one MObj (costume 0, frame 0)."""
    if mobj["flags"] & ~(MOBJ_PALETTE | MOBJ_ALPHA):
        raise HatError(f"MObj flags {mobj['flags']:#x}: only palette and alpha are lowered")
    out = []
    if mobj["flags"] & MOBJ_PALETTE:
        pal = mobj["palette"]
        if pal is None or pal[0] != model:
            raise HatError("palette outside the model file")
        out.append((0xFD100000, ("tex", pal[1], 32 if mobj["siz"] == 0 else 512)))
        if mobj["flags"] & MOBJ_ALPHA:
            count = 0xFF if mobj["siz"] == 1 else 0xF
            out += [(0xE8000000, 0), (0xF5000100, 0x05000000), (0xE6000000, 0),
                    (0xF0000000, 0x05000000 | (count << 14)), (0xE7000000, 0)]
    if mobj["flags"] & MOBJ_ALPHA:
        spr = mobj["sprite"]
        if spr is None or spr[0] != model:
            raise HatError("sprite outside the model file")
        out.append((0xFD000000 | (mobj["fmt"] << 21) | (mobj["siz"] << 19), ("pending", spr[1])))
    return out


def _source_commands(data: bytes, off: int) -> list:
    cmds = []
    while True:
        if off + 8 > len(data):
            raise HatError(f"list at {off:#x} runs off its file")
        w0, w1 = struct.unpack_from(">II", data, off)
        cmds.append((off, w0 >> 24, w0, w1))
        if (w0 >> 24) == OP_ENDDL:
            return cmds
        off += 8


def _blocks(cmds):
    """The list cut into spans: each run of state commands with the vertex
    loads and triangles after it, and the material its call names (or the
    one before it, when it calls none)."""
    blocks, cur, drew, mat = [], [], False, None
    for c in cmds:
        _pc, op, w0, w1 = c
        if drew and op not in TRIANGLE_OPS and op != OP_VTX:
            blocks.append((mat, cur))
            cur, drew = [], False
        if op == OP_DL:
            if (w1 >> 24) != SEGMENT_E or ((w0 >> 16) & 0xFF) != 0:
                raise HatError(f"call at {c[0]:#x} is not a material list")
            mat = (w1 & 0xFFFFFF) // 8
        if op in TRIANGLE_OPS:
            drew = True
        cur.append(c)
    blocks.append((mat, cur))
    return blocks


def _keep_spans(files, model, cmds, mobjs, keep, stop_at_vertices=False):
    """The list's commands with only the named materials' spans: the texture
    state and geometry of the others dropped (their other state kept), every
    kept material call inlined. Pending images are sized by the load that
    reads them. With stop_at_vertices, the commands up to the first kept
    span's first vertex load."""
    out = []
    for mat, block in _blocks(cmds):
        keep_now = mat in keep
        for pc, op, w0, w1 in block:
            if op == OP_DL:
                if keep_now:
                    out += _material(mobjs[mat], model)
                continue
            if not keep_now and (op in TEXTURE_STATE or op == OP_VTX or op in TRIANGLE_OPS):
                continue
            if op == OP_VTX:
                if stop_at_vertices:
                    return out
                s = files.ptr(model, pc + 4)
                n = (w0 >> 12) & 0xFF
                out.append((w0, ("vtx", s[1], 16 * n)))
                continue
            if op == OP_ENDDL:
                break
            _keep_command(files, model, out, pc, op, w0, w1)
    if stop_at_vertices:
        raise HatError("no span of the named material loads vertices")
    return out


def _keep_command(files, model, out, pc, op, w0, w1):
    """Appends one kept command: triangles split into G_TRI1s, an image
    pending until the load that reads it gives its size."""
    if op in TRIANGLE_OPS:
        tris = [w0 & 0xFFFFFF] if op == 0x05 else [w0 & 0xFFFFFF, w1 & 0xFFFFFF]
        out.extend(((0x05 << 24) | t, 0) for t in tris)
        return
    if op == OP_SETTIMG:
        s = files.ptr(model, pc + 4)
        out.append((w0, ("pending", s[1])))
        return
    if op in (OP_LOADBLOCK, OP_LOADTLUT):
        for i in range(len(out) - 1, -1, -1):
            if isinstance(out[i][1], tuple) and out[i][1][0] == "pending":
                if op == OP_LOADBLOCK:
                    siz = (out[i][0] >> 19) & 3
                    size = int((((w1 >> 12) & 0xFFF) + 1) * SIZ_BYTES[siz] + 0.5)
                else:
                    size = (((w1 >> 14) & 0x3FF) + 1) * 2
                out[i] = (out[i][0], ("tex", out[i][1][1], size))
                break
    out.append((w0, w1))


def _vertex(x, y, z, s, t, n):
    return struct.pack(">hhhhhhbbbB", int(round(x)), int(round(y)), int(round(z)), 0,
                       int(round(s * 32)), int(round(t * 32)),
                       int(round(n[0] * 127)), int(round(n[1] * 127)), int(round(n[2] * 127)), 255)


def _unit(v):
    m = math.sqrt(sum(c * c for c in v)) or 1.0
    return [c / m for c in v]


def _band_geometry(spec: dict):
    """The ring's and tails' vertices (packed) and triangles (index lists)."""
    seg = spec["segments"]
    (y0, y1), (r0, r1) = spec["y"], spec["radius"]
    (t0, t1), width = spec["texel_rows"], spec["texels_around"]
    slope = (r0 - r1) / float(y1 - y0)
    ring = []
    for row, (y, r, t) in enumerate(((y1, r1, t0), (y0, r0, t1))):
        for c in range(seg + 1):
            a = 2.0 * math.pi * c / seg
            ring.append(_vertex(r * math.sin(a), y, r * math.cos(a), width * c / seg, t,
                                _unit((math.sin(a), slope, math.cos(a)))))
    tris = []
    for c in range(seg):
        top, bot = c, seg + 1 + c
        tris.append((top, bot, bot + 1))
        tris.append((top, bot + 1, top + 1))
    tails, tail_tris = [], []
    for quad in spec.get("tails", ()):
        base = len(tails)
        n = _unit((0.0, 0.3, -1.0))
        for corner, (s, t) in zip(quad, ((0.0, t0), (8.0, t0), (8.0, t1), (0.0, t1))):
            tails.append(_vertex(corner[0], corner[1], corner[2], s, t, n))
        tail_tris += [(base, base + 3, base + 2), (base, base + 2, base + 1)]
    return [ring, tails], [tris, tail_tris]


def _list_commands(files: _Files, facts: dict, spec: dict, index: int) -> list:
    """One spec's commands, ending in G_ENDDL. Payload keys carry the list's
    index, so each list transforms its own copy of what it loads."""
    model = facts["model"]
    if spec["kind"] == "part":
        dl, mobjs = _part_list(files, facts, spec["joint"], spec["part"], spec["detail"])
    else:
        dl, mobjs = _joint_list(files, facts, spec["joint"], spec["detail"])
    cmds = _source_commands(files.data[model], dl)
    generated = []
    if spec["kind"] in ("spans", "part"):
        keep = set(spec["materials"]) if spec["kind"] == "spans" else {None}
        out = _keep_spans(files, model, cmds, mobjs, keep)
    elif spec["kind"] == "band":
        out = _keep_spans(files, model, cmds, mobjs, {spec["material"]}, stop_at_vertices=True)
        out.append(CLEAR_CULL)
        for verts, tris in zip(*_band_geometry(spec)):
            if not verts:
                continue
            n = len(verts)
            generated.append(((0x01 << 24) | (n << 12) | ((n * 2) & 0xFF),
                              ("gen", index, len(generated), b"".join(verts))))
            generated += [((0x05 << 24) | (a * 2 << 16) | (b * 2 << 8) | (c * 2), 0)
                          for a, b, c in tris]
    else:
        raise HatError(f"hat kind {spec['kind']}")
    commands = []
    for w0, w1 in out + generated:
        if isinstance(w1, tuple) and w1[0] == "pending":
            raise HatError(f"image {w1[1]:#x} has no load")
        if (w0 >> 24) == 0xFC and "combine" in spec:
            w0, w1 = spec["combine"]
        if isinstance(w1, tuple) and w1[0] in ("vtx", "tex"):
            w1 = w1 + (index,)
        commands.append((w0, w1))
    return commands + [(0xDF000000, 0)]


def _payload(src: bytes, spec: dict, key: tuple) -> bytes:
    """The bytes one payload key places: generated vertices, or copied ones
    scaled, moved and tinted, or copied texels (IA intensity raised)."""
    if key[0] == "gen":
        return key[3]
    raw = bytearray(src[key[1]:key[1] + key[2]])
    if key[0] == "vtx":
        scale, offset = spec.get("scale", 1.0), spec.get("offset", (0, 0, 0))
        sc = scale if isinstance(scale, (tuple, list)) else (scale,) * 3
        tint = spec.get("vertex_tint")
        for v in range(len(raw) // 16):
            p = struct.unpack_from(">hhh", raw, 16 * v)
            q = [int(round(p[a] * sc[a] + offset[a])) for a in range(3)]
            if any(abs(c) > 0x7FF for c in q):
                raise HatError(f"vertex {q} past the VERTEX16 lattice")
            struct.pack_into(">hhh", raw, 16 * v, *q)
            if tint is not None:
                for c in range(3):
                    raw[16 * v + 12 + c] = (raw[16 * v + 12 + c] * tint[c] + 127) // 255
    elif spec.get("ia_intensity_max"):
        for i in range(len(raw)):
            raw[i] |= 0xF0
    return bytes(raw)


def build_lists(rom: R.Rom, facts: dict, specs, file_id: int) -> tuple[bytes, list]:
    """One file (O2R container) with a list per spec, and the lists' offsets
    in the file: the lists, then the vertices, texels and palettes they
    load, each 8-aligned; every slot an internal relocation."""
    files = _Files(rom, (facts["main"], facts["model"]))
    src = files.data[facts["model"]]
    commands, offsets = [], []
    for index, spec in enumerate(specs):
        offsets.append(8 * len(commands))
        commands += [(w0, w1, index) for w0, w1 in _list_commands(files, facts, spec, index)]
    body = bytearray(8 * len(commands))
    placed = {}
    relocs = []
    for i, (w0, w1, index) in enumerate(commands):
        if isinstance(w1, tuple):
            if w1 not in placed:
                while len(body) % 8:
                    body.append(0)
                placed[w1] = len(body)
                body += _payload(src, specs[index], w1)
            relocs.append(8 * i + 4)
            w1 = placed[w1]
        struct.pack_into(">II", body, 8 * i, w0, w1)
    if len(body) > 0x40000:
        raise HatError("hat past the relocation word range")
    first, prev = NO_RELOC, None
    for slot in relocs:
        target = struct.unpack_from(">I", body, slot)[0]
        struct.pack_into(">I", body, slot, (NO_RELOC << 16) | (target >> 2))
        if prev is None:
            first = slot // 4
        else:
            word = struct.unpack_from(">I", body, prev)[0]
            struct.pack_into(">I", body, prev, ((slot // 4) << 16) | (word & 0xFFFF))
        prev = slot
    out_blob = bytearray(R.O2R_HEADER)
    out_blob += struct.pack("<IHHI", file_id, first, NO_RELOC, 0)
    out_blob += struct.pack("<I", len(body))
    out_blob += body
    return bytes(out_blob), offsets


def build(rom: R.Rom, facts: dict, spec: dict, file_id: int) -> tuple[bytes, int]:
    """The hat's file (O2R container) and its list's offset in the file."""
    blob, offsets = build_lists(rom, facts, [spec], file_id)
    return blob, offsets[0]
