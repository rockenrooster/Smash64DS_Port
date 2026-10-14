#!/usr/bin/env python3
"""P6 authored export: a base content's export plus an authored content's spec.

An authored content (scripts/p4/contents.json `base`, docs/P6/P6_PLAN.md A7)
has no Remix donor to export. Its export directory is written here, in the
layout scripts/p4/remix_export.py writes (resolved.json, events.json,
summary.txt), from its base content's export and its committed spec
(scripts/p6/<name>/spec.json). Every P4 rule after the export then runs on it
unchanged.

The base's data stays the base's: its files, clips, scripts and tables keep
their ids, so the authored content shares them with the base in a match. The
export gains the content's identity (`content`, `base`) and, from the spec,
what the authored content changes. An empty spec, or none, reproduces the base
export but for identity (F4): the A1 null clone.

    p6_author.py --base-export builds/p4-staging/export-x/roy --content ike \\
        --out builds/p4-staging/export-x/ike [--spec scripts/p6/ike/spec.json]

The output derives from the user's ROM through the base export: build output
only, never tracked.
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
sys.path.insert(0, str(HERE.parent / "p4"))
import p4_contents  # noqa: E402

# Spec keys the author applies (A7 adds file substitutions, appended motions
# and statuses, kind-table rows, attribute deltas, sword trails and colour
# animations). Identity keys only describe the spec.
SPEC_IDENTITY = {"schema", "content", "comment"}
SPEC_SCHEMA = "p6-spec-v1"
# Spec keys this author applies.
SPEC_APPLIED = {"files", "motions", "menu_motions", "statuses", "attributes", "kind_tables", "cpu"}
# "files": the content's own files by role: the descriptor slot each
# replaces. Its main and model (model_writer.py); a special file of its own
# (p6_gate.py: Palutena's entry gate in special 2), which only the content's
# own code reads, through its own storage (p4_contents.special_storage).
FILE_ROLES = {"main": 0, "model": 3, "special1": 5, "special2": 6, "special3": 7, "special4": 8}
# New file ids live in the P6 window (P6_PLAN.md A1 record: 0x1A00-0x1BFF),
# 0x100 a content: its main and model first, its special files from 0x10,
# its clips from ANIM_FIRST.
P6_FILE_WINDOW = range(0x1A00, 0x1C00)
CONTENT_WINDOW = {"ike": 0x1A00, "palutena": 0x1B00}
ANIM_FIRST = 0x20
NO_SCRIPT = 0x80000000          # a motion row with no script
# Pointer commands' edges in events.json (remix_export.py's kinds).
EDGE_KIND = {"Goto": "goto", "Subroutine": "call", "SetParallelScript": "parallel",
             "SetDamageThrown": "damage-thrown"}


class AuthorError(SystemExit):
    pass


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_spec(path: Path | None, name: str) -> tuple[dict, str | None]:
    """The content's spec ({} when it has none) and its digest."""
    if path is None or not path.exists():
        return {}, None
    spec = json.loads(path.read_text(encoding="utf-8"))
    if spec.get("schema") != SPEC_SCHEMA:
        raise AuthorError(f"{path}: schema {spec.get('schema')!r}, expected {SPEC_SCHEMA}")
    if spec.get("content", name) != name:
        raise AuthorError(f"{path}: spec of {spec['content']!r}, not {name!r}")
    unknown = sorted(set(spec) - SPEC_IDENTITY - SPEC_APPLIED)
    if unknown:
        raise AuthorError(f"{path}: spec keys {unknown} need the authored export (A7)")
    return spec, sha256(path)


