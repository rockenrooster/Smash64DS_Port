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


def rgba5551(palette) -> bytes:
    out = bytearray()
    for c in palette:
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
        self.app = None

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
        if src is None:
            cmds = self.synth_span(m)
        else:
            cmds = [_cmd(c) for c in src]
            loads = any(g.op in (N.G_SETTIMG, N.G_LOADBLOCK, N.G_LOADTILE) for g in cmds)
            if loads and not m.get("texture"):
                # A recoloured texture: drawn in its colour alone.
                cmds = self.synth_span(m)
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
                if keep_mobj:
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

    def synth_span(self, m: dict) -> list[N.Gfx]:
        """A span for a material the base never drew (an added part)."""
        prim = int(m["prim"], 16)
        tex = m.get("texture")
        if not tex:
            return [N.Gfx(0xE7000000, 0), N.Gfx(0xFC327E05, 0xFF17FDFF), N.Gfx(0xD7000000, 0),
                    N.Gfx(0xFA000000, prim)]
        w, h = int(tex["width"]), int(tex["height"])
        tex_at, pal_at = self.new_texture(m)
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
                 N.Gfx(0xFD100000, pal_at // 4, ("intern", fid, pal_at)),
                 N.Gfx(0xE6000000, 0), N.Gfx(0xF0000000, 0x0503C000), N.Gfx(0xE7000000, 0),
                 N.Gfx(0xD7000002, 0xFFFFFFFF),
                 N.Gfx(0xF2000000, (((w - 1) << 2) << 12) | ((h - 1) << 2)),
                 N.Gfx(0xFD500000, tex_at // 4, ("intern", fid, tex_at)),
                 N.Gfx(0xE6000000, 0),
                 N.Gfx(0xF3000000, (7 << 24) | ((w * h // 4 - 1) << 12) | (2048 // line)),
                 N.Gfx(0xE7000000, 0)]
        return cmds

    def new_texture(self, m: dict) -> tuple:
        """Append a new material's texels and palette (once)."""
        tex = m["texture"]
        key = tex.get("file")
        if key in self.new_textures:
            return self.new_textures[key]
        app = self.app
        app.align(8)
        tex_at = app.put(ci4_bytes(tex["texels"]))
        app.align(8)
        pal_at = app.put(rgba5551(tex["palette"]))
        self.new_textures[key] = (tex_at, pal_at)
        return tex_at, pal_at

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
        app.align(8)
        return app.put(b"".join(self.vtx_bytes(i, self.mats[mi]) for i in idx))

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
        if image is None or palette is None:
            raise WriterError(f"material {tex.get('file')}: no base image or palette to replace")
        out.append({"image": image, "texels": tex["texels"], "palette": tex["palette"], "palette_at": [palette]})
    return out


def joint_rows(fm: N.FighterModel, detail: str, scene: dict, offsets: dict[int, int]) -> bytes:
    """The detail's JointTree: the base's rows with the look's translations
    and the new lists."""
    rows = fm.trees[detail]
    worlds = [np.array(j["world"], dtype=float) for j in scene["joints"]]
    out = bytearray()
    for index, row in enumerate(rows):
        r = N.Joint(row.depth, row.dl, row.translate, row.rotate, row.scale)
        if index < len(worlds):
            p = scene["joints"][index]["parent"]
            local = worlds[index] if p < 0 else np.linalg.inv(worlds[p]) @ worlds[index]
            r.translate = tuple(float(c) for c in local[:3, 3])
            r.dl = offsets.get(index)
        out += struct.pack(">II9f", r.depth, 0, *r.translate, *r.rotate, *r.scale)
    return bytes(out)


def write(base_export: Path, scenes: dict[str, Path], main_id: int, model_id: int, out: Path,
          alias: frozenset = frozenset(), textures: list | None = None) -> dict:
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
    for detail, path in scenes.items():
        scene = json.loads(path.read_text(encoding="utf-8"))
        for t in scene_texture_writes(fm, scene):
            report["textures"].append({"image": t["image"], "palettes": write_in_place(
                fm, model, t["image"], t["texels"], t["palette"], t["palette_at"])})
    app = Appender(model)
    lay = fm.lay
    for detail, path in scenes.items():
        scene = json.loads(path.read_text(encoding="utf-8"))
        lw = ListWriter(fm, model, scene, detail, None)
        offsets = lw.build(app)
        app.align(8)
        tree_at = app.here
        app.put(joint_rows(fm, detail, scene, offsets))
        for index in range(len(fm.trees[detail])):
            if offsets.get(index) is not None:
                app.slot(tree_at + index * N.DOBJ_DESC_SIZE + 4, ("intern", model_id, offsets[index]))
        part = fm.commonparts + DETAIL_INDEX[detail] * lay["sizeof(FTCommonPart)"]
        main.extern[part + lay["FTCommonPart.dobjdesc"]] = (model_id, tree_at)
        # Model-part rows: a reset puts every joint back on its variant 0,
        # so variant 0 of each joint names our list for it; an aliased
        # joint's other variants too (the look has one form there).
        retargeted = []
        for slot, joint, variant, cell_detail in modelpart_dl_slots(fm):
            if cell_detail != detail or joint not in offsets or slot not in main.extern:
                continue
            if variant == 0 or joint in alias:
                main.extern[slot] = (model_id, offsets[joint])
                retargeted.append([joint, variant])
        report["details"][detail] = {"tree": tree_at, "lists": {str(j): o for j, o in sorted(offsets.items())},
                                     "model_parts": retargeted,
                                     "triangles": len(scene["triangles"])}
    out.mkdir(parents=True, exist_ok=True)
    (out / f"{main_id:04x}").write_bytes(main.to_bytes())
    (out / f"{model_id:04x}").write_bytes(model.to_bytes())
    report["model_size"] = len(model.data)
    (out / "writer.json").write_text(json.dumps(report, indent=1) + "\n", encoding="utf-8")
    return report


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
    args = ap.parse_args()
    scenes = {}
    for s in args.scene:
        detail, path = s.split("=", 1)
        if detail not in DETAIL_INDEX:
            raise WriterError(f"--scene {s}: detail must be high or low")
        scenes[detail] = Path(path)
    alias = frozenset(int(x) for x in args.alias.split(",") if x)
    textures = [t for path in args.textures for t in json.loads(path.read_text(encoding="utf-8"))]
    report = write(args.base_export, scenes, args.main_id, args.model_id, args.out, alias, textures)
    print(json.dumps(report["details"], indent=1))
    print(f"model {args.model_id:#x}: {report['model_size']} bytes -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
