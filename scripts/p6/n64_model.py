#!/usr/bin/env python3
"""SSB64 fighter models (P6 A2): the reloc container, display lists,
JointTrees, vertices and materials, read and written.

A content's model is two files of its export (`o2r/<id>`): the main file,
whose FTAttributes name the parts (FTCommonPart per detail: the JointTree,
the MObjSub lists, the costume matanims; the model-part rows; the texture
parts), and the model file holding the JointTrees, display lists, vertices,
textures and palettes they point at. Both are O2R resources: a 0x40-byte
resource header, the file id, the heads of the internal and external pointer
chains, the external file ids and the big-endian body. A pointer slot's word
holds the next slot of its chain (high half, in words) and its target (low
half, in words); an external slot's file is the next unused external id.

`RelocFile` reads and re-serializes that container from its pointer maps;
`walk_dl` decodes a display list into `Gfx` commands and `encode_dl` writes
them back; `FighterModel` reads both JointTrees and resolves every triangle's
corners through the F3DEX2 vertex cache as the RSP sees them (a joint's
triangles may name vertices its parent's list loaded: the cross-joint seams).
`census` counts triangles and vertices per joint and detail, and `f1` is the
round-trip proof of P6_PLAN.md's A2 (F1).

The parsing the owner generator and the hat scripts each carry stays where it
is until those tools move onto this module; F1 keeps both readings honest.
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
from dataclasses import dataclass, field
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(REPO / "scripts" / "p4"))
import _paths  # noqa: E402,F401
import ft_layout  # noqa: E402

O2R_HEADER_SIZE = 0x40
NO_RELOC = 0xFFFF

# F3DEX2 opcodes the fighter models use.
G_VTX = 0x01
G_TRI1 = 0x05
G_TRI2 = 0x06
G_QUAD = 0x07
G_TEXTURE = 0xD7
G_POPMTX = 0xD8
G_GEOMETRYMODE = 0xD9
G_MTX = 0xDA
G_MOVEWORD = 0xDB
G_MOVEMEM = 0xDC
G_DL = 0xDE
G_ENDDL = 0xDF
G_SETOTHERMODE_L = 0xE2
G_SETOTHERMODE_H = 0xE3
G_RDPLOADSYNC = 0xE6
G_RDPPIPESYNC = 0xE7
G_RDPTILESYNC = 0xE8
G_LOADTLUT = 0xF0
G_SETTILESIZE = 0xF2
G_LOADBLOCK = 0xF3
G_LOADTILE = 0xF4
G_SETTILE = 0xF5
G_SETPRIMCOLOR = 0xFA
G_SETENVCOLOR = 0xFB
G_SETCOMBINE = 0xFC
G_SETTIMG = 0xFD

OP_NAMES = {
    G_VTX: "VTX", G_TRI1: "TRI1", G_TRI2: "TRI2", G_QUAD: "QUAD",
    G_TEXTURE: "TEXTURE", G_POPMTX: "POPMTX", G_GEOMETRYMODE: "GEOMETRYMODE",
    G_MTX: "MTX", G_MOVEWORD: "MOVEWORD", G_MOVEMEM: "MOVEMEM", G_DL: "DL",
    G_ENDDL: "ENDDL", G_SETOTHERMODE_L: "SETOTHERMODE_L",
    G_SETOTHERMODE_H: "SETOTHERMODE_H", G_RDPLOADSYNC: "LOADSYNC",
    G_RDPPIPESYNC: "PIPESYNC", G_RDPTILESYNC: "TILESYNC", G_LOADTLUT: "LOADTLUT",
    G_SETTILESIZE: "SETTILESIZE", G_LOADBLOCK: "LOADBLOCK", G_LOADTILE: "LOADTILE",
    G_SETTILE: "SETTILE", G_SETPRIMCOLOR: "SETPRIMCOLOR",
    G_SETENVCOLOR: "SETENVCOLOR", G_SETCOMBINE: "SETCOMBINE", G_SETTIMG: "SETTIMG",
}
SEGMENT_E = 0x0E
VERTEX_CACHE = 32         # F3DEX2's vertex buffer
VERTEX_SIZE = 16          # Vtx: s16 xyz, u16 flag, s16 st, u8 rgba (or normals)
DOBJ_DESC_SIZE = 44       # DObjDesc: u32 id, Gfx *dl, Vec3f translate, rotate, scale
JOINT_TREE_END = 18       # DObjDesc.id of the sentinel row
SIZ_BYTES = {0: 0.5, 1: 1, 2: 2, 3: 4}


class ModelError(Exception):
    pass


# ---------------------------------------------------------------- container

@dataclass
class RelocFile:
    """One O2R resource: header, pointer maps and the big-endian body."""
    header: bytes
    file_id: int
    data: bytearray
    intern: dict = field(default_factory=dict)    # slot -> target offset
    extern: dict = field(default_factory=dict)    # slot -> (file id, target offset)
    spare_externs: list = field(default_factory=list)  # listed ids no slot uses

    @classmethod
    def read(cls, blob: bytes) -> "RelocFile":
        if len(blob) < O2R_HEADER_SIZE + 16 or blob[4:8] != b"OLER":
            raise ModelError("not an O2R resource")
        file_id, head_int, head_ext, n = struct.unpack_from("<IHHI", blob, O2R_HEADER_SIZE)
        externs = list(struct.unpack_from(f"<{n}H", blob, O2R_HEADER_SIZE + 12))
        at = O2R_HEADER_SIZE + 12 + 2 * n
        size = struct.unpack_from("<I", blob, at)[0]
        data = bytearray(blob[at + 4:at + 4 + size])
        if at + 4 + size != len(blob):
            raise ModelError(f"file {file_id:#x}: body ends at {at + 4 + size:#x} of {len(blob):#x}")
        f = cls(bytes(blob[:O2R_HEADER_SIZE]), file_id, data)
        for head, kind in ((head_int, "intern"), (head_ext, "extern")):
            w, k = head, 0
            seen = set()
            while w != NO_RELOC:
                if w in seen or w * 4 + 4 > len(data):
                    raise ModelError(f"file {file_id:#x}: broken {kind} chain at word {w:#x}")
                seen.add(w)
                val = struct.unpack_from(">I", data, w * 4)[0]
                if kind == "intern":
                    f.intern[w * 4] = (val & 0xFFFF) * 4
                else:
                    f.extern[w * 4] = (externs[k], (val & 0xFFFF) * 4)
                    k += 1
                w = val >> 16
            if kind == "extern":
                if k > n:
                    raise ModelError(f"file {file_id:#x}: {k} extern slots for {n} ids")
                # Some vanilla files list ids no slot uses (Marth's model:
                # four of file 0x12b, no extern chain); keep them.
                f.spare_externs = externs[k:]
        return f

    @classmethod
    def load(cls, path: Path) -> "RelocFile":
        return cls.read(path.read_bytes())

    def to_bytes(self) -> bytes:
        data = bytearray(self.data)
        heads = []
        externs = []
        for kind, slots in (("intern", self.intern), ("extern", self.extern)):
            order = sorted(slots)
            for i, slot in enumerate(order):
                nxt = order[i + 1] // 4 if i + 1 < len(order) else NO_RELOC
                target = slots[slot] if kind == "intern" else slots[slot][1]
                if kind == "extern":
                    externs.append(slots[slot][0])
                if target % 4 or target // 4 > 0xFFFF or slot % 4:
                    raise ModelError(f"file {self.file_id:#x}: slot {slot:#x} -> {target:#x} not encodable")
                struct.pack_into(">I", data, slot, (nxt << 16) | (target // 4))
            heads.append(order[0] // 4 if order else NO_RELOC)
        externs += self.spare_externs
        out = bytearray(self.header)
        out += struct.pack("<IHHI", self.file_id, heads[0], heads[1], len(externs))
        out += struct.pack(f"<{len(externs)}H", *externs)
        out += struct.pack("<I", len(data))
        out += data
        return bytes(out)

    def ptr(self, off: int):
        if off in self.intern:
            return ("intern", self.file_id, self.intern[off])
        if off in self.extern:
            fid, target = self.extern[off]
            return ("extern", fid, target)
        return None

    def u8(self, off: int) -> int:
        return self.data[off]

    def u16(self, off: int) -> int:
        return struct.unpack_from(">H", self.data, off)[0]

    def u32(self, off: int) -> int:
        return struct.unpack_from(">I", self.data, off)[0]

    def f32(self, off: int) -> float:
        return struct.unpack_from(">f", self.data, off)[0]


# ------------------------------------------------------------ display lists

@dataclass
class Gfx:
    """One F3DEX2 command. `target` is w1's reloc target, if w1 is a slot:
    ("intern", file, offset) or ("extern", file, offset)."""
    w0: int
    w1: int
    target: tuple | None = None

    @property
    def op(self) -> int:
        return self.w0 >> 24

    def text(self) -> str:
        name = OP_NAMES.get(self.op, f"{self.op:#04x}")
        s = f"{name:14s} {self.w0:08x} {self.w1:08x}"
        if self.op == G_VTX:
            n, end = (self.w0 >> 12) & 0xFF, (self.w0 >> 1) & 0x7F
            s += f"  n={n} v0={end - n}"
        elif self.op == G_TRI1:
            s += f"  {tri1(self)}"
        elif self.op == G_TRI2:
            s += f"  {tri2(self)}"
        if self.target is not None:
            s += f"  -> {self.target[0]} {self.target[1]:#x}:{self.target[2]:#x}"
        return s


def tri1(g: Gfx) -> tuple:
    return (((g.w0 >> 16) & 0xFF) // 2, ((g.w0 >> 8) & 0xFF) // 2, (g.w0 & 0xFF) // 2)


def tri2(g: Gfx) -> tuple:
    return (tri1(g), (((g.w1 >> 16) & 0xFF) // 2, ((g.w1 >> 8) & 0xFF) // 2, (g.w1 & 0xFF) // 2))


def vtx_load(g: Gfx) -> tuple[int, int]:
    """A G_VTX's (count, first cache slot)."""
    n, end = (g.w0 >> 12) & 0xFF, (g.w0 >> 1) & 0x7F
    return n, end - n