def author(base_export: Path, name: str, spec_path: Path | None, out: Path,
           staging: Path | None = None) -> dict:
    registry = p4_contents.load_registry()
    row = p4_contents.row_of(name, registry)
    if "base" not in row:
        raise AuthorError(f"{name}: not an authored content (no base in contents.json)")
    base = p4_contents.row_of(row["base"], registry)
    resolved_path = base_export / "resolved.json"
    if not resolved_path.exists():
        raise AuthorError(f"{resolved_path}: no export of the base {base['name']}")
    resolved = json.loads(resolved_path.read_text(encoding="utf-8"))
    if resolved.get("fighter") != base["remix"] or "content" in resolved:
        raise AuthorError(f"{base_export}: not {base['name']}'s Remix export")
    spec, spec_sha = load_spec(spec_path, name)

    resolved["content"] = name
    resolved["base"] = base["name"]
    resolved["authored"] = {
        "base_resolved_sha256": sha256(resolved_path),
        "base_events_sha256": sha256(base_export / "events.json"),
        "spec_sha256": spec_sha,
    }
    out.mkdir(parents=True, exist_ok=True)
    substitute_files(resolved, spec.get("files") or {}, out)
    events = json.loads((base_export / "events.json").read_text(encoding="utf-8"))
    author_audio(resolved, name)
    rom = None
    if staging is not None:
        import remix_rom
        rom = remix_rom.Rom(staging / "ssb64asm.z64")
    author_motions(resolved, events, spec.get("motions") or {}, name, out, rom,
                   spec.get("menu_motions") or {})
    author_statuses(resolved, spec.get("statuses") or {})
    author_attributes(resolved, spec.get("attributes") or {})
    own_main(resolved, name)
    author_kind_tables(resolved, spec.get("kind_tables") or {})
    author_cpu(resolved, events, spec.get("cpu"), out, staging)
    (out / "events.json").write_text(json.dumps(events, indent=1) + "\n", encoding="utf-8", newline="\n")
    summary = base_export / "summary.txt"
    text = summary.read_text(encoding="utf-8") if summary.exists() else ""
    (out / "summary.txt").write_text(
        f"authored content {name} (P6): base {base['name']}'s export, "
        f"spec {'none' if spec_sha is None else spec_sha[:12]}\n" + text,
        encoding="utf-8", newline="\n")
    # resolved.json last: the Makefile's target, so a stopped run leaves none.
    (out / "resolved.json").write_text(json.dumps(resolved, indent=1) + "\n",
                                       encoding="utf-8", newline="\n")
    return {"content": name, "base": base["name"], "spec": spec_sha, "keys": sorted(spec)}


def substitute_files(resolved: dict, files: dict, out: Path) -> None:
    """The spec's own files replace the base's in the descriptor and the
    file closure: each copied into <out>/o2r under its new id, the closure
    row rebuilt from it and every other row's references renamed. The files
    are build outputs (builds/p6-art/<name>/), never tracked (D10)."""
    if not files:
        return
    desc = resolved["descriptor"]
    mapping = {}
    o2r = out / "o2r"
    o2r.mkdir(parents=True, exist_ok=True)
    rows = {}
    for role, entry in sorted(files.items()):
        if role not in FILE_ROLES:
            raise AuthorError(f"spec files: unknown role {role!r} (one of {sorted(FILE_ROLES)})")
        old = desc["file_ids"][FILE_ROLES[role]]
        new = int(entry["id"], 0)
        if new not in P6_FILE_WINDOW:
            raise AuthorError(f"spec files {role}: id {new:#x} outside the P6 window")
        src = REPO / entry["from"]
        if not src.exists():
            raise AuthorError(f"spec files {role}: {src} not built (the look's writer makes it)")
        blob = src.read_bytes()
        if blob[4:8] != b"OLER" or struct.unpack_from("<I", blob, 0x40)[0] != new:
            raise AuthorError(f"spec files {role}: {src} is not an O2R container of file {new:#x}")
        n = struct.unpack_from("<I", blob, 0x48)[0]
        externs = list(struct.unpack_from(f"<{n}H", blob, 0x4C))
        size = struct.unpack_from("<I", blob, 0x4C + 2 * n)[0]
        (o2r / f"{new:04x}").write_bytes(blob)
        mapping[old] = new
        rows[old] = {"file_id": new, "vanilla_id": False, "bytes": size, "externs": externs}
        resolved["authored"].setdefault("files", {})[role] = [old, new]
        resolved["authored"].setdefault("files_sha256", {})[role] = hashlib.sha256(blob).hexdigest()
    before = closure_reach(resolved)
    desc["file_ids"] = [mapping.get(x, x) for x in desc["file_ids"]]
    specials = {old: new for role, (old, new) in resolved["authored"]["files"].items()
                if role.startswith("special")}
    if specials:
        main_old = next((old for role, (old, _) in resolved["authored"]["files"].items()
                         if role == "main"), None)
        if main_old is None:
            raise AuthorError("spec files: a special file of the content's own needs its own main")
        rows[main_old] = retarget_main_specials(o2r, rows[main_old], specials)
        resolved["authored"]["files_sha256"]["main"] = hashlib.sha256(
            (o2r / f"{rows[main_old]['file_id']:04x}").read_bytes()).hexdigest()
    closure = []
    for row in resolved["file_closure"]:
        if row["file_id"] in rows:
            closure.append(rows[row["file_id"]])
            continue
        if any(x in mapping for x in row.get("externs", ())):
            raise AuthorError(f"file {row['file_id']:#x} names a replaced file; it would need its own copy")
        closure.append(row)
    resolved["file_closure"] = closure
    # A file only a replaced one named (Peach's turnip model, which her turnip
    # attributes name) leaves the closure with it.
    orphans = {r["file_id"] for r in closure} & (before - closure_reach(resolved))
    if orphans:
        resolved["file_closure"] = [r for r in closure if r["file_id"] not in orphans]
        resolved["authored"]["files_dropped"] = sorted(orphans)


