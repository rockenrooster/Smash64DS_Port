#!/usr/bin/env python3
"""P7: every fighter's attack values (owner p7h, 2026-10-19: a "character
balance table" -- damage, knockback, speed and the rest for all attacks).

Read-only over what the build itself reads:
  * the originals: BattleShip's motion scripts (decomp relocData
    *MainMotion.c), their motion descs (ft/ftdata.c) and motion enums
    (ft/ftdef.h, ft/ftchar/*/*.h);
  * the Remix and EXTRA contents: the P4 export (builds/p4-staging/export-x/
    <name>: resolved.json, events.json) with each command's words read from
    the staging ROM, plus the generator's owner edits
    (scripts/p4/generate_p4_fighter.py OWNER_HITBOX_SCALES, OWNER_ANIM_RATES);
  * the authored contents: scripts/p6/<name>/motions/*.ms over spec.json,
    the rest their base's.

The values are the game data's, so the output stays local (artifacts/, never
committed): `--json` for the data, `--html` for the page.

Frames are the script's animation frames, 1-based (a box made at frame 0 of
the clip is "1"); a box still out at the script's end runs "to the end". A
content with an animation rate (Sheik 1.1x) plays its frames that much faster.
"""
from __future__ import annotations

import argparse
import html
import json
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DECOMP = REPO / "decomp" / "BattleShip-main" / "decomp" / "src"
STAGING = REPO / "builds" / "p4-staging" / "extra-96621af"
EXPORT = REPO / "builds" / "p4-staging" / "export-x"
sys.path.insert(0, str(REPO / "scripts" / "p4"))
sys.path.insert(0, str(REPO / "scripts" / "p6"))

ORIGINALS = [
    # (title, decomp name, MainMotion source)
    ("Mario", "Mario", "202_MarioMainMotion.c"),
    ("Luigi", "Luigi", "220_LuigiMainMotion.c"),
    ("Donkey Kong", "Donkey", "212_DonkeyMainMotion.c"),
    ("Link", "Link", "224_LinkMainMotion.c"),
    ("Samus", "Samus", "216_SamusMainMotion.c"),
    ("Captain Falcon", "Captain", "235_CaptainMainMotion.c"),
    ("Ness", "Ness", "238_NessMainMotion.c"),
    ("Yoshi", "Yoshi", "246_YoshiMainMotion.c"),
    ("Kirby", "Kirby", "228_KirbyMainMotion.c"),
    ("Fox", "Fox", "208_FoxMainMotion.c"),
    ("Pikachu", "Pikachu", "242_PikachuMainMotion.c"),
    ("Jigglypuff", "Purin", "232_PurinMainMotion.c"),
]

TITLES = {"metaknight": "Meta Knight", "mrgaw": "Mr. Game & Watch", "piano": "Mad Piano",
          "dedede": "King Dedede", "lanky": "Lanky Kong", "crash": "Crash Bandicoot"}

ELEMENTS = {0: "normal", 1: "fire", 2: "electric", 3: "slash", 4: "coin",
            5: "freezing", 6: "sleep"}

# Friendly names and order for the motions a table shows.
MOVE_LABELS = [
    ("Attack11", "Jab 1"), ("Attack12", "Jab 2"), ("Attack13", "Jab 3"),
    ("Attack100Start", "Rapid jab start"), ("Attack100Loop", "Rapid jab"),
    ("Attack100End", "Rapid jab end"), ("AttackDash", "Dash attack"),
    ("AttackS3Hi", "Forward tilt (up)"), ("AttackS3HiS", "Forward tilt (up-mid)"),
    ("AttackS3", "Forward tilt"), ("AttackS3LwS", "Forward tilt (down-mid)"),
    ("AttackS3Lw", "Forward tilt (down)"), ("AttackHi3F", "Up tilt (forward)"),
    ("AttackHi3", "Up tilt"), ("AttackHi3B", "Up tilt (back)"),
    ("AttackLw3", "Down tilt"), ("AttackS4Hi", "Forward smash (up)"),
    ("AttackS4HiS", "Forward smash (up-mid)"), ("AttackS4", "Forward smash"),
    ("AttackS4LwS", "Forward smash (down-mid)"), ("AttackS4Lw", "Forward smash (down)"),
    ("AttackHi4", "Up smash"), ("AttackLw4", "Down smash"),
    ("AttackAirN", "Neutral air"), ("AttackAirF", "Forward air"),
    ("AttackAirB", "Back air"), ("AttackAirHi", "Up air"), ("AttackAirLw", "Down air"),
    ("Catch", "Grab"), ("ThrowF", "Forward throw"), ("ThrowB", "Back throw"),
]
MOVE_ORDER = {name: i for i, (name, _) in enumerate(MOVE_LABELS)}
MOVE_LABEL = dict(MOVE_LABELS)
TAIL_LABELS = {
    "DownAttackD": "Getup attack (face down)", "DownAttackU": "Getup attack (face up)",
    "CliffAttackQuick": "Ledge attack (<100%)", "CliffAttackSlow": "Ledge attack (100%+)",
}


# Motions that are not the fighter's own attacks: item swings and throws
# (the items' values), entries, taunts, the thrown statuses.
SKIP_MOTIONS = re.compile(r"^(BatSwing|HarisenSwing|StarRodSwing|SwordSwing|Hammer|ItemSwing|"
                          r"ItemThrow|LightThrow|HeavyThrow|Thrown|LGunShoot|FireFlowerShoot|"
                          r"Appear|Entry|Appeal|Taunt|Damage|Capture|Rebirth|Win|Lose)")
SPECIAL_KINDS = {"N": "Neutral special", "S": "Side special", "Hi": "Up special",
                 "Lw": "Down special", "F": "Side special"}
REMIX_SPECIALS = {"NSP": "Neutral special", "USP": "Up special", "DSP": "Down special",
                  "SSP": "Side special", "FSP": "Side special"}


def move_label(name: str) -> str:
    if name in MOVE_LABEL:
        return MOVE_LABEL[name]
    if name in TAIL_LABELS:
        return TAIL_LABELS[name]
    m = re.match(r"^(Copy\w+?)?Special(Air)?(N|S|Hi|Lw|F)(.*)$", name)
    if m:
        label = SPECIAL_KINDS[m.group(3)] + (" (air)" if m.group(2) else "")
        tail = m.group(4).strip(" _")
        if m.group(1):
            label = f"{m.group(1)} {label}"
        return f"{label}: {tail}" if tail else label
    for key, label in REMIX_SPECIALS.items():
        if name.upper().startswith(key):
            tail = name[len(key):].strip(" _")
            return f"{label}: {tail}" if tail else label
    m = re.match(r"^CliffAttack(Quick|Slow)\d?$", name)
    if m:
        return TAIL_LABELS["CliffAttack" + m.group(1)]
    return name


def move_rank(name: str) -> tuple:
    if name in MOVE_ORDER:
        return (0, MOVE_ORDER[name], name)
    if name.startswith(("CliffAttack", "DownAttack")):
        return (2, 0, name)
    return (1, 0, move_label(name))


