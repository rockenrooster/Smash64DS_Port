#!/usr/bin/env python3
"""P4 source adapter, stage 1: export one Remix fighter's resolved donor data.

Input is a disposable staging build of the pinned Remix tree (see
docs/P4/P4_0_SOURCE_ADMISSION.md for the recipe): the assembled reference ROM,
bass's `-sym` log and the staged source. Nothing under decomp/ is written.

Output (in --out):
  resolved.json   character descriptor, effective motion descs (param array),
                  status descs (action array) with named callbacks, menu motions,
                  per-kind Remix table rows, inheritance diffs against the parent
  events.json     typed event graph for every reachable motion-script root
  o2r/<id hex>    Torch SSB64:RELOC containers for the fighter's file closure
  summary.txt     human-readable inventory with every unresolved item listed

Unknown opcodes, out-of-region pointers, unknown callbacks and zero-time loops
are recorded as explicit failures; the exit status is non-zero when any exist
unless --allow-failures is given.
"""
from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import remix_rom as R  # noqa: E402

REPO = Path(__file__).resolve().parents[2]
DECOMP = REPO / "decomp" / "BattleShip-main" / "decomp"

STRUCT_TABLE_ROM = 0x92610
ACTION_ARRAY_TABLE_ROM = 0xA6F40
SHARED_ACTION_ARRAY_ROM = 0xA45D8
SHARED_ACTION_COUNT = 0xDC
STATUS_DESC_SIZE = 0x14
MOTION_DESC_SIZE = 0xC
NO_SCRIPT = 0x80000000

VANILLA_IDS = {
    "MARIO": 0x00, "FOX": 0x01, "DONKEY": 0x02, "DK": 0x02, "SAMUS": 0x03,
    "LUIGI": 0x04, "LINK": 0x05, "YOSHI": 0x06, "CAPTAIN": 0x07, "KIRBY": 0x08,
    "PIKACHU": 0x09, "JIGGLY": 0x0A, "JIGGLYPUFF": 0x0A, "NESS": 0x0B,
    "BOSS": 0x0C, "METAL": 0x0D,
}
PARENT_HEADERS = {
    0x00: "ftmario/ftmario.h", 0x01: "ftfox/ftfox.h", 0x02: "ftdonkey/ftdonkey.h",
    0x03: "ftsamus/ftsamus.h", 0x04: "ftmario/ftmario.h", 0x05: "ftlink/ftlink.h",
    0x06: "ftyoshi/ftyoshi.h", 0x07: "ftcaptain/ftcaptain.h", 0x08: "ftkirby/ftkirby.h",
    0x09: "ftpikachu/ftpikachu.h", 0x0A: "ftpurin/ftpurin.h", 0x0B: "ftness/ftness.h",
}

