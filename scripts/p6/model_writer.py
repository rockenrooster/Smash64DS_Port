#!/usr/bin/env python3
"""P6 A2 writer: a content's model and main files from a look's scenes.

The base content's model file is copied and the look's details are appended
to it: for each detail scene (model_scene.py's form, as look_r.py compose
writes it) new vertex arrays, one display list per joint and a new
JointTree; the main file's FTCommonPart for that detail and every model-part
row that named the base's list for a joint are pointed at them. The base's
own lists stay in the file (model-part variants the look does not replace
still draw from them); `--trim` is the memory pass's.

Lists are written in the base's idiom (P6_PLAN.md A2: "material spans of the
kinds the base uses"): the joint's prelude (lights, geometry mode) as the
base drew it, then per material the span of state commands the base used for
it (model_scene.py records them per material and joint), then vertex loads of
at most 32 vertices and TRI2/TRI1 commands, then the base's closing
commands. A triangle whose corners lie on two joints (a parent and its child)
is drawn by the child from vertices its parent's list left in the vertex
cache: the parent's list ends with a load of exactly those vertices (slots
0..k-1, under the material state the child's seam triangles use) and the
child's own loads start at slot k -- the pattern the base's lists use.

    model_writer.py --base-export builds/p4-staging/export/peach \\
        --scene low=builds/p6-art/palutena/pp/low_v57/scene.json \\
        --main-id 0x1a00 --model-id 0x1a01 --out builds/p6-art/palutena/files

Writes <out>/<main id>, <out>/<model id> (O2R resources) and
<out>/writer.json (what was written). Build output only: the files carry the
base's data (D10).
"""
from __future__ import annotations

import argparse
import copy
import json
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import n64_model as N  # noqa: E402

DETAIL_INDEX = {"high": 0, "low": 1}
CACHE = N.VERTEX_CACHE
G_CULL_BACK = 0x00000400


class WriterError(SystemExit):
    pass


def _cmd(spec) -> N.Gfx:
    """A recorded command ([w0 hex, w1 hex, target]) as a Gfx."""
    w0, w1, target = spec
    return N.Gfx(int(w0, 16), int(w1, 16), None if target is None else tuple(target))


def _is_light_or_mode(g: N.Gfx) -> bool:
    return g.op in (N.G_MOVEWORD, N.G_GEOMETRYMODE)


def _ends_two_sided(cmds: list[N.Gfx]) -> bool:
    """Whether the list's last G_GEOMETRYMODE that touches the back cull
    clears it (G_GEOMETRYMODE: w0's low 24 bits the AND mask, w1 the OR)."""
    culled = True
    for g in cmds:
        if g.op != N.G_GEOMETRYMODE:
            continue
        if not (g.w0 & G_CULL_BACK):
            culled = False
        if g.w1 & G_CULL_BACK:
            culled = True
    return not culled


# A costume MObj's palette list: as long as the base's (Peach's lists hold
# 16), the last costume repeated past the look's own.
COSTUME_SLOTS = 16
MOBJSUB_SIZE = 0x78
MOBJSUB_PALETTES = 0x2C
MOBJ_PALETTE = 0x0004


def palette_mobj_template(fm: N.FighterModel, detail: str) -> tuple | None:
    """The base's first palette-only MObjSub (its bytes, its `palettes` and
    `sprites` emptied) and that MObj's costume animation (palette = costume):
    the template of a costume MObj."""
    for joint in range(len(fm.trees[detail]) - 1):
        mobjs = fm.mobjs(detail, joint)
        for k, mo in enumerate(mobjs):
            if mo.flags != MOBJ_PALETTE or not mo.palettes:
                continue
            table = fm.costume_matanims[detail]
            if table is None:
                return None
            arr = fm.model.ptr(table + 4 * joint)
            script = fm.model.ptr(arr[2] + 4 * k) if arr else None
            if script is None:
                continue
            raw = bytearray(fm.model.data[mo.offset:mo.offset + MOBJSUB_SIZE])
            raw[MOBJSUB_PALETTES:MOBJSUB_PALETTES + 4] = bytes(4)
            raw[4:8] = bytes(4)
            return bytes(raw), script[2]
    return None


def ci4_bytes(texels) -> bytes:
    """Rows of 4-bit indices, two to a byte, high nibble first."""
    out = bytearray()
    for row in texels:
        row = list(row)
        if len(row) % 2:
            raise WriterError("CI4 rows must have an even width")
        for a, b in zip(row[0::2], row[1::2]):
            out.append(((int(a) & 15) << 4) | (int(b) & 15))
    return bytes(out)


# Owner p6s (Palutena): "Brighter colors: lets go with the A+B (4th
# mockup)". A: every palette the writer writes lifted (--lift-palettes,
# lift_colour): the whites brighter and cleaner, the greens and blues
# brighter and more saturated. B: the lists' ambient light raised
# (--ambient, the preludes' G_MW_LIGHTCOL light 2).
LIFT_PALETTES = False


def lift_colour(rgb) -> list:
    """A palette colour lifted (owner p6s, mock-up A): a white (saturation
    under 0.2, value over 0.5) to value * 1.22 + 0.06 at half its
    saturation; a green (hue 70-170) value * 1.4, saturation * 1.15; a blue
    (hue 185-255) value * 1.35, saturation * 1.1."""
    import colorsys
    r, g, b = (c / 255.0 for c in rgb[:3])
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    hd = h * 360.0
    if s < 0.2 and v > 0.5:
        v, s = min(1.0, v * 1.22 + 0.06), s * 0.5
    elif 70.0 <= hd <= 170.0 and s > 0.2:
        v, s = min(1.0, v * 1.4), min(1.0, s * 1.15)
    elif 185.0 <= hd <= 255.0 and s > 0.2:
        v, s = min(1.0, v * 1.35), min(1.0, s * 1.1)
    r, g, b = colorsys.hsv_to_rgb(h, s, v)
    return [round(r * 255), round(g * 255), round(b * 255)] + list(rgb[3:])


def ambient_preludes(scene: dict, rgb: int) -> None:
    """The scene's preludes with light 2's colour (the ambient: G_MOVEWORD
    G_MW_LIGHTCOL at 0x18 and 0x1C) set to rgb."""
    for cmds in (scene.get("preludes") or {}).values():
        for c in cmds or ():
            if int(c[0], 16) in (0xDB0A0018, 0xDB0A001C):
                c[1] = f"{(rgb << 8) & 0xFFFFFF00:08x}"