def closure_reach(resolved: dict) -> set[int]:
    """The closure files the descriptor's files name, through their externs."""
    externs = {r["file_id"]: r.get("externs", ()) for r in resolved["file_closure"]}
    seen: set[int] = set()
    todo = [x for x in resolved["descriptor"]["file_ids"] if x]
    while todo:
        fid = todo.pop()
        if fid in seen or fid not in externs:
            continue
        seen.add(fid)
        todo.extend(externs[fid])
    return seen


def retarget_main_specials(o2r: Path, row: dict, specials: dict[int, int]) -> dict:
    """The content's main with the header words that named the base's
    special files (old -> new in `specials`) naming the content's own, at
    their start. The source loader finds a special file only in the main's
    extern closure (battleship_ftmanager.c, lbRelocGetStatusBufferFile), so
    the word keeps the file loading; Remix's code read the base's file
    through it, the content's own code reads its storage. The row rebuilt
    from the rewritten file."""
    import n64_model as N
    path = o2r / f"{row['file_id']:04x}"
    f = N.RelocFile.load(path)
    for slot, (fid, _) in sorted(f.extern.items()):
        if fid in specials:
            f.extern[slot] = (specials[fid], 0)
    blob = f.to_bytes()
    path.write_bytes(blob)
    n = struct.unpack_from("<I", blob, 0x48)[0]
    return dict(row, externs=list(struct.unpack_from(f"<{n}H", blob, 0x4C)))


_CUE_IDS: dict[str, int] = {}


def _cues(line: str) -> str:
    """A script line with each `cue:<name>` value replaced by the cue's FGM
    id (scripts/p6/<name>/audio.json, p6_audio.cue_ids)."""
    if "cue:" not in line:
        return line
    out = []
    for token in line.split():
        key, _, value = token.partition("=")
        if value.startswith("cue:"):
            cue = value[4:]
            if cue not in _CUE_IDS:
                raise AuthorError(f"{line!r}: no cue {cue!r} in the content's audio.json")
            token = f"{key}={_CUE_IDS[cue]:#x}"
        out.append(token)
    return " ".join(out)


def author_audio(resolved: dict, name: str) -> None:
    """The content's sounds (scripts/p6/<name>/audio.json, p6_audio.py): the
    base's FGM ids it plays mapped to its own cues, which the generator
    applies to its scripts, attributes and kind tables; its own scripts name
    cues as cue:<name>."""
    import p6_audio
    _CUE_IDS.clear()
    _CUE_IDS.update(p6_audio.cue_ids(name))
    fgm_map = p6_audio.fgm_map(name)
    if fgm_map:
        resolved["authored"]["fgm_map"] = {str(k): v for k, v in sorted(fgm_map.items())}
        resolved["authored"]["audio_sha256"] = sha256(p6_audio.spec_path(name))