# ---------------------------------------------------------------------------
# Motion events. Lengths follow ftMainParseMotionEvent's ftMotionEventAdvance
# types (decomp/.../ft/ftmain.c); Remix's custom commands key on the first byte
# >= 0xD0 (src/Command.asm load_command_, COMMAND_LENGTH per scope).
# ---------------------------------------------------------------------------
EVENTS = [
    ("End", 4), ("SyncWait", 4), ("AsyncWait", 4), ("MakeAttackColl", 20),
    ("MakeAttackCollScaled", 20), ("ClearAttackCollID", 4), ("ClearAttackCollAll", 4),
    ("SetAttackCollOffset", 8), ("SetAttackCollDamage", 4), ("SetAttackCollSize", 4),
    ("SetAttackCollSoundLevel", 4), ("RefreshAttackCollID", 4), ("SetThrow", 8),
    ("SetDamageThrown", 8), ("PlayFGM", 4), ("PlayLoopSFXStoreInfo", 4),
    ("StopLoopSFX", 4), ("PlayVoiceStoreInfo", 4), ("PlayLoopVoiceStoreInfo", 4),
    ("PlayFGMStoreInfo", 4), ("PlaySmashVoice", 4), ("SetFlag0", 4), ("SetFlag1", 4),
    ("SetFlag2", 4), ("SetFlag3", 4), ("SetAirJumpAdd", 4), ("SetAirJumpMax", 4),
    ("SetHitStatusPartAll", 4), ("SetHitStatusPartID", 4), ("SetHitStatusAll", 4),
    ("ResetDamageCollPartAll", 4), ("SetDamageCollPartID", 16), ("LoopBegin", 4),
    ("LoopEnd", 4), ("Subroutine", 8), ("Return", 4), ("Goto", 8), ("PauseScript", 4),
    ("Effect", 16), ("EffectItemHold", 16), ("SetModelPartID", 4),
    ("ResetModelPartAll", 4), ("HideModelPartAll", 4), ("SetTexturePartID", 4),
    ("SetColAnim", 4), ("ResetColAnim", 4), ("SetParallelScript", 8),
    ("SetSlopeContour", 4), ("HideItem", 4), ("MakeRumble", 4), ("StopRumble", 4),
    ("SetAfterImage", 4),
]
EV_END, EV_SYNCWAIT, EV_ASYNCWAIT = 0, 1, 2
EV_SETTHROW, EV_SETDAMAGETHROWN = 12, 13
EV_LOOPBEGIN, EV_LOOPEND, EV_SUBROUTINE, EV_RETURN, EV_GOTO = 32, 33, 34, 35, 36
EV_PARALLEL = 46
REMIX_CUSTOM = {
    0xD0: ("RemixFrameSpeedMultiplier", 4), 0xD1: ("RemixArmour", 4),
    0xD2: ("RemixHitboxDirection", 4), 0xD3: ("RemixTranslationMultiplier", 4),
    0xD4: ("RemixYVelocity", 4), 0xD5: ("RemixFastFall", 4),
    0xD6: ("RemixRandomSFX", 8), 0xD7: ("RemixSetKinetic", 4),
    0xD8: ("RemixSetHitboxFGM", 4), 0xD9: ("RemixSetEnvColor", 8),
    0xDA: ("RemixSwitchDirection", 4), 0xDB: ("RemixGotoMovesetFile", 4),
    0xDC: ("RemixLVoiceSFX", 8),
}


class Failures:
    def __init__(self):
        self.items: list[str] = []

    def add(self, msg: str):
        self.items.append(msg)


def parse_enum_values(paths: list[Path]) -> dict[str, int]:
    """Evaluate simple C enums (implicit increments and aliases to earlier names)."""
    values: dict[str, int] = {}
    for path in paths:
        text = re.sub(r"//[^\n]*|/\*.*?\*/", "", path.read_text(encoding="utf-8",
                                                                errors="replace"), flags=re.S)
        for body in re.findall(r"enum\s+\w*\s*\{(.*?)\}", text, flags=re.S):
            cur = -1
            for item in body.split(","):
                item = item.strip()
                if not item:
                    continue
                if "=" in item:
                    name, expr = (s.strip() for s in item.split("=", 1))
                    expr = expr.strip("() ")
                    m = re.fullmatch(r"(\w+)\s*([+-])\s*(\w+)", expr)
                    try:
                        if expr in values:
                            cur = values[expr]
                        elif m and m.group(1) in values:
                            off = int(m.group(3), 0)
                            cur = values[m.group(1)] + (off if m.group(2) == "+" else -off)
                        else:
                            cur = int(expr, 0)
                    except ValueError:
                        continue
                else:
                    name = item
                    cur += 1
                if re.fullmatch(r"\w+", name):
                    values[name] = cur
    return values