def rgba5551(palette) -> bytes:
    out = bytearray()
    for c in palette:
        if LIFT_PALETTES:
            c = lift_colour(c)
        r, g, b = (int(x) for x in c[:3])
        a = int(c[3]) if len(c) > 3 else 255
        out += struct.pack(">H", ((r >> 3) << 11) | ((g >> 3) << 6) | ((b >> 3) << 1) | (1 if a >= 128 else 0))
    return bytes(out)


def png_indices(path: Path, palette) -> list:
    """A PNG's pixels as indices into `palette` (exact colours expected;
    the nearest otherwise)."""
    from PIL import Image
    img = Image.open(path).convert("RGBA")
    pal = np.array([list(c[:3]) for c in palette], dtype=float)
    a = np.asarray(img, dtype=float)[:, :, :3]
    d = ((a[:, :, None, :] - pal[None, None, :, :]) ** 2).sum(3)
    return d.argmin(2).tolist()


def _log2(n: int) -> int:
    k = 0
    while (1 << k) < n:
        k += 1
    return k


# Cut-out textures as Captain Falcon's high model draws them: alpha compare
# on threshold, the edge render mode, blend colour 0 (the DS alpha test's
# reference: pass when alpha > 0); then back to the opaque mode.
CUTOUT_ON = [N.Gfx(0xE2001E01, 0x00000001), N.Gfx(0xE200001C, 0xC4113078)]
CUTOUT_OFF = [N.Gfx(0xE2001E01, 0x00000000), N.Gfx(0xE200001C, 0xC4112078)]


class Appender:
    """Bytes appended to a RelocFile, with their pointer slots."""

    def __init__(self, f: N.RelocFile):
        self.f = f
        pad = (-len(f.data)) % 8
        f.data += bytes(pad)

    @property
    def here(self) -> int:
        return len(self.f.data)

    def align(self, n: int = 8):
        self.f.data += bytes((-len(self.f.data)) % n)

    def put(self, blob: bytes) -> int:
        at = len(self.f.data)
        self.f.data += blob
        return at

    def slot(self, at: int, target: tuple):
        """A pointer at `at` to (kind, file, offset)."""
        kind, fid, off = target
        if kind == "intern":
            if fid != self.f.file_id:
                raise WriterError(f"intern target in file {fid:#x}")
            self.f.intern[at] = off
            self.f.extern.pop(at, None)
        else:
            self.f.extern[at] = (fid, off)
            self.f.intern.pop(at, None)


