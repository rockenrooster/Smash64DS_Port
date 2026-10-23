#!/usr/bin/env python3
"""P4 S8: Remix's Kirby hats as native copy-hat images.

A Remix hat (KirbyHats.add_hat) re-skins a source copy hat: Kirby's joint-6
model part keeps the source hat's materials and tracks, and its display
lists come from KIRBY_CUSTOM_HATS (use_extended_special_parts_). This module
compiles each hat a power of the enabled contents draws through the owner
generator's own copy-hat path (generate_nds_native_owners.py
build_p2_kirby_hat_runtime_context, which also carries the trio body baked
against the hat's own head): the hat's two lists and their vertices are
appended to Kirby's model payload, standing in for the source hat's model
part, and the textures they load move to one texture file per content, so
the lists name them through extern relocations and the image carries them
as foreign images the runtime reads from that file.

The runtime keeps the source hat's part on the live DObj (its offset is the
root identity), binds the Remix hat's image in the source hat's place, and
publishes the Remix hat id as the trio head, whose program this module
emits beside the source heads' (the head list is regenerated with them).

    p4_kirby_hats.py --staging <remix staging> --manifest <gen>/kirby/manifest.json
        --out <gen>/kirby

Outputs (build output: derived from the user's ROM):
  hats/nds_p4_kirby_hats.generated.h     image types and the image X-macro
  hats/nds_p4_kirby_hats.generated.inc   the hats' trio programs and head list
  hats/nds_native_fighter_p4kirbyhat_<id>_<detail>.image.c
  hats/src/kirby_<id>                    synthesized payloads (generator input)
  o2r/<file>                             each content's hat textures
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(REPO / "scripts" / "fighters"))
import remix_rom as R  # noqa: E402
import _paths  # noqa: E402,F401
import generate_nds_native_owners as O  # noqa: E402
import generate_nds_native_owner_images as I  # noqa: E402

KIRBY_MODEL_FILE = 0x148
NO_RELOC = 0xFFFF
DETAILS = ("high", "low")
# G_SETTIMG sizes in bytes per texel (G_IM_SIZ_4b .. 32b).
SIZ_BYTES = {0: 0.5, 1: 1, 2: 2, 3: 4}
OP_VTX, OP_DL, OP_ENDDL, OP_SETTIMG = 0x01, 0xDE, 0xDF, 0xFD
OP_LOADBLOCK, OP_LOADTLUT, OP_LOADTILE, OP_NOOP = 0xF3, 0xF0, 0xF4, 0x00
SEGMENT_E = 0x0E
TRIANGLE_OPS = (0x05, 0x06, 0x07)  # G_TRI1, G_TRI2, G_QUAD
OP_MOVEWORD, G_MW_LIGHTCOL = 0xDB, 0x0A


class HatError(RuntimeError):
    pass


def walk_list(data: bytes, slots: dict, off: int, label: str):
    """A Remix hat list: its commands, vertex ranges and texture ranges.

    The lists are linear (their only calls are the MObjs' segment-E lists);
    a texture's size is its load's (LOADBLOCK texels, LOADTLUT colours) at
    the image the list itself set, never one a material list set. An image
    no load reads before the next image, material list or the end (Wolf's
    hat: its load's opcode was made a pipe sync) is dropped from the list:
    the RDP's image register is all it changes."""
    cmds, vtx, tex = [], [], []
    image = None
    image_index = None
    dangling = set()
    pc = off
    while True:
        if pc + 8 > len(data):
            raise HatError(f"{label}: list at {off:#x} runs off its file")
        w0, w1 = struct.unpack_from(">II", data, pc)
        op = w0 >> 24
        cmds.append((pc, op, w0, w1))
        if op == OP_VTX:
            s = slots.get(pc + 4)
            if s is None or s[0] != "intern":
                raise HatError(f"{label}: vertex load at {pc:#x} is not internal")
            vtx.append((s[1], ((w0 >> 12) & 0xFF) * 16))
        elif op == OP_SETTIMG:
            s = slots.get(pc + 4)
            if s is None or s[0] != "intern":
                raise HatError(f"{label}: image at {pc:#x} is not internal")
            if image_index is not None:
                dangling.add(image_index)
            image = (s[1], (w0 >> 19) & 3)
            image_index = pc
        elif op == OP_LOADBLOCK and image is not None:
            texels = ((w1 >> 12) & 0xFFF) + 1
            tex.append((image[0], int(texels * SIZ_BYTES[image[1]] + 0.5)))
            image_index = None
        elif op == OP_LOADTLUT and image is not None:
            tex.append((image[0], (((w1 >> 14) & 0x3FF) + 1) * 2))
            image_index = None
        elif op == OP_LOADTILE:
            raise HatError(f"{label}: LOADTILE at {pc:#x} is not lowered")
        elif op == OP_DL:
            if (w1 >> 24) != SEGMENT_E or ((w0 >> 16) & 0xFF) != 0:
                raise HatError(f"{label}: call at {pc:#x} is not a material list")
            if image_index is not None:
                dangling.add(image_index)
            image = None
            image_index = None
        if op == OP_ENDDL:
            if image_index is not None:
                dangling.add(image_index)
            return [c for c in cmds if c[0] not in dangling], vtx, tex
        pc += 8


def merge(ranges):
    out = []
    for a, n in sorted(ranges):
        b = a + n
        if out and a <= out[-1][1]:
            out[-1][1] = max(out[-1][1], b)
        else:
            out.append([a, b])
    return out


def o2r_container(file_id: int, data: bytes, rel_int=NO_RELOC, rel_ext=NO_RELOC,
                  ext_ids=()) -> bytes:
    out = bytearray(R.O2R_HEADER)
    out += struct.pack("<IHHI", file_id, rel_int, rel_ext, len(ext_ids))
    out += struct.pack(f"<{len(ext_ids)}H", *ext_ids)
    out += struct.pack("<I", len(data))
    out += data
    return bytes(out)


class HatSource:
    """One Remix hat: its lists (high, low), vertex and texture ranges."""

    def __init__(self, rom: R.Rom, hat: int, row: dict):
        self.hat = hat
        self.base = row["base"]
        self.file = row["file"]
        self.data = rom.file_bytes(self.file)
        self.slots = rom.reloc_slots(self.file)
        self.lists = {}
        self.vtx = []
        self.tex = []
        for detail, off in zip(DETAILS, row["dl"]):
            cmds, vtx, tex = walk_list(self.data, self.slots, off, f"hat {hat:#x} {detail}")
            self.lists[detail] = cmds
            self.vtx += vtx
            self.tex += tex


def texture_file(hats: list[HatSource]):
    """One content's texture file: every range its hats' lists load."""
    blob = bytearray()
    where = {}
    for h in hats:
        for a, b in merge(h.tex):
            key = (h.file, a, b)
            if key in where:
                continue
            while len(blob) % 8:
                blob.append(0)
            where[key] = len(blob)
            blob += h.data[a:b]
    return bytes(blob), where


def single_material_calls(cmds, label: str):
    """Between two triangle runs the source draws nothing, so of two calls to
    the same material list there (Dedede's hat opens with one, then sets its
    lights and calls it again before its first load) only the last one's
    state reaches a triangle: the earlier is dropped. Two different lists in
    one span would each leave state behind, which the decoder does not
    lower."""
    keep = list(cmds)
    span = []
    for index, cmd in enumerate(cmds):
        if cmd[1] in TRIANGLE_OPS:
            span = []
            continue
        if cmd[1] == OP_DL:
            for earlier in span:
                if cmds[earlier][3] != cmd[3]:
                    raise HatError(f"{label}: two material lists before one run "
                                   f"({cmds[earlier][3]:#x}, {cmd[3]:#x})")
                keep[earlier] = None
            span = [index]
    return [c for c in keep if c is not None]


def loadable(hat: HatSource, cmd) -> bool:
    """Every vertex the load names fits the GX VERTEX16 lattice (12.4)."""
    pc, _op, w0, _w1 = cmd
    at = hat.slots[pc + 4][1]
    for k in range((w0 >> 12) & 0xFF):
        if any(abs(v) > 0x7FF for v in struct.unpack_from(">hhh", hat.data, at + 16 * k)):
            return False
    return True


KIRBY_MAIN_O2R = "decomp/BattleShip-main/BattleShip_o2r/reloc_fighters_main/KirbyMain"
MODELPARTS_DESC = 0xCC   # dKirbyMain_modelparts_desc_0x0CC: [modelpart][detail]
MODELPART_SIZE = 20


MOBJSUB_FLAGS = 0x30
MOBJ_ALPHA, MOBJ_SPLIT, MOBJ_PALETTE, MOBJ_FRAC = 0x1, 0x2, 0x4, 0x10
MOBJ_TEXTURE, MOBJ_PRIM, MOBJ_ENV, MOBJ_BLEND = 0x80, 0x200, 0x400, 0x800
MOBJ_LIGHT1, MOBJ_LIGHT2 = 0x1000, 0x2000


def material_list(flags: int) -> list[str]:
    """The commands gcDrawMObjForDObj writes for one MObj, in order."""
    if flags == 0:
        flags = MOBJ_TEXTURE | 0x20 | MOBJ_ALPHA
    cmds = []
    if flags & MOBJ_PALETTE:
        cmds.append("palette")
        if flags & (MOBJ_SPLIT | MOBJ_ALPHA):
            cmds += ["sync", "tile", "sync", "tlut", "sync"]
    cmds += ["light"] * (2 * bool(flags & MOBJ_LIGHT1) + 2 * bool(flags & MOBJ_LIGHT2))
    if flags & (MOBJ_PRIM | MOBJ_FRAC | 0x8):
        cmds.append("prim")
    if flags & MOBJ_ENV:
        cmds.append("env")
    if flags & MOBJ_BLEND:
        cmds.append("blend")
    if flags & (MOBJ_FRAC | MOBJ_SPLIT):
        cmds.append("image_next")
        if flags & (MOBJ_FRAC | MOBJ_ALPHA):
            cmds += ["sync", "block", "sync"]
    if flags & (MOBJ_FRAC | MOBJ_ALPHA):
        cmds.append("image")
    cmds += ["tile_size"] * (bool(flags & 0x20) + bool(flags & 0x40))
    if flags & MOBJ_TEXTURE:
        cmds.append("texture")
    return cmds + ["end"]


def source_hat_mobj_counts() -> dict:
    """(source hat, detail) -> the material list length of each MObj its
    model part gives joint 6. gcDrawMObjForDObj's segment-E table holds one
    branch per MObj, and the MObjs' lists follow the table in order, so a
    call past the table runs into them."""
    def load(rel: str):
        path = REPO / rel
        data = path.read_bytes()
        fid = struct.unpack_from("<I", data, O.O2R_RESOURCE_HEADER_SIZE)[0]
        return O.stage_manifest.load_o2r(REPO, O.stage_manifest.InputSpec(
            rel, hashlib.sha256(data).hexdigest(), fid))

    main = load(KIRBY_MAIN_O2R)
    model = load(str(O.P2_O2R_ASSETS["kirby"][0]).replace("\\", "/"))
    counts = {}
    for part in range(15):
        for d, detail in enumerate(DETAILS):
            ref = main.pointer_at(MODELPARTS_DESC + (part * 2 + d) * MODELPART_SIZE + 4)
            lengths = []
            if ref is not None:
                if ref.asset_id != model.file_id:
                    raise HatError(f"source hat {part} {detail}: MObjSubs in file {ref.asset_id:#x}")
                while True:
                    sub = model.pointer_at(ref.offset + 4 * len(lengths))
                    if sub is None:
                        break
                    flags = struct.unpack_from(">H", model.payload, sub.offset + MOBJSUB_FLAGS)[0]
                    lengths.append(material_list(flags))
            counts[(part, detail)] = lengths
    return counts


def synthesize(hat: HatSource, tex_where: dict, tex_file: int, vanilla: bytes,
               mobj_counts: dict):
    """Kirby's model O2R with the hat's lists and vertices appended (its
    vertex loads internal relocations, its images extern ones into the
    texture file). Returns the container, each detail's list offset and the
    model's own data size."""
    H = O.O2R_RESOURCE_HEADER_SIZE
    file_id, rel_int, rel_ext, ext_count = struct.unpack_from("<IHHI", vanilla, H)
    if file_id != KIRBY_MODEL_FILE:
        raise HatError(f"Kirby model O2R names file {file_id:#x}")
    ext_ids = list(struct.unpack_from(f"<{ext_count}H", vanilla, H + 12))
    dpos = H + 12 + ext_count * 2
    dsize = struct.unpack_from("<I", vanilla, dpos)[0]
    body = bytearray(vanilla[dpos + 4:dpos + 4 + dsize])
    body += bytes(((len(body) + 15) & ~15) - len(body))
    vmap = {}
    for a, b in merge(hat.vtx):
        vmap[(a, b)] = len(body)
        body += hat.data[a:b]

    def moved(table, addr, what):
        for key, new in table.items():
            if key[-2] <= addr < key[-1]:
                return new + addr - key[-2]
        raise HatError(f"hat {hat.hat:#x}: {what} {addr:#x} outside its ranges")

    tex_table = {k: v for k, v in tex_where.items() if k[0] == hat.file}
    new_intern, new_extern, offsets = [], [], {}
    for detail in DETAILS:
        while len(body) % 8:
            body.append(0)
        offsets[detail] = len(body)
        cmds = hat.lists[detail]
        last_tri = max((i for i, c in enumerate(cmds) if c[1] in TRIANGLE_OPS), default=-1)
        tail_loads = [c for c in cmds[last_tri + 1:] if c[1] == OP_VTX]
        # Vertices a list loads after its last triangle are for the next list
        # (Bowser's and Dedede's open mouths load the trio body's): one
        # degenerate triangle on the last load's first slot keeps them in the
        # head's decode, so the body bakes against them as the source draws
        # it. A tail load whose words are no positions the DS can draw
        # (Lanky's hat: (-6400, 0, 0), (-1006, 32261, -233)) feeds no list
        # of his power and is left out.
        if any(not loadable(hat, c) for c in tail_loads):
            dropped = {c[0] for c in tail_loads}
            cmds = [c for c in cmds if c[0] not in dropped]
            tail_loads = []
        # The light colours a list restores after its last triangle are the
        # next list's (Remix's hats put Kirby's back for his body); on the DS
        # every root starts from its own light preamble.
        cmds = [c for i, c in enumerate(cmds)
                if not (i > last_tri and c[1] == OP_MOVEWORD and
                        ((c[2] >> 16) & 0xFF) == G_MW_LIGHTCOL)]
        # A material call past the source hat's MObjs (Remix's lists keep
        # their own model's slots: Banjo's hat calls 12 on Falcon's two) runs,
        # on the N64, into the MObjs' lists, which follow the table, from the
        # command it lands on to that list's end. Landing at or before the
        # list's texture image, it sets that MObj's material (drawn as the
        # whole MObj); past it, only its tile sizes and texture scale, which
        # the DS leaves as they were (the call is dropped).
        lists = mobj_counts[(hat.base, detail)]
        remapped = []
        for c in cmds:
            if c[1] == OP_DL and (c[3] >> 24) == SEGMENT_E:
                index = (c[3] & 0xFFFFFF) // 8
                if index >= len(lists):
                    command, j = index - len(lists), 0
                    while j < len(lists) and command >= len(lists[j]):
                        command -= len(lists[j])
                        j += 1
                    if j == len(lists):
                        raise HatError(f"hat {hat.hat:#x} {detail}: material {index} past "
                                       f"the source hat's {len(lists)} MObjs' lists")
                    sets = [k for k, kind in enumerate(lists[j])
                            if kind in ("palette", "tlut", "prim", "env", "blend",
                                        "image_next", "block", "image")]
                    if not sets or command > sets[-1]:
                        continue
                    c = (c[0], c[1], c[2], (SEGMENT_E << 24) | (8 * j))
            remapped.append(c)
        cmds = single_material_calls(remapped, f"hat {hat.hat:#x} {detail}")
        for pc, op, w0, w1 in cmds:
            if op == OP_NOOP:  # G_SPNOOP: no state
                continue
            if op == OP_ENDDL and tail_loads:
                last = tail_loads[-1][2]
                first_slot = ((last >> 1) & 0x7F) - ((last >> 12) & 0xFF)
                body += struct.pack(">II", (0x05 << 24) | ((first_slot * 2) * 0x10101), 0)
            at = len(body)
            body += struct.pack(">II", w0, w1)
            if op == OP_VTX:
                new_intern.append((at + 4, moved(vmap, hat.slots[pc + 4][1], "vertex")))
            elif op == OP_SETTIMG:
                new_extern.append((at + 4, moved(tex_table, hat.slots[pc + 4][1], "image")))
    if len(body) > 0x40000:
        raise HatError(f"hat {hat.hat:#x}: payload past the relocation word range")

    def chain_end(first):
        w, last = first, None
        while w != NO_RELOC:
            last = w * 4
            w = struct.unpack_from(">I", body, last)[0] >> 16
        return last

    def link(first, items):
        prev = chain_end(first) if first != NO_RELOC else None
        for slot, target in items:
            struct.pack_into(">I", body, slot, (NO_RELOC << 16) | (target >> 2))
            if prev is None:
                first = slot // 4
            else:
                word = struct.unpack_from(">I", body, prev)[0]
                struct.pack_into(">I", body, prev, ((slot // 4) << 16) | (word & 0xFFFF))
            prev = slot
        return first

    rel_int = link(rel_int, new_intern)
    rel_ext = link(rel_ext, new_extern)
    ext_ids += [tex_file] * len(new_extern)
    return o2r_container(file_id, bytes(body), rel_int, rel_ext, ext_ids), offsets, dsize


class GeneratorState:
    """The owner generator's module state a hat build patches, restored after."""

    NAMES = ("P2_O2R_ASSETS", "P2_MODEL_PART_ROOT_VARIANTS", "KIRBY_COPY_HAT_MODEL_PART_IDS",
             "KIRBY_TRIO_CONTEXTS", "KIRBY_TRIO_EXTRA_JOINT_HEADS",
             "build_kirby_trio_faithful_specs")

    def __init__(self):
        self.saved = {n: getattr(O, n) for n in self.NAMES}
        self.saved["P2_O2R_ASSETS"] = dict(O.P2_O2R_ASSETS)
        self.saved["P2_MODEL_PART_ROOT_VARIANTS"] = {
            k: dict(v) for k, v in O.P2_MODEL_PART_ROOT_VARIANTS.items()}

    def restore(self):
        for n, v in self.saved.items():
            if n == "P2_O2R_ASSETS":
                O.P2_O2R_ASSETS.clear()
                O.P2_O2R_ASSETS.update(v)
            elif n == "P2_MODEL_PART_ROOT_VARIANTS":
                O.P2_MODEL_PART_ROOT_VARIANTS.clear()
                O.P2_MODEL_PART_ROOT_VARIANTS.update({k: dict(d) for k, d in v.items()})
            else:
                setattr(O, n, v)
        clear_caches()

    def patch(self, payload_path: Path, base: int, offsets: dict):
        rel = payload_path.resolve().relative_to(REPO.resolve())
        sha = hashlib.sha256(payload_path.read_bytes()).hexdigest()
        O.P2_O2R_ASSETS["kirby"] = (rel, KIRBY_MODEL_FILE, sha)
        for detail in DETAILS:
            specs = list(O.P2_MODEL_PART_ROOT_VARIANTS["kirby"][detail])
            specs[base - 1] = (specs[base - 1][0], offsets[detail])
            O.P2_MODEL_PART_ROOT_VARIANTS["kirby"][detail] = tuple(specs)
        if base not in O.KIRBY_COPY_HAT_MODEL_PART_IDS:
            O.KIRBY_COPY_HAT_MODEL_PART_IDS = tuple(O.KIRBY_COPY_HAT_MODEL_PART_IDS) + (base,)
        if (base, 0) not in O.KIRBY_TRIO_CONTEXTS:
            O.KIRBY_TRIO_CONTEXTS = tuple(O.KIRBY_TRIO_CONTEXTS) + ((base, 0),)
        # A power's inhale-like statuses open hidden joints 7 and 19 (and 30,
        # which draws nothing); joint 18 has no part outside Link's copy, so
        # every hat's trio program is the nine-root one, the Yoshi-swallow
        # face's too.
        O.KIRBY_TRIO_EXTRA_JOINT_HEADS = ()
        faithful = self.saved["build_kirby_trio_faithful_specs"]

        def nine_roots(canonical_roots, detail, head_mp):
            specs = [s for s in faithful(canonical_roots, detail, head_mp)
                     if s[0] != O.KIRBY_TRIO_AUX18_OFFSET]
            return tuple((offset, binding) for binding, (offset, _b) in enumerate(specs))

        O.build_kirby_trio_faithful_specs = nine_roots
        clear_caches()


def clear_caches():
    O._KIRBY_HAT_CONTEXT_CACHE.clear()
    O._CANONICAL_CACHE_READ_CACHE.clear()
    O._PAIR_LAYOUT_CACHE.pop("kirby", None)


def build_hat(hat: HatSource, detail: str, resident: dict, vanilla_size: int,
              source_offset: int):
    """The hat's image context and its trio program at one detail."""
    context = O.build_p2_kirby_hat_runtime_context(REPO, detail, hat.base)
    if "trio_body_root" not in context:
        raise HatError(f"hat {hat.hat:#x} {detail}: image carries no trio body")
    faithful = O.build_kirby_trio_context_program(REPO, detail, hat.base)
    head_offset = context["roots"][0][0]
    body = context["trio_body_root"]
    roots, lights, owners = [], [], []
    for index, row in enumerate(faithful["roots"]):
        offset = row[0]
        if offset == body[0]:
            roots.append(tuple(body[:7]))
            lights.append(body[7])
            owners.append(1)
        elif index == 0:
            if offset != head_offset:
                raise HatError(f"hat {hat.hat:#x} {detail}: program head {offset:#x} "
                               f"is not the image's {head_offset:#x}")
            roots.append(tuple(context["roots"][0][:7]))
            lights.append(context["light_preamble_indices"][0])
            owners.append(1)
        else:
            if offset not in resident:
                raise HatError(f"hat {hat.hat:#x} {detail}: root {offset:#x} is not "
                               "one of Kirby's resident roots")
            resident_row, light = resident[offset]
            roots.append(tuple(resident_row[:7]))
            lights.append(light)
            owners.append(0)
    if len(roots) != O.KIRBY_TRIO_BASE_ROOT_COUNT:
        raise HatError(f"hat {hat.hat:#x} {detail}: {len(roots)} trio roots")
    # The live DObj keeps the source hat's part: its list's offset is the
    # head's identity. The image's vertex loads name the appended copy,
    # which the runtime never reads (the image carries the positions):
    # point them at the model's start so its span checks hold.
    roots[0] = (source_offset,) + roots[0][1:]
    context["roots"][0] = (source_offset,) + tuple(context["roots"][0][1:])
    context["vertex"] = [
        (row[0], row[1], row[2], row[3], 0 if row[4] >= vanilla_size else row[4], *row[5:])
        for row in context["vertex"]]
    program = {"roots": roots, "lights": lights, "owners": owners,
               "cross": list(faithful["cross_slots"])}
    return context, program


def image_type(hat: int, detail: str) -> str:
    return f"NDSNativeP4KirbyHat{hat}{detail.title()}Image"


def image_prefix(hat: int, detail: str) -> str:
    return f"NDS_P4_KIRBY_HAT_{hat}_{detail.upper()}"


def render_types(hat: int, detail: str, context: dict) -> list[str]:
    members = I._kirby_hat_members(context)
    name = image_type(hat, detail)
    lines = [f"/* Remix Kirby hat {hat:#x}, {detail} detail. */",
             f"typedef struct {name}", "{", "    u32 abi_tag[1];"]
    lines += I.member_decl_lines(members)
    lines += [f"}} {name};", ""]
    lines += I.member_count_lines(image_prefix(hat, detail), members)
    lines.append("")
    return lines


def render_image(hat: int, detail: str, context: dict) -> str:
    text = I.render_kirby_hat_image(f"P4_{hat}", detail, context)
    text = text.replace(f"NDSNativeKirbyHatP4_{hat}{detail.title()}Image", image_type(hat, detail))
    text = text.replace(f"gNdsNativeKirbyHatP4_{hat}{detail.title()}Image",
                        f"gNdsNativeP4KirbyHat{hat}{detail.title()}Image")
    text = text.replace("#include <nds/generated/nds_native_fighter_image.generated.h>",
                        "#include \"nds_p4_kirby_hats.generated.h\"")
    return text.replace("Kirby deferred copy-hat modelpart", "Remix Kirby hat for copy modelpart")


def root_row(row, light) -> str:
    return "{{ 0x{:08x}u, {}u, {}u, {}u, {}u, {}u, {}u, {}u }}".format(*row[:7], light)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--manifest", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    rows = {int(k, 16): v for k, v in manifest.get("custom_hats", {}).items()}
    powers = [p for p in manifest["powers"] if p.get("hats")]
    rom = R.Rom(args.staging / "ssb64asm.z64")
    hats_dir = args.out / "hats"
    src_dir = hats_dir / "src"
    o2r_dir = args.out / "o2r"
    for d in (hats_dir, src_dir, o2r_dir):
        d.mkdir(parents=True, exist_ok=True)

    state = GeneratorState()
    mobj_counts = source_hat_mobj_counts()
    vanilla_path = REPO / O.P2_O2R_ASSETS["kirby"][0]
    vanilla = vanilla_path.read_bytes()
    source_heads = tuple(O.KIRBY_TRIO_SECTION_HEADS)
    resident = {}
    source_offsets = {}
    for detail in DETAILS:
        ctx = O.build_p2_owner_runtime_context(REPO, "kirby", detail)
        resident[detail] = {row[0]: (row, light)
                            for row, light in zip(ctx["roots"], ctx["light_preamble_indices"])}
        source_offsets[detail] = [spec[1] for spec in O.P2_MODEL_PART_ROOT_VARIANTS["kirby"][detail]]

    built = []  # (hat, base, {detail: (context, program)})
    texture_bytes = {}
    done_files: set[int] = set()
    for power in powers:
        # An authored content's power (P6) shares its base's hats and their
        # texture file (p4_kirby.py): built once.
        if power["hat_texture_file"] in done_files:
            continue
        done_files.add(power["hat_texture_file"])
        sources = [HatSource(rom, h, rows[h]) for h in power["hats"]]
        tex_blob, tex_where = texture_file(sources)
        # A content whose hats take all their textures from the source hat's
        # materials (Wolf's, Dedede's) still ships its row's file.
        tex_blob = tex_blob or bytes(16)
        tex_file = power["hat_texture_file"]
        (o2r_dir / f"{tex_file:04x}").write_bytes(o2r_container(tex_file, tex_blob))
        texture_bytes[power["content"]] = len(tex_blob)
        for hat in sources:
            payload, offsets, vanilla_size = synthesize(hat, tex_where, tex_file, vanilla,
                                                        mobj_counts)
            path = src_dir / f"kirby_{hat.hat:02x}"
            path.write_bytes(payload)
            state.patch(path, hat.base, offsets)
            try:
                per_detail = {}
                for detail in DETAILS:
                    try:
                        per_detail[detail] = build_hat(
                            hat, detail, resident[detail], vanilla_size,
                            source_offsets[detail][hat.base - 1])
                    except ValueError as error:
                        raise HatError(f"hat {hat.hat:#x} {detail}: {error}") from error
            finally:
                state.restore()
            built.append((hat.hat, hat.base, power["content"], per_detail))

    header = [
        "/* Generated by scripts/p4/p4_kirby_hats.py: Remix's Kirby hats as native",
        " * copy-hat images (S8). Derived from the user's ROM: build output only. */",
        "#ifndef NDS_P4_KIRBY_HATS_GENERATED_H",
        "#define NDS_P4_KIRBY_HATS_GENERATED_H",
        "",
        "#include <nds/generated/nds_native_fighter_image.generated.h>",
        "",
        f"#define NDS_P4_KIRBY_HATS_PRESENT 1",
        f"#define NDS_P4_KIRBY_HAT_COUNT {len(built)}u",
        "",
    ]
    for hat, _base, _content, per_detail in built:
        for detail in DETAILS:
            header += render_types(hat, detail, per_detail[detail][0])
    header += ["/* hat id, source hat, detail, image type, count prefix, NitroFS path */",
               "#define NDS_P4_KIRBY_HAT_IMAGES(X) \\"]
    image_rows = []
    for hat, base, _content, _per in built:
        for index, detail in enumerate(DETAILS):
            image_rows.append(f"    X({hat}u, {base}u, {index}u, {image_type(hat, detail)}, "
                              f"{image_prefix(hat, detail)}, "
                              f"NDS_NITRO_FIGHTERS_P4 \"kirby_hat_{hat:02x}_{detail}.bin\")")
    header += [r + (" \\" if i + 1 < len(image_rows) else "") for i, r in enumerate(image_rows)]
    header += ["", "#endif /* NDS_P4_KIRBY_HATS_GENERATED_H */", ""]
    (hats_dir / "nds_p4_kirby_hats.generated.h").write_text("\n".join(header), encoding="utf-8",
                                                            newline="\n")

    # The build compiles every image file here: one a content no longer in
    # the roster left (Banjo's, retired 2026-10-17) would be stale.
    wanted = {f"nds_native_fighter_p4kirbyhat_{hat:02x}_{detail}.image.c"
              for hat, _base, _content, _per in built for detail in DETAILS}
    for old in hats_dir.glob("nds_native_fighter_p4kirbyhat_*.image.c"):
        if old.name not in wanted:
            old.unlink()
    for hat, _base, _content, per_detail in built:
        for detail in DETAILS:
            (hats_dir / f"nds_native_fighter_p4kirbyhat_{hat:02x}_{detail}.image.c").write_text(
                render_image(hat, detail, per_detail[detail][0]), encoding="utf-8", newline="\n")

    inc = [
        "/* Generated by scripts/p4/p4_kirby_hats.py: each Remix Kirby hat's trio",
        " * program (head = the hat id, rows 0 and 1 from its image), then Kirby's",
        " * head list with the source heads first. Build output only. */",
        "#if defined(NDS_NATIVE_KIRBY_TRIO_BODY_PRESENT) && defined(NDS_NATIVE_KIRBY_ROOT_PROGRAMS_PRESENT)",
        "",
    ]
    for hat, _base, _content, per_detail in built:
        for detail, suffix in (("high", ""), ("low", "Low")):
            program = per_detail[detail][1]
            inc += O.emit_rows("NDSNativeRoot", f"sNdsNativeKirbyTrioHead{hat}Roots{suffix}",
                               [root_row(r, l) for r, l in zip(program["roots"], program["lights"])])
            inc += O.emit_rows("u8", f"sNdsNativeKirbyTrioHead{hat}CrossPaletteSlots{suffix}",
                               [f"{v}u" for v in program["cross"]])
        high = per_detail["high"][1]
        inc += O.emit_rows("u8", f"sNdsNativeKirbyTrioHead{hat}BindingParents",
                           ["255u"] * len(high["roots"]))
        inc += O.emit_rows("u8", f"sNdsNativeKirbyTrioHead{hat}SourceOwners",
                           [f"{v}u" for v in high["owners"]])
        if high["owners"] != per_detail["low"][1]["owners"]:
            raise HatError(f"hat {hat:#x}: details' source owners differ")
        inc += [f"#define NDS_NATIVE_KIRBY_TRIO_SOURCE_OWNERS_{hat} "
                f"sNdsNativeKirbyTrioHead{hat}SourceOwners",
                f"#define NDS_NATIVE_KIRBY_TRIO_SOURCE_OWNER_COUNT_{hat} {len(high['owners'])}u",
                ""]
    heads = list(source_heads) + [hat for hat, *_ in built]
    if len(set(heads)) != len(heads):
        raise HatError(f"head ids collide: {heads}")
    inc += ["#undef NDS_NATIVE_KIRBY_TRIO_HEAD_LIST",
            "#define NDS_NATIVE_KIRBY_TRIO_HEAD_LIST(X_) \\",
            "    " + " ".join(f"X_({h})" for h in heads),
            "#undef NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT",
            f"#define NDS_NATIVE_KIRBY_TRIO_HEAD_COUNT {len(heads)}u",
            "",
            "#endif", ""]
    (hats_dir / "nds_p4_kirby_hats.generated.inc").write_text("\n".join(inc), encoding="utf-8",
                                                              newline="\n")
    summary = {f"{hat:#x}": {"base": base, "content": content,
                             "roots": {d: [hex(r[0]) for r in per[d][1]["roots"][:2]] for d in DETAILS}}
               for hat, base, content, per in built}
    (hats_dir / "manifest.json").write_text(
        json.dumps({"hats": summary, "texture_bytes": texture_bytes}, indent=1) + "\n",
        encoding="utf-8", newline="\n")
    print(f"p4 kirby hats: {len(built)} hats, textures "
          + ", ".join(f"{c} {n} B" for c, n in texture_bytes.items()))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except HatError as error:
        raise SystemExit(f"p4_kirby_hats: {error}")