def status_names(parent_id: int) -> dict[int, str]:
    paths = [DECOMP / "src" / "ft" / "ftdef.h"]
    hdr = PARENT_HEADERS.get(parent_id)
    if hdr:
        paths.append(DECOMP / "src" / "ft" / "ftchar" / hdr)
    values = parse_enum_values(paths)
    prefix = {0x01: "nFTFoxStatus", 0x00: "nFTMarioStatus", 0x07: "nFTCaptainStatus",
              0x06: "nFTYoshiStatus", 0x04: "nFTLuigiStatus", 0x05: "nFTLinkStatus",
              0x02: "nFTDonkeyStatus", 0x03: "nFTSamusStatus", 0x08: "nFTKirbyStatus",
              0x09: "nFTPikachuStatus", 0x0A: "nFTPurinStatus", 0x0B: "nFTNessStatus"}.get(parent_id)
    out: dict[int, str] = {}
    for name, val in values.items():
        if name.startswith("nFTCommonStatus") and val < SHARED_ACTION_COUNT:
            out.setdefault(val, name)
        elif prefix and name.startswith(prefix) and val >= SHARED_ACTION_COUNT:
            if not name.endswith(("ScopeStart", "ScopeEnd")):
                out.setdefault(val, name)
    return out


def parse_define_character(src: Path, name: str) -> list[str]:
    text = (src / "Character.asm").read_text(encoding="utf-8", errors="replace")
    m = re.search(r"^\s*define_character\(\s*" + re.escape(name) + r"\s*,(.*?)\)\s*$",
                  text, flags=re.M)
    if not m:
        raise SystemExit(f"define_character({name}, ...) not found")
    return [a.strip() for a in m.group(1).split(",")]


def parse_table_sizes(src: Path) -> dict[str, int]:
    text = (src / "Character.asm").read_text(encoding="utf-8", errors="replace")
    body = text[text.index("macro define_character(name, parent"):]
    body = body[:body.index("macro copy_gfx_parameters")]
    sizes = {}
    for t, size in re.findall(r"add_to_table\((\w+),\s*id\.\{name\},\s*id\.\{parent\},\s*(0x[0-9A-Fa-f]+)\)", body):
        sizes[t] = int(size, 16)
    for t in re.findall(r"add_to_id_table\((\w+),\s*id\.\{name\},\s*id\.\{parent\}\)", body):
        sizes[t] = 4
    for t in re.findall(r"add_to_(?:jab_3|rapid_jab)_table\((\w+),", body):
        sizes[t] = 4
    # jab_3 and rapid_jab (ENABLED/DISABLED by parent) are written in place.
    for t in re.findall(r"origin\s+(\w+)\.TABLE_ORIGIN\s*\+\s*\(id\.\{name\}\s*\*\s*0x4\)", body):
        sizes.setdefault(t, 4)
    return sizes