# --------------------------------------------------------------------------
# Hitbox timelines (the motion script's clock).
# --------------------------------------------------------------------------

class Timeline:
    def __init__(self):
        self.open: dict[int, dict] = {}
        self.boxes: list[dict] = []
        self.throw: dict | None = None
        self.speed: list[tuple[int, float]] = []

    def make(self, t: int, box: dict):
        self.close(box["id"], t)
        box = dict(box, start=t, end=None)
        self.open[box["id"]] = box
        self.boxes.append(box)

    def close(self, aid: int, t: int):
        box = self.open.pop(aid, None)
        if box is not None:
            box["end"] = t
            if box["end"] <= box["start"]:
                self.boxes.remove(box)

    def close_all(self, t: int):
        for aid in list(self.open):
            self.close(aid, t)

    def modify(self, aid: int, **kw):
        if aid in self.open:
            self.open[aid].update(kw)


def summarize(tl: Timeline) -> list[dict]:
    """One row per distinct hit (damage, angle, knockback, element), its
    frames (1-based, inclusive) and its boxes' sizes."""
    rows: dict[tuple, dict] = {}
    for b in tl.boxes:
        key = (b["damage"], b["angle"], b["kbg"], b["wkb"], b["bkb"], b["element"],
               b.get("shield", 0))
        r = rows.setdefault(key, {"damage": b["damage"], "angle": b["angle"],
                                  "kbg": b["kbg"], "wkb": b["wkb"], "bkb": b["bkb"],
                                  "element": ELEMENTS.get(b["element"], str(b["element"])),
                                  "shield": b.get("shield", 0), "spans": [],
                                  "size_min": b["size"], "size_max": b["size"],
                                  "boxes": 0})
        r["spans"].append((b["start"] + 1, b["end"]))
        r["size_min"] = min(r["size_min"], b["size"])
        r["size_max"] = max(r["size_max"], b["size"])
        r["boxes"] += 1
    out = []
    for r in rows.values():
        spans = sorted(set(r["spans"]), key=lambda s: (s[0], -1 if s[1] is None else s[1]))
        merged: list[list] = []
        for s, e in spans:
            if merged and merged[-1][1] is not None and s <= merged[-1][1] + 1:
                if e is None or merged[-1][1] < e:
                    merged[-1][1] = e
            elif merged and merged[-1][1] is None:
                continue
            else:
                merged.append([s, e])
        r["frames"] = ", ".join(f"{s}-{e}" if e is not None else f"{s}-end"
                                for s, e in merged)
        r["first"] = merged[0][0] if merged else None
        del r["spans"]
        out.append(r)
    out.sort(key=lambda r: (r["first"] or 0, -r["damage"]))
    return out


def box_from_words(w: list[int]) -> dict:
    """MakeAttackColl's five words (ft/ftdef.h ftMotionCommandMakeAttackColl)."""
    def s16(v):
        return v - 0x10000 if v & 0x8000 else v
    return {
        "id": (w[0] >> 23) & 7, "joint": (w[0] >> 13) & 0x7F,
        "damage": (w[0] >> 5) & 0xFF, "element": w[0] & 0xF,
        "size": (w[1] >> 16) & 0xFFFF,
        "angle": (w[3] >> 22) & 0x3FF, "kbg": (w[3] >> 12) & 0x3FF,
        "wkb": (w[3] >> 2) & 0x3FF, "shield": (w[4] >> 24) & 0xFF,
        "bkb": (w[4] >> 7) & 0x3FF,
    }


# --------------------------------------------------------------------------
# The originals: BattleShip's C sources.
# --------------------------------------------------------------------------

def preprocess(text: str) -> str:
    out = []
    stack = []  # (keeping, taken)
    keeping = True
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("#if"):
            if "REGION_US" in s and "!" not in s:
                cond = True
            elif "REGION_JP" in s or "REGION_PAL" in s or "REGION_CN" in s:
                cond = "!" in s
            else:
                cond = True
            stack.append((keeping, cond))
            keeping = keeping and cond
            continue
        if s.startswith("#else") and stack:
            parent, cond = stack[-1]
            keeping = parent and not cond
            continue
        if s.startswith("#elif") and stack:
            parent, cond = stack[-1]
            keeping = False
            continue
        if s.startswith("#endif") and stack:
            keeping = stack.pop()[0]
            continue
        if keeping:
            out.append(line)
    return "\n".join(out)


def split_args(s: str) -> list[str]:
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def as_int(s: str):
    s = s.strip()
    m = re.fullmatch(r"\(\s*\w+\s*\)\s*(.+)", s)
    if m:
        s = m.group(1)
    try:
        return int(s, 0)
    except ValueError:
        return s


def parse_motion_source(path: Path) -> tuple[dict, dict]:
    text = preprocess(path.read_text(encoding="utf-8", errors="replace"))
    scripts: dict[str, list] = {}
    throws: dict[str, list] = {}
    for m in re.finditer(r"(?:ftMotionCommand|u32)\s+(\w+)\[\]\s*=\s*\{(.*?)\n\};", text, re.S):
        cmds = []
        for c in re.finditer(r"^\s*(?:(ftMotion\w+)\s*\((.*?)\)|(0x[0-9A-Fa-f]+))\s*,?\s*$",
                             m.group(2), re.M):
            if c.group(3):
                # A raw word: only the clock's two opcodes matter here.
                w = int(c.group(3), 16)
                if (w >> 26) == 1:
                    cmds.append(("ftMotionCommandWait", [w & 0x3FFFFFF]))
                elif (w >> 26) == 2:
                    cmds.append(("ftMotionCommandWaitAsync", [w & 0x3FFFFFF]))
                continue
            cmds.append((c.group(1), [as_int(a) for a in split_args(c.group(2))]))
        scripts[m.group(1)] = cmds
    for m in re.finditer(r"FTThrowHitDesc\s+(\w+)\[\]\s*=\s*\{(.*?)\n\};", text, re.S):
        rows = []
        for r in re.finditer(r"\{([^{}]*)\}", m.group(2)):
            rows.append([as_int(v) for v in split_args(r.group(1))])
        throws[m.group(1)] = rows
    return scripts, throws