def assemble_block(lines: list[str], ram: bytearray, events: dict, label: str,
                   referrer: str) -> tuple[int, str]:
    """Script text (mscript.py, one command a line) assembled at the end of
    the authored RAM window (remix_rom.AUTHORED_RAM) and listed in
    events.json as a block the generator copies like Remix's own; pointer
    targets name Remix blocks (ram:/file:) or NULL. Its address and key."""
    import mscript
    from remix_rom import AUTHORED_RAM
    start = AUTHORED_RAM + len(ram)
    block_key = f"ram:{start:#010x}"
    cmds, edges, exit_ = [], [], None
    for line in lines:
        words, pointers = mscript.assemble(_cues(line))
        for k, target in pointers.items():
            # A Remix RAM target is written as its address, as Remix's own
            # blocks hold it, so the generator's round trip (which decodes
            # the authored RAM) follows the same edge as the copied block.
            if target.startswith("ram:"):
                words[k] = int(target[4:], 16)
            elif target != "NULL":
                raise AuthorError(f"{label}: {line!r}: only ram: targets (or NULL) in authored scripts")
        at = AUTHORED_RAM + len(ram)
        op = line.split()[0]
        rec = {"at": f"ram:{at:#010x}", "op": op, "words": [words[0]]}
        for target in pointers.values():
            if target == "NULL":
                rec["to"] = None
                rec["null"] = True
            else:
                rec["to"] = target
                if op in EDGE_KIND:
                    edges.append({"kind": EDGE_KIND[op], "to": target})
        cmds.append(rec)
        ram += b"".join(struct.pack(">I", w) for w in words)
        if op in ("End", "Return", "Goto"):
            exit_ = op.lower()
            break
    if exit_ is None:
        raise AuthorError(f"{label}: the script does not end (End, Return or Goto)")
    events["blocks"][block_key] = {"start": block_key, "label": label, "referrers": [referrer],
                                   "commands": cmds, "exit": exit_, "edges": edges}
    return start, block_key


def edited_script(rom, events: dict, row: dict, edits: dict, index: int) -> list[str]:
    """The base's own script for a motion as text, edited (A5, a reused move):
    "damage_scale" multiplies every MakeAttackColl's damage (rounded, at
    least 1), "element" maps elements ({"1": 3}: fire to slash), "drop" lists
    command names to leave out, "frame_speed" puts a RemixFrameSpeedMultiplier
    first (flag 0: animation and script clock together). The command words
    come from the staged ROM; a script with pointer commands is refused."""
    import mscript
    if rom is None:
        raise AuthorError(f"spec motions {index}: script_edits need the staged ROM (--staging)")
    unknown = set(edits) - {"damage_scale", "element", "drop", "frame_speed", "comment"}
    if unknown:
        raise AuthorError(f"spec motions {index}: unknown script_edits {sorted(unknown)}")
    root = row.get("script_root")
    if row.get("script") in (None, 0, NO_SCRIPT) or root not in events["blocks"]:
        raise AuthorError(f"spec motions {index}: script_edits on a motion with no script of its own")
    block = events["blocks"][root]
    if block.get("edges") or any(c.get("to") for c in block["commands"]):
        raise AuthorError(f"spec motions {index}: script_edits on a script with pointer commands")
    out = []
    if "frame_speed" in edits:
        bits = struct.unpack(">I", struct.pack(">f", float(edits["frame_speed"])))[0]
        out.append(f"RemixFrameSpeedMultiplier value={(bits + 0x8000) >> 16}")
    scale = float(edits.get("damage_scale", 1.0))
    element = {int(str(k), 0): int(v) for k, v in (edits.get("element") or {}).items()}
    drop = set(edits.get("drop") or ())
    for text in mscript.block_commands(rom, events, root):
        op, fields = mscript.parse(text)
        if op in drop:
            continue
        if op == "MakeAttackColl":
            fields["damage"] = max(1, int(fields["damage"] * scale + 0.5))
            fields["element"] = element.get(fields["element"], fields["element"])
            text = op + " " + " ".join(f"{k}={v}" for k, v in fields.items())
        out.append(text)
    return out


