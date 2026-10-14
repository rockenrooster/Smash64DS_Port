#!/usr/bin/env python3
"""P6 A12: a CPU's attack rows from a fighter's clips and scripts (F5).

A CPU attack row (FTComputerAttack; Remix's AI.add_attack_behaviour) says
when a move's hitboxes come out and where they reach: the first frame any
hitbox is active (ftComputer predicts both fighters that many frames ahead)
and the box its spheres sweep while active, x forward and y up from the
fighter's origin, in world units. ftComputer picks the move when its target
stands in that box.

Here the box is measured: each frame of the move's clip is posed on the
fighter's skeleton (anim.pose; the clip's slots go to the rows the fighter
makes DObjs for, FTAttributes.setup_parts; scale tracks over the rows' rest
scale; FTAttributes.translate_scales on the rows a scaled clip keys; the
TransN slot moving the root), each hitbox the script holds that frame
(mscript.attack_timeline) is placed by its joint (0 TopN: the fighter's
origin; 1 TransN; 4 and up the model rows, joint minus four), scaled by
FTAttributes.size (the TopN's scale; radii are not scaled) and the box takes
its whole sphere, as the source's rows do. The lab's in-game hitbox positions
(pos_curr) match these within 2 units (Palutena's forward tilt, Kirby's jab).
Options for what a script does not show: `offsets_z` (a move whose code
moves its TopN hitboxes: Palutena's Explosive Flame at the distance the
stick picks) and `travel` ("run": the dash attack slides on from run speed,
less the traction each frame, ftPhysicsApplyGroundVelFriction).

    coverage.py --staging <remix staging> --export <export dir> [--attacks JAB,...]
        prints each attack's row

    coverage.py --staging ... --check-vanilla Kirby,Fox,Mario
        F5: each vanilla fighter's moves (its struct, clips and motion
        scripts read from the original image) measured against the decomp's
        dFTComputerAttacks<name>: the rows the source measured from the move
        itself within TOLERANCE and a frame

CPU rows for an authored content (cpu_rows, from p6_author.py): the base's
own rows, named from its Remix source (src/<Base>/AI/Attacks.asm, in table
order), each measured on the content's own clips and scripts unless the
spec keeps the base's numbers (a grab box is no attack) or drops the row (a
move with no hitbox: Palutena's Warp).
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "p4"))
import anim as A  # noqa: E402
import mscript  # noqa: E402
import n64_model as N  # noqa: E402

ROW_JOINT = 4                  # model row r draws joint r + 4 (nFTPartsJointCommonStart)
SCALE_UNIT = 4096.0            # tracks 7-9 (p4_anim_compact._ev16_scale)
JOINT_TOPN, JOINT_TRANSN = 0, 1
# FTAttributes floats (decomp ft/fttypes.h): traction, run_speed.
ATTR_TRACTION, ATTR_RUN_SPEED = 0x24, 0x30
# Remix's attack names (AI.asm ATTACK_TABLE) -> the motion the move plays:
# the shared action array's (every fighter's same motion ids).
COMMON = {
    "JAB": 165, "FTILT": 170, "UTILT": 174, "DTILT": 176, "FSMASH": 179, "USMASH": 182,
    "DSMASH": 183, "GRAB": 146, "DASH_ATTACK": 167, "NAIR": 184, "FAIR": 185, "BAIR": 186,
    "UAIR": 187, "DAIR": 188,
}
# The decomp's row names for the same motions (F5 check).
VANILLA = {"Attack11": 165, "AttackS3": 170, "AttackS4": 179, "AttackHi3": 174, "AttackHi4": 182,
           "AttackLw3": 176, "AttackLw4": 183, "AttackAirN": 184, "AttackAirF": 185, "AttackAirB": 186,
           "AttackAirHi": 187, "AttackAirLw": 188}
# F5: an honestly measured source row's edges within this many units. Rows
# the source copied from another move (Kirby's AttackS3 is his Attack11's),
# or that cover travel or a grab box no script shows, are listed, not held to it.
TOLERANCE = 60.0
END_ROW = [-1, 0, 0, 0.0, 0.0, 0.0, 0.0]


class CoverageError(SystemExit):
    pass


def content_rom(staging: Path, export: Path, resolved: dict | None = None):
    """The staged image with a content's authored files, clips, copies and
    script window over it (generate_p4_fighter.py's OverlayRom), and its
    resolved export (read from `export` unless given)."""
    import remix_rom as R
    rom = R.Rom(staging / "ssb64asm.z64")
    if resolved is None:
        resolved = json.loads((export / "resolved.json").read_text(encoding="utf-8"))
    authored = resolved.get("authored") or {}
    files = authored.get("files") or {}
    ids = ([new for _old, new in files.values()] + list((authored.get("anims") or {}).values()) +
           list(authored.get("skirt_clips") or []))
    ram = authored.get("ram")
    copies = authored.get("copies") or {}
    if not (ids or ram or copies):
        return rom, resolved
    blobs = {new: (export / "o2r" / f"{new:04x}").read_bytes() for new in ids}
    for old, new in copies.values():
        blob = bytearray(rom.o2r(old))
        struct.pack_into("<I", blob, 0x40, new)
        blobs[new] = bytes(blob)
    windows = {ram["base"]: (export / ram["file"]).read_bytes()} if ram else None
    return R.OverlayRom(rom, blobs, windows), resolved


def first_slot(anim_flags: int) -> int:
    """The clip's leading slots before its rows: TransN, XRotN and YRotN
    for the flags that name them (bits 31, 30, 29)."""
    return bin((anim_flags >> 29) & 7).count("1")


def _matrix(rot, trans, scale=(1.0, 1.0, 1.0)):
    """A DObj's transform: translate, rotate, scale (syMatrixTraRotRpyRSca)."""
    m = np.eye(4)
    m[:3, :3] = A.euler(*rot) @ np.diag(scale)
    m[:3, 3] = trans
    return m


class Fighter:
    """A content's skeleton, attributes, clips and scripts."""

    def __init__(self, rom, resolved: dict, events: dict):
        self.rom, self.resolved, self.events = rom, resolved, events
        desc = resolved["descriptor"]
        self.fm = N.FighterModel(N.RelocFile.read(rom.o2r(desc["file_ids"][0])),
                                 N.RelocFile.read(rom.o2r(desc["file_ids"][3])), desc["o_attributes"])
        fm = self.fm
        self.skel = A.Skeleton.from_model(fm, "high")
        self.rest_scale = [tuple(j.scale) for j in fm.trees["high"][:-1]]
        self.size, = struct.unpack_from(">f", fm.main.data, fm.attr)
        self.traction, = struct.unpack_from(">f", fm.main.data, fm.attr + ATTR_TRACTION)
        self.run_speed, = struct.unpack_from(">f", fm.main.data, fm.attr + ATTR_RUN_SPEED)
        self.translate_scales = self._translate_scales()
        self.rows = self._created_rows()

    def _translate_scales(self):
        """FTAttributes.translate_scales (one Vec3f per fighter joint), or None."""
        fm = self.fm
        ref = fm.main.ptr(fm.attr + fm.lay["FTAttributes.translate_scales"])
        if ref is None:
            return None
        if ref[0] != "intern":
            raise CoverageError("translate_scales outside the main file")
        return [struct.unpack_from(">3f", fm.main.data, ref[2] + 12 * j) for j in range(37)]

    def _created_rows(self):
        """The tree rows the fighter makes DObjs for (FTAttributes.setup_parts,
        two u32 masks from bit 31 of the first: lbCommonSetupFighterPartsDObjs
        skips the others), or None for all. A clip's slots go to these in
        order (Kirby's tree has 27 rows; his clips, 22 slots)."""
        fm = self.fm
        ref = fm.main.ptr(fm.attr + fm.lay["FTAttributes.setup_parts"])
        if ref is None:
            return None
        if ref[0] != "intern":
            raise CoverageError("setup_parts outside the main file")
        masks = struct.unpack_from(">2I", fm.main.data, ref[2])
        return [i for i in range(len(self.skel.parents)) if (masks[i // 32] >> (31 - i % 32)) & 1]

    def _slot_clip(self, clip: A.Clip, lead: int) -> A.Clip:
        if self.rows is None:
            return clip
        out = A.Clip(clip.slots)
        for slot, st in clip.streams.items():
            k = slot - lead
            if k < 0:
                out.streams[slot] = st
            elif k < len(self.rows):
                out.streams[lead + self.rows[k]] = st
        return out

    def pose_frames(self, clip: A.Clip, anim_flags: int, frames: int):
        """Per frame: the root's TransN offset and the rows' model-space
        matrices."""
        lead = first_slot(anim_flags)
        clip = self._slot_clip(clip, lead)
        out = []
        for f in range(frames + 1):
            trans, rot = A.pose(clip, self.skel, f, lead)
            scale = [list(v) for v in self.rest_scale]
            for slot, st in clip.streams.items():
                row = slot - lead
                if not 0 <= row < len(scale):
                    continue
                g = min(f, st.length)
                for k in range(3):
                    t = st.tracks.get(7 + k)
                    if t is not None:
                        scale[row][k] = float(t.values[max(g - t.lead, 0)]) / SCALE_UNIT
                if self.translate_scales is not None and any(4 <= k <= 6 for k in st.tracks):
                    s = self.translate_scales[row + ROW_JOINT]
                    trans[row] = [trans[row][k] * s[k] for k in range(3)]
            root = np.zeros(3)
            if (anim_flags >> 31) & 1 and 0 in clip.streams:
                st = clip.streams[0]
                g = min(f, st.length)
                root = np.array([float(st.tracks[4 + k].values[max(g - st.tracks[4 + k].lead, 0)]) / A.TRANS_UNIT
                                 if (4 + k) in st.tracks else 0.0 for k in range(3)])
            mats = []
            for i, p in enumerate(self.skel.parents):
                m = _matrix(rot[i], trans[i], scale[i])
                mats.append(m if p < 0 else mats[p] @ m)
            out.append((root, mats))
        return out

    def script(self, motion: dict) -> list[str]:
        if self.events is None:
            return _file_script(self.rom, self.resolved["descriptor"]["file_ids"][1], motion["script"])
        root = motion.get("script_root")
        if root is None or root not in self.events["blocks"]:
            return []
        return mscript.block_commands(self.rom, self.events, root)

    def measure(self, motion_index: int, offsets_z=None, travel=None):
        """(first active frame, last, near x, far x, near y, far y), or None
        if the motion's script makes no hitbox."""
        motion = next((m for m in self.resolved["motions"] if m["index"] == motion_index), None)
        if motion is None or not motion["anim_file_id"]:
            return None
        commands = self.script(motion)
        if not commands:
            return None
        clip = A.decode(self.rom.o2r(motion["anim_file_id"]))
        length = max((st.length for st in clip.streams.values()), default=0)
        timeline = mscript.attack_timeline(commands, length)
        active = [f for f, hits in enumerate(timeline) if hits]
        if not active:
            return None
        frames = self.pose_frames(clip, motion["anim_flags"], active[-1])
        xs, ys = [], []
        for f in active:
            root, mats = frames[f]
            slide = 0.0
            if travel == "run":
                slide = sum(max(self.run_speed - k * self.traction, 0.0) for k in range(1, f + 1))
            elif travel is not None:
                raise CoverageError(f"travel {travel!r}")
            for hit in timeline[f]:
                j = hit["joint_id"]
                zs = offsets_z if (offsets_z is not None and j == JOINT_TOPN) else (hit["off"][2],)
                for oz in zs:
                    off = np.array([hit["off"][0], hit["off"][1], oz, 1.0], dtype=float)
                    if j == JOINT_TOPN:
                        p = off[:3]
                    elif j < ROW_JOINT:
                        p = root + off[:3]
                    else:
                        p = root + (mats[j - ROW_JOINT] @ off)[:3]
                    p = p * self.size
                    r = hit["radius"]
                    xs += [p[2] + slide - r, p[2] + slide + r]
                    ys += [p[1] - r, p[1] + r]
        return (active[0], active[-1], min(xs), max(xs), min(ys), max(ys))


# The original image's fighter structs (remix_export.py STRUCT_TABLE_ROM) by
# decomp name.
VANILLA_IDS = {"Mario": 0, "Fox": 1, "Donkey": 2, "Samus": 3, "Luigi": 4, "Link": 5, "Yoshi": 6,
               "Captain": 7, "Kirby": 8, "Pikachu": 9, "Purin": 10, "Ness": 11}
STRUCT_TABLE_ROM = 0x92610
MOTION_DESC_SIZE = 0xC


def _file_script(rom, motion_file: int, offset: int) -> list[str]:
    """A vanilla motion script as text: its commands from `offset` in the
    motion file to the first End (straight line; pointer words read 0)."""
    if offset in (0, 0x80000000) or offset & 0x80000000:
        return []
    data = rom.file_bytes(motion_file)
    out = []
    at = offset
    while at + 4 <= len(data):
        n = mscript.length(int.from_bytes(data[at:at + 4], "big")) // 4
        words = [int.from_bytes(data[at + 4 * k:at + 4 * k + 4], "big") for k in range(n)]
        text = mscript.disassemble(words, {})
        out.append(text)
        at += 4 * n
        if text.split()[0] in ("End", "Goto", "Return"):
            break
    return out


def vanilla(staging: Path, name: str) -> Fighter:
    """A vanilla fighter from the original image: its struct's files and
    attributes, its motion descriptions, its scripts from its motion file."""
    import remix_rom as R
    rom = R.Rom(staging / "roms" / "original.z64")
    ram = rom.u32(STRUCT_TABLE_ROM + VANILLA_IDS[name] * 4)
    w = struct.unpack(">30I", rom.read_ram(ram, 0x78))
    motions = []
    for i in range(w[0x6C // 4]):
        a, sc, fl = struct.unpack(">III", rom.read_ram(w[0x64 // 4] + i * MOTION_DESC_SIZE, 12))
        motions.append({"index": i, "anim_file_id": a, "script": sc, "anim_flags": fl})
    resolved = {"descriptor": {"file_ids": list(w[0:9]), "o_attributes": w[0x60 // 4]}, "motions": motions}
    return Fighter(rom, resolved, None)


def load(staging: Path, export: Path, resolved: dict | None = None, events: dict | None = None) -> Fighter:
    rom, resolved = content_rom(staging, export, resolved)
    if events is None:
        events = json.loads((export / "events.json").read_text(encoding="utf-8"))
    return Fighter(rom, resolved, events)


def source_names(asm_text: str) -> list[list[str]]:
    """AI/Attacks.asm's attack names in table order, per table (grounded,
    aerial): add_attack_behaviour(NAME, ...) and
    add_custom_attack_behaviour(AI.ROUTINE.NAME, ...), END_ATTACKS between."""
    tables, cur = [], []
    for line in asm_text.splitlines():
        line = line.split("//")[0]
        m = re.search(r"AI\.add_(custom_)?attack_behaviour\(\s*(?:AI\.ROUTINE\.)?(\w+)", line)
        if m:
            cur.append(m.group(2))
        elif "AI.END_ATTACKS()" in line:
            tables.append(cur)
            cur = []
    return tables


def base_rows(rom, resolved: dict) -> list[list]:
    """The base's attack rows as assembled (Character.ai_behaviour: grounded
    rows, END, aerial rows, END), generate_p4_fighter.py's reading."""
    at = int(resolved["kind_tables"]["ai_behaviour"]["value"], 16)
    rows, ends = [], 0
    while ends < 2:
        row = list(struct.unpack(">iiiffff", rom.read_ram(at, 0x1C)))
        at += 0x1C
        rows.append(row)
        ends += row[0] == -1
        if len(rows) > 64:
            raise CoverageError("ai_behaviour: no second END within 64 rows")
    return rows


def cpu_rows(fighter: Fighter, asm_text: str, spec: dict) -> tuple[list, list]:
    """The content's attack rows and their names: the base's rows (named
    from its AI/Attacks.asm) measured on the content's own moves.
    spec: {"drop": [names], "keep": [names: the base's numbers],
    "measure": {name: {"motion", "offsets_z", "travel"}}}."""
    names = source_names(asm_text)
    rows = base_rows(fighter.rom, fighter.resolved)
    flat = [n for table in names for n in table + ["END"]]
    if len(flat) != len(rows) or any((n == "END") != (r[0] == -1) for n, r in zip(flat, rows)):
        raise CoverageError(f"AI/Attacks.asm names {len(flat)} rows, the table has {len(rows)}")
    drop, keep = set(spec.get("drop", ())), set(spec.get("keep", ()))
    measure = spec.get("measure") or {}
    unknown = (drop | keep | set(measure)) - set(flat)
    if unknown:
        raise CoverageError(f"cpu: no base row {sorted(unknown)}")
    out, out_names = [], []
    for name, base in zip(flat, rows):
        if name == "END":
            out.append(list(END_ROW))
            out_names.append("END")
            continue
        if name in drop:
            continue
        if name in keep:
            out.append(base)
            out_names.append(name + " (base)")
            continue
        opts = measure.get(name, {})
        motion = opts.get("motion", COMMON.get(name))
        if motion is None:
            raise CoverageError(f"cpu {name}: no motion (spec measure.{name}.motion)")
        m = fighter.measure(motion, opts.get("offsets_z"), opts.get("travel"))
        if m is None:
            raise CoverageError(f"cpu {name}: motion {motion} makes no hitbox (keep or drop it)")
        out.append([base[0], m[0], m[1], *(float(round(v)) for v in m[2:])])
        out_names.append(name)
    return out, out_names


def vanilla_rows(name: str) -> dict:
    """The decomp's dFTComputerAttacks<name> rows by their comment names."""
    text = (HERE.parents[1] / "decomp/BattleShip-main/decomp/src/ft/ftcomputer.c").read_text(encoding="utf-8")
    body = text[text.index(f"dFTComputerAttacks{name}["):]
    body = body[:body.index("};")]
    out = {}
    for m in re.finditer(r"//\s*(\w+)\s*\{\s*(\w+),[^\n]*\n\s*(-?\d+),[^\n]*\n\s*(-?\d+),[^\n]*\n"
                         r"\s*(-?[\d.]+)F,[^\n]*\n\s*(-?[\d.]+)F,[^\n]*\n\s*(-?[\d.]+)F,[^\n]*\n"
                         r"\s*(-?[\d.]+)F", body):
        out.setdefault(m.group(1), (int(m.group(3)), int(m.group(4)), *(float(m.group(k)) for k in range(5, 9))))
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--export", type=Path)
    ap.add_argument("--attacks", default=",".join(COMMON))
    ap.add_argument("--check-vanilla", help="decomp fighter names (Kirby,Fox): F5 against their rows")
    args = ap.parse_args()
    if args.check_vanilla:
        status = 0
        for fname in args.check_vanilla.split(","):
            fighter = vanilla(args.staging, fname)
            source = vanilla_rows(fname)
            held, listed = [], []
            for name, motion in VANILLA.items():
                if name not in source:
                    continue
                m = fighter.measure(motion)
                if m is None:
                    print(f"{fname} {name:12s} no hitbox")
                    continue
                s = source[name]
                d = max(abs(m[k] - s[k]) for k in range(2, 6))
                ok = d <= TOLERANCE and abs(m[0] - s[0]) <= 1
                (held if ok else listed).append(name)
                print(f"{fname} {name:12s} measured f{m[0]}-{m[1]} x {m[2]:6.0f} {m[3]:6.0f} y {m[4]:6.0f} {m[5]:6.0f}"
                      f" | decomp f{s[0]}-{s[1]} x {s[2]:6.0f} {s[3]:6.0f} y {s[4]:6.0f} {s[5]:6.0f}"
                      f" | edge {d:4.0f}" + ("" if ok else "  (not held)"))
            print(f"F5 {fname}: {len(held)} rows within {TOLERANCE:.0f} units and a frame: {', '.join(held)}; "
                  f"not held: {', '.join(listed) or 'none'}")
            status |= len(held) < 4
        return status
    if args.export is None:
        ap.error("--export is required without --check-vanilla")
    fighter = load(args.staging, args.export)
    for name in args.attacks.split(","):
        if name not in COMMON:
            continue
        m = fighter.measure(COMMON[name])
        print(f"{name:12s} " + ("no hitbox" if m is None else
                                f"frames {m[0]:3d}-{m[1]:3d}  x {m[2]:7.0f} {m[3]:7.0f}  y {m[4]:7.0f} {m[5]:7.0f}"))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