class ListWriter:
    """One detail's display lists from its scene."""

    def __init__(self, fm: N.FighterModel, model: N.RelocFile, scene: dict, detail: str, base_scene: dict | None):
        self.fm = fm
        self.model = model
        self.scene = scene
        self.detail = detail
        self.base_scene = base_scene
        self.parents = [j["parent"] for j in scene["joints"]]
        self.verts = scene["vertices"]
        self.mats = scene["materials"]
        self.new_textures: dict = {}
        self.vertex_blobs: dict[bytes, int] = {}
        self.app = None
        # joint -> the materials its list calls through segment E (their
        # costume palettes as MObjs, in call order).
        self.mobj_materials: dict[int, list[int]] = {}

    def ancestors(self, j: int) -> list[int]:
        out = []
        while j >= 0:
            out.append(j)
            j = self.parents[j]
        return out

    def drawing_joint(self, tri: dict) -> int:
        js = {self.verts[i]["joint"] for i in tri["v"]}
        deep = max(js, key=lambda j: len(self.ancestors(j)))
        if not all(j in self.ancestors(deep) for j in js):
            raise WriterError(f"triangle {tri['v']} spans joints {sorted(js)} that are not one chain")
        if len(js) > 2:
            raise WriterError(f"triangle {tri['v']} spans three joints {sorted(js)}")
        return deep

    # -- vertices ---------------------------------------------------------
    def vtx_bytes(self, i: int, material: dict) -> bytes:
        v = self.verts[i]
        x, y, z = (int(round(c)) for c in v["local"])
        for c in (x, y, z):
            if not -0x8000 <= c <= 0x7FFF:
                raise WriterError(f"vertex {i}: local {v['local']} overflows s16")
        if "uv" in v and (v.get("st") in (None, (0, 0), [0, 0])):
            tex = material.get("texture") or {}
            w, h = tex.get("width", 32), tex.get("height", 32)
            u, vv = v["uv"]
            s, t = int(round(u * w * 32)), int(round((1.0 - vv) * h * 32))
        else:
            s, t = (int(c) for c in v["st"])
        n = [int(c) & 0xFF for c in v.get("normal", (0, 0, 0))]
        a = v.get("rgba", (0, 0, 0, 255))[3] if v.get("rgba") else 255
        return struct.pack(">hhhHhh", x, y, z, int(v.get("flag", 0)), s, t) + bytes((n[0], n[1], n[2], a & 0xFF))

    # -- spans ------------------------------------------------------------
    def span(self, mi: int, joint: int) -> list[N.Gfx]:
        """The material's state commands for a list of `joint`."""
        m = self.mats[mi]
        spans = m.get("spans") or {}
        own = spans.get(str(joint))
        src = own or next((c for c in spans.values() if c), None)
        keep_mobj = False
        # A synthesized span's segment-E call is its own costume MObj.
        synth = src is None
        if src is None:
            cmds = self.synth_span(m, mi, joint)
        else:
            cmds = [_cmd(c) for c in src]
            loads = any(g.op in (N.G_SETTIMG, N.G_LOADBLOCK, N.G_LOADTILE) for g in cmds)
            if loads and not m.get("texture"):
                # A recoloured texture: drawn in its colour alone.
                cmds = self.synth_span(m, mi, joint)
                synth = True
            else:
                span_prim = next((g.w1 for g in cmds if g.op == N.G_SETPRIMCOLOR), None)
                recolored = span_prim is not None and span_prim != int(m["prim"], 16)
                # Another joint's MObj index means nothing here; a recoloured
                # material must not take its MObj's (costume) colour.
                keep_mobj = bool(own) and not recolored
        out = []
        prim = int(m["prim"], 16)
        base_id = self.fm.model.file_id
        for g in cmds:
            if g.target is not None and g.target[1] == base_id:
                # The written file is the base's with more appended: the
                # same offsets, under its own id.
                g = N.Gfx(g.w0, g.w1, ("intern", self.model.file_id, g.target[2]))
            if _is_light_or_mode(g):
                continue            # the prelude set them
            if g.op == N.G_DL and (g.w1 >> 24) == N.SEGMENT_E:
                if keep_mobj or synth:
                    out.append(g)
                    continue
                pal = self.mobj_palette(m, joint)
                if pal is not None:
                    # The MObj loaded the palette's address: load it instead.
                    out.append(N.Gfx(0xFD100000, pal[2] // 4, pal))
                continue
            if g.op == N.G_SETPRIMCOLOR:
                g = N.Gfx(g.w0, prim, None)
            out.append(g)
        if m.get("two_sided"):
            out.insert(0, N.Gfx((N.G_GEOMETRYMODE << 24) | (0xFFFFFF & ~G_CULL_BACK), 0, None))
        return out

    def mobj_palette(self, m: dict, joint: int):
        """The palette an MObj-loaded textured span uses (costume 0), when
        the span is re-emitted without its MObj."""
        if not m.get("texture"):
            return None
        pal = m["texture"].get("palette_offset")
        if pal is not None:
            return ("intern", self.model.file_id, pal)
        for j, cmds in (m.get("spans") or {}).items():
            if not cmds:
                continue
            for c in cmds:
                g = _cmd(c)
                if g.op == N.G_DL and (g.w1 >> 24) == N.SEGMENT_E:
                    mobjs = self.fm.mobjs(self.detail, int(j))
                    k = (g.w1 & 0xFFFFFF) // 8
                    if k < len(mobjs) and mobjs[k].palettes:
                        return ("intern", self.model.file_id, mobjs[k].palettes[0])
        return None

    def synth_span(self, m: dict, mi: int = -1, joint: int = -1) -> list[N.Gfx]:
        """A span for a material the base never drew (an added part). A
        texture with costume palettes takes its palette from an MObj (the
        joint's segment-E material `k`), as the base's costume materials do."""
        prim = int(m["prim"], 16)
        tex = m.get("texture")
        if not tex:
            return [N.Gfx(0xE7000000, 0), N.Gfx(0xFC327E05, 0xFF17FDFF), N.Gfx(0xD7000000, 0),
                    N.Gfx(0xFA000000, prim)]
        w, h = int(tex["width"]), int(tex["height"])
        tex_at, pal_at = self.new_texture(m)
        palette_cmd = N.Gfx(0xFD100000, pal_at // 4, ("intern", self.model.file_id, pal_at))
        if tex.get("costume_palettes") and joint >= 0:
            calls = self.mobj_materials.setdefault(joint, [])
            if mi not in calls:
                calls.append(mi)
            palette_cmd = N.Gfx(0xDE000000, (N.SEGMENT_E << 24) | (8 * calls.index(mi)), None)
        cm = {"clamp": 2, "mirror": 1, "repeat": 0}
        wrap = tex.get("wrap") or ["clamp", "clamp"]
        cms, cmt = cm[wrap[0]], cm[wrap[1]]
        ms, mt = _log2(w), _log2(h)
        line = max(1, w // 16)
        fid = self.model.file_id
        cmds = [N.Gfx(0xE7000000, 0), N.Gfx(0xE3001001, 0x00008000)]
        if tex.get("alpha"):
            cmds += CUTOUT_ON
        cmds += [N.Gfx(0xFC127E05, 0xFF17F3FF)]
        if tex.get("alpha"):
            cmds += [N.Gfx(0xF9000000, 0)]
        cmds += [N.Gfx(0xE7000000, 0), N.Gfx(0xFA000000, 0xFFFFFFFF), N.Gfx(0xE8000000, 0),
                 N.Gfx(0xF5000100, 0x05000000),
                 N.Gfx(0xF5500000, (7 << 24) | (mt << 14) | (ms << 4)),
                 N.Gfx(0xF5400000 | (line << 9), (cmt << 18) | (mt << 14) | (cms << 8) | (ms << 4)),
                 palette_cmd,
                 N.Gfx(0xE6000000, 0), N.Gfx(0xF0000000, 0x0503C000), N.Gfx(0xE7000000, 0),
                 N.Gfx(0xD7000002, 0xFFFFFFFF),
                 N.Gfx(0xF2000000, (((w - 1) << 2) << 12) | ((h - 1) << 2)),
                 N.Gfx(0xFD500000, tex_at // 4, ("intern", fid, tex_at)),
                 N.Gfx(0xE6000000, 0),
                 N.Gfx(0xF3000000, (7 << 24) | ((w * h // 4 - 1) << 12) | (2048 // line)),
                 N.Gfx(0xE7000000, 0)]
        return cmds

    def new_texture(self, m: dict) -> tuple:
        """Append a new material's texels and palette (once); with costume
        palettes, each costume's palette and their pointer list (the MObj's
        `palettes`, COSTUME_SLOTS long, the last repeated)."""
        tex = m["texture"]
        key = tex.get("file")
        if key in self.new_textures:
            return self.new_textures[key][:2]
        app = self.app
        app.align(8)
        tex_at = app.put(ci4_bytes(tex["texels"]))
        app.align(8)
        pal_at = app.put(rgba5551(tex["palette"]))
        list_at = None
        if tex.get("costume_palettes"):
            pals = []
            for pal in tex["costume_palettes"]:
                app.align(8)
                pals.append(app.put(rgba5551(pal)))
            pals += [pals[-1]] * max(0, COSTUME_SLOTS - len(pals))
            app.align(8)
            list_at = app.put(bytes(4 * len(pals)))
            for k, at in enumerate(pals):
                app.slot(list_at + 4 * k, ("intern", self.model.file_id, at))
        self.new_textures[key] = (tex_at, pal_at, list_at)
        return tex_at, pal_at

    def write_mobjs(self, app: "Appender", template: tuple) -> dict:
        """The costume MObjs each joint's list calls: an MObjSub a material
        (the base's palette MObjSub, its `palettes` ours), and for each joint
        its MObjSub list (NULL-ended) and costume animation list (the base's
        palette animation: the costume's frame picks its palette).
        Returns joint -> (MObjSub list offset, animation list offset)."""
        sub_bytes, script_at = template
        fid = self.model.file_id
        subs: dict[int, int] = {}
        out = {}
        for joint, calls in sorted(self.mobj_materials.items()):
            for mi in calls:
                if mi in subs:
                    continue
                list_at = self.new_textures[self.mats[mi]["texture"]["file"]][2]
                app.align(8)
                at = app.put(sub_bytes)
                app.slot(at + MOBJSUB_PALETTES, ("intern", fid, list_at))
                subs[mi] = at
            app.align(4)
            sub_list = app.put(bytes(4 * (len(calls) + 1)))
            for k, mi in enumerate(calls):
                app.slot(sub_list + 4 * k, ("intern", fid, subs[mi]))
            anim_list = app.put(bytes(4 * (len(calls) + 1)))
            for k in range(len(calls)):
                app.slot(anim_list + 4 * k, ("intern", fid, script_at))
            out[joint] = (sub_list, anim_list)
        return out

    # -- lists ------------------------------------------------------------
    def build(self, app: Appender) -> dict[int, int]:
        """Write every joint's list; return joint -> list offset."""
        self.app = app
        by_joint: dict[int, list[dict]] = {}
        for t in self.scene["triangles"]:
            by_joint.setdefault(self.drawing_joint(t), []).append(t)
        njoints = len(self.scene["joints"])
        # The vertices each child takes from its parent's last load.
        seam: dict[int, list[int]] = {}
        seam_mat: dict[int, int] = {}
        for j, tris in by_joint.items():
            p = self.parents[j]
            need = []
            for t in tris:
                for i in t["v"]:
                    if self.verts[i]["joint"] != j and i not in need:
                        need.append(i)
                if any(self.verts[i]["joint"] != j for i in t["v"]) and j not in seam_mat:
                    seam_mat[j] = t["material"]
            if need:
                if self.verts[need[0]]["joint"] != p:
                    raise WriterError(f"joint {j}: seam vertices on joint {self.verts[need[0]]['joint']}, not its parent {p}")
                if len(need) > 16:
                    raise WriterError(f"joint {j}: {len(need)} seam vertices")
                seam[j] = need
        # Draw order: the tree's, depth-first by row; a child with a seam
        # must be drawn right after its parent.
        offsets = {}
        prelude_of = self.scene.get("preludes") or (self.base_scene or {}).get("preludes") or {}
        default_prelude = prelude_of.get("2") or []
        base_rows = self.fm.trees[self.detail]
        self._by_joint, self._seam, self._seam_mat = by_joint, seam, seam_mat
        self._prelude_of, self._default_prelude = prelude_of, default_prelude
        for j in range(njoints):
            tris = by_joint.get(j, [])
            kids_seam = [c for c in seam if self.parents[c] == j]
            if not tris and not kids_seam:
                if j < len(base_rows) and base_rows[j].dl is not None:
                    # The base drew here (a NULL low row falls back to the
                    # high list): an empty list, so nothing of the base shows.
                    app.align(8)
                    offsets[j] = app.put(struct.pack(">II", N.G_ENDDL << 24, 0))
                continue
            for c in kids_seam:
                if c != j + 1:
                    raise WriterError(f"joint {c} takes seam vertices from {j} but is not drawn right after it")
            offsets[j] = self.write_list(app, j, tris, seam.get(j, []),
                                         [(c, seam[c], seam_mat[c]) for c in kids_seam],
                                         prelude_of.get(str(j)) or default_prelude)
        return offsets

    def write_part_list(self, app: Appender, part: dict) -> int:
        """A part's variant list (look_parts.py): its joint's own triangles,
        then the part's cards (their material and vertices appended to the
        scene's), before the children's seam loads, as the joint's list."""
        j = part["joint"]
        mi = len(self.mats)
        self.mats.append(part["material"])
        base = len(self.verts)
        self.verts.extend(part["vertices"])
        tris = list(self._by_joint.get(j, []))
        tris += [{"joint": j, "v": [base + i for i in t], "material": mi} for t in part["triangles"]]
        kids_seam = [c for c in self._seam if self.parents[c] == j]
        return self.write_list(app, j, tris, self._seam.get(j, []),
                               [(c, self._seam[c], self._seam_mat[c]) for c in kids_seam],
                               self._prelude_of.get(str(j)) or self._default_prelude)

    def write_list(self, app: Appender, joint: int, tris: list[dict], from_parent: list[int],
                   children: list, prelude: list) -> int:
        cmds: list[N.Gfx] = []
        # Prelude: the base's commands up to its lights and geometry mode.
        pre = [_cmd(c) for c in prelude]
        last = max((k for k, g in enumerate(pre) if _is_light_or_mode(g)), default=-1)
        cmds += pre[:last + 1]
        k0 = len(from_parent)                    # cache slots the parent left us
        slot_of = {i: s for s, i in enumerate(from_parent)}
        textured_mode = False
        # Group by material in first-use order.
        order = []
        groups: dict[int, list[dict]] = {}
        for t in tris:
            if t["material"] not in groups:
                order.append(t["material"])
                groups[t["material"]] = []
            groups[t["material"]].append(t)
        for mi in order:
            span = self.span(mi, joint)
            cmds += span
            if any(g.op == N.G_SETOTHERMODE_H and (g.w1 & 0x8000) for g in span):
                textured_mode = True
            cmds += self.draw_batches(app, groups[mi], mi, k0, slot_of)
            if (self.mats[mi].get("texture") or {}).get("alpha") and not self.mats[mi].get("spans"):
                cmds += CUTOUT_OFF
            if self.mats[mi].get("two_sided"):
                cmds.append(N.Gfx((N.G_GEOMETRYMODE << 24) | 0xFFFFFF, G_CULL_BACK, None))
        # Seam vertices for the children: one last load, under the state of
        # the material their triangles use.
        for child, need, mi in children:
            cmds += self.span(mi, joint)
            vat = self.put_vertices(app, need, mi)
            cmds.append(N.make_vtx(len(need), 0, ("intern", self.model.file_id, vat)))
            # Every list ends in the default state, the seam's material
            # included: the next list (the child) starts from it, and the
            # native owner learns each list's runs from a reset state (a
            # two-sided seam material left the torso's first run culled
            # where its runtime state was not: the lean path declined
            # Palutena's every draw).
            if (self.mats[mi].get("texture") or {}).get("alpha") and not self.mats[mi].get("spans"):
                cmds += CUTOUT_OFF
            if self.mats[mi].get("two_sided"):
                cmds.append(N.Gfx((N.G_GEOMETRYMODE << 24) | 0xFFFFFF, G_CULL_BACK, None))
        # The base's prelude can clear the cull (Roy's two-sided parts); the
        # base restored it after its own triangles, in a span the look does
        # not keep. A list left two-sided handed the next joint's list a
        # no-cull state the runtime resets: the lean path declined Ike.
        if _ends_two_sided(cmds):
            cmds.append(N.Gfx((N.G_GEOMETRYMODE << 24) | 0xFFFFFF, G_CULL_BACK, None))
        cmds.append(N.Gfx(0xE7000000, 0))
        if textured_mode:
            cmds.append(N.Gfx(0xE3001001, 0x00000000))
        cmds.append(N.Gfx(N.G_ENDDL << 24, 0))
        app.align(8)
        at = app.here
        body, slots = N.encode_dl(cmds)
        app.put(body)
        for off, target in slots.items():
            app.slot(at + off, target)
        return at

    def put_vertices(self, app: Appender, idx: list[int], mi: int) -> int:
        """The vertices' bytes, appended once: a part's variants load the
        same seam vertices as the joint's own list (five pillar variants on
        Palutena's hips row re-appended her torso's seam, +12 KB)."""
        blob = b"".join(self.vtx_bytes(i, self.mats[mi]) for i in idx)
        if blob not in self.vertex_blobs:
            app.align(8)
            self.vertex_blobs[blob] = app.put(blob)
        return self.vertex_blobs[blob]

    def draw_batches(self, app: Appender, tris: list[dict], mi: int, k0: int, slot_of: dict) -> list[N.Gfx]:
        """Vertex loads and triangles for one material span. Parent-loaded
        vertices sit in slots 0..k0-1 for the whole list."""
        out = []
        room = CACHE - k0
        pending = list(tris)
        while pending:
            batch, used = [], []
            rest = []
            for t in pending:
                own = [i for i in t["v"] if i not in slot_of]
                new = [i for i in own if i not in used]
                if len(used) + len(new) <= room:
                    used += new
                    batch.append(t)
                else:
                    rest.append(t)
            if not batch:
                raise WriterError("a triangle does not fit the vertex cache")
            if used:
                vat = self.put_vertices(app, used, mi)
                out.append(N.make_vtx(len(used), k0, ("intern", self.model.file_id, vat)))
            slot = dict(slot_of)
            for s, i in enumerate(used):
                slot[i] = k0 + s
            tri_slots = [tuple(slot[i] for i in t["v"]) for t in batch]
            for a in range(0, len(tri_slots) - 1, 2):
                out.append(N.make_tri2(tri_slots[a], tri_slots[a + 1]))
            if len(tri_slots) % 2:
                out.append(N.make_tri1(*tri_slots[-1]))
            pending = rest
        return out


def mobj_palettes_with(fm: N.FighterModel, palette_offset: int) -> list:
    """Every palette of each MObj (either detail, any joint) whose palette
    list holds `palette_offset`: a costume's copies of that palette."""
    out = {palette_offset}
    for detail in N.DETAILS:
        for joint in range(len(fm.trees[detail]) - 1):
            for mo in fm.mobjs(detail, joint):
                if palette_offset in mo.palettes:
                    out.update(mo.palettes)
    return sorted(out)


def write_in_place(fm: N.FighterModel, model: N.RelocFile, image: int, texels, palette, palette_at) -> list:
    """Overwrite a base texture's texels and its palettes (every costume's)."""
    blob = ci4_bytes(texels)
    end = image + len(blob)
    if end > len(model.data):
        raise WriterError(f"texture at {image:#x} runs off the file")
    model.data[image:end] = blob
    written = []
    pal = rgba5551(palette)
    for p in palette_at:
        for q in mobj_palettes_with(fm, p):
            model.data[q:q + len(pal)] = pal
            written.append(q)
    return sorted(set(written))


def scene_texture_writes(fm: N.FighterModel, scene: dict) -> list:
    """In-place writes a scene asks for: a base material (it has spans)
    whose texture now carries its own texels and palette (a drawn
    replacement, look_r.py "replace" "@name")."""
    out = []
    for m in scene["materials"]:
        tex = m.get("texture") or {}
        if not m.get("spans") or "texels" not in tex:
            continue
        if tex.get("texel_scale"):
            # A drawing scaled in texel space (look_r.py "texel_scale"):
            # every image the material object shows (its frames: the eye's
            # blinks) scaled in place; the palettes stay.
            images = []
            for joint, cmds in m["spans"].items():
                for c in cmds or ():
                    g = _cmd(c)
                    if g.op == N.G_SETTIMG and g.target is not None and (g.w0 >> 21) & 7 == 2:
                        images.append(g.target[2])
                    elif g.op == N.G_DL and (g.w1 >> 24) == N.SEGMENT_E:
                        mobjs = fm.mobjs(scene.get("detail", "low"), int(joint))
                        k = (g.w1 & 0xFFFFFF) // 8
                        if k < len(mobjs):
                            pals = set(mobjs[k].palettes)
                            images += [s for s in mobjs[k].sprites if s not in pals]
            images = sorted(set(images))
            if not images:
                raise WriterError(f"material {tex.get('file')}: no base image to scale")
            out.append({"image": images[0], "scale_images": images, "width": int(tex["width"]),
                        "height": int(tex["height"]), "params": tex["texel_scale"]})
            continue
        image = palette = None
        for joint, cmds in m["spans"].items():
            for c in cmds or ():
                g = _cmd(c)
                if g.op == N.G_SETTIMG and g.target is not None:
                    if (g.w0 >> 21) & 7 == 2:          # CI: the texels
                        image = g.target[2]
                    else:
                        palette = g.target[2]
                elif g.op == N.G_DL and (g.w1 >> 24) == N.SEGMENT_E and palette is None:
                    mobjs = fm.mobjs(scene.get("detail", "low"), int(joint))
                    k = (g.w1 & 0xFFFFFF) // 8
                    if k < len(mobjs) and mobjs[k].palettes:
                        palette = mobjs[k].palettes[0]
        if tex.get("palette_rules") and palette is not None:
            # A recolour by palette rules (look_r.py): the base's texels
            # stay (the material object's frames too); every costume's copy
            # of the palette takes the rules in its own entry order
            # (write_rules_in_place).
            out.append({"image": image if image is not None else palette, "rules": tex["palette_rules"],
                        "palette_at": [palette]})
            continue
        if image is None or palette is None:
            raise WriterError(f"material {tex.get('file')}: no base image or palette to replace")
        out.append({"image": image, "texels": tex["texels"], "palette": tex["palette"], "palette_at": [palette]})
    return out


def write_scaled_in_place(model: N.RelocFile, images: list, width: int, height: int, params: dict) -> list:
    """CI4 images scaled about a texel (nearest-neighbour), each image's
    corner index filling what the drawing leaves, in place."""
    k = float(params["scale"])
    cx, cy = (float(c) for c in params["centre"])
    for at in images:
        raw = model.data[at:at + width * height // 2]
        old = [[(raw[(y * width + x) // 2] >> (4 if x % 2 == 0 else 0)) & 15 for x in range(width)]
               for y in range(height)]
        fill = old[0][0]
        new = []
        for y in range(height):
            row = []
            for x in range(width):
                ix = int(round(cx + (x + 0.5 - cx) / k - 0.5))
                iy = int(round(cy + (y + 0.5 - cy) / k - 0.5))
                row.append(old[iy][ix] if (0 <= ix < width and 0 <= iy < height) else fill)
            new.append(row)
        model.data[at:at + width * height // 2] = ci4_bytes(new)
    return images


def write_rules_in_place(fm: N.FighterModel, model: N.RelocFile, rules: list, palette_at: list) -> list:
    """Each costume's copy of a base palette (mobj_palettes_with) recoloured
    by look_r.py's palette rules, entry by entry, in place."""
    sys.path.insert(0, str(HERE))
    import look_r
    written = []
    for p in palette_at:
        for q in mobj_palettes_with(fm, p):
            for k in range(16):
                v = struct.unpack_from(">H", model.data, q + 2 * k)[0]
                rgba = [((v >> 11) & 31) * 255 // 31, ((v >> 6) & 31) * 255 // 31,
                        ((v >> 1) & 31) * 255 // 31, 255 if v & 1 else 0]
                model.data[q + 2 * k:q + 2 * k + 2] = rgba5551([look_r.palette_rule_colour(rgba, rules)])
            written.append(q)
    return sorted(set(written))


DEPTH_MASK = 0xFFF


def tree_depths(rows: list, scene: dict) -> list[int]:
    """The tree's id words (depth in the low 12 bits) with the scene's
    parents: a row the scene hangs from another parent than the base's
    (look_skirt.py: Palutena's hem on row 27, Peach's last row, from row 26)
    takes its parent's depth plus one. lbCommonSetupFighterPartsDObjs hangs
    each row from the last row before it one level up, so the new parent
    must be that row (row 27 follows row 26's list)."""
    ids = [row.depth for row in rows]
    joints = scene["joints"]
    count = min(len(rows) - 1, len(joints))
    for i in range(count):
        p = joints[i]["parent"]
        want = 0 if p < 0 else (ids[p] & DEPTH_MASK) + 1
        if (ids[i] & DEPTH_MASK) != want:
            ids[i] = (ids[i] & ~DEPTH_MASK) | want
    for i in range(count):
        d = ids[i] & DEPTH_MASK
        parent = -1 if d == 0 else max((k for k in range(i) if (ids[k] & DEPTH_MASK) == d - 1), default=-2)
        if parent != joints[i]["parent"]:
            raise WriterError(f"row {i}: the scene hangs it from row {joints[i]['parent']}, "
                              f"the tree's order from row {parent}")
    return ids


def tree_descs(fm: N.FighterModel, detail: str, scene: dict) -> list:
    """The detail's tree rows: the base's, then any row the scene adds past
    them (its "base_row": the base row whose rest it starts from), then the
    base's terminator."""
    rows = fm.trees[detail]
    joints = scene["joints"]
    extra = [rows[int(j["base_row"])] for j in joints[len(rows) - 1:]]
    return list(rows[:-1]) + extra + [rows[-1]]


def joint_rows(fm: N.FighterModel, detail: str, scene: dict, offsets: dict[int, int]) -> bytes:
    """The detail's JointTree: the base's rows with the look's translations,
    parents (tree_depths), added rows (tree_descs) and the new lists."""
    rows = tree_descs(fm, detail, scene)
    worlds = [np.array(j["world"], dtype=float) for j in scene["joints"]]
    ids = tree_depths(rows, scene)
    out = bytearray()
    for index, row in enumerate(rows):
        r = N.Joint(ids[index], row.dl, row.translate, row.rotate, row.scale)
        if index < len(worlds):
            p = scene["joints"][index]["parent"]
            local = worlds[index] if p < 0 else np.linalg.inv(worlds[p]) @ worlds[index]
            r.translate = tuple(float(c) for c in local[:3, 3])
            r.dl = offsets.get(index)
        out += struct.pack(">II9f", r.depth, 0, *r.translate, *r.rotate, *r.scale)
    return bytes(out)


def write(base_export: Path, scenes: dict[str, Path], main_id: int, model_id: int, out: Path,
          alias: frozenset = frozenset(), textures: list | None = None, ambient: int | None = None) -> dict:
    fm = N.FighterModel.from_export(base_export)
    model = N.RelocFile(fm.model.header, model_id, bytearray(fm.model.data), dict(fm.model.intern),
                        dict(fm.model.extern), list(fm.model.spare_externs))
    main = N.RelocFile(fm.main.header, main_id, bytearray(fm.main.data), dict(fm.main.intern),
                       {s: ((model_id if fid == fm.model.file_id else fid), off)
                        for s, (fid, off) in fm.main.extern.items()},
                       [model_id if x == fm.model.file_id else x for x in fm.main.spare_externs])
    report = {"base_model": fm.model.file_id, "base_main": fm.main.file_id, "details": {}, "textures": []}
    # In-place texture writes first (the file's own offsets).
    for t in textures or ():
        image = int(t["image"], 0) if isinstance(t["image"], str) else int(t["image"])
        texels = t.get("texels") or png_indices(Path(t["png"]), t["palette"])
        at = [int(x, 0) if isinstance(x, str) else int(x) for x in t["palette_at"]]
        report["textures"].append({"image": image, "palettes": write_in_place(fm, model, image, texels, t["palette"], at)})
    # Each in-place write once: two details from one scene (Ike's low look
    # for both) name the same base images and palettes, and a recolour or a
    # scale applied twice would compound.
    done_writes = set()
    for detail, path in scenes.items():
        scene = json.loads(path.read_text(encoding="utf-8"))
        for t in scene_texture_writes(fm, scene):
            key = (t["image"], tuple(t.get("palette_at", ())), "rules" in t, "scale_images" in t)
            if key in done_writes:
                continue
            done_writes.add(key)
            if "rules" in t:
                report["textures"].append({"image": t["image"], "palettes": write_rules_in_place(
                    fm, model, t["rules"], t["palette_at"])})
                continue
            if "scale_images" in t:
                report["textures"].append({"image": t["image"], "scaled": write_scaled_in_place(
                    model, t["scale_images"], t["width"], t["height"], t["params"])})
                continue
            report["textures"].append({"image": t["image"], "palettes": write_in_place(
                fm, model, t["image"], t["texels"], t["palette"], t["palette_at"])})
    app = Appender(model)
    lay = fm.lay
    # New textures by name, shared by the details (one copy of each).
    shared_textures: dict = {}
    # Parts (look_parts.py): joint -> variant -> detail -> list offset; the
    # tree's lists and the costume MObj lists, for the parts' descs.
    part_lists: dict = {}
    tree_lists: dict = {}
    joint_mobjs: dict = {}
    for detail, path in scenes.items():
        scene = json.loads(path.read_text(encoding="utf-8"))
        if ambient is not None:
            ambient_preludes(scene, ambient)
        lw = ListWriter(fm, model, scene, detail, None)
        lw.new_textures = shared_textures
        offsets = lw.build(app)
        for part in scene.get("parts") or ():
            part_lists.setdefault(part["joint"], {}).setdefault(part["variant"], {})[detail] = \
                lw.write_part_list(app, part)
        tree_lists[detail] = dict(offsets)
        mobj_lists = {}
        if lw.mobj_materials:
            template = palette_mobj_template(fm, detail)
            if template is None:
                raise WriterError(f"{detail}: costume palettes, but the base has no palette MObj to copy")
            mobj_lists = lw.write_mobjs(app, template)
            joint_mobjs[detail] = mobj_lists
            for joint, (sub_list, anim_list) in mobj_lists.items():
                for table, target in ((fm.mobjsubs[detail], sub_list), (fm.costume_matanims[detail], anim_list)):
                    if table is None:
                        raise WriterError(f"{detail}: the base has no MObj tables")
                    app.slot(table + 4 * joint, ("intern", model_id, target))
            report.setdefault("costume_mobjs", {})[detail] = {
                str(j): [lw.mats[mi]["texture"]["file"] for mi in calls]
                for j, calls in sorted(lw.mobj_materials.items())}
        app.align(8)
        tree_at = app.here
        app.put(joint_rows(fm, detail, scene, offsets))
        for index in range(len(tree_descs(fm, detail, scene))):
            if offsets.get(index) is not None:
                app.slot(tree_at + index * N.DOBJ_DESC_SIZE + 4, ("intern", model_id, offsets[index]))
        part = fm.commonparts + DETAIL_INDEX[detail] * lay["sizeof(FTCommonPart)"]
        main.extern[part + lay["FTCommonPart.dobjdesc"]] = (model_id, tree_at)
        # Model-part rows: a reset puts every joint back on its variant 0,
        # so variant 0 of each joint names our list for it; an aliased
        # joint's other variants too (the look has one form there).
        # A part switch puts the part's MObjs on the joint
        # (ftParamSetModelPartID): a retargeted part takes the list's costume
        # MObjs and their animations, and no main animation of the base's.
        retargeted = []
        part_dl = lay["FTModelPart.dl"]
        for slot, joint, variant, cell_detail in modelpart_dl_slots(fm):
            if cell_detail != detail or joint not in offsets or slot not in main.extern:
                continue
            if variant == 0 or joint in alias:
                main.extern[slot] = (model_id, offsets[joint])
                retargeted.append([joint, variant])
                if joint in mobj_lists:
                    base = slot - part_dl
                    sub_list, anim_list = mobj_lists[joint]
                    for field, target in (("FTModelPart.mobjsubs", sub_list),
                                          ("FTModelPart.costume_matanim_joints", anim_list)):
                        at = base + lay[field]
                        main.intern.pop(at, None)
                        main.extern[at] = (model_id, target)
                    at = base + lay["FTModelPart.main_matanim_joints"]
                    main.intern.pop(at, None)
                    main.extern.pop(at, None)
                    main.data[at:at + 4] = bytes(4)
        report["details"][detail] = {"tree": tree_at, "lists": {str(j): o for j, o in sorted(offsets.items())},
                                     "model_parts": retargeted,
                                     "triangles": len(scene["triangles"])}
    if part_lists:
        report["parts"] = write_part_descs(fm, main, model_id, part_lists, tree_lists, joint_mobjs, alias)
    # A row the base never made a DObj for (FTAttributes.setup_parts) that a
    # detail now draws (Palutena's hem on row 27): its bit set, so the tree
    # makes it.
    made = set_setup_parts(fm, main, {j for lists in tree_lists.values() for j in lists})
    if made:
        report["setup_parts_added"] = made
    # A row's role moved to another row (look_skirt.py: Peach's held joint,
    # row 27, to row 28): the attributes that name its joint name the new one.
    roles = json.loads(next(iter(scenes.values())).read_text(encoding="utf-8")).get("joint_roles") or []
    if roles:
        report["joint_roles"] = move_joint_roles(fm, main, roles)
    # A look on its own proportions plays the base's clips at its limb
    # lengths: FTAttributes.translate_scales (look_blend.py), one Vec3f per
    # fighter joint, appended to the main file.
    scales = [json.loads(p.read_text(encoding="utf-8")).get("translate_scales") for p in scenes.values()]
    scales = [s for s in scales if s]
    if scales:
        if any(s != scales[0] for s in scales):
            raise WriterError("the details' translate_scales differ")
        slot = fm.attr + lay["FTAttributes.translate_scales"]
        if slot in main.intern or slot in main.extern or any(main.data[slot:slot + 4]):
            raise WriterError("the base's main already has translate_scales")
        mapp = Appender(main)
        at = mapp.put(b"".join(struct.pack(">3f", *(float(c) for c in v)) for v in scales[0]))
        mapp.slot(slot, ("intern", main_id, at))
        report["translate_scales"] = at
    out.mkdir(parents=True, exist_ok=True)
    (out / f"{main_id:04x}").write_bytes(main.to_bytes())
    (out / f"{model_id:04x}").write_bytes(model.to_bytes())
    report["model_size"] = len(model.data)
    (out / "writer.json").write_text(json.dumps(report, indent=1) + "\n", encoding="utf-8")
    return report


def move_joint_roles(fm: N.FighterModel, main: N.RelocFile, roles: list) -> list:
    """Each role's joint (row + 4) renamed in FTAttributes: the held-item
    joints, the feet, the effect joints, and the named hidden parts' root
    and parent (FTHiddenPart: root, parent, part index, kind)."""
    lay = fm.lay
    done = []
    for role in roles:
        old, new = int(role["from"]) + 4, int(role["to"]) + 4
        fields = [lay["FTAttributes.joint_itemheavy_id"], lay["FTAttributes.joint_itemlight_id"],
                  lay["FTAttributes.joint_rfoot_id"], lay["FTAttributes.joint_lfoot_id"]]
        fields += [lay["FTAttributes.effect_joint_ids"] + 4 * k for k in range(5)]
        for off in fields:
            at = fm.attr + off
            if struct.unpack_from(">i", main.data, at)[0] == old:
                struct.pack_into(">i", main.data, at, new)
                done.append([off, old, new])
        ref = main.ptr(fm.attr + lay["FTAttributes.hiddenparts"])
        for part in role.get("hidden_parts", ()):
            if ref is None or ref[0] != "intern":
                raise WriterError("the role names hidden parts but hiddenparts is not in the main file")
            at = ref[2] + 16 * int(part)
            for k in range(2):
                if struct.unpack_from(">i", main.data, at + 4 * k)[0] == old:
                    struct.pack_into(">i", main.data, at + 4 * k, new)
                    done.append([f"hidden {part}.{k}", old, new])
    return done


def set_setup_parts(fm: N.FighterModel, main: N.RelocFile, rows: set) -> list[int]:
    """Set FTAttributes.setup_parts' bit (two u32 masks from bit 31 of the
    first) for each of these rows the base does not make; returns them."""
    ref = main.ptr(fm.attr + fm.lay["FTAttributes.setup_parts"])
    if ref is None or ref[0] != "intern":
        raise WriterError("setup_parts is not in the main file")
    masks = list(struct.unpack_from(">2I", main.data, ref[2]))
    added = []
    for row in sorted(rows):
        word, bit = row // 32, 31 - row % 32
        if not (masks[word] >> bit) & 1:
            masks[word] |= 1 << bit
            added.append(row)
    struct.pack_into(">2I", main.data, ref[2], *masks)
    return added


def write_part_descs(fm: N.FighterModel, main: N.RelocFile, model_id: int, part_lists: dict,
                     tree_lists: dict, joint_mobjs: dict, alias: frozenset) -> dict:
    """Each part joint's FTModelPartDesc, appended to the main file and named
    by FTModelPartContainer: the base's variants (as the writer retargeted
    them) or, for a joint the base had none for, variant 0 drawing the tree's
    list with the joint's MObjs (a reset puts every joint with a desc back on
    variant 0); then the parts' variants, the same MObjs, no main animation.
    Cells run [variant][high, low]."""
    lay = fm.lay
    cell = lay["sizeof(FTModelPart)"]
    container = main.ptr(fm.attr + lay["FTAttributes.modelparts_container"])
    if container is None or container[0] != "intern":
        raise WriterError("the base has no model-part container to name a part's desc")
    existing: dict[int, dict] = {}
    for slot, joint, variant, detail in modelpart_dl_slots(fm):
        existing.setdefault(joint, {})[(variant, detail)] = slot - lay["FTModelPart.dl"]
    mapp = Appender(main)
    out = {}
    for joint, variants in sorted(part_lists.items()):
        old = existing.get(joint, {})
        count = max([v + 1 for v in variants] + [v + 1 for v, _ in old])
        mapp.align(8)
        at = mapp.put(bytes(cell * 2 * count))
        for v in range(count):
            for d, detail in enumerate(("high", "low")):
                c = at + cell * (2 * v + d)
                if v in variants:
                    src = None
                    dl = variants[v][detail]
                elif (v, detail) in old:
                    src = old[(v, detail)]
                    dl = None
                elif v == 0:
                    src = None
                    dl = tree_lists[detail][joint]
                else:
                    raise WriterError(f"joint {joint}: variant {v} has no list")
                if src is not None:
                    # The base's cell as the writer left it (retargeted lists
                    # and MObjs for variant 0 and aliased joints).
                    for field in ("FTModelPart.dl", "FTModelPart.mobjsubs",
                                  "FTModelPart.costume_matanim_joints", "FTModelPart.main_matanim_joints"):
                        off = lay[field]
                        target = main.ptr(src + off)
                        if target is not None:
                            mapp.slot(c + off, target)
                    main.data[c + lay["FTModelPart.flags"]] = main.data[src + lay["FTModelPart.flags"]]
                    continue
                mapp.slot(c + lay["FTModelPart.dl"], ("extern", model_id, dl))
                lists = (joint_mobjs.get(detail) or {}).get(joint)
                if lists is not None:
                    mapp.slot(c + lay["FTModelPart.mobjsubs"], ("extern", model_id, lists[0]))
                    mapp.slot(c + lay["FTModelPart.costume_matanim_joints"], ("extern", model_id, lists[1]))
                else:
                    for field, table in (("FTModelPart.mobjsubs", fm.mobjsubs[detail]),
                                         ("FTModelPart.costume_matanim_joints", fm.costume_matanims[detail])):
                        target = fm.model.ptr(table + 4 * joint) if table is not None else None
                        if target is not None:
                            mapp.slot(c + lay[field], ("extern", model_id, target[2]))
        mapp.slot(container[2] + 4 * joint, ("intern", main.file_id, at))
        out[str(joint)] = {"desc": at, "variants": count}
    return out


def modelpart_dl_slots(fm: N.FighterModel) -> list[tuple[int, int, int, str]]:
    """Every FTModelPart.dl cell of the main file: (slot, joint, variant,
    detail). FTModelPartContainer holds one FTModelPartDesc pointer a joint
    (its index is the joint's row); a desc is [variant][high, low] cells,
    as many variants as fit before the next desc."""
    lay = fm.lay
    container = fm.main.ptr(fm.attr + lay["FTAttributes.modelparts_container"])
    if container is None or container[0] != "intern":
        return []
    descs = {}
    for joint in range(len(fm.trees["high"]) - 1):
        ref = fm.main.ptr(container[2] + 4 * joint)
        if ref is not None and ref[0] == "intern":
            descs[ref[2]] = joint
    starts = sorted(set(descs) | {container[2]})
    cell = lay["sizeof(FTModelPart)"]
    out = []
    for at, joint in sorted(descs.items()):
        end = min([s for s in starts if s > at], default=at + 16 * cell)
        n = (end - at) // cell
        for k in range(n):
            slot = at + k * cell + lay["FTModelPart.dl"]
            out.append((slot, joint, k // 2, "high" if k % 2 == 0 else "low"))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--base-export", type=Path, required=True)
    ap.add_argument("--scene", action="append", required=True, help="detail=path/to/scene.json")
    ap.add_argument("--main-id", type=lambda s: int(s, 0), required=True)
    ap.add_argument("--model-id", type=lambda s: int(s, 0), required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--alias", default="", help="joints whose every model-part variant draws the look's list")
    ap.add_argument("--textures", type=Path, action="append", default=[],
                    help="in-place texture writes (face_frames.py writer.json)")
    ap.add_argument("--lift-palettes", action="store_true",
                    help="every palette written lifted (lift_colour: owner p6s, Palutena brighter)")
    ap.add_argument("--ambient", type=lambda s: int(s, 16),
                    help="the lists' ambient light colour, RRGGBB hex (the base's preludes')")
    args = ap.parse_args()
    global LIFT_PALETTES
    LIFT_PALETTES = bool(args.lift_palettes)
    scenes = {}
    for s in args.scene:
        detail, path = s.split("=", 1)
        if detail not in DETAIL_INDEX:
            raise WriterError(f"--scene {s}: detail must be high or low")
        scenes[detail] = Path(path)
    alias = frozenset(int(x) for x in args.alias.split(",") if x)
    textures = [t for path in args.textures for t in json.loads(path.read_text(encoding="utf-8"))]
    report = write(args.base_export, scenes, args.main_id, args.model_id, args.out, alias, textures, args.ambient)
    print(json.dumps(report["details"], indent=1))
    print(f"model {args.model_id:#x}: {report['model_size']} bytes -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