def author_motions(resolved: dict, events: dict, motions: dict, name: str, out: Path,
                   rom=None, menu_motions: dict | None = None) -> None:
    """The spec's motions (index -> {"anim", "anim_flags", "script",
    "script_edits"}); the index one past the base's last appends a motion
    (Ike's jab 3: the base's own row for it is another move):
    - "anim": a built clip (an O2R container under builds/, anim.py), copied
      into <out>/o2r under the next id of the content's window and added to
      the file closure (once: motions naming the same clip share it, as the
      entry's two sides do); "anim_id" names a clip already in the closure;
    - "script": a committed motion script (mscript.py text, one command a
      line, "#" comments), assembled into the authored RAM window
      (<out>/authored_ram.bin, assemble_block);
    - "script_edits": the base's own script with edits (edited_script),
      assembled the same way.
    The motion row takes the new clip and script; the generator reads both
    through OverlayRom. `menu_motions` gives the menu (submotion) rows
    their own scripts the same way: the Results and select poses (1-4 the
    wins, 5 the clap)."""
    if not motions and not menu_motions:
        return
    if name not in CONTENT_WINDOW:
        raise AuthorError(f"{name}: no P6 file window")
    window = CONTENT_WINDOW[name]
    o2r = out / "o2r"
    o2r.mkdir(parents=True, exist_ok=True)
    rows = {m["index"]: m for m in resolved["motions"]}
    authored = resolved["authored"]
    ram = bytearray()
    replaced = set()
    next_id = window + ANIM_FIRST
    clips: dict[str, int] = {}
    for key in sorted(motions, key=lambda k: int(k, 0)):
        entry = motions[key]
        index = int(key, 0)
        row = rows.get(index)
        if row is None:
            if index != len(resolved["motions"]):
                raise AuthorError(f"spec motions: no motion {index} in the base "
                                  f"(the next new one is {len(resolved['motions'])})")
            row = {"index": index, "anim_file_id": 0, "script": NO_SCRIPT, "anim_flags": 0,
                   "inherited": {"anim_file_id": False, "script": False, "anim_flags": False},
                   "script_root": None}
            resolved["motions"].append(row)
            rows[index] = row
            authored.setdefault("motions_appended", []).append(index)
        unknown = set(entry) - {"anim", "anim_id", "anim_flags", "script", "script_edits", "comment"}
        if unknown:
            raise AuthorError(f"spec motions {index}: unknown keys {sorted(unknown)}")
        if "anim" in entry and entry["anim"] in clips:
            fid = clips[entry["anim"]]
            replaced.add(row["anim_file_id"])
            row["anim_file_id"] = fid
            row["inherited"]["anim_file_id"] = False
            authored.setdefault("anims", {})[str(index)] = fid
        elif "anim" in entry:
            src = REPO / entry["anim"]
            if not src.exists():
                raise AuthorError(f"spec motions {index}: {src} not built (anim.py makes it)")
            blob = bytearray(src.read_bytes())
            if blob[4:8] != b"OLER":
                raise AuthorError(f"spec motions {index}: {src} is not an O2R container")
            fid = next_id
            next_id += 1
            if fid >= window + 0x100:
                raise AuthorError(f"{name}: more clips than its file window holds")
            struct.pack_into("<I", blob, 0x40, fid)
            (o2r / f"{fid:04x}").write_bytes(bytes(blob))
            n = struct.unpack_from("<I", blob, 0x48)[0]
            size = struct.unpack_from("<I", blob, 0x4C + 2 * n)[0]
            resolved["file_closure"].append({"file_id": fid, "vanilla_id": False, "bytes": size, "externs": []})
            replaced.add(row["anim_file_id"])
            row["anim_file_id"] = fid
            row["inherited"]["anim_file_id"] = False
            authored.setdefault("anims", {})[str(index)] = fid
            authored.setdefault("anims_sha256", {})[str(index)] = hashlib.sha256(bytes(blob)).hexdigest()
            clips[entry["anim"]] = fid
        if "anim_id" in entry:
            # A clip already in the content's closure (Quick Draw's slash is
            # Roy's forward-tilt clip): named, not copied.
            fid = int(str(entry["anim_id"]), 0)
            if fid not in {r["file_id"] for r in resolved["file_closure"]}:
                raise AuthorError(f"spec motions {index}: anim_id {fid:#x} is not in the closure")
            if row["anim_file_id"]:
                replaced.add(row["anim_file_id"])
            row["anim_file_id"] = fid
            row["inherited"]["anim_file_id"] = False
        if "anim_flags" in entry:
            row["anim_flags"] = int(str(entry["anim_flags"]), 0)
            row["inherited"]["anim_flags"] = False
        if "script" in entry and "script_edits" in entry:
            raise AuthorError(f"spec motions {index}: a script or script_edits, not both")
        lines = None
        if "script" in entry:
            src = REPO / entry["script"]
            lines = [ln.split("#", 1)[0].strip() for ln in src.read_text(encoding="utf-8").splitlines()]
            lines = [ln for ln in lines if ln]
            authored.setdefault("scripts_sha256", {})[str(index)] = sha256(src)
        elif "script_edits" in entry:
            lines = edited_script(rom, events, row, entry["script_edits"], index)
            authored.setdefault("script_edits", {})[str(index)] = entry["script_edits"]
        if lines is not None:
            start, block_key = assemble_block(lines, ram, events, f"{name}.motion{index:#x}",
                                              f"motion:{index:#x}")
            row["script"] = start
            row["inherited"]["script"] = False
            row["script_root"] = block_key
    menu_rows = {m["index"]: m for m in resolved["menu_motions"]}
    for key in sorted(menu_motions or {}, key=lambda k: int(k, 0)):
        entry = menu_motions[key]
        index = int(key, 0)
        row = menu_rows.get(index)
        if row is None:
            raise AuthorError(f"spec menu_motions: no menu motion {index} in the base")
        unknown = set(entry) - {"script", "comment"}
        if unknown:
            raise AuthorError(f"spec menu_motions {index}: unknown keys {sorted(unknown)} (a script only)")
        src = REPO / entry["script"]
        lines = [ln.split("#", 1)[0].strip() for ln in src.read_text(encoding="utf-8").splitlines()]
        lines = [ln for ln in lines if ln]
        authored.setdefault("menu_scripts_sha256", {})[str(index)] = sha256(src)
        start, block_key = assemble_block(lines, ram, events, f"{name}.menu{index:#x}", f"menu:{index:#x}")
        row["script"] = start
        row["inherited"]["script"] = False
        row["script_root"] = block_key
    # A clip the content no longer plays leaves its closure: shipped unused,
    # the generator would not compact it, and a base sharing it ships the
    # compacted bytes under the same id.
    used = {m["anim_file_id"] for m in resolved["motions"] + resolved["menu_motions"]}
    dropped = sorted(old for old in replaced if old not in used)
    if dropped:
        resolved["file_closure"] = [r for r in resolved["file_closure"] if r["file_id"] not in dropped]
        authored["anims_dropped"] = dropped
    if ram:
        from remix_rom import AUTHORED_RAM
        (out / "authored_ram.bin").write_bytes(bytes(ram))
        authored["ram"] = {"base": AUTHORED_RAM, "file": "authored_ram.bin", "bytes": len(ram),
                           "sha256": hashlib.sha256(bytes(ram)).hexdigest()}