def make_vtx(n: int, v0: int, target: tuple) -> Gfx:
    if not (1 <= n <= VERTEX_CACHE and 0 <= v0 and v0 + n <= VERTEX_CACHE):
        raise ModelError(f"vertex load n={n} v0={v0} leaves the cache")
    return Gfx((G_VTX << 24) | (n << 12) | ((v0 + n) << 1), 0, target)


def make_tri1(a: int, b: int, c: int) -> Gfx:
    return Gfx((G_TRI1 << 24) | (a * 2 << 16) | (b * 2 << 8) | (c * 2), 0)


def make_tri2(t0: tuple, t1: tuple) -> Gfx:
    return Gfx((G_TRI2 << 24) | (t0[0] * 2 << 16) | (t0[1] * 2 << 8) | (t0[2] * 2),
               (t1[0] * 2 << 16) | (t1[1] * 2 << 8) | (t1[2] * 2))


def walk_dl(f: RelocFile, off: int, limit: int = 4096) -> list[tuple[int, Gfx]]:
    """The commands of the list at `off` up to its G_ENDDL, with their offsets.

    The fighter lists are linear: their calls are the MObjs' segment-E lists
    (resolved per costume at run time), which are not followed."""
    out = []
    pc = off
    for _ in range(limit):
        if pc + 8 > len(f.data):
            raise ModelError(f"file {f.file_id:#x}: list at {off:#x} runs off the file")
        w0, w1 = struct.unpack_from(">II", f.data, pc)
        g = Gfx(w0, w1, f.ptr(pc + 4))
        out.append((pc, g))
        if g.op == G_ENDDL:
            return out
        if g.op == G_DL and ((w0 >> 16) & 0xFF) == 1 and g.target is None:
            raise ModelError(f"file {f.file_id:#x}: branch at {pc:#x} leaves the file")
        pc += 8
    raise ModelError(f"file {f.file_id:#x}: list at {off:#x} has no end in {limit} commands")


