#!/usr/bin/env python3
"""P4 S8: Kirby's copies of the enabled P4 contents, lowered into DS inputs.

Remix gives Kirby a status of his own for every power it adds
(Character.add_new_action(KIRBY, ...) in src/Kirby/Kirby.asm): Kirby's own
animations and motion scripts, with the power's routines in its callback
slots. remix_export.py --fighter KIRBY exports Kirby's extended action and
parameter arrays (KirbyExporter). This adapter keeps the statuses of the
enabled contents' powers and writes, under --out (a build directory: every
output derives from the user's ROM):

  nds_p4_kirby.generated.h  the DS status ids (dense from 0x12F, Kirby's first
                            id past his 83 vanilla specials) and motion ids
                            (dense from 276) the ported routines set.
  nds_p4_kirby.generated.c  the status rows (callbacks as hand ports or decomp
                            names, as generate_p4_fighter.py's), each power's
                            row (hat, Kirby's neutral-special starters, the
                            clone swaps of Kirby.clone_power_action_swap_)
                            and the files the runtime may load.
  nds_p4_kirby.tables.c     the motion rows, objcopied into nitro:/p4/kirby.tab
                            and read into the heap only for a match with a
                            Kirby and a content whose power he can take.
  o2r/<id>                  the power animations (p4_anim_compact.py) and one
                            script file holding every selected status's Remix
                            motion script, its pointers internal relocations.
                            Its rows carry FTANIM_FLAG submotion-script (0x10):
                            ftMainSetStatus then takes the script from
                            *p_file_submotion, which Kirby (no submotion file)
                            points at the loaded script file in such a match.

    p4_kirby.py --staging <remix staging> --kirby-export <dir> --export-root <dir>
        --contents "falco ganondorf ..." --out <dir>
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import remix_rom as R  # noqa: E402
import generate_p4_fighter as G  # noqa: E402

VANILLA_SPECIAL_COUNT = 83        # Kirby's specials 0xDC..0x12E
STATUS_FIRST = 0xDC + VANILLA_SPECIAL_COUNT
VANILLA_MOTION_COUNT = 276        # dFTKirbyData.mainmotion_array_count
KIRBY_MOTION_FILE = 0xE4
# The script file: above every content's synthesized motion file
# (SYNTH_FILE_BASE + content id * 0x10).
SCRIPT_FILE_ID = 0x17F0
FLAG_SUBMOTION_SCRIPT = 0x10
FLAG_ANIMJOINT = 0x8
FLAG_SHIELDPOSE = 0x2
FIRST_CUSTOM_HAT = 0xF            # KirbyHats.asm: ids past the source's
HAT_ROW_BYTES = 0x20              # KirbyHats.extended_special_parts rows
DEFERRED_HATS = 0x3FF8            # source hats 3..13: per-match native images
# KirbyHats.file_index: File.asm KIRBY_CUSTOM_HATS and KIRBY_CUSTOM_HATS_2.
HAT_FILES = (0xA8D, 0x1533)
# Each power's Remix hat textures (p4_kirby_hats.py): one file per content,
# past the synthesized files (SYNTH_FILE_BASE .. SCRIPT_FILE_ID).
HAT_TEXTURE_FILE_BASE = 0x1800
POWER_HAT_SLOTS = 4               # NDSP4KirbyPower.hats
KIRBY_MODEL_FILE = 0x148
# A power's weapon on Kirby's joint 17 (special part index 0x11) draws beside
# his body as a P4 sidecar root: its rows carry the joint with this bit, so
# they never answer a content's own joint-17 lookup (Wolf's gun).
KIRBY_SIDECAR_BIT = 0x80

# The status-name prefix of each content's power (Kirby.asm add_new_action).
POWER_PREFIX = {
    "falco": "FALCO", "ganondorf": "GND", "wolf": "WOLF", "bowser": "BOWSER",
    "marth": "MARTH", "roy": "ROY", "wario": "WARIO", "peach": "PEACH",
    "crash": "CRASH", "lanky": "LANKY", "sheik": "SHEIK", "banjo": "BANJO",
    "sonic": "SONIC", "dedede": "DEDEDE",
}
# Vanilla fighter ids (Character.id) for the parent rows of kirby_*_nsp.
VANILLA_IDS = {"MARIO": 0, "FOX": 1, "DONKEY": 2, "SAMUS": 3, "LUIGI": 4, "LINK": 5,
               "YOSHI": 6, "CAPTAIN": 7, "KIRBY": 8, "PIKACHU": 9, "JIGGLY": 10,
               "NESS": 11}


class KirbyError(SystemExit):
    pass


class _EmptyBase:
    """A remix_rom.Rom whose motion file is empty: MotionSynth then builds a
    file of the copied Remix streams alone."""

    def __init__(self, rom: R.Rom):
        self.rom = rom

    def file_bytes(self, fid: int) -> bytes:
        return b"" if fid == KIRBY_MOTION_FILE else self.rom.file_bytes(fid)

    def entry(self, fid: int):
        return R.TableEntry(False, 0, 0xFFFF, 0, 0xFFFF, 0)

    def reloc_slots(self, fid: int) -> dict:
        return {}

    def extern_ids(self, fid: int) -> list:
        return []

    def read_ram(self, ram: int, size: int) -> bytes:
        return self.rom.read_ram(ram, size)


def c_ident(name: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", name).upper()


def power_weapons(hats_asm: str) -> dict[int, int]:
    """KirbyHats.use_extended_special_parts_pause_: Remix hat id -> the
    offset, in Kirby's model file, of the special part whose list the power
    holds on joint 17 (Marth's and Roy's swords, Wolf's and Lanky's guns)."""
    return {int(m[1], 16): int(m[2], 16) for m in re.finditer(
        r"addiu\s+t6,\s*r0,\s*(0x[0-9A-Fa-f]+)[^\n]*\n\s*li\s+t7,\s*(0x[0-9A-Fa-f]+)",
        hats_asm)}


def clone_swaps(kirby_asm: str, new_names: list[str]) -> dict[str, list[tuple[int, int]]]:
    """Kirby.clone_power_action_swap_: Remix power name -> [(status, clone)]."""
    labels = dict((m.group(2), m.group(1)) for m in re.finditer(
        r"lli\s+t8,\s*Character\.id\.(\w+)[^\n]*\n\s*beq\s+t7,\s*t8,\s*(_\w+)", kirby_asm))
    out: dict[str, list[tuple[int, int]]] = {}
    for label, power in labels.items():
        m = re.search(rf"^\s*{re.escape(label)}:\s*$", kirby_asm, flags=re.M)
        if m is None:
            raise KirbyError(f"clone swap block {label} not found")
        rows = []
        for line in kirby_asm[m.end():].splitlines():
            line = line.split("//")[0].strip()
            if line.startswith("b ") or line.startswith("b\t"):
                break
            c = re.match(r"clone_action\(\s*(\w+)\s*,\s*([\w.]+)\s*\)", line)
            if c is None:
                continue
            base = int(c.group(1), 0)
            clone = c.group(2)
            if clone.startswith("Action."):
                clone = STATUS_FIRST + new_names.index(clone[len("Action."):])
            else:
                clone = int(clone, 0)
            rows.append((base, clone))
        out[power] = rows
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--kirby-export", type=Path, required=True)
    ap.add_argument("--export-root", type=Path, required=True)
    ap.add_argument("--contents", required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()

    resolved = json.loads((args.kirby_export / "resolved.json").read_text(encoding="utf-8"))
    if resolved["fighter"] != "KIRBY" or resolved["vanilla_special_count"] != VANILLA_SPECIAL_COUNT \
            or resolved["vanilla_motion_count"] != VANILLA_MOTION_COUNT:
        raise KirbyError("not a KirbyExporter export of Remix's Kirby")
    rom = R.Rom(args.staging / "ssb64asm.z64")
    sym, by_addr = R.load_symbols(args.staging / "logfile.log")
    decomp_names = R.load_decomp_symbols(G.DECOMP_SYMBOLS)
    registry = {row["name"]: row for row in
                json.loads((HERE / "contents.json").read_text(encoding="utf-8"))["contents"]}
    contents = [c for c in args.contents.split() if c in POWER_PREFIX]
    kirby_asm = (args.staging / "src" / "Kirby" / "Kirby.asm").read_text(encoding="utf-8",
                                                                         errors="replace")
    new_names = [s["name"] for s in resolved["statuses"] if s.get("remix_new")]

    # The statuses of each enabled power, in Remix's order.
    selected = []
    owner = {}
    for s in resolved["statuses"]:
        if not s.get("remix_new"):
            continue
        for c in contents:
            if s["name"].startswith(POWER_PREFIX[c] + "_"):
                selected.append(s)
                owner[s["status_id"]] = c
    status_map = {s["status_id"]: STATUS_FIRST + i for i, s in enumerate(selected)}
    motion_ids = sorted({s["motion_id"] for s in selected if s["motion_id"] >= 0})
    for m in motion_ids:
        if m < VANILLA_MOTION_COUNT:
            raise KirbyError(f"power status names vanilla motion {m:#x}")
    motion_map = {m: VANILLA_MOTION_COUNT + i for i, m in enumerate(motion_ids)}
    motions = {m["index"]: m for m in resolved["motions"]}

    # The script file: every block the selected motions' scripts reach.
    from remix_export import EventDecoder, Failures, script_root  # noqa: E402
    fail = Failures()
    dec = EventDecoder(rom, by_addr, KIRBY_MOTION_FILE, fail)
    roots = []
    for m in motion_ids:
        script = motions[m]["script"]
        if script == G.NO_SCRIPT:
            continue
        if not (script & 0x80000000) or script < R.REMIX_CODE_RAM:
            raise KirbyError(f"motion {m:#x}: script {script:#x} is not a Remix stream")
        dec.decode(script_root(script, KIRBY_MOTION_FILE), f"motion:{m:#x}")
        roots.append((f"motion:{m:#x}", script))
    if fail.items:
        raise KirbyError("script decode failed: " + "; ".join(fail.items[:5]))
    for key, block in dec.blocks.items():
        if not key.startswith("ram:"):
            raise KirbyError(f"power script reaches {key} outside the Remix streams")
        for cmd in block["commands"]:
            if cmd["op"] == "RemixGotoMovesetFile":
                raise KirbyError(f"{cmd['at']}: GO_TO_FILE into Kirby's motion file")
    events = {"blocks": dec.blocks, "data_refs": dec.data_refs}
    synth = G.MotionSynth(_EmptyBase(rom), KIRBY_MOTION_FILE, events, SCRIPT_FILE_ID)
    script_o2r = synth.build()
    checked = G.round_trip(rom, script_o2r, synth, roots)

    out = args.out
    o2r_out = out / "o2r"
    o2r_out.mkdir(parents=True, exist_ok=True)
    for old in o2r_out.iterdir():
        old.unlink()
    shipped = {SCRIPT_FILE_ID: hashlib.sha256(script_o2r).hexdigest()}
    (o2r_out / f"{SCRIPT_FILE_ID:04x}").write_bytes(script_o2r)

    # The power animations (Kirby's own; Roy's are Marth's files). Remix
    # reuses some of Kirby's vanilla animations (his inhale for Dedede's
    # power, his Falcon Punch for Ganondorf's): those keep the vanilla id the
    # port already ships.
    import p4_anim_compact  # noqa: E402

    vanilla = R.Rom(args.staging / "roms" / "ssb.rom")
    anim_flags: dict[int, int] = {}
    for m in motion_ids:
        a, f = motions[m]["anim_file_id"], motions[m]["anim_flags"]
        if a == 0 or (f & FLAG_SHIELDPOSE):
            continue
        if a < vanilla.file_count:
            if vanilla.o2r(a) != rom.o2r(a):
                raise KirbyError(f"animation {a:#x} is a vanilla id with Remix bytes")
            continue
        anim_flags[a] = anim_flags.get(a, 0) | f
    converted32: set[int] = set()
    anim_sizes: dict[int, int] = {}
    for a, flags in sorted(anim_flags.items()):
        if rom.extern_ids(a):
            raise KirbyError(f"animation {a:#x} names other files")
        blob = rom.o2r(a)
        if flags & FLAG_ANIMJOINT:
            new = p4_anim_compact.convert32_o2r(blob, G.ANIM_EPS)
            if new is not None:
                converted32.add(a)
        else:
            new = p4_anim_compact.compact_o2r(blob, G.ANIM_EPS)
        if new is not None:
            blob = new
        (o2r_out / f"{a:04x}").write_bytes(blob)
        shipped[a] = hashlib.sha256(blob).hexdigest()
        ext_n = struct.unpack_from("<I", blob, 0x48)[0]
        anim_sizes[a] = struct.unpack_from("<I", blob, 0x4C + 2 * ext_n)[0]

    rows = []
    for m in motion_ids:
        d = motions[m]
        flags = d["anim_flags"]
        if d["anim_file_id"] in converted32:
            flags &= ~FLAG_ANIMJOINT
        script = d["script"]
        if script == G.NO_SCRIPT:
            offset = G.NO_SCRIPT
        else:
            offset = synth.map_ram(script)
            flags |= FLAG_SUBMOTION_SCRIPT
        rows.append((d["anim_file_id"], offset, flags))

    # The status rows.
    slots = ("proc_update", "proc_interrupt", "proc_physics", "proc_map")
    specials = []
    for s in selected:
        procs = []
        for proc in slots:
            nm = s.get(proc + "_name")
            if s[proc] == 0:
                procs.append("NULL")
            elif nm in G.CALLBACK_PORTS:
                procs.append(G.CALLBACK_PORTS[nm])
            elif nm and "." not in nm:
                procs.append(nm)
            else:
                raise KirbyError(f"Kirby status {s['status_id']:#x} {s['name']} {proc}: "
                                 f"{nm or hex(s[proc])} has no DS port")
        h = s["sflags"]
        specials.append({"status": status_map[s["status_id"]], "remix": s["status_id"],
                         "name": s["name"], "content": owner[s["status_id"]],
                         "motion_id": motion_map.get(s["motion_id"], s["motion_id"]),
                         "attack_id": s["attack_id"], "unused": (h >> 13) & 7,
                         "is_smash_attack": (h >> 12) & 1, "ga": (h >> 11) & 1,
                         "is_projectile": (h >> 10) & 1, "stat_attack_id": h & 0x3FF,
                         "procs": procs})

    # Each power: hat, starters and clone swaps.
    swaps = clone_swaps(kirby_asm, new_names)
    inhale = sym["Character.kirby_inhale_struct.table"]
    ground_nsp = sym["Character.kirby_ground_nsp.table"]
    air_nsp = sym["Character.kirby_air_nsp.table"]
    hats = sym["KirbyHats.extended_special_parts"]

    def hat_base_of(hat: int) -> int:
        if hat < FIRST_CUSTOM_HAT:
            return hat
        return struct.unpack(">i", rom.read_ram(
            hats + (hat - FIRST_CUSTOM_HAT) * HAT_ROW_BYTES, 4))[0]

    # The hats a power's own scripts set on joint 6 (Bowser's and Dedede's
    # open mouths, Kazooie's), by status: Remix's custom ids draw from the
    # extended special parts; the port shows each one's source hat until
    # the Remix hats are drawn.
    def script_hats(status: dict) -> set[int]:
        found: set[int] = set()
        if status["motion_id"] < 0:
            return found
        script = motions[status["motion_id"]]["script"]
        if script == G.NO_SCRIPT:
            return found
        pending = ["ram:%#010x" % script]
        seen: set[str] = set()
        while pending:
            key = pending.pop()
            if key in seen or key not in dec.blocks:
                continue
            seen.add(key)
            for cmd in dec.blocks[key]["commands"]:
                if cmd["op"] == "SetModelPartID":
                    w = cmd["words"][0]
                    joint, part = (w >> 19) & 0x7F, w & 0x7FFFF
                    if joint == 6 and part < 0x40000:
                        found.add(part)
            pending.extend(e["to"] for e in dec.blocks[key].get("edges", ()))
        return found

    custom_hats = list(range(FIRST_CUSTOM_HAT, FIRST_CUSTOM_HAT +
                             (sym["KirbyHats.temp_special_parts"] - hats) // HAT_ROW_BYTES))
    hat_bases = [hat_base_of(h) for h in custom_hats]

    # A Remix hat's row (KirbyHats.add_hat): the source hat it re-skins and
    # its own lists, high and low, in the file its index names. The roster's
    # hats override only the list (images and tracks -1: the source hat's).
    def hat_row(hat: int) -> dict:
        words = struct.unpack(">8i", rom.read_ram(
            hats + (hat - FIRST_CUSTOM_HAT) * HAT_ROW_BYTES, HAT_ROW_BYTES))
        base, dl_hi, img_hi, trk_hi, dl_lo, img_lo, trk_lo, file_index = words
        if (img_hi, trk_hi, img_lo, trk_lo) != (-1, -1, -1, -1) or dl_hi < 0 or dl_lo < 0:
            raise KirbyError(f"hat {hat:#x}: row {words} overrides more than its lists")
        if not 0 <= base < 15 or file_index not in (0, 1):
            raise KirbyError(f"hat {hat:#x}: row {words}")
        return {"base": base, "file": HAT_FILES[file_index], "dl": [dl_hi, dl_lo]}

    hat_rows: dict[int, dict] = {}
    powers = []
    proto = set()
    for c in contents:
        export = json.loads((args.export_root / c / "resolved.json").read_text(encoding="utf-8"))
        remix_id = export["remix_kind_id"]
        parent_id = export["parent_kind_id"]
        copy_id, _, _, _ = struct.unpack(">hhfi", rom.read_ram(inhale + 0xC * remix_id, 0xC))
        if copy_id != remix_id:
            raise KirbyError(f"{c}: Kirby takes power {copy_id}, not its own {remix_id}")
        _, hat_id, _, _ = struct.unpack(">hhfi", rom.read_ram(inhale + 0xC * copy_id, 0xC))
        # KirbyHats.add_hat rows (ids from 0xF): the source hat each re-skins.
        if hat_id < FIRST_CUSTOM_HAT:
            hat_base = hat_id
        else:
            hat_base = struct.unpack(">i", rom.read_ram(
                hats + (hat_id - FIRST_CUSTOM_HAT) * HAT_ROW_BYTES, 4))[0]
        if not 3 <= hat_base <= 13:
            raise KirbyError(f"{c}: hat {hat_id:#x} re-skins {hat_base}, not a copy hat")
        # The power's hats: its own, then the ones its scripts set on joint 6.
        # A Remix hat (id from 0xF) is drawn from its own native image
        # (p4_kirby_hats.py); a source hat id from the deferred source image.
        shown = [hat_id]
        for s in selected:
            if owner[s["status_id"]] != c:
                continue
            for part in sorted(script_hats(s)):
                base = hat_base_of(part)
                if not 0 <= base < 15:
                    raise KirbyError(f"{c}: script hat {part:#x} re-skins {base}")
                if part not in shown:
                    shown.append(part)
        remix_hats = [h for h in shown if h >= FIRST_CUSTOM_HAT]
        if len(remix_hats) > POWER_HAT_SLOTS:
            raise KirbyError(f"{c}: {len(remix_hats)} Remix hats, room for {POWER_HAT_SLOTS}")
        for h in remix_hats:
            hat_rows.setdefault(h, hat_row(h))
        # Hats 3..13 are the deferred images a match admits; 0-2 and 14
        # (the inhale and Yoshi-swallow faces) stay in Kirby's own owner.
        hat_mask = 0
        for h in shown:
            if h < FIRST_CUSTOM_HAT:
                hat_mask |= 1 << h
        hat_mask &= DEFERRED_HATS
        starters = []
        for table in (ground_nsp, air_nsp):
            value = rom.u32_ram(table + 4 * copy_id)
            if value == rom.u32_ram(table + 4 * parent_id):
                starters.append("NULL")
                continue
            if value >= R.REMIX_CODE_RAM:
                nm = (by_addr.get(value) or [None])[0]
            else:
                nm = decomp_names.get(value)
            if nm in G.CALLBACK_PORTS:
                starters.append("ndsP4Proc_" + G.CALLBACK_PORTS[nm])
                proto.add(G.CALLBACK_PORTS[nm])
            elif nm and "." not in nm:
                starters.append("ndsP4Proc_" + nm)
                proto.add(nm)
            else:
                raise KirbyError(f"{c}: Kirby's neutral special {value:#010x} ({nm}) has no DS port")
        clone_rows = []
        for base, clone in swaps.get(registry[c]["remix"], ()):
            if clone not in status_map:
                raise KirbyError(f"{c}: clone swap {base:#x} -> {clone:#x} is not a power status")
            clone_rows.append((base, status_map[clone]))
        powers.append({"content": c, "id": registry[c]["id"], "hat": hat_id, "hat_base": hat_base,
                       "hat_mask": hat_mask, "hats": remix_hats,
                       "hat_texture_file": (HAT_TEXTURE_FILE_BASE + registry[c]["id"]
                                            if remix_hats else 0),
                       "starters": starters, "clones": clone_rows})
    for p in powers:
        if p["hat_texture_file"]:
            shipped[p["hat_texture_file"]] = "p4_kirby_hats.py"

    # The weapons the powers hold: each its own file, a sidecar root of the
    # power's content (articles_<content>.json beside o2r/, for
    # generate_nds_entry_effects.py --p4), loaded for a match with the power.
    weapons = power_weapons((args.staging / "src" / "KirbyHats.asm").read_text(
        encoding="utf-8", errors="replace"))
    model_slots = rom.reloc_slots(KIRBY_MODEL_FILE)
    for p in powers:
        p["weapon"] = None
        part = weapons.get(p["hat"])
        if part is None:
            continue
        slot = model_slots.get(part)
        if slot is None or slot[0] != "extern":
            raise KirbyError(f"{p['content']}: weapon part {part:#x} names no file's list")
        _, wfile, woff = slot
        blob = rom.o2r(wfile)
        (out / "o2r" / f"{wfile:04x}").write_bytes(blob)
        shipped[wfile] = hashlib.sha256(blob).hexdigest()
        p["weapon"] = (wfile, woff)
        row = {"article": "kirby_weapon", "file_id": wfile, "storage": None, "via": 0,
               "offset": woff, "entry": False, "texid": None, "palette": None, "head": 0,
               "sidecar_joint": 17 | KIRBY_SIDECAR_BIT, "material": False,
               "via_base": False, "via_bias": 0}
        (out / f"articles_{p['content']}.json").write_text(
            json.dumps({"content": p["content"], "roots": [row]}, indent=1) + "\n",
            encoding="utf-8", newline="\n")

    # Outputs.
    header = [
        "/* Generated by scripts/p4/p4_kirby.py: Kirby's statuses for the enabled",
        " * P4 contents' powers (S8). Derived from the user's ROM: build output only. */",
        "#ifndef NDS_P4_KIRBY_GENERATED_H",
        "#define NDS_P4_KIRBY_GENERATED_H",
        f"#define NDS_P4_KIRBY_STATUS_FIRST {STATUS_FIRST:#x}",
        f"#define NDS_P4_KIRBY_STATUS_COUNT {len(specials)}",
        f"#define NDS_P4_KIRBY_MOTION_FIRST {VANILLA_MOTION_COUNT}",
        f"#define NDS_P4_KIRBY_MOTION_COUNT {len(rows)}",
        f"#define NDS_P4_KIRBY_SCRIPT_FILE {SCRIPT_FILE_ID:#x}",
    ]
    for r in specials:
        header.append(f"#define NDS_P4_KIRBY_STATUS_{c_ident(r['name'])} {r['status']:#x}"
                      f" /* Remix {r['remix']:#x} */")
    header += ["#endif", ""]
    (out / "nds_p4_kirby.generated.h").write_text("\n".join(header), encoding="utf-8", newline="\n")

    lines = [
        "/* Generated by scripts/p4/p4_kirby.py: Kirby's copies of the enabled P4",
        " * contents (S8). Derived from the user's ROM: build output only. */",
        "#include <nds/nds_p4.h>",
        "",
    ]
    proto |= {p for r in specials for p in r["procs"] if p != "NULL"}
    for p in sorted(proto):
        lines.append(f'void ndsP4Proc_{p}(GObj *fighter_gobj) __asm__("{p}");')
    lines += ["", "/* Kirby's power statuses from 0x12F: motion, motion attack, status flags",
              " * and callbacks, as generate_p4_fighter.py's special rows. */",
              f"FTStatusDesc gNdsP4KirbySpecialStatusDescs[{max(len(specials), 1)}] = {{"]
    for r in specials:
        lines.append(f"    /* {r['status']:#x} {r['name']} (Remix {r['remix']:#x}) */")
        lines.append(f"    {{ {{ {r['motion_id']}, {r['attack_id']} }}, "
                     f"{{ {{ {r['unused']}, {r['is_smash_attack']}, {r['ga']}, "
                     f"{r['is_projectile']}, {r['stat_attack_id']} }} }}, "
                     f"{', '.join(p if p == 'NULL' else 'ndsP4Proc_' + p for p in r['procs'])} }},")
    if not specials:
        lines.append("    { { 0, 0 } },")
    lines += ["};", f"const u32 gNdsP4KirbySpecialStatusCount = {len(specials)};", ""]
    clone_all = []
    lines += ["/* Kirby.clone_power_action_swap_: a status the parent's copy routine sets,",
              " * and the power's status Remix sets in its place. */"]
    for p in powers:
        p["clone_first"] = len(clone_all)
        clone_all.extend(p["clones"])
    lines.append(f"const NDSP4KirbyClone gNdsP4KirbyClones[{max(len(clone_all), 1)}] = {{")
    lines += [f"    {{ {b:#x}, {c:#x} }}," for b, c in clone_all] or ["    { 0, 0 },"]
    lines += ["};", "",
              "/* Each content's power: Remix hat id, Kirby's neutral-special starters",
              " * (kirby_ground_nsp / kirby_air_nsp; NULL is the parent's copy routine),",
              " * and its clone swaps. */",
              "const NDSP4KirbyPower gNdsP4KirbyPowers[NDS_P4_CONTENT_LIMIT] = {"]
    for p in powers:
        hats = ", ".join(f"{h:#x}" for h in p["hats"]) or "0"
        lines.append(f"    [{p['id']}] = {{ .hat_id = {p['hat']:#x}, .hat_base = {p['hat_base']}, "
                     f".clone_count = {len(p['clones'])}, .clone_first = {p['clone_first']}, "
                     f".hat_count = {len(p['hats'])}, .hat_mask = {p['hat_mask']:#06x}, "
                     f".hats = {{ {hats} }}, .hat_texture_file = {p['hat_texture_file']:#06x}, "
                     f".weapon_file = {p['weapon'][0] if p['weapon'] else 0:#06x}, "
                     f".nsp_ground = {p['starters'][0]}, .nsp_air = {p['starters'][1]} }},"
                     f" /* {p['content']} */")
    lines += ["};", "",
              "/* KirbyHats.add_hat: the source hat each Remix hat id from 0xF re-skins. */",
              f"const u8 gNdsP4KirbyHatBases[{len(hat_bases)}] = {{",
              "    " + ", ".join(str(b if b >= 0 else 0) for b in hat_bases) + ",",
              "};", f"const u32 gNdsP4KirbyHatBaseCount = {len(hat_bases)};", "",
              "/* Files a match with a power loads (nitro:/reloc/p4/<id>). */",
              f"const u16 gNdsP4KirbyRelocAssets[] = {{"]
    ids = [f"{fid:#06x}" for fid in sorted(shipped)]
    for i in range(0, len(ids), 12):
        lines.append("    " + ", ".join(ids[i:i + 12]) + ",")
    anims = sorted(anim_flags)
    lines += ["};", f"const u32 gNdsP4KirbyRelocAssetCount = {len(shipped)};", "",
              "/* The power animations; bit 15 marks AObjEvent32 (FTANIM_FLAG_ANIMJOINT). */",
              f"const u16 gNdsP4KirbyAnims[{max(len(anims), 1)}] = {{",
              "    " + (", ".join(f"{a | (0x8000 if (anim_flags[a] & FLAG_ANIMJOINT) and a not in converted32 else 0):#06x}"
                                for a in anims) or "0") + ",",
              "};", f"const u32 gNdsP4KirbyAnimCount = {len(anims)};", ""]
    # SwordTrail.asm rows Kirby's power scripts can select: his own id's
    # (Falco's Phantasm trail) and any fighter's (0xFFFF: Marth's blade).
    trails = [t for t in resolved.get("sword_trails", ())
              if t["character"] in (resolved["remix_kind_id"], 0xFFFF)]
    lines += ["/* Remix sword trails Kirby's power scripts select (SwordTrail.asm). */",
              f"const NDSP4SwordTrail gNdsP4KirbySwordTrails[{max(len(trails), 1)}] = {{"]
    for t in trails:
        if t["id"] > 0xFF or t["model_part"] > 0xFF or t["axis"] > 2:
            raise KirbyError(f"sword trail {t['id']}: unsupported row {t}")
        lines.append(f"    {{ {t['id']}, {t['model_part']}, {t['axis']}, 0, "
                     f"{t['colour_1']:#010x}u, {t['colour_2']:#010x}u, "
                     f"{G.c_float(t['start'])}, {G.c_float(t['end'])} }},")
    if not trails:
        lines.append("    { 0, 0, 0, 0, 0, 0, 0.0F, 0.0F },")
    lines += ["};", f"const u32 gNdsP4KirbySwordTrailCount = {len(trails)};", ""]
    (out / "nds_p4_kirby.generated.c").write_text("\n".join(lines), encoding="utf-8", newline="\n")

    tables = [
        "/* Generated by scripts/p4/p4_kirby.py: Kirby's power motion rows, objcopied",
        " * from .p4_tables into nitro:/p4/kirby.tab. Script offsets are into the",
        f" * script file {SCRIPT_FILE_ID:#x}. Build output only. */",
        "#include <stddef.h>",
        "#include <nds/nds_p4.h>",
        "",
        "typedef struct",
        "{",
        "    NDSP4TablesHeader header;",
        f"    FTMotionDesc motions[{max(len(rows), 1)}];",
        "} NDSP4KirbyTables;",
        "",
        "const NDSP4KirbyTables gNdsP4KirbyTables",
        '    __attribute__((section(".p4_tables"), used, aligned(4))) =',
        "{",
        "    { NDS_P4_TABLES_MAGIC, NDS_P4_TABLES_VERSION, sizeof(NDSP4KirbyTables),",
        f"      offsetof(NDSP4KirbyTables, motions), {len(rows)}, 0, 0, 0, 0, 0 }},",
        "    {",
    ]
    for (a, s, f), m in zip(rows, motion_ids):
        tables.append(f"        {{ {a:#06x}, (intptr_t){s:#010x}, {{ .word = {f:#010x} }} }}, "
                      f"/* {motion_map[m]} (Remix {m:#x}) */")
    if not rows:
        tables.append("        { 0, (intptr_t)0x80000000, { .word = 0 } },")
    tables += ["    },", "};", ""]
    (out / "nds_p4_kirby.tables.c").write_text("\n".join(tables), encoding="utf-8", newline="\n")

    manifest = {
        "schema": "p4-ds-kirby-v1",
        "reference_rom_sha1": resolved["provenance"]["reference_rom_sha1"],
        "contents": contents,
        "statuses": [{k: r[k] for k in ("status", "remix", "name", "content", "motion_id")}
                     for r in specials],
        "script_file": {"file_id": SCRIPT_FILE_ID, "bytes": len(synth.data),
                        "intern_slots": synth.intern_count, "round_trip_roots": checked},
        "animations": {f"{a:#x}": anim_sizes.get(a) for a in anims},
        "largest_animation": max(anim_sizes.values(), default=0),
        "custom_hats": {f"{h:#x}": r for h, r in sorted(hat_rows.items())},
        "powers": [{"content": p["content"], "hat": p["hat"], "hat_base": p["hat_base"],
                    "hat_mask": hex(p["hat_mask"]), "hats": p["hats"],
                    "hat_texture_file": p["hat_texture_file"],
                    "weapon": [hex(v) for v in p["weapon"]] if p["weapon"] else None,
                    "starters": p["starters"],
                    "clones": [[hex(b), hex(c)] for b, c in p["clones"]]} for p in powers],
        "files": {f"{k:#06x}": v for k, v in sorted(shipped.items())},
    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8",
                                       newline="\n")
    print(f"p4 kirby: {len(specials)} statuses, {len(rows)} motions, {len(anims)} animations, "
          f"script file {len(synth.data)} B ({checked} roots), {len(powers)} powers")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