def run_c_script(scripts, throws, sym, tl: Timeline, t: int = 0, depth: int = 0,
                 visited: set | None = None) -> int:
    visited = visited if visited is not None else set()
    if depth > 8 or sym not in scripts:
        return t
    cmds = scripts[sym]
    loops: list[tuple[int, int]] = []
    i = 0
    steps = 0
    while i < len(cmds) and steps < 2000:
        steps += 1
        name, a = cmds[i]
        n = name.replace("ftMotionCommand", "").replace("ftMotion", "")
        if n == "Wait":
            t += a[0]
        elif n == "WaitAsync":
            t = max(t, a[0])
        elif n in ("MakeAttackColl", "MakeAttackCollScaled"):
            tl.make(t, {"id": a[0], "joint": a[2], "damage": a[3], "element": a[5],
                        "size": a[6], "angle": a[10], "kbg": a[11], "wkb": a[12],
                        "shield": a[14], "bkb": a[17]})
        elif n == "ClearAttackColl":
            tl.close(a[0], t)
        elif n == "ClearAttackCollAll":
            tl.close_all(t)
        elif n == "SetAttackCollDamage":
            tl.modify(a[0], damage=a[1])
        elif n == "SetAttackCollSize":
            tl.modify(a[0], size=a[1])
        elif n == "Subroutine":
            t = run_c_script(scripts, throws, str(a[0]), tl, t, depth + 1, visited)
        elif n == "Return":
            return t
        elif n == "Goto":
            target = str(a[0])
            if target in visited or target == sym:
                return t
            visited.add(target)
            return run_c_script(scripts, throws, target, tl, t, depth + 1, visited)
        elif n == "SetParallelScript":
            run_c_script(scripts, throws, str(a[0]), tl, t, depth + 1, set(visited))
        elif n == "LoopBegin":
            loops.append((i, a[0]))
        elif n == "LoopEnd" and loops:
            start, count = loops[-1]
            if count > 1:
                loops[-1] = (start, count - 1)
                i = start
            else:
                loops.pop()
        elif n == "End":
            break
        elif n == "SetThrow":
            rows = throws.get(str(a[0]))
            if rows:
                tl.throw = {"damage": rows[0][1], "angle": rows[0][2], "kbg": rows[0][3],
                            "wkb": rows[0][4], "bkb": rows[0][5],
                            "element": ELEMENTS.get(rows[0][6], str(rows[0][6]))}
        i += 1
    return t


def parse_enum(text: str, enum_name: str, known: dict) -> dict:
    m = re.search(r"typedef\s+enum\s+" + enum_name + r"\s*\{(.*?)\}\s*" + enum_name, text, re.S)
    if not m:
        return {}
    body = re.sub(r"//[^\n]*", "", m.group(1))
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    values = {}
    value = -1
    for item in body.split(","):
        item = item.strip()
        if not item:
            continue
        if "=" in item:
            name, expr = (x.strip() for x in item.split("=", 1))
            expr = expr.strip()
            if expr in values:
                value = values[expr]
            elif expr in known:
                value = known[expr]
            else:
                try:
                    value = int(expr, 0)
                except ValueError:
                    value += 1
        else:
            name = item
            value += 1
        values[name] = value
    return values


def originals() -> list[dict]:
    ftdef = (DECOMP / "ft" / "ftdef.h").read_text(encoding="utf-8", errors="replace")
    common = parse_enum(ftdef, "FTCommonMotion", {})
    ftdata = preprocess((DECOMP / "ft" / "ftdata.c").read_text(encoding="utf-8", errors="replace"))
    fighters = []
    for title, name, src in ORIGINALS:
        scripts, throws = parse_motion_source(DECOMP / "relocData" / src)
        m = re.search(r"FTMotionDesc\s+dFT" + name + r"MotionDescs\[\]\s*=\s*\{(.*?)\n\};", ftdata, re.S)
        # Three values a desc. Some entries are written without their braces
        # (brace elision), so the values are read flat and grouped by three;
        # reading braced groups only put every later move two rows early.
        flat = [x.strip() for x in split_args(re.sub(r"[{}]", " ", m.group(1))) if x.strip()]
        descs = [flat[i:i + 3] for i in range(0, len(flat) - 2, 3)]
        header = None
        for h in (DECOMP / "ft" / "ftchar").glob(f"ft{name.lower()}/ft{name.lower()}.h"):
            header = h.read_text(encoding="utf-8", errors="replace")
        special = parse_enum(header or "", f"ft{name}Motion", common) if header else {}
        names = {v: k.replace("nFTCommonMotion", "") for k, v in common.items()
                 if not k.endswith(("Start", "End")) or k.endswith(("Attack100Start", "Attack100End"))}
        names.update({v: k.replace(f"nFT{name}Motion", "") for k, v in special.items()})
        moves = []
        for index, d in enumerate(descs):
            script = d[1].strip() if len(d) > 1 else ""
            if script.startswith("0x8") or script == "0" or script not in scripts:
                continue
            tl = Timeline()
            run_c_script(scripts, throws, script, tl)
            hits = summarize(tl)
            if not hits and tl.throw is None:
                continue
            motion = names.get(index, f"Motion{index}")
            if SKIP_MOTIONS.match(motion):
                continue
            moves.append({"motion": motion, "motion_id": index, "label": move_label(motion),
                          "hits": hits, "throw": tl.throw})
        moves.sort(key=lambda mv: move_rank(mv["motion"]))
        fighters.append({"name": title, "group": "Original", "notes": "", "moves": moves})
    return fighters


# --------------------------------------------------------------------------
# Remix / EXTRA contents: the P4 export over the staging ROM.
# --------------------------------------------------------------------------

def action_names(remix: str) -> dict[int, str]:
    names = {}
    action = (STAGING / "src" / "Action.asm").read_text(encoding="utf-8", errors="replace")
    for m in re.finditer(r"constant\s+(\w+)\(0x([0-9A-Fa-f]+)\)", action):
        names.setdefault(int(m.group(2), 16), m.group(1))
    title = remix.title().replace("_", "")
    for path in list((STAGING / "src").glob(f"{title}/*.asm")) + \
            list((STAGING / "extra_characters").glob("*/main.asm")):
        if path.parent.name.lower() != remix.lower() and path.parent.name.lower() != title.lower():
            continue
        for m in re.finditer(r"^\s*string_0x([0-9A-Fa-f]+):;\s*String\.insert\(\"([^\"]+)\"\)",
                             path.read_text(encoding="utf-8", errors="replace"), re.M):
            names[int(m.group(1), 16)] = m.group(2)
    return names


COMMON_ACTION_MOTION = {
    # Remix's common action names -> the table's motion names.
    "Jab1": "Attack11", "Jab2": "Attack12", "Jab3": "Attack13", "DashAttack": "AttackDash",
    "FTiltHigh": "AttackS3Hi", "FTiltMidHigh": "AttackS3HiS", "FTilt": "AttackS3",
    "FTiltMidLow": "AttackS3LwS", "FTiltLow": "AttackS3Lw", "UTilt": "AttackHi3",
    "DTilt": "AttackLw3", "FSmashHigh": "AttackS4Hi", "FSmashMidHigh": "AttackS4HiS",
    "FSmash": "AttackS4", "FSmashMidLow": "AttackS4LwS", "FSmashLow": "AttackS4Lw",
    "USmash": "AttackHi4", "DSmash": "AttackLw4", "AttackAirN": "AttackAirN",
    "AttackAirF": "AttackAirF", "AttackAirB": "AttackAirB", "AttackAirU": "AttackAirHi",
    "AttackAirD": "AttackAirLw", "Grab": "Catch", "ThrowF": "ThrowF", "ThrowB": "ThrowB",
}