def encode_dl(cmds: list[Gfx]) -> tuple[bytes, dict[int, tuple]]:
    """A list's bytes and its pointer slots (offset in the list -> target)."""
    out = bytearray()
    slots = {}
    for g in cmds:
        if g.target is not None:
            slots[len(out) + 4] = g.target
            # The slot word is rewritten by the container; the low half is
            # its target, the high half the chain.
            w1 = g.target[2] // 4
        else:
            w1 = g.w1
        out += struct.pack(">II", g.w0, w1)
    return bytes(out), slots


# ---------------------------------------------------------------- the model

@dataclass
class Joint:
    """A JointTree row (DObjDesc)."""
    depth: int
    dl: int | None            # display list offset in the model file
    translate: tuple
    rotate: tuple
    scale: tuple

    def pack(self) -> bytes:
        return struct.pack(">II9f", self.depth, 0 if self.dl is None else self.dl // 4,
                           *self.translate, *self.rotate, *self.scale)


@dataclass
class Vertex:
    x: int
    y: int
    z: int
    flag: int
    s: int
    t: int
    rgba: tuple               # or (nx, ny, nz, a) as signed bytes when lit

    @classmethod
    def unpack(cls, data: bytes, off: int) -> "Vertex":
        x, y, z, flag, s, t = struct.unpack_from(">hhhHhh", data, off)
        return cls(x, y, z, flag, s, t, tuple(data[off + 12:off + 16]))

    def pack(self) -> bytes:
        return struct.pack(">hhhHhh", self.x, self.y, self.z, self.flag, self.s, self.t) + bytes(self.rgba)

    def normal(self) -> tuple:
        return tuple(b - 256 if b > 127 else b for b in self.rgba[:3])


