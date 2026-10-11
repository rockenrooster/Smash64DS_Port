#!/usr/bin/env python3
"""P4 colour animations: Remix's extended GFX routines the roster uses.

Remix adds colour-animation scripts after the source's 86 (GFXRoutine.asm:
ids 0x56 on, `gfx_routine_table_extended` rows of {script, priority,
end-on-action-change}). A P4 content's motion scripts start them with the
source's SetColAnim command, and its ported routines with
ftParamCheckSetFighterColAnimID, so the port's colour-animation table
(dGMColScriptsDescs, src/port/reloc_backend_compat_shims.c) is extended with
the rows the enabled contents reach: every SetColAnim id >= 0x56 in their
exported motion events, plus the ids their ported code sets (CODE_COLANIMS).

The scripts are read from the Remix image and re-encoded in the port's
colour-command layout: ARM allocates GMColEvent's u32 bitfields from the
least significant bit, so each word's fields keep their order and widths but
pack from the bottom (opcode in bits 0-5); a goto, subroutine or parallel
pointer becomes the address of its target in the generated word array.

    p4_colanims.py --staging <remix staging> --export-root <export root>
        --contents "falco wolf ..." --out <nds_p4_colanims.generated.inc>
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import remix_rom as R  # noqa: E402

FIRST_ID = 0x56          # nGMColAnimEnumCount; Remix's first new id
ROW_BYTES = 8            # {script*, u8 priority, u8 bool_action, u16 pad}
EV_SETCOLANIM = 44       # remix_export.EVENTS index of SetColAnim

# Ids set by the contents' ported routines (not by motion scripts).
CODE_COLANIMS = {
    "sheik": (0x6D,),    # SheikNSP full charge, GFXRoutine.id.SHEIK_CHARGE
    "dedede": (0x74,),   # DededeDSP full charge, GFXRoutine.id.DEDEDE_CHARGE
}

# GMColEvent opcodes (gm/gmdef.h) -> (field widths of the first word, MSB
# first, as the source declares them; extra words and their kind).
DEFAULT = (6, 26)
LAYOUT = {
    0: (DEFAULT, ()),                     # End
    1: (DEFAULT, ()),                     # Wait
    2: (DEFAULT, ("ptr",)),               # Goto
    3: (DEFAULT, ()),                     # LoopBegin
    4: (DEFAULT, ()),                     # LoopEnd
    5: (DEFAULT, ("ptr",)),               # Subroutine
    6: (DEFAULT, ()),                     # Return
    7: (DEFAULT, ("ptr",)),               # SetParallelScript
    8: (DEFAULT, ()),                     # ClearColorAll
    9: (DEFAULT, ("rgba",)),              # SetColor1
    10: (DEFAULT, ("rgba",)),             # BlendColor1
    11: (DEFAULT, ("rgba",)),             # SetColor2
    12: (DEFAULT, ("rgba",)),             # BlendColor2
    13: ((6, 7, 9, 10), ("h2", "h2", "h2")),  # Effect
    14: ((6, 7, 9, 10), ("h2", "h2", "h2")),  # EffectItemHoldOffset
    15: ((6, 13, 13), ()),                # SetLight
    16: (DEFAULT, ()),                    # ClearLight
    17: (DEFAULT, ()),                    # PlayFGM
    18: (DEFAULT, ()),                    # SetSkeletonID
}
NAMES = ["End", "Wait", "Goto", "LoopBegin", "LoopEnd", "Subroutine", "Return",
         "SetParallelScript", "ClearColorAll", "SetColor1", "BlendColor1", "SetColor2",
         "BlendColor2", "Effect", "EffectItemHoldOffset", "SetLight", "ClearLight",
         "PlayFGM", "SetSkeletonID"]
TERMINATORS = {0, 2, 6}   # End, Goto, Return: the stream does not fall through


class ColError(Exception):
    pass


def repack(word: int, widths) -> int:
    """MSB-first fields (N64) -> the same fields packed LSB-first (ARM)."""
    fields = []
    shift = 32
    for w in widths:
        shift -= w
        fields.append((word >> shift) & ((1 << w) - 1))
    out = 0
    pos = 0
    for value, w in zip(fields, widths):
        out |= value << pos
        pos += w
    return out


def used_ids(export_root: Path, contents: list[str]) -> dict[int, set[str]]:
    ids: dict[int, set[str]] = {}
    for name in contents:
        events = json.loads((export_root / name / "events.json").read_text(encoding="utf-8"))
        for block in events["blocks"].values():
            for cmd in block["commands"]:
                if cmd["op"] == "SetColAnim":
                    cid = (cmd["words"][0] >> 18) & 0xFF
                    if cid >= FIRST_ID:
                        ids.setdefault(cid, set()).add(name)
        for cid in CODE_COLANIMS.get(name, ()):
            ids.setdefault(cid, set()).add(name)
    return ids


def decode(rom: R.Rom, starts: list[int]) -> tuple[dict[int, tuple], list[int]]:
    """Every word reachable from the starts: addr -> (kind, value, note)."""
    words: dict[int, tuple] = {}
    pending = list(starts)
    while pending:
        pc = pending.pop()
        while pc not in words:
            w = struct.unpack(">I", rom.read_ram(pc, 4))[0]
            op = w >> 26
            if op not in LAYOUT:
                raise ColError(f"{pc:#010x}: unknown colour opcode {op}")
            first, extra = LAYOUT[op]
            words[pc] = ("word", repack(w, first), f"{NAMES[op]} {w:#010x}")
            at = pc + 4
            for kind in extra:
                v = struct.unpack(">I", rom.read_ram(at, 4))[0]
                if kind == "ptr":
                    words[at] = ("ptr", v, "")
                    pending.append(v)
                elif kind == "rgba":
                    words[at] = ("word", repack(v, (8, 8, 8, 8)), f"rgba {v:#010x}")
                else:
                    words[at] = ("word", repack(v, (16, 16)), f"{v:#010x}")
                at += 4
            if op in TERMINATORS:
                break
            pc = at
    return words, sorted(words)


def build(staging: Path, export_root: Path, contents: list[str]) -> str:
    rom = R.Rom(staging / "ssb64asm.z64")
    names, _ = R.load_symbols(staging / "logfile.log")
    table = names.get("GFXRoutine.gfx_routine_table_extended")
    if table is None:
        raise ColError("no GFXRoutine.gfx_routine_table_extended in the bass log")
    ids = used_ids(export_root, contents)
    rows = {}
    for cid in sorted(ids):
        script, priority, bool_action = struct.unpack(
            ">IBB", rom.read_ram(table + (cid - FIRST_ID) * ROW_BYTES, 6))
        rows[cid] = (script, priority, bool_action)
    words, order = decode(rom, [r[0] for r in rows.values()]) if rows else ({}, [])
    index = {addr: i for i, addr in enumerate(order)}
    lines = [
        "/* Generated by scripts/p4/p4_colanims.py: Remix's extended colour",
        " * animations the enabled P4 contents reach, in the port's LSB-first",
        " * colour-command layout. Included by reloc_backend_compat_shims.c. */",
    ]
    last = max(rows) if rows else FIRST_ID - 1
    lines.append(f"#define NDS_P4_COLANIM_LAST {last:#x}")
    if not rows:
        lines.append("#define NDS_P4_COLANIM_ROWS")
        return "\n".join(lines) + "\n"
    by_name = {}
    for nm, addr in names.items():
        if nm.startswith("GFXRoutine.") and addr in words:
            by_name.setdefault(addr, nm)
    lines.append(f"static u32 sNdsP4ColScriptWords[{len(order)}] = {{")
    for i, addr in enumerate(order):
        kind, value, note = words[addr]
        label = f" {by_name[addr]}" if addr in by_name else ""
        if i and order[i - 1] + 4 != addr:
            lines.append("    /* -- */")
        if kind == "ptr":
            if value not in index:
                raise ColError(f"{addr:#010x}: pointer {value:#010x} outside the decoded scripts")
            lines.append(f"    (u32)(uintptr_t)&sNdsP4ColScriptWords[{index[value]}], "
                         f"/* {addr:#010x} -> {value:#010x}{label} */")
        else:
            lines.append(f"    {value:#010x}u, /* {addr:#010x} {note}{label} */")
    lines.append("};")
    lines.append("#define NDS_P4_COLANIM_ROWS \\")
    for cid, (script, priority, bool_action) in sorted(rows.items()):
        users = ", ".join(sorted(ids[cid]))
        lines.append(f"    [{cid:#x}] = {{ &sNdsP4ColScriptWords[{index[script]}], {priority}, "
                     f"{'TRUE' if bool_action else 'FALSE'} }}, /* {users} */ \\")
    lines.append("")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--staging", type=Path, required=True)
    ap.add_argument("--export-root", type=Path, required=True)
    ap.add_argument("--contents", required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    text = build(args.staging, args.export_root, args.contents.split())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text, encoding="utf-8", newline="\n")
    print(f"p4 colanims: {text.count('[0x')} rows")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