class StagingRom:
    def __init__(self):
        import remix_rom as R
        self.R = R
        self.rom = R.Rom(STAGING / "ssb64asm.z64")
        self._files: dict[int, bytes] = {}

    def words(self, at: str, n: int) -> list[int]:
        kind, *rest = at.split(":")
        if kind == "file":
            fid, off = int(rest[0], 16), int(rest[1], 16)
            data = self._files.get(fid)
            if data is None:
                data = self._files[fid] = self.rom.file_bytes(fid)
            return [int.from_bytes(data[off + 4 * k:off + 4 * k + 4], "big") for k in range(n)]
        addr = int(rest[0], 16)
        return [self.rom.u32_ram(addr + 4 * k) for k in range(n)]


def run_events(blocks: dict, rom: StagingRom, data_refs: dict, root: str, tl: Timeline,
               t: int = 0, depth: int = 0, visited: set | None = None,
               size_scale: float = 1.0) -> int:
    visited = visited if visited is not None else set()
    if depth > 8 or root not in blocks:
        return t
    block = blocks[root]
    edges = {"call": [], "goto": [], "parallel": []}
    for e in block.get("edges", ()):
        if isinstance(e, dict) and e.get("kind") in edges:
            edges[e["kind"]].append(e["to"])
    loops: list[tuple[int, int]] = []
    cmds = block["commands"]
    i = 0
    steps = 0
    while i < len(cmds) and steps < 2000:
        steps += 1
        c = cmds[i]
        op = c["op"]
        w0 = c["words"][0]
        if op == "SyncWait":
            t += w0 & 0x3FFFFFF
        elif op == "AsyncWait":
            t = max(t, w0 & 0x3FFFFFF)
        elif op in ("MakeAttackColl", "MakeAttackCollScaled"):
            box = box_from_words(rom.words(c["at"], 5))
            box["size"] = int(round(box["size"] * size_scale))
            tl.make(t, box)
        elif op == "ClearAttackColl":
            tl.close((w0 >> 23) & 7, t)
        elif op == "ClearAttackCollAll":
            tl.close_all(t)
        elif op == "SetAttackCollDamage":
            tl.modify((w0 >> 23) & 7, damage=w0 & 0xFF)
        elif op == "SetAttackCollSize":
            tl.modify((w0 >> 23) & 7, size=int(round((w0 & 0xFFFF) * size_scale)))
        elif op == "Subroutine" and edges["call"]:
            t = run_events(blocks, rom, data_refs, edges["call"].pop(0), tl, t, depth + 1,
                           visited, size_scale)
        elif op == "Return":
            return t
        elif op in ("Goto", "RemixGotoMovesetFile"):
            if edges["goto"]:
                target = edges["goto"].pop(0)
                if target not in visited and target != root:
                    visited.add(target)
                    return run_events(blocks, rom, data_refs, target, tl, t, depth + 1,
                                      visited, size_scale)
            return t
        elif op == "SetParallelScript" and edges["parallel"]:
            run_events(blocks, rom, data_refs, edges["parallel"].pop(0), tl, t, depth + 1,
                       set(visited), size_scale)
        elif op == "LoopBegin":
            loops.append((i, w0 & 0x3FFFFFF))
        elif op == "LoopEnd" and loops:
            start, count = loops[-1]
            if count > 1:
                loops[-1] = (start, count - 1)
                i = start
            else:
                loops.pop()
        elif op == "End":
            break
        elif op == "SetThrow":
            target = data_refs.get(c["at"])
            if target is None:
                # The pointer word itself: a file's internal reloc (target
                # word index in its low half) or a RAM address.
                ptr = rom.words(c["at"], 2)[1]
                kind, *rest = c["at"].split(":")
                if kind == "file":
                    target = f"file:{rest[0]}:{(ptr & 0xFFFF) * 4:#x}"
                elif ptr & 0x80000000:
                    target = f"ram:{ptr:#x}"
            if target is not None:
                w = rom.words(target, 7)
                sw = [v - (1 << 32) if v & 0x80000000 else v for v in w]
                tl.throw = {"damage": sw[1], "angle": sw[2], "kbg": sw[3], "wkb": sw[4],
                            "bkb": sw[5], "element": ELEMENTS.get(sw[6], str(sw[6]))}
        elif op == "RemixFrameSpeedMultiplier":
            import struct
            speed = struct.unpack(">f", ((w0 & 0xFFFF) << 16).to_bytes(4, "big"))[0]
            tl.speed.append((t, round(speed, 3)))
        i += 1
    return t


def next_loc(at: str, n: int) -> str:
    kind, *rest = at.split(":")
    if kind == "file":
        return f"file:{rest[0]}:{int(rest[1], 16) + n:#x}"
    return f"ram:{int(rest[0], 16) + n:#x}"


def contents(rom: StagingRom) -> list[dict]:
    import p4_contents
    import generate_p4_fighter as G
    fighters = []
    by_name = {}
    for row in p4_contents.load_registry():
        if "base" in row:
            continue
        name = row["name"]
        export = EXPORT / name
        resolved = json.loads((export / "resolved.json").read_text(encoding="utf-8"))
        events = json.loads((export / "events.json").read_text(encoding="utf-8"))
        blocks = events["blocks"]
        # SetThrow's data: the command's second word is the pointer.
        refs = {}
        for r in events.get("data_refs", ()):
            if r.get("kind") == "throw_data":
                refs[r["from"]] = r["to"]
        set_throw_refs = {}
        for b in blocks.values():
            for c in b["commands"]:
                if c["op"] == "SetThrow":
                    tgt = refs.get(next_loc(c["at"], 4))
                    if tgt is not None:
                        set_throw_refs[c["at"]] = tgt
        remix = p4_contents.remix_name(row) if hasattr(p4_contents, "remix_name") else row.get("remix", name.upper())
        anames = action_names(remix)
        motion_name = {}
        for st in resolved["statuses"]:
            mid = st.get("motion_id", -1)
            if mid is None or mid < 0:
                continue
            nm = st.get("name")
            if nm:
                nm = re.sub(r"^nFT\w+?Status", "", nm)
            else:
                nm = anames.get(st["status_id"], f"Action{st['status_id']:#x}")
                nm = COMMON_ACTION_MOTION.get(nm, nm)
            motion_name.setdefault(mid, nm)
        scale_rows = G.OWNER_HITBOX_SCALES.get(name, {})
        scaled_motions = set(scale_rows.get("motions", ()))
        moves = []
        for mo in resolved["motions"]:
            root = mo.get("script_root")
            if not root:
                continue
            tl = Timeline()
            scale = scale_rows.get("scale", 1.0) if mo["index"] in scaled_motions else 1.0
            run_events(blocks, rom, set_throw_refs, root, tl, size_scale=scale)
            hits = summarize(tl)
            if not hits and tl.throw is None:
                continue
            motion = motion_name.get(mo["index"], f"Motion{mo['index']}")
            if SKIP_MOTIONS.match(motion):
                continue
            entry = {"motion": motion, "motion_id": mo["index"], "label": move_label(motion),
                     "hits": hits, "throw": tl.throw}
            if tl.speed:
                entry["speed"] = tl.speed
            moves.append(entry)
        moves.sort(key=lambda mv: move_rank(mv["motion"]))
        notes = []
        if name in G.OWNER_ANIM_RATES:
            notes.append(f"animations at {G.OWNER_ANIM_RATES[name]}x")
        if scaled_motions:
            notes.append(f"boxes {scale_rows.get('scale')}x in motions {sorted(scaled_motions)}")
        fighter = {"name": TITLES.get(name, row.get("title", name.title())), "group": "Remix/EXTRA",
                   "notes": "; ".join(notes), "moves": moves, "key": name}
        fighters.append(fighter)
        by_name[name] = fighter
    return fighters, by_name