def author_statuses(resolved: dict, statuses: dict) -> None:
    """The spec's statuses (id -> {"motion", "attack_id", "sflags", "procs",
    "name"}): the id one past the base's last appends a special status that
    the content's rows name (Ike's jab 3, from jab_3_action). "procs" are the
    four routines (update, interrupt, physics, map) by name, decomp
    functions or DS ports the generator knows, or null."""
    if not statuses:
        return
    table = resolved["statuses"]
    motions = {m["index"] for m in resolved["motions"]}
    slots = ("proc_update", "proc_interrupt", "proc_physics", "proc_map")
    for key in sorted(statuses, key=lambda k: int(k, 0)):
        entry = statuses[key]
        sid = int(key, 0)
        following = max(s["status_id"] for s in table) + 1
        if sid != following:
            raise AuthorError(f"spec statuses: {sid} is not the next status ({following}); "
                              "a base status keeps its row")
        unknown = set(entry) - {"motion", "attack_id", "sflags", "procs", "name", "comment"}
        if unknown:
            raise AuthorError(f"spec statuses {sid}: unknown keys {sorted(unknown)}")
        motion = int(entry["motion"])
        if motion not in motions:
            raise AuthorError(f"spec statuses {sid}: no motion {motion}")
        procs = entry["procs"]
        if len(procs) != len(slots):
            raise AuthorError(f"spec statuses {sid}: procs lists {len(slots)} routines")
        row = {"motion_id": motion, "attack_id": int(entry.get("attack_id", 0)),
               "sflags": int(entry.get("sflags", 0)), "status_id": sid, "name": entry.get("name"),
               "inherited": {k: False for k in ("motion_id", "attack_id", "sflags") + slots},
               "parent_procs": None}
        for slot, routine in zip(slots, procs):
            # The generator names a routine by its name; 0 is NULL.
            row[slot] = 1 if routine else 0
            if routine:
                row[slot + "_name"] = routine
        table.append(row)
        resolved["authored"].setdefault("statuses_appended", []).append(sid)


def author_attributes(resolved: dict, attributes: dict) -> None:
    """The spec's FTAttributes values (field -> float): the generator sets
    them in the content's main file (Ike: Captain's jab follow-up window)."""
    if attributes:
        resolved["authored"]["attributes"] = {k: float(v) for k, v in sorted(attributes.items())}