@dataclass
class Corner:
    joint: int                # the joint whose list loaded the vertex
    address: int              # its Vtx's offset in the model file


@dataclass
class Triangle:
    joint: int                # the joint whose list draws it
    corners: tuple            # three Corners
    state: tuple              # (combine w0, w1, geometry mode, texture on, image, prim colour, segment-E call)


DETAILS = ("high", "low")

# MObjSub.flags (sys/objtypes.h MOBJ_FLAG_*; objdisplay.c gcDrawMObjForDObj):
# what the material's segment-E list sets.
MOBJ_SPRITES = 0x0001       # MOBJ_FLAG_ALPHA: image register = sprites[curr]
                            # (face frames), and the palette below is loaded
MOBJ_PALETTE = 0x0004       # image register = palettes[costume] (RGBA16)
MOBJ_PRIMCOLOR = 0x0200     # G_SETPRIMCOLOR from `primcolor`
MOBJSUB_SIZE = 0x78


@dataclass
class MObjSub:
    """A material object (sys/objtypes.h MObjSub): the fields a fighter's
    materials use."""
    offset: int
    fmt: int
    siz: int
    sprites: list             # image offsets (face frames)
    palettes: list            # palette offsets, one a costume
    flags: int
    block_fmt: int
    block_siz: int
    primcolor: int            # RGBA8888
    width: int
    height: int

    @classmethod
    def read(cls, f: RelocFile, off: int) -> "MObjSub":
        def lst(slot: int, limit: int = 16) -> list:
            ref = f.ptr(off + slot)
            if ref is None:
                return []
            out = []
            for i in range(limit):
                item = f.ptr(ref[2] + 4 * i)
                if item is None:
                    break
                out.append(item[2])
            return out
        return cls(off, f.u8(off + 2), f.u8(off + 3), lst(0x04), lst(0x2C), f.u16(off + 0x30),
                   f.u8(off + 0x32), f.u8(off + 0x33), f.u32(off + 0x50),
                   f.u16(off + 0x0C), f.u16(off + 0x0E))