class Exporter:
    def __init__(self, staging: Path, fighter: str, failures: Failures):
        self.staging = staging
        self.fighter = fighter
        self.fail = failures
        self.rom = R.Rom(staging / "ssb64asm.z64")
        self.sym, self.sym_by_addr = R.load_symbols(staging / "logfile.log")
        self.decomp_sym = R.load_decomp_symbols(DECOMP / "symbols" / "symbols_us.txt")
        self.args = parse_define_character(staging / "src", fighter)
        self.parent_name = self.args[0]
        if self.parent_name not in VANILLA_IDS:
            raise SystemExit(f"unsupported parent {self.parent_name}")
        self.parent_id = VANILLA_IDS[self.parent_name]
        self.add_actions = int(self.args[11], 0)
        self.struct_ram = self.sym[f"Character.{fighter}_character_struct"]
        self.kind_id = self._find_kind_id()

    def _find_kind_id(self) -> int:
        for k in range(0, 0x80):
            if self.rom.u32(STRUCT_TABLE_ROM + k * 4) == self.struct_ram:
                return k
        raise SystemExit("character struct not found in STRUCT_TABLE")

    def name_code(self, addr: int) -> str | None:
        if addr == 0:
            return None
        if addr >= R.REMIX_CODE_RAM:
            names = self.sym_by_addr.get(addr)
            return names[0] if names else None
        return self.decomp_sym.get(addr)

    # -- sword trails ---------------------------------------------------------
    def sword_trails(self) -> list[dict]:
        """SwordTrail.asm's table, indexed by the SET AFTERIMAGE command's
        is_itemswing value: 0 and 1 are the vanilla trails (empty slots); 2 on
        are Remix's add_sword_trail rows (struct: u16 character, 0xFFFF any;
        u8 model part; u8 axis; RGBA32 base and tip colours; f32 start and end
        positions along the axis)."""
        table = self.sym["SwordTrail.sword_trail_table"]
        following = min(a for a in self.sym_by_addr if a > table)
        rows = []
        for trail_id in range((following - table) // 4):
            ptr = self.rom.u32_ram(table + trail_id * 4)
            if ptr == 0:
                continue
            ch, part, axis, c1, c2, start, end = struct.unpack(
                ">HBBIIff", self.rom.read_ram(ptr, 20))
            if axis > 2:
                self.fail.add(f"sword trail {trail_id}: axis {axis}")
            rows.append({"id": trail_id, "character": ch, "model_part": part,
                         "axis": axis, "colour_1": c1, "colour_2": c2,
                         "start": start, "end": end})
        return rows

    # -- descriptors ----------------------------------------------------------
    def read_struct(self, ram: int) -> dict:
        raw = self.rom.read_ram(ram, 0x78)
        w = struct.unpack(">30I", raw)
        return {
            "file_ids": list(w[0:9]),
            "file_main_size": w[9],
            "o_attributes": w[0x60 // 4],
            "mainmotion": w[0x64 // 4],
            "submotion": w[0x68 // 4],
            "mainmotion_count": w[0x6C // 4],
            "submotion_count_ptr": w[0x70 // 4],
            "file_anim_size": w[0x74 // 4],
            "particles": list(w[0x4C // 4:0x60 // 4]),
        }

    def read_motion_descs(self, ram: int, count: int) -> list[dict]:
        out = []
        for i in range(count):
            a, s, f = struct.unpack(">III", self.rom.read_ram(ram + i * MOTION_DESC_SIZE, 12))
            out.append({"index": i, "anim_file_id": a, "script": s, "anim_flags": f})
        return out

    def read_menu_descs(self, ram: int, count: int, vanilla: bool) -> list[dict]:
        out = []
        for i in range(count):
            if vanilla:
                off = ram - R.MENU_OVERLAY_RAM_MINUS_ROM + i * 12
                a, s, f = struct.unpack(">III", self.rom.data[off:off + 12])
            else:
                a, s, f = struct.unpack(">III", self.rom.read_ram(ram + i * 12, 12))
            out.append({"index": i, "anim_file_id": a, "script": s, "anim_flags": f})
        return out

    def read_status_descs(self, rom_off: int, count: int) -> list[dict]:
        out = []
        for i in range(count):
            mf, sf, u, it, ph, mp = struct.unpack(
                ">HHIIII", self.rom.data[rom_off + i * 20:rom_off + i * 20 + 20])
            motion_id = mf >> 6
            if motion_id & 0x200:
                motion_id -= 0x400  # s16 : 10; -1 and -2 are ftMainSetStatus sentinels
            out.append({
                "motion_id": motion_id,
                "attack_id": mf & 0x3F, "sflags": sf,
                "proc_update": u, "proc_interrupt": it, "proc_physics": ph, "proc_map": mp,
            })
        return out

    def export(self) -> dict:
        rom = self.rom
        me = self.read_struct(self.struct_ram)
        parent_struct_ram = rom.u32(STRUCT_TABLE_ROM + self.parent_id * 4)
        parent = self.read_struct(parent_struct_ram)
        names = status_names(self.parent_id)

        motions = self.read_motion_descs(me["mainmotion"], me["mainmotion_count"])
        parent_motions = self.read_motion_descs(parent["mainmotion"], parent["mainmotion_count"])
        for m in motions:
            p = parent_motions[m["index"]] if m["index"] < len(parent_motions) else None
            m["inherited"] = {k: (p is not None and p[k] == m[k])
                              for k in ("anim_file_id", "script", "anim_flags")}

        menu_count = 0xF
        menus = self.read_menu_descs(me["submotion"], menu_count, vanilla=False)
        parent_menus = self.read_menu_descs(parent["submotion"], menu_count, vanilla=True)
        for m in menus:
            p = parent_menus[m["index"]]
            m["inherited"] = {k: p[k] == m[k] for k in ("anim_file_id", "script", "anim_flags")}

        shared = self.read_status_descs(SHARED_ACTION_ARRAY_ROM, SHARED_ACTION_COUNT)
        parent_actions_ram = rom.u32(ACTION_ARRAY_TABLE_ROM + self.parent_id * 4)
        own_action_ram = self.sym[f"Character.{self.fighter}_action_array"]
        own_count = (int(self._action_size(), 0) // STATUS_DESC_SIZE) + self.add_actions
        specials = self.read_status_descs(rom.remix_rom_offset(own_action_ram), own_count)
        parent_specials = self.read_status_descs(
            parent_actions_ram - R.FT_OVERLAY_RAM_MINUS_ROM, int(self._action_size(), 0) // STATUS_DESC_SIZE)

        statuses = []
        for sid, d in enumerate(shared + specials):
            row = dict(d)
            row["status_id"] = sid
            row["name"] = names.get(sid)
            if sid >= SHARED_ACTION_COUNT:
                k = sid - SHARED_ACTION_COUNT
                p = parent_specials[k] if k < len(parent_specials) else None
                row["inherited"] = {f: (p is not None and p[f] == d[f]) for f in d}
                # The parent's callbacks for this status (None past the
                # parent's table: a Remix add_new_action status).
                row["parent_procs"] = None if p is None else [
                    self.name_code(p[proc]) if p[proc] else None
                    for proc in ("proc_update", "proc_interrupt", "proc_physics", "proc_map")]
            for proc in ("proc_update", "proc_interrupt", "proc_physics", "proc_map"):
                addr = d[proc]
                if addr:
                    nm = self.name_code(addr)
                    row[proc + "_name"] = nm
                    if nm is None:
                        self.fail.add(f"status {sid:#x} {proc} {addr:#010x}: no symbol")
            mid = d["motion_id"]
            if mid < -2 or mid >= len(motions):
                self.fail.add(f"status {sid:#x} motion {mid:#x} beyond {len(motions)} motion descs")
            statuses.append(row)

        tables = self.read_kind_tables()
        return {
            "fighter": self.fighter, "remix_kind_id": self.kind_id,
            "parent": self.parent_name, "parent_kind_id": self.parent_id,
            "define_character_args": self.args, "descriptor": me,
            "parent_descriptor": parent, "motions": motions, "menu_motions": menus,
            "statuses": statuses, "kind_tables": tables,
        }

    def _action_size(self) -> str:
        text = (self.staging / "src" / "Character.asm").read_text(encoding="utf-8", errors="replace")
        body = text[text.index("scope action_array_size {"):]
        m = re.search(r"constant\s+" + re.escape(self.parent_name) + r"\((0x[0-9A-Fa-f]+)\)", body)
        return m.group(1)

    def read_kind_tables(self) -> dict:
        sizes = parse_table_sizes(self.staging / "src")
        out = {}
        for table, size in sorted(sizes.items()):
            base = self.sym.get(f"Character.{table}.table")
            if base is None:
                self.fail.add(f"kind table {table}: no `table` symbol")
                continue
            own = self.rom.read_ram(base + self.kind_id * size, size)
            par = self.rom.read_ram(base + self.parent_id * size, size)
            out[table] = {"entry_size": size, "value": own.hex(), "parent_value": par.hex(),
                          "same_as_parent": own == par}
        return out


# ---------------------------------------------------------------------------
# Event graph
# ---------------------------------------------------------------------------
class EventDecoder:
    """Decode motion scripts in two address spaces: ('file', id, off) for vanilla
    motion files (pointers are reloc slots) and ('ram', addr) for Remix inserts
    (pointers are absolute RAM)."""

    def __init__(self, rom: R.Rom, sym_by_addr: dict, motion_file: int, failures: Failures):
        self.rom = rom
        self.sym_by_addr = sym_by_addr
        self.motion_file = motion_file
        self.fail = failures
        self.slots: dict[int, dict] = {}
        self.blocks: dict[str, dict] = {}
        self.data_refs: list[dict] = []

    @staticmethod
    def key(loc: tuple) -> str:
        return f"file:{loc[1]:#x}:{loc[2]:#x}" if loc[0] == "file" else f"ram:{loc[1]:#010x}"

    def _word(self, loc: tuple) -> int | None:
        if loc[0] == "file":
            data = self.rom.file_bytes(loc[1])
            if loc[2] + 4 > len(data):
                return None
            return int.from_bytes(data[loc[2]:loc[2] + 4], "big")
        try:
            return self.rom.u32_ram(loc[1])
        except R.RomError:
            return None

    def _advance(self, loc: tuple, n: int) -> tuple:
        return (loc[0], loc[1], loc[2] + n) if loc[0] == "file" else ("ram", loc[1] + n)

    def _pointer(self, loc: tuple, owner: str) -> tuple | None:
        """Resolve the pointer word at loc (second word of a pointer command)."""
        if loc[0] == "file":
            fid = loc[1]
            if fid not in self.slots:
                self.slots[fid] = self.rom.reloc_slots(fid)
            slot = self.slots[fid].get(loc[2])
            if slot is None:
                self.fail.add(f"{owner}: pointer word at {self.key(loc)} is not a reloc slot")
                return None
            if slot[0] == "intern":
                return ("file", fid, slot[1])
            return ("file", slot[1], slot[2])
        value = self._word(loc)
        # A vanilla menu script in overlay 1 (read_ram maps it) jumps within
        # overlay 1 (Bowser's 0x80391BC4 to 0x80391C90).
        if value is not None and R.MENU_OVERLAY_RAM[0] <= value < R.MENU_OVERLAY_RAM[1]:
            return ("ram", value)
        if value is None or value < R.REMIX_CODE_RAM:
            self.fail.add(f"{owner}: pointer {value!r} at {self.key(loc)} outside the Remix region")
            return None
        return ("ram", value)

    def label(self, loc: tuple) -> str | None:
        if loc[0] == "ram":
            names = self.sym_by_addr.get(loc[1])
            return names[0] if names else None
        return None

    def decode(self, root: tuple, origin: str):
        work = [(root, origin)]
        while work:
            loc, why = work.pop()
            k = self.key(loc)
            if k in self.blocks:
                self.blocks[k]["referrers"].append(why)
                continue
            block = {"start": k, "label": self.label(loc), "referrers": [why],
                     "commands": [], "exit": None, "edges": []}
            self.blocks[k] = block
            cur = loc
            seen_wait = False
            for _ in range(4096):
                w = self._word(cur)
                if w is None:
                    self.fail.add(f"{k}: stream runs off its region at {self.key(cur)}")
                    block["exit"] = "off-region"
                    break
                first = w >> 24
                if first >= 0xD0:
                    if first not in REMIX_CUSTOM:
                        self.fail.add(f"{k}: unknown custom command {w:#010x} at {self.key(cur)}")
                        block["exit"] = "unknown-command"
                        break
                    name, length = REMIX_CUSTOM[first]
                    cmd = {"at": self.key(cur), "op": name, "words": [w]}
                    if first == 0xD6:
                        target = self._pointer(self._advance(cur, 4), k)
                        cmd["data"] = self.key(target) if target else None
                        if target:
                            self.data_refs.append({"kind": "random_sfx_table", "from": cmd["at"], "to": cmd["data"]})
                    block["commands"].append(cmd)
                    if first == 0xDB:
                        target = ("file", self.motion_file, w & 0xFFFF)
                        block["exit"] = "goto-moveset-file"
                        block["edges"].append({"kind": "goto", "to": self.key(target)})
                        work.append((target, f"{k}:goto-file"))
                        break
                    cur = self._advance(cur, length)
                    continue
                op = w >> 26
                if op >= len(EVENTS):
                    self.fail.add(f"{k}: unknown opcode {op} ({w:#010x}) at {self.key(cur)}")
                    block["exit"] = "unknown-command"
                    break
                name, length = EVENTS[op]
                cmd = {"at": self.key(cur), "op": name, "words": [w]}
                block["commands"].append(cmd)
                if op in (EV_SYNCWAIT, EV_ASYNCWAIT):
                    seen_wait = True
                if op == EV_END:
                    block["exit"] = "end"
                    break
                if op == EV_RETURN:
                    block["exit"] = "return"
                    break
                if op in (EV_GOTO, EV_SUBROUTINE, EV_PARALLEL, EV_SETTHROW, EV_SETDAMAGETHROWN):
                    target = self._pointer(self._advance(cur, 4), k)
                    cmd["to"] = self.key(target) if target else None
                    if op == EV_SETTHROW:
                        if target:
                            self.data_refs.append({"kind": "throw_data", "from": cmd["at"], "to": cmd["to"]})
                    elif target:
                        kind = {EV_GOTO: "goto", EV_SUBROUTINE: "call",
                                EV_PARALLEL: "parallel", EV_SETDAMAGETHROWN: "damage-thrown"}[op]
                        block["edges"].append({"kind": kind, "to": cmd["to"]})
                        if op == EV_GOTO and target == loc and not seen_wait:
                            self.fail.add(f"{k}: zero-time goto loop")
                        work.append((target, f"{k}:{kind}"))
                    if op == EV_GOTO:
                        block["exit"] = "goto"
                        break
                cur = self._advance(cur, length)
            else:
                self.fail.add(f"{k}: no exit within 4096 commands")
                block["exit"] = "unbounded"

    def mark_fallthrough(self):
        """A block that reaches another block's start without an exit falls into it."""
        starts = set(self.blocks)
        for k, block in self.blocks.items():
            for cmd in block["commands"][1:]:
                if cmd["at"] in starts and cmd["at"] != k:
                    block.setdefault("falls_into", []).append(cmd["at"])


def script_root(script: int, motion_file: int) -> tuple | None:
    if script == NO_SCRIPT:
        return None
    if script & 0x80000000:
        return ("ram", script)
    return ("file", motion_file, script)


def file_closure(rom: R.Rom, roots: set[int]) -> list[int]:
    seen: set[int] = set()
    stack = sorted(roots)
    while stack:
        fid = stack.pop()
        if fid in seen or fid == 0:
            continue
        seen.add(fid)
        for ext in rom.extern_ids(fid):
            if ext not in seen:
                stack.append(ext)
    return sorted(seen)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--fighter", required=True, help="Remix define_character name, e.g. FALCO")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--no-o2r", action="store_true")
    ap.add_argument("--allow-failures", action="store_true")
    args = ap.parse_args()

    fail = Failures()
    ex = Exporter(args.staging, args.fighter, fail)
    resolved = ex.export()
    motion_file = resolved["descriptor"]["file_ids"][1]

    dec = EventDecoder(ex.rom, ex.sym_by_addr, motion_file, fail)
    for m in resolved["motions"]:
        root = script_root(m["script"], motion_file)
        m["script_root"] = dec.key(root) if root else None
        if root:
            dec.decode(root, f"motion:{m['index']:#x}")
    for m in resolved["menu_motions"]:
        root = script_root(m["script"], motion_file)
        m["script_root"] = dec.key(root) if root else None
        if root:
            dec.decode(root, f"menu:{m['index']:#x}")
    dec.mark_fallthrough()

    file_roots = {f for f in resolved["descriptor"]["file_ids"] if f}
    file_roots |= {m["anim_file_id"] for m in resolved["motions"] + resolved["menu_motions"]
                   if m["anim_file_id"]}
    closure = file_closure(ex.rom, file_roots)
    vanilla_rom = R.Rom(args.staging / "roms" / "ssb.rom")
    resolved["file_closure"] = []
    for f in closure:
        row = {"file_id": f, "vanilla_id": f < vanilla_rom.file_count,
               "bytes": len(ex.rom.file_bytes(f)), "externs": ex.rom.extern_ids(f)}
        if row["vanilla_id"]:
            # A vanilla ID whose donor bytes changed is donor content, not the
            # vanilla asset the DS build already ships for that ID.
            row["vanilla_identical"] = ex.rom.o2r(f) == vanilla_rom.o2r(f)
        resolved["file_closure"].append(row)

    resolved["sword_trails"] = ex.sword_trails()

    staging = args.staging
    resolved["provenance"] = {
        "reference_rom_sha1": R.sha1(ex.rom.data),
        "symbols_sha1": R.sha1((staging / "logfile.log").read_bytes()),
        "file_count": ex.rom.file_count,
    }
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "resolved.json").write_text(json.dumps(resolved, indent=1), encoding="utf-8")
    (args.out / "events.json").write_text(json.dumps(
        {"blocks": dec.blocks, "data_refs": dec.data_refs}, indent=1), encoding="utf-8")
    if not args.no_o2r:
        od = args.out / "o2r"
        od.mkdir(exist_ok=True)
        for f in closure:
            (od / f"{f:04x}").write_bytes(ex.rom.o2r(f))

    falls = {k: b["falls_into"] for k, b in dec.blocks.items() if b.get("falls_into")}
    lines = [
        f"fighter {args.fighter} remix kind {resolved['remix_kind_id']:#x} parent {resolved['parent']}",
        f"motions {len(resolved['motions'])} (own anim {sum(not m['inherited']['anim_file_id'] for m in resolved['motions'])},"
        f" own script {sum(not m['inherited']['script'] for m in resolved['motions'])})",
        f"statuses {len(resolved['statuses'])}; special overrides "
        f"{sum(1 for s in resolved['statuses'] if s.get('inherited') and not all(s['inherited'].values()))}",
        f"event blocks {len(dec.blocks)}; data refs {len(dec.data_refs)}; fall-through {len(falls)}",
        f"file closure {len(closure)} ({sum(1 for r in resolved['file_closure'] if not r['vanilla_id'])} donor-only, "
        f"{sum(1 for r in resolved['file_closure'] if r['vanilla_id'] and not r['vanilla_identical'])} vanilla IDs with donor bytes: "
        f"{[hex(r['file_id']) for r in resolved['file_closure'] if r['vanilla_id'] and not r['vanilla_identical']]})",
        f"kind tables {len(resolved['kind_tables'])}; differing from parent "
        f"{sorted(t for t, v in resolved['kind_tables'].items() if not v['same_as_parent'])}",
        f"failures {len(fail.items)}",
    ] + [f"  FAIL {x}" for x in fail.items] + [f"  fallthrough {k} -> {v}" for k, v in falls.items()]
    (args.out / "summary.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))
    return 0 if (not fail.items or args.allow_failures) else 1


if __name__ == "__main__":
    sys.exit(main())