# --------------------------------------------------------------------------
# Authored contents: their .ms scripts over spec.json, the rest their base's.
# --------------------------------------------------------------------------

def run_ms(path: Path, tl: Timeline):
    import mscript
    text = path.read_text(encoding="utf-8")
    t = 0
    loops = []
    lines = [ln.strip() for ln in text.splitlines() if ln.strip() and not ln.strip().startswith("#")]
    i = 0
    while i < len(lines):
        parts = lines[i].split()
        op = parts[0]
        kv = {}
        for p in parts[1:]:
            if "=" in p:
                k, v = p.split("=", 1)
                try:
                    kv[k] = int(v, 0)
                except ValueError:
                    kv[k] = v
        if op == "SyncWait":
            t += kv.get("frames", 0)
        elif op == "AsyncWait":
            t = max(t, kv.get("frames", 0))
        elif op in ("MakeAttackColl", "MakeAttackCollScaled"):
            tl.make(t, {"id": kv.get("attack_id", 0), "joint": kv.get("joint_id", 0),
                        "damage": kv.get("damage", 0), "element": kv.get("element", 0),
                        "size": kv.get("size", 0), "angle": kv.get("angle", 0),
                        "kbg": kv.get("knockback_scale", 0), "wkb": kv.get("knockback_weight", 0),
                        "shield": kv.get("shield_damage", 0), "bkb": kv.get("knockback_base", 0)})
        elif op == "ClearAttackColl":
            tl.close(kv.get("attack_id", 0), t)
        elif op == "ClearAttackCollAll":
            tl.close_all(t)
        elif op == "LoopBegin":
            loops.append((i, kv.get("count", 1)))
        elif op == "LoopEnd" and loops:
            start, count = loops[-1]
            if count > 1:
                loops[-1] = (start, count - 1)
                i = start
            else:
                loops.pop()
        elif op == "End":
            break
        i += 1


def code_hits(name: str) -> list[dict]:
    """Hits a content's own code makes (no motion script names them), read
    from its source's constants."""
    if name != "palutena":
        return []
    src = (REPO / "src" / "port" / "nds_p4_palutena.c").read_text(encoding="utf-8")
    k = {m.group(1): float(m.group(2)) for m in
         re.finditer(r"#define (PAL_FLAME_\w+) (-?[0-9.]+)F?\b", src)}
    burn = {"damage": int(k["PAL_FLAME_BURN_DAMAGE"]), "angle": 0, "kbg": 100,
            "wkb": int(k["PAL_FLAME_BURN_KNOCKBACK_WEIGHT"]), "bkb": 0, "element": "fire",
            "shield": 0, "size_min": int(k["PAL_FLAME_BURN_SIZE"]),
            "size_max": int(k["PAL_FLAME_BURN_SIZE"]), "boxes": 1,
            "frames": f"fire {int(k['PAL_FLAME_BURN_AT'])}-{int(k['PAL_FLAME_BLAST_AT']) - 1}, "
                      f"every {int(k['PAL_FLAME_BURN_EVERY'])}"}
    blast = {"damage": int(k["PAL_FLAME_BLAST_DAMAGE"]), "angle": int(k["PAL_FLAME_BLAST_ANGLE"]),
             "kbg": int(k["PAL_FLAME_BLAST_KNOCKBACK_SCALE"]), "wkb": 0,
             "bkb": int(k["PAL_FLAME_BLAST_KNOCKBACK_BASE"]), "element": "fire", "shield": 0,
             "size_min": int(k["PAL_FLAME_BLAST_SIZE"]), "size_max": int(k["PAL_FLAME_BLAST_SIZE"]),
             "boxes": 1,
             "frames": f"fire {int(k['PAL_FLAME_BLAST_AT'])}-"
                       f"{int(k['PAL_FLAME_BLAST_AT'] + k['PAL_FLAME_BLAST_TICS']) - 1}"}
    return [{"motion": "SpecialS", "motion_id": -1, "label": "Side special: Explosive Flame",
             "hits": [burn, blast], "throw": None,
             "note": "a weapon of its own (nds_p4_palutena.c): frames from the fire's making"}]


def code_notes(name: str, moves: list[dict]) -> None:
    """Notes for moves whose boxes a content's code moves: the script's
    numbers alone understate them. Offsets are as the script writes them;
    the game scales them by the fighter's model size (attr->size)."""
    if name != "palutena":
        return
    src = (REPO / "src" / "port" / "nds_p4_palutena.c").read_text(encoding="utf-8")
    m = re.search(r"sNdsP4PalutenaPillarColumn\[2\]\[2\] = \{\s*\{ ([0-9.]+)F, ([0-9.]+)F \},"
                  r".*?\{ ([0-9.]+)F, ([0-9.]+)F \}", src, re.S)
    if m is None:
        return
    lo, hi = float(m.group(1)), float(m.group(4))
    for mv in moves:
        if mv["motion"] == "AttackHi4" and not mv.get("inherited") and mv["hits"]:
            r = max(h["size_max"] for h in mv["hits"]) / 2
            mv["note"] = (f"her code sweeps the boxes up and down a column every frame "
                          f"(nds_p4_palutena.c): {lo:g} to {hi:g} above her floor in script units "
                          f"plus the {r:g} radius, x her model size (1.185 in game: about 2,900 tall, "
                          f"{2 * r:g} wide)")


def authored(by_name: dict) -> list[dict]:
    import p4_contents
    fighters = []
    for row in p4_contents.load_registry():
        if "base" not in row:
            continue
        name = row["name"]
        spec = json.loads((REPO / "scripts" / "p6" / name / "spec.json").read_text(encoding="utf-8"))
        base = by_name.get(row["base"])
        base_moves = {mv["motion_id"]: mv for mv in (base["moves"] if base else [])}
        moves = []
        seen = set()
        for key, m in spec["motions"].items():
            index = int(key)
            motion = m.get("comment", "").split(":")[0].strip() or f"Motion{index}"
            if SKIP_MOTIONS.match(motion):
                seen.add(index)
                continue
            if "script" in m:
                tl = Timeline()
                run_ms(REPO / m["script"], tl)
                hits = summarize(tl)
                seen.add(index)
                if hits:
                    moves.append({"motion": motion, "motion_id": index, "label": move_label(motion),
                                  "hits": hits, "throw": None})
        for index, mv in base_moves.items():
            if index not in seen and str(index) not in spec["motions"]:
                moves.append(dict(mv, inherited=row["base"]))
            elif index not in seen and "script" not in spec["motions"].get(str(index), {}):
                moves.append(dict(mv, inherited=row["base"]))
        moves += code_hits(name)
        code_notes(name, moves)
        moves.sort(key=lambda mv: move_rank(mv["motion"]))
        fighters.append({"name": TITLES.get(name, row.get("title", name.title())), "group": "Authored",
                         "notes": f"scripts of her/his own; the rest {row['base']}'s",
                         "moves": moves, "key": name})
    return fighters