class FighterModel:
    """A fighter's model as its main file's FTAttributes describe it."""

    def __init__(self, main: RelocFile, model: RelocFile, attr: int):
        self.main = main
        self.model = model
        self.attr = attr
        lay = ft_layout.layout()
        self.lay = lay
        container = main.ptr(attr + lay["FTAttributes.commonparts_container"])
        if container is None or container[0] != "intern":
            raise ModelError("main: commonparts_container is not an internal pointer")
        self.commonparts = container[2]
        self.trees: dict[str, list[Joint]] = {}
        self.tree_offsets: dict[str, int] = {}
        self.mobjsubs: dict[str, int | None] = {}
        self.costume_matanims: dict[str, int | None] = {}
        self.dl_pairs = bool(main.u8(self.commonparts + lay["FTCommonPart.flags"]) & 1)
        for index, detail in enumerate(DETAILS):
            part = self.commonparts + index * lay["sizeof(FTCommonPart)"]
            ref = main.ptr(part + lay["FTCommonPart.dobjdesc"])
            if ref is None or ref[0] != "extern" or ref[1] != model.file_id:
                raise ModelError(f"main: {detail} JointTree is not in the model file")
            self.tree_offsets[detail] = ref[2]
            self.trees[detail] = self._read_tree(ref[2])
            sub = main.ptr(part + lay["FTCommonPart.p_mobjsubs"])
            mat = main.ptr(part + lay["FTCommonPart.p_costume_matanim_joints"])
            self.mobjsubs[detail] = sub[2] if sub else None
            self.costume_matanims[detail] = mat[2] if mat else None

    @classmethod
    def from_export(cls, export_dir: Path, o2r_dir: Path | None = None) -> "FighterModel":
        """A content's model from its export (resolved.json's descriptor: file
        ids [0] main, [3] model; o_attributes) and its O2R directory."""
        resolved = json.loads((export_dir / "resolved.json").read_text(encoding="utf-8"))
        desc = resolved["descriptor"]
        o2r = o2r_dir or (export_dir / "o2r")
        main_id, model_id = desc["file_ids"][0], desc["file_ids"][3]
        return cls(RelocFile.load(o2r / f"{main_id:04x}"), RelocFile.load(o2r / f"{model_id:04x}"),
                   desc["o_attributes"])

    def _read_tree(self, off: int) -> list[Joint]:
        joints = []
        for index in range(64):
            at = off + index * DOBJ_DESC_SIZE
            depth = self.model.u32(at)
            ref = self.model.ptr(at + 4)
            if ref is not None and ref[0] != "intern":
                raise ModelError(f"JointTree row {index}: list in another file")
            vals = struct.unpack_from(">9f", self.model.data, at + 8)
            joints.append(Joint(depth, ref[2] if ref else None, vals[0:3], vals[3:6], vals[6:9]))
            if depth == JOINT_TREE_END:
                return joints
        raise ModelError(f"JointTree at {off:#x} has no end row")

    def lists(self, detail: str) -> list[int | None]:
        """Each joint's display list: Low falls back to High where its own is
        NULL (lbCommonSetupFighterPartsDObjs)."""
        own = [j.dl for j in self.trees[detail][:-1]]
        if detail == "low":
            high = [j.dl for j in self.trees["high"][:-1]]
            own = [o if o is not None else h for o, h in zip(own, high)]
        return own

    def parents(self, detail: str = "high") -> list[int]:
        """Each joint's parent row from the depths (-1 for the root)."""
        out, stack = [], []
        for index, joint in enumerate(self.trees[detail][:-1]):
            while stack and self.trees[detail][stack[-1]].depth >= joint.depth:
                stack.pop()
            out.append(stack[-1] if stack else -1)
            stack.append(index)
        return out

    def vertex(self, address: int) -> Vertex:
        return Vertex.unpack(self.model.data, address)

    def triangles(self, detail: str) -> list[Triangle]:
        """Every triangle the JointTree draws, in draw order, each corner
        resolved through the vertex cache to the list and Vtx that loaded it."""
        cache: list[Corner | None] = [None] * VERTEX_CACHE
        out = []
        state = [0, 0, 0, 0, None, 0, None]
        for joint, dl in enumerate(self.lists(detail)):
            if dl is None:
                continue
            for pc, g in walk_dl(self.model, dl):
                op = g.op
                if op == G_VTX:
                    n, v0 = vtx_load(g)
                    if g.target is None or g.target[0] != "intern":
                        raise ModelError(f"vertex load at {pc:#x} is not internal")
                    for i in range(n):
                        cache[v0 + i] = Corner(joint, g.target[2] + i * VERTEX_SIZE)
                elif op in (G_TRI1, G_TRI2):
                    for tri in ((tri1(g),) if op == G_TRI1 else tri2(g)):
                        corners = tuple(cache[i] for i in tri)
                        if any(c is None for c in corners):
                            raise ModelError(f"triangle at {pc:#x} names an empty cache slot")
                        out.append(Triangle(joint, corners, tuple(state)))
                elif op == G_SETCOMBINE:
                    state[0], state[1] = g.w0, g.w1
                elif op == G_GEOMETRYMODE:
                    state[2] = (state[2] & (g.w0 & 0xFFFFFF)) | g.w1
                elif op == G_TEXTURE:
                    state[3] = g.w1 != 0 and (g.w0 & 0xFF) != 0
                elif op == G_SETTIMG:
                    state[4] = g.target[2] if g.target else None
                elif op == G_SETPRIMCOLOR:
                    state[5] = g.w1
                elif op == G_DL and (g.w1 >> 24) == SEGMENT_E:
                    state[6] = g.w1
        return out

    def mobjs(self, detail: str, joint: int) -> list["MObjSub"]:
        """A joint's MObjSubs, in the order its list's segment-E calls index
        them (0x0E000000, 0x0E000008, ...)."""
        table = self.mobjsubs[detail]
        if table is None:
            return []
        ref = self.model.ptr(table + 4 * joint)
        if ref is None:
            return []
        out = []
        at = ref[2]
        starts = self._mobj_list_starts()
        while True:
            sub = self.model.ptr(at)
            if sub is None:
                return out
            mo = MObjSub.read(self.model, sub[2])
            # A list ends where the next MObj list starts: the reader stops
            # only at a non-pointer word, and Roy's six-costume palette lists
            # sit back to back, so it ran on into the next material's
            # palettes (and the eye's frames).
            for slot, name in ((0x04, "sprites"), (0x2C, "palettes")):
                lref = self.model.ptr(sub[2] + slot)
                if lref is not None:
                    nxt = next((s for s in starts if s > lref[2]), None)
                    if nxt is not None:
                        setattr(mo, name, getattr(mo, name)[:(nxt - lref[2]) // 4])
            out.append(mo)
            at += 4

    def _mobj_list_starts(self) -> list[int]:
        """Where every MObjSub's frame and palette lists start (both
        details), sorted."""
        cached = getattr(self, "_list_starts_cache", None)
        if cached is not None:
            return cached
        starts = set()
        for detail in DETAILS:
            table = self.mobjsubs[detail]
            if table is None:
                continue
            for joint in range(len(self.trees[detail])):
                ref = self.model.ptr(table + 4 * joint)
                if ref is None:
                    continue
                at = ref[2]
                while True:
                    sub = self.model.ptr(at)
                    if sub is None:
                        break
                    for slot in (0x04, 0x2C):
                        lref = self.model.ptr(sub[2] + slot)
                        if lref is not None:
                            starts.add(lref[2])
                    at += 4
        self._list_starts_cache = sorted(starts)
        return self._list_starts_cache

    def all_lists(self) -> set[int]:
        """Every display list the trees and model parts name."""
        out = {dl for d in DETAILS for dl in self.lists(d) if dl is not None}
        out |= {dl for dl in self.model_part_lists() if dl is not None}
        return out

    def model_part_lists(self) -> list[int]:
        """The display lists of every model-part row (FTModelPart.dl), each
        detail, as far as the container's rows reach."""
        lay = self.lay
        container = self.main.ptr(self.attr + lay["FTAttributes.modelparts_container"])
        if container is None or container[0] != "intern":
            return []
        rows = []
        for index in range(37 - 4):
            ref = self.main.ptr(container[2] + 4 * index)
            if ref is not None and ref[0] == "intern":
                rows.append(ref[2])
        starts = sorted(set(rows) | {container[2]})
        out = []
        for base in rows:
            bound = min([s for s in starts if s > base], default=len(self.main.data))
            at = base
            while at + 20 <= bound:
                ref = self.main.ptr(at + lay["FTModelPart.dl"])
                if ref is not None and ref[0] == "extern" and ref[1] == self.model.file_id:
                    out.append(ref[2])
                at += 20
        return out


def census(fm: FighterModel) -> dict:
    """Triangles and vertices a joint draws, per detail (A1's base budgets)."""
    out = {}
    for detail in DETAILS:
        rows = {}
        for tri in fm.triangles(detail):
            row = rows.setdefault(tri.joint, {"triangles": 0, "vertices": set(), "cross": 0})
            row["triangles"] += 1
            for c in tri.corners:
                if c.joint == tri.joint:
                    row["vertices"].add(c.address)
                else:
                    row["cross"] += 1
        out[detail] = {j: {"triangles": r["triangles"], "vertices": len(r["vertices"]),
                           "cross_corners": r["cross"]} for j, r in sorted(rows.items())}
    return out


def f1(export_dir: Path, o2r_dir: Path | None = None) -> list[str]:
    """F1: the model and main files re-serialize byte-identical, and every
    display list and JointTree row the model names re-encodes to its bytes."""
    fm = FighterModel.from_export(export_dir, o2r_dir)
    problems = []
    for f in (fm.main, fm.model):
        o2r = o2r_dir or (export_dir / "o2r")
        blob = (o2r / f"{f.file_id:04x}").read_bytes()
        if f.to_bytes() != blob:
            problems.append(f"file {f.file_id:#x}: container does not round-trip")
    for dl in sorted(fm.all_lists()):
        cmds = walk_dl(fm.model, dl)
        raw = bytes(fm.model.data[dl:cmds[-1][0] + 8])
        body, slots = encode_dl([g for _pc, g in cmds])
        # Slot words carry the chain in their high half: compare the targets.
        for at, target in slots.items():
            if fm.model.ptr(dl + at) != target:
                problems.append(f"list {dl:#x}: slot {at:#x} target differs")
        masked = bytearray(raw)
        for at in slots:
            struct.pack_into(">I", masked, at, struct.unpack_from(">I", masked, at)[0] & 0xFFFF)
        if bytes(masked) != body:
            problems.append(f"list {dl:#x}: re-encoded bytes differ")
    for detail in DETAILS:
        off = fm.tree_offsets[detail]
        for index, joint in enumerate(fm.trees[detail]):
            raw = bytearray(fm.model.data[off + index * DOBJ_DESC_SIZE:off + (index + 1) * DOBJ_DESC_SIZE])
            if joint.dl is not None:
                struct.pack_into(">I", raw, 4, struct.unpack_from(">I", raw, 4)[0] & 0xFFFF)
            if joint.pack() != bytes(raw):
                problems.append(f"{detail} JointTree row {index}: re-encoded bytes differ")
    return problems


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("export", type=Path, nargs="+", help="a content's export directory")
    ap.add_argument("--o2r", type=Path, help="its O2R directory (default <export>/o2r)")
    ap.add_argument("--census", action="store_true", help="triangles and vertices per joint")
    ap.add_argument("--dl", type=lambda s: int(s, 0), help="disassemble the list at this offset")
    args = ap.parse_args()
    status = 0
    for export in args.export:
        if args.dl is not None:
            fm = FighterModel.from_export(export, args.o2r)
            for pc, g in walk_dl(fm.model, args.dl):
                print(f"{pc:6x}: {g.text()}")
            continue
        if args.census:
            fm = FighterModel.from_export(export, args.o2r)
            rows = census(fm)
            for detail in DETAILS:
                total = sum(r["triangles"] for r in rows[detail].values())
                print(f"{export.name} {detail}: {total} triangles")
                for joint, r in rows[detail].items():
                    print(f"  joint {joint:2d}: {r['triangles']:4d} tris {r['vertices']:4d} verts "
                          f"{r['cross_corners']:3d} cross corners")
            continue
        problems = f1(export, args.o2r)
        print(f"{export.name}: F1 {'ok' if not problems else 'FAILED'}")
        for p in problems[:20]:
            print("  " + p)
        status |= bool(problems)
    return status


if __name__ == "__main__":
    sys.exit(main())