def own_main(resolved: dict, name: str) -> None:
    """An authored content with its own motion scripts gets its own
    synthesized motion file (generate_p4_fighter.fighter_spec), and the
    main file names the motion file; its attributes are the main file's
    too. A main still the base's is then shipped as a copy under the
    content's main id (the window's first), its bytes the base's until the
    generator points it at the content's motion file and sets its
    attributes (authored "copies": role -> [base id, own id])."""
    if "ram" not in resolved["authored"] and "attributes" not in resolved["authored"]:
        return
    desc = resolved["descriptor"]
    old = desc["file_ids"][FILE_ROLES["main"]]
    if old in P6_FILE_WINDOW:
        return
    new = CONTENT_WINDOW[name] + FILE_ROLES["main"]
    closure = []
    for row in resolved["file_closure"]:
        if row["file_id"] == old:
            row = dict(row, file_id=new, vanilla_id=False)
        elif old in row.get("externs", ()):
            raise AuthorError(f"file {row['file_id']:#x} names the base's main; it would need its own copy")
        closure.append(row)
    resolved["file_closure"] = closure
    desc["file_ids"][FILE_ROLES["main"]] = new
    resolved["authored"].setdefault("copies", {})["main"] = [old, new]


def author_kind_tables(resolved: dict, rows: dict) -> None:
    """The spec's kind-table rows (name -> "parent", or a value): Remix's
    per-kind tables the generator turns into the content's rows. "parent"
    takes the setup parent's entry (Ike: Captain's jab chain where Roy cuts
    his after jab 1)."""
    tables = resolved["kind_tables"]
    for name, value in sorted(rows.items()):
        if name not in tables:
            raise AuthorError(f"spec kind_tables: no table {name!r} in the base")
        row = tables[name]
        if isinstance(value, dict):
            # An authored value the generator reads in place of the row's
            # code ({"status": N}: jab_3_action naming an appended status;
            # {"routine": name}: a special starter in nds_p4_<name>.c).
            row["authored"] = value
            resolved["authored"].setdefault("kind_tables", {})[name] = value
            continue
        new = row["parent_value"] if value == "parent" else str(value)
        row["value"] = new
        row["same_as_parent"] = new == row["parent_value"]
        resolved["authored"].setdefault("kind_tables", {})[name] = new


def author_cpu(resolved: dict, events: dict, cpu: dict | None, out: Path, staging: Path | None) -> None:
    """The spec's CPU attack rows (A12): the base's rows, named from its
    Remix source (src/<Base>/AI/Attacks.asm), measured on the content's own
    clips and scripts by coverage.py ("drop" a move with no hitbox, "keep"
    the base's numbers, "measure" options), into authored.cpu_attacks for
    the generator."""
    if not cpu:
        return
    if staging is None:
        raise AuthorError("spec cpu needs the staged ROM (--staging)")
    import coverage
    folder = next((d for d in (staging / "src").iterdir()
                   if d.is_dir() and d.name.upper() == resolved["fighter"]), None)
    asm = folder / "AI" / "Attacks.asm" if folder is not None else None
    if asm is None or not asm.exists():
        raise AuthorError(f"spec cpu: no AI/Attacks.asm for {resolved['fighter']} under {staging / 'src'}")
    fighter = coverage.load(staging, out, resolved, events)
    rows, names = coverage.cpu_rows(fighter, asm.read_text(encoding="utf-8", errors="replace"), cpu)
    resolved["authored"]["cpu_attacks"] = rows
    resolved["authored"]["cpu_names"] = names


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--base-export", type=Path, required=True,
                    help="the base content's remix_export.py directory")
    ap.add_argument("--content", required=True, help="the authored content's registry name")
    ap.add_argument("--spec", type=Path, help="its spec (default scripts/p6/<name>/spec.json)")
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--staging", type=Path,
                    help="the P4 staging directory (ssb64asm.z64): script_edits read the base's words")
    args = ap.parse_args()
    spec = args.spec if args.spec is not None else HERE / args.content / "spec.json"
    done = author(args.base_export, args.content, spec, args.out, args.staging)
    print(f"p6 author: {done['content']} from {done['base']}, spec "
          f"{'none' if done['spec'] is None else done['spec'][:12]} -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