def write_html(data: list[dict], path: Path) -> None:
    page = (Path(__file__).resolve().parent / "balance_page.html").read_text(encoding="utf-8")
    slim = []
    for f in data:
        moves = []
        for mv in f["moves"]:
            m = {"l": mv["label"], "m": mv["motion"],
                 "h": [[h["frames"], h["first"] if "first" in h else None, h["damage"], h["angle"],
                        h["kbg"], h["bkb"], h["wkb"], h["size_max"], h["element"]]
                       for h in mv["hits"]]}
            if mv.get("throw"):
                t = mv["throw"]
                m["t"] = [t["damage"], t["angle"], t["kbg"], t["bkb"], t["wkb"], t["element"]]
            if mv.get("inherited"):
                m["i"] = mv["inherited"]
            if mv.get("note"):
                m["n"] = mv["note"]
            if mv.get("speed"):
                m["s"] = mv["speed"]
            moves.append(m)
        slim.append({"n": f["name"], "g": f["group"], "x": f.get("notes", ""), "mv": moves})
    path.write_text(page.replace("<!--__STATIC__-->", static_html(slim)), encoding="utf-8")


COMPARE_KEYS = [
    ("Attack11", "Jab 1"), ("Attack12", "Jab 2"), ("Attack13", "Jab 3"), ("Attack100Loop", "Rapid jab"),
    ("AttackDash", "Dash attack"), ("AttackS3", "Forward tilt"), ("AttackHi3", "Up tilt"),
    ("AttackLw3", "Down tilt"), ("AttackS4", "Forward smash"), ("AttackHi4", "Up smash"),
    ("AttackLw4", "Down smash"), ("AttackAirN", "Neutral air"), ("AttackAirF", "Forward air"),
    ("AttackAirB", "Back air"), ("AttackAirHi", "Up air"), ("AttackAirLw", "Down air"),
    ("Catch", "Grab"), ("ThrowF", "Forward throw"), ("ThrowB", "Back throw"),
]

# Knockback is compared at one reference: a standard-weight target (weight 1.0)
# at 100% once the hit lands, the game's own formula (ftParamGetCommonKnockback,
# ft/ftparam.c) with no handicap and the damage ratio at 100.
REF_PERCENT = 100
# A move is flagged when its value sits this many robust standard deviations
# from the roster's median for the same move (median absolute deviation).
OUTLIER_Z = 2.5
METRICS = (("dmg", "Total damage"), ("kb", "KB at 100%"), ("kbg", "KBG"), ("bkb", "BKB"),
           ("start", "Startup"), ("active", "Active frames"))
SCORED = ("dmg", "kb", "start")


def kb_at(d, kbg, bkb, wkb) -> float:
    if wkb:
        base = (1.0 + 10.0 * wkb * 0.05) * 1.4 + 18.0
    else:
        base = (REF_PERCENT * 0.1 + REF_PERCENT * d * 0.05) * 1.4 + 18.0
    return min(2500.0, base * kbg * 0.01 + bkb)


def move_damage(hits: list, key: str) -> int:
    """The move's damage when every hit lands. A box out in two or more
    separate windows with the same values is a multi-hit (the script refreshes
    it), so it counts once a window; the strongest single-window box (a clean
    hit, or a multi-hit's finisher) adds once. Clean and late boxes are single
    windows and count as the stronger one; a rapid jab loops, so it counts its
    strongest hit."""
    if key == "Attack100Loop":
        return max(h[2] for h in hits)
    multi: dict[str, tuple] = {}
    single = 0
    for h in hits:
        spans = [p for p in str(h[0]).split(",") if p.strip()]
        if len(spans) >= 2:
            d, _n = multi.get(str(h[0]), (0, 0))
            multi[str(h[0])] = (max(d, h[2]), len(spans))
        else:
            single = max(single, h[2])
    return sum(d * n for d, n in multi.values()) + single


def active_frames(hits: list):
    """Frames any box of the move is out (its 1-based spans unioned); None when
    a box stays out to the move's end (then its length is the motion's)."""
    out = set()
    for h in hits:
        for part in str(h[0]).split(","):
            lo, _, hi = part.strip().partition("-")
            if not lo.isdigit():
                continue
            if not hi.isdigit():
                return None
            out.update(range(int(lo), int(hi) + 1))
    return len(out) or None


def move_metrics(mv: dict, key: str):
    """The move's comparable numbers: its strongest damage, its strongest
    knockback at the reference, its first active frame, and the row shown."""
    if key.startswith("Throw") and mv.get("t"):
        t = mv["t"]
        return {"dmg": t[0], "kb": round(kb_at(t[0], t[2], t[3], t[4])), "start": None,
                "kbg": t[2], "bkb": t[3],
                "row": ("throw", t[0], t[1], t[2], t[3], t[4], "", t[5]), "hits": len(mv["h"]),
                "active": None}
    if not mv["h"]:
        return None
    best = max(mv["h"], key=lambda h: (h[2], kb_at(h[2], h[4], h[5], h[6])))
    firsts = [h[1] for h in mv["h"] if h[1] is not None]
    # A grab's boxes catch rather than hit, and a move whose every box has 0
    # damage takes its damage from code: neither is compared on damage or power.
    strong = key != "Catch" and max(h[2] for h in mv["h"]) > 0
    return {"dmg": move_damage(mv["h"], key) if strong else None,
            "kb": round(max(kb_at(h[2], h[4], h[5], h[6]) for h in mv["h"])) if strong else None,
            "kbg": best[4] if strong else None, "bkb": best[5] if strong else None,
            "start": min(firsts) if firsts else None,
            "row": (best[0], best[2], best[3], best[4], best[5], best[6], best[7], best[8]),
            "hits": len(mv["h"]), "active": active_frames(mv["h"])}


def _median(v: list) -> float:
    v = sorted(v)
    n = len(v)
    return v[n // 2] if n % 2 else (v[n // 2 - 1] + v[n // 2]) / 2


def _robust_z(x, values: list) -> float:
    med = _median(values)
    mad = _median([abs(y - med) for y in values])
    if mad > 0:
        return 0.6745 * (x - med) / mad
    mean_abs = sum(abs(y - med) for y in values) / len(values)
    return 0.7979 * (x - med) / mean_abs if mean_abs > 0 else 0.0


def analyse(slim: list[dict]) -> tuple[dict, list, dict]:
    """Per-move roster metrics, the outlier list and each fighter's scores.

    A fighter's Damage, Power and Speed scores are the mean percentile of their
    standard moves (the compare list) against every fighter that has the same
    move: 50 is the roster middle, 100 the top of every move. Overall is the
    mean of the three."""
    table: dict[str, list] = {}
    for key, _title in COMPARE_KEYS:
        rows = []
        for fi, f in enumerate(slim):
            mv = next((m for m in f["mv"] if m["m"] == key), None)
            mm = move_metrics(mv, key) if mv is not None else None
            if mm is not None:
                mm.update({"fi": fi, "inh": mv.get("i"), "note": mv.get("n"), "flags": {}})
                rows.append(mm)
        table[key] = rows
    pct: dict[int, dict] = {fi: {m: [] for m, _l in METRICS} for fi in range(len(slim))}
    outliers = []
    titles = dict(COMPARE_KEYS)
    for key, rows in table.items():
        for metric, label in METRICS:
            vals = [r[metric] for r in rows if r[metric] is not None]
            if len(vals) < 5:
                continue
            med = _median(vals)
            for r in rows:
                x = r[metric]
                if x is None:
                    continue
                less = sum(1 for y in vals if y < x)
                same = sum(1 for y in vals if y == x)
                p = (less + 0.5 * same) / len(vals)
                pct[r["fi"]][metric].append(1.0 - p if metric == "start" else p)
                z = _robust_z(x, vals)
                gap = abs(x - med)
                far = gap >= (max(3.0, 0.3 * med) if metric == "start" else
                              max(4.0, 0.5 * med) if metric == "active" else
                              max(10.0, 0.25 * med) if metric in ("kbg", "bkb") else 0.25 * med)
                if abs(z) >= OUTLIER_Z and far:
                    stronger = (z < 0) if metric == "start" else (z > 0)
                    r["flags"][metric] = "hi" if stronger else "lo"
                    outliers.append({"fi": r["fi"], "key": key, "move": titles[key], "metric": label,
                                     "value": x, "median": med, "z": z, "stronger": stronger})
    outliers.sort(key=lambda o: -abs(o["z"]))
    scores = {}
    for fi, p in pct.items():
        s = {k: (round(100 * sum(v) / len(v)) if v else None) for k, v in p.items()}
        parts = [s[k] for k in SCORED if s[k] is not None]
        s["overall"] = round(sum(parts) / len(parts)) if parts else None
        s["moves"] = len(p["dmg"])
        s["flags"] = sum(1 for o in outliers if o["fi"] == fi)
        scores[fi] = s
    return table, outliers, scores


def static_html(slim: list[dict]) -> str:
    """Every table written out whole: the page needs no script to show them."""
    e = html.escape
    table, outliers, scores = analyse(slim)

    def ang(a):
        return "361 (S)" if a == 361 else str(a)

    def el(x):
        return f'<span class="el {e(str(x))}">{e(str(x))}</span>' if x and x != "normal" else ""

    def anchor(name):
        return "f-" + re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-")

    def rad(size):
        """The game's radius: a script's size halved (ft/ftmain.c, size * 0.5F)."""
        return "" if size in ("", None) else f"{size / 2:g}"

    def note(text):
        return f'<span class="note">{e(text)}</span>' if text else ""

    def cols(widths):
        return "<colgroup>" + "".join(f'<col style="width:{w}">' for w in widths) + "</colgroup>"

    def th(label, num=False):
        return f'<th{" class=num" if num else ""}>{label}</th>'

    def flag_text(o, short=False):
        sign = "+" if o["z"] > 0 else "-"
        word = "stronger" if o["stronger"] else "weaker"
        med = f'{o["median"]:g}'
        if short:
            return f'{o["metric"]} {o["value"]} vs {med}'
        return f'{word}: {o["metric"]} {o["value"]} vs roster {med} ({sign}{abs(o["z"]):.1f})'

    flags_by_move: dict[tuple, list] = {}
    for o in outliers:
        flags_by_move.setdefault((o["fi"], o["key"]), []).append(o)

    def flag_cell(fi, key):
        fl = flags_by_move.get((fi, key), [])
        return " ".join(f'<span class="flag {"hi" if o["stronger"] else "lo"}" title="{e(flag_text(o))}">'
                        f'{e(flag_text(o, True))}</span>' for o in fl)

    out = ['<nav class="toc" aria-label="Contents">',
           '<p><b>Start</b> <a href="#scores">Fighter scores</a> <a href="#outliers">Outlier attacks</a></p>']
    for g, label in (("Original", "Original 12"), ("Remix/EXTRA", "Remix &amp; EXTRA"),
                     ("Authored", "Authored")):
        links = " ".join(f'<a href="#{anchor(f["n"])}">{e(f["n"])}</a>' for f in slim if f["g"] == g)
        out.append(f'<p><b>{label}</b> {links}</p>')
    out.append('<p><b>Compare</b> ' + " ".join(
        f'<a href="#m-{k.lower()}">{e(t)}</a>' for k, t in COMPARE_KEYS) + "</p></nav>")

    # Fighter scores, best overall first.
    order = sorted(range(len(slim)), key=lambda fi: (-(scores[fi]["overall"] or 0), slim[fi]["n"]))
    rank = {fi: i + 1 for i, fi in enumerate(order)}

    def bar(v):
        if v is None:
            return '<td class="num">-</td>'
        return f'<td class="num score"><span class="bar" style="width:{v}%"></span><span class="v">{v}</span></td>'

    body = "".join(
        f'<tr><td class="num">{rank[fi]}</td><td class="move"><a href="#{anchor(slim[fi]["n"])}">{e(slim[fi]["n"])}</a></td>'
        f'{bar(scores[fi]["overall"])}{bar(scores[fi]["dmg"])}{bar(scores[fi]["kb"])}{bar(scores[fi]["kbg"])}'
        f'{bar(scores[fi]["bkb"])}{bar(scores[fi]["start"])}'
        f'<td class="num">{scores[fi]["moves"]}</td><td class="num">{scores[fi]["flags"] or ""}</td></tr>'
        for fi in order)
    out.append('<section class="sheet" id="scores"><div class="sheet-head"><h3>Fighter scores</h3>'
               '<span class="meta">standard moves only (the compare list); 50 = the roster middle; '
               'click a heading to sort</span></div><div class="scroll"><table class="sortable fixed" style="min-width:980px">'
               + cols(["56px", "190px", "110px", "110px", "110px", "110px", "110px", "110px", "70px", "auto"])
               + "<thead><tr>" + th("Rank", True) + th("Fighter") + th("Overall", True) + th("Damage", True)
               + th("Power", True) + th("KBG", True) + th("BKB", True) + th("Speed", True) + th("Moves", True) + th("Outliers", True)
               + f"</tr></thead><tbody>{body}</tbody></table></div></section>")

    # Outlier attacks, most extreme first.
    body = "".join(
        f'<tr class="{"hi" if o["stronger"] else "lo"}"><td class="move"><a href="#{anchor(slim[o["fi"]]["n"])}">'
        f'{e(slim[o["fi"]]["n"])}</a></td><td><a href="#m-{o["key"].lower()}">{e(o["move"])}</a></td>'
        f'<td>{e(o["metric"])}</td><td class="num">{o["value"]}</td><td class="num">{o["median"]:g}</td>'
        f'<td class="num">{"+" if o["z"] > 0 else "-"}{abs(o["z"]):.1f}</td>'
        f'<td><span class="flag {"hi" if o["stronger"] else "lo"}">{"stronger" if o["stronger"] else "weaker"}</span></td></tr>'
        for o in outliers)
    out.append('<section class="sheet" id="outliers"><div class="sheet-head"><h3>Outlier attacks</h3>'
               f'<span class="meta">{len(outliers)} values at least {OUTLIER_Z:g} robust deviations and 25% from the roster median '
               'for the same move, most extreme first</span></div><div class="scroll"><table class="sortable fixed" style="min-width:760px">'
               + cols(["190px", "150px", "120px", "90px", "110px", "100px", "auto"])
               + "<thead><tr>" + th("Fighter") + th("Move") + th("Stat") + th("Value", True) + th("Roster median", True)
               + th("Deviation", True) + th("Reads as") + f"</tr></thead><tbody>{body}</tbody></table></div></section>")

    # One sheet per fighter.
    fcols = cols(["190px", "150px", "120px", "64px", "76px", "56px", "56px", "56px", "56px", "72px", "92px", "auto"])
    head = ("<thead><tr>" + th("Move") + th("Script") + th("Frames") + th("Dmg %", True) + th("Angle", True)
            + th("KBG", True) + th("BKB", True) + th("Fixed", True) + th("Radius", True) + th("KB 100%", True)
            + th("Element") + th("Outliers") + "</tr></thead>")
    for fi, f in enumerate(slim):
        rows = []
        for mv in f["mv"]:
            tags = (f'<span class="tag">from {e(mv["i"])}</span>' if mv.get("i") else "") + \
                   (f'<span class="tag" title="{e(mv["n"])}">code</span>' if mv.get("n") else "")
            lines = [(h[0], h[2], h[3], h[4], h[5], h[6], h[7], h[8]) for h in mv["h"]]
            if mv.get("t"):
                t = mv["t"]
                lines.append(("throw", t[0], t[1], t[2], t[3], t[4], "", t[5]))
            flags = flag_cell(fi, mv["m"])
            for k, l in enumerate(lines):
                kb = round(kb_at(l[1], l[3], l[4], l[5]))
                rows.append(f'<tr class="{"first" if k == 0 else "cont"}{" flagged" if flags else ""}">'
                            f'<td class="move">{e(mv["l"])}{tags if k == 0 else ""}</td><td class="sub">{e(mv["m"])}</td>'
                            f'<td class="frames">{e(str(l[0]))}</td><td class="num dmg">{l[1]}</td>'
                            f'<td class="num">{ang(l[2])}</td><td class="num">{l[3]}</td>'
                            f'<td class="num">{l[4]}</td><td class="num">{l[5]}</td>'
                            f'<td class="num">{rad(l[6])}</td><td class="num">{kb}</td><td>{el(l[7])}</td>'
                            f'<td class="flags">{(note(mv.get("n")) + flags) if k == 0 else ""}</td></tr>')
        s = scores[fi]
        group = "Remix &amp; EXTRA" if f["g"] == "Remix/EXTRA" else e(f["g"])
        meta = (f'{group} · {len(f["mv"])} moves · score {s["overall"]} (rank {rank[fi]} of {len(slim)}): '
                f'damage {s["dmg"]}, power {s["kb"]} (KBG {s["kbg"]}, BKB {s["bkb"]}), speed {s["start"]}'
                + (f' · {e(f["x"])}' if f.get("x") else ""))
        out.append(f'<section class="sheet" id="{anchor(f["n"])}"><div class="sheet-head"><h3>{e(f["n"])}</h3>'
                   f'<span class="meta">{meta}</span></div><div class="scroll"><table class="fixed" style="min-width:1060px">'
                   f'{fcols}{head}<tbody>{"".join(rows)}</tbody></table></div></section>')

    # One sheet per standard move across the roster.
    ccols = cols(["190px", "72px", "120px", "60px", "64px", "70px", "76px", "56px", "56px", "56px", "56px", "72px", "92px", "52px", "auto"])
    chead = ("<thead><tr>" + th("Fighter") + th("Startup", True) + th("Frames") + th("Active", True) + th("Dmg %", True)
             + th("Total %", True) + th("Angle", True) + th("KBG", True) + th("BKB", True) + th("Fixed", True)
             + th("Radius", True) + th("KB 100%", True)
             + th("Element") + th("Hits", True) + th("Outliers") + "</tr></thead>")
    for key, title in COMPARE_KEYS:
        rows = sorted(table[key], key=lambda r: (-(r["dmg"] or 0), slim[r["fi"]]["n"]))
        vals = {m: [r[m] for r in rows if r[m] is not None] for m, _ in METRICS}
        med = {m: (_median(v) if v else None) for m, v in vals.items()}

        def cell(r, m, text):
            c = r["flags"].get(m)
            return f'<td class="num{" dmg" if m == "dmg" else ""}{" " + c if c else ""}">{text}</td>'

        body = "".join(
            f'<tr><td class="move"><a href="#{anchor(slim[r["fi"]]["n"])}">{e(slim[r["fi"]]["n"])}</a>'
            f'{f"<span class=tag>from {e(r["inh"])}</span>" if r["inh"] else ""}'
            f'{"<span class=tag>code</span>" if r["note"] else ""}</td>'
            + cell(r, "start", "" if r["start"] is None else r["start"])
            + f'<td class="frames">{e(str(r["row"][0]))}</td>'
            + cell(r, "active", "" if r["active"] is None else r["active"])
            + f'<td class="num dmg">{r["row"][1]}</td>'
            + cell(r, "dmg", "" if r["dmg"] is None else r["dmg"])
            + f'<td class="num">{ang(r["row"][2])}</td>' + cell(r, "kbg", r["row"][3]) + cell(r, "bkb", r["row"][4])
            + f'<td class="num">{r["row"][5]}</td><td class="num">{rad(r["row"][6])}</td>' + cell(r, "kb", "" if r["kb"] is None else r["kb"])
            + f'<td>{el(r["row"][7])}</td><td class="num">{r["hits"]}</td><td class="flags">{note(r["note"])}{flag_cell(r["fi"], key)}</td></tr>'
            for r in rows)
        medtxt = ", ".join(f'{lbl.lower()} {med[m]:g}' for m, lbl in METRICS if med[m] is not None)
        out.append(f'<section class="sheet" id="m-{key.lower()}"><div class="sheet-head"><h3>{e(title)}</h3>'
                   f'<span class="meta">{len(rows)} fighters · strongest hit of each, highest damage first · '
                   f'roster median: {medtxt}</span></div><div class="scroll"><table class="sortable fixed" style="min-width:1230px">'
                   f'{ccols}{chead}<tbody>{body}</tbody></table></div></section>')
    return "\n".join(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--json", type=Path)
    ap.add_argument("--html", type=Path)
    args = ap.parse_args()
    data = originals()
    rom = StagingRom()
    remix, by_name = contents(rom)
    data += remix + authored(by_name)
    if args.json:
        args.json.write_text(json.dumps(data, indent=1), encoding="utf-8")
    if args.html:
        write_html(data, args.html)
    for f in data:
        print(f"{f['name']}: {len(f['moves'])} moves")
    return 0


if __name__ == "__main__":
    sys.exit(main())
