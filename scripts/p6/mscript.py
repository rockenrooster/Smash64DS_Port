#!/usr/bin/env python3
"""P6 A5: fighter motion scripts as text, and back (docs/P6/P6_PLAN.md A5).

A motion script is a stream of commands: the first word's top 6 bits name the
op (remix_export.py EVENTS, the names events.json prints), Remix's custom
commands (0xD0-0xDC) take the whole top byte. Field layouts are BattleShip's
motion-event structs (src/ft/fttypes.h FTMotionEvent*): bit fields from the
word's most significant bit down, so a field list per word is the layout.
Bits a struct leaves unnamed are `pad` fields: written only when not zero,
and always kept, so text and bytes round-trip exactly.

Text form, one command a line:

    MakeAttackColl attack_id=0 joint_id=12 damage=12 size=240 angle=361 ...
    SyncWait frames=4
    Goto -> file:0x136e:0x1a4

A pointer field (Subroutine, Goto, SetParallelScript, SetThrow,
SetDamageThrown, RemixRandomSFX) names its target: `file:<id>:<offset>` (a
reloc slot of a motion file, whose word the container's chain writes),
`ram:<address>` (Remix's code region) or `NULL`.

    mscript.py --check builds/p4-staging/extra-96621af builds/p4-staging/export-x
        F3: every command of every export's events.json disassembles and
        re-assembles to its words (pointers to their targets)
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "p4"))
sys.path.insert(0, str(HERE.parent))

# op -> (name, words): each word a list of (field, bits, signed); the first
# word's list starts after the opcode (6 bits; 8 for Remix's customs).
# "ptr" as a field's bits marks a whole pointer word.
U, S = False, True
DEFAULT = [[("value", 26, U)]]
OPS: dict[int, tuple[str, list]] = {
    0: ("End", DEFAULT),
    1: ("SyncWait", [[("frames", 26, U)]]),
    2: ("AsyncWait", [[("frames", 26, U)]]),
    3: ("MakeAttackColl", [
        [("attack_id", 3, U), ("group_id", 3, U), ("joint_id", 7, S), ("damage", 8, U),
         ("can_rebound", 1, U), ("element", 4, U)],
        [("size", 16, U), ("off_x", 16, S)],
        [("off_y", 16, S), ("off_z", 16, S)],
        [("angle", 10, S), ("knockback_scale", 10, U), ("knockback_weight", 10, U),
         ("is_hit_ground_air", 2, U)],
        [("shield_damage", 8, S), ("fgm_level", 3, U), ("fgm_kind", 4, U), ("knockback_base", 10, U)]]),
    5: ("ClearAttackCollID", DEFAULT),
    6: ("ClearAttackCollAll", DEFAULT),
    7: ("SetAttackCollOffset", [[("attack_id", 3, U), ("off_x", 16, S)],
                                [("off_y", 16, S), ("off_z", 16, S)]]),
    8: ("SetAttackCollDamage", [[("attack_id", 3, U), ("damage", 8, U)]]),
    9: ("SetAttackCollSize", [[("attack_id", 3, U), ("size", 16, U)]]),
    10: ("SetAttackCollSoundLevel", [[("attack_id", 3, U), ("fgm_level", 3, U)]]),
    11: ("RefreshAttackCollID", DEFAULT),
    12: ("SetThrow", [[], [("target", "ptr", U)]]),
    13: ("SetDamageThrown", [[], [("target", "ptr", U)]]),
    14: ("PlayFGM", DEFAULT),
    15: ("PlayLoopSFXStoreInfo", DEFAULT),
    16: ("StopLoopSFX", DEFAULT),
    17: ("PlayVoiceStoreInfo", DEFAULT),
    18: ("PlayLoopVoiceStoreInfo", DEFAULT),
    19: ("PlayFGMStoreInfo", DEFAULT),
    20: ("PlaySmashVoice", DEFAULT),
    21: ("SetFlag0", DEFAULT),
    22: ("SetFlag1", DEFAULT),
    23: ("SetFlag2", DEFAULT),
    24: ("SetFlag3", DEFAULT),
    25: ("SetAirJumpAdd", DEFAULT),
    26: ("SetAirJumpMax", DEFAULT),
    27: ("SetHitStatusPartAll", DEFAULT),
    28: ("SetHitStatusPartID", [[("joint_id", 7, S), ("hitstatus", 19, U)]]),
    29: ("SetHitStatusAll", DEFAULT),
    30: ("ResetDamageCollPartAll", DEFAULT),
    31: ("SetDamageCollPartID", [[("joint_id", 7, S)],
                                [("off_x", 16, S), ("off_y", 16, S)],
                                [("off_z", 16, S), ("size_x", 16, S)],
                                [("size_y", 16, S), ("size_z", 16, S)]]),
    32: ("LoopBegin", [[("count", 26, U)]]),
    33: ("LoopEnd", DEFAULT),
    34: ("Subroutine", [[], [("target", "ptr", U)]]),
    35: ("Return", DEFAULT),
    36: ("Goto", [[], [("target", "ptr", U)]]),
    37: ("PauseScript", DEFAULT),
    38: ("Effect", [[("joint_id", 7, S), ("effect_id", 9, U), ("flag", 10, U)],
                    [("off_x", 16, S), ("off_y", 16, S)],
                    [("off_z", 16, S), ("rng_x", 16, S)],
                    [("rng_y", 16, S), ("rng_z", 16, S)]]),
    40: ("SetModelPartID", [[("joint_id", 7, S), ("modelpart_id", 19, S)]]),
    41: ("ResetModelPartAll", DEFAULT),
    42: ("HideModelPartAll", DEFAULT),
    43: ("SetTexturePartID", [[("texturepart_id", 6, U), ("frame", 20, U)]]),
    44: ("SetColAnim", [[("colanim_id", 8, U), ("length", 18, U)]]),
    45: ("ResetColAnim", DEFAULT),
    46: ("SetParallelScript", [[], [("target", "ptr", U)]]),
    47: ("SetSlopeContour", [[("pad", 23, U), ("flags", 3, U)]]),
    48: ("HideItem", DEFAULT),
    49: ("MakeRumble", [[("length", 13, U), ("rumble_id", 13, U)]]),
    50: ("StopRumble", [[("rumble_id", 26, U)]]),
    51: ("SetAfterImage", [[("is_itemswing", 8, U), ("drawstatus", 18, S)]]),
}
OPS[4] = ("MakeAttackCollScaled", OPS[3][1])
OPS[39] = ("EffectItemHold", OPS[38][1])
# Remix's customs: the top byte names them (remix_export.py REMIX_CUSTOM).
REMIX = {
    0xD0: ("RemixFrameSpeedMultiplier", [[("value", 24, U)]]),
    0xD1: ("RemixArmour", [[("value", 24, U)]]),
    0xD2: ("RemixHitboxDirection", [[("value", 24, U)]]),
    0xD3: ("RemixTranslationMultiplier", [[("value", 24, U)]]),
    0xD4: ("RemixYVelocity", [[("value", 24, U)]]),
    0xD5: ("RemixFastFall", [[("value", 24, U)]]),
    0xD6: ("RemixRandomSFX", [[("value", 24, U)], [("target", "ptr", U)]]),
    0xD7: ("RemixSetKinetic", [[("value", 24, U)]]),
    0xD8: ("RemixSetHitboxFGM", [[("value", 24, U)]]),
    0xD9: ("RemixSetEnvColor", [[("value", 24, U)], [("color", 32, U)]]),
    0xDA: ("RemixSwitchDirection", [[("value", 24, U)]]),
    0xDB: ("RemixGotoMovesetFile", [[("value", 24, U)]]),
    0xDC: ("RemixLVoiceSFX", [[("value", 24, U)], [("value2", 32, U)]]),
}
BY_NAME = {name: (op, words, False) for op, (name, words) in OPS.items()}
BY_NAME.update({name: (op, words, True) for op, (name, words) in REMIX.items()})


class ScriptError(ValueError):
    pass


def length(first_word: int) -> int:
    """A command's size in bytes from its first word."""
    top = first_word >> 24
    if top >= 0xD0:
        if top not in REMIX:
            raise ScriptError(f"unknown Remix command {first_word:#010x}")
        return 4 * len(REMIX[top][1])
    op = first_word >> 26
    if op not in OPS:
        raise ScriptError(f"unknown op {op} ({first_word:#010x})")
    return 4 * len(OPS[op][1])


def _layout(words_spec, k: int) -> tuple[int, list]:
    """(first bit, fields with an implicit trailing pad) of word k."""
    return words_spec[k]


def _fields_of(spec_word: list, top_bits: int) -> list:
    """The word's fields from bit (32 - top_bits - 1) down, the unnamed low
    bits as a trailing pad."""
    used = sum(b for _, b, _ in spec_word if b != "ptr")
    avail = 32 - top_bits
    out = list(spec_word)
    if any(b == "ptr" for _, b, _ in spec_word):
        return out
    if used > avail:
        raise ScriptError(f"fields of {used} bits in a {avail}-bit word")
    if used < avail:
        out.append(("pad" if not any(n == "pad" for n, _, _ in spec_word) else "pad_low", avail - used, U))
    return out


def disassemble(words: list[int], pointers: dict[int, str] | None = None) -> str:
    """One command's text. `pointers`: word index -> target text."""
    w0 = words[0]
    top = w0 >> 24
    if top >= 0xD0:
        name, spec = REMIX[top]
        top_bits = 8
    else:
        name, spec = OPS[w0 >> 26]
        top_bits = 6
    if len(words) != len(spec):
        raise ScriptError(f"{name}: {len(words)} words for {len(spec)}")
    parts = [name]
    for k, (word, spec_word) in enumerate(zip(words, spec)):
        if any(b == "ptr" for _, b, _ in spec_word):
            target = (pointers or {}).get(k)
            parts.append("-> " + (target if target is not None else f"word:{word:#010x}"))
            continue
        bit = 32 - (top_bits if k == 0 else 0)
        for field, bits, signed in _fields_of(spec_word, top_bits if k == 0 else 0):
            bit -= bits
            value = (word >> bit) & ((1 << bits) - 1)
            if signed and value >> (bits - 1):
                value -= 1 << bits
            if field.startswith("pad"):
                if value:
                    parts.append(f"{field}{k}={value:#x}")
                continue
            parts.append(f"{field}={value}")
    return " ".join(parts)


def parse(text: str) -> tuple[str, dict]:
    """A command's name and its named fields (pointer targets under "->")."""
    name, *rest = text.split()
    out = {}
    i = 0
    while i < len(rest):
        if rest[i] == "->":
            out.setdefault("->", []).append(rest[i + 1])
            i += 2
            continue
        k, v = rest[i].split("=", 1)
        out[k] = int(v, 0)
        i += 1
    return name, out


def attack_timeline(commands: list[str], frames: int) -> list[list[dict]]:
    """Active attack collisions per frame (0..frames) of a straight-line
    script: MakeAttackColl, SetAttackCollOffset/Size, ClearAttackCollID/All,
    timed by SyncWait (frames from now) and AsyncWait (to an animation
    frame). Each entry: attack_id, joint_id, off (x, y, z) and radius
    (size * 0.5, ftMainUpdateMotionEventsDefault), in the joint's frame."""
    active: dict[int, dict] = {}
    out: list[list[dict]] = []
    t = 0

    def emit(until: int):
        while len(out) <= min(until, frames):
            out.append([dict(a) for a in active.values()])
    for text in commands:
        name, f = parse(text)
        if name == "SyncWait":
            emit(t + f["frames"] - 1)
            t += f["frames"]
        elif name == "AsyncWait":
            if f["frames"] > t:
                emit(f["frames"] - 1)
                t = f["frames"]
        elif name == "MakeAttackColl":
            active[f["attack_id"]] = {"attack_id": f["attack_id"], "joint_id": f["joint_id"],
                                      "off": (f["off_x"], f["off_y"], f["off_z"]), "radius": f["size"] * 0.5,
                                      "damage": f["damage"]}
        elif name == "SetAttackCollOffset" and f["attack_id"] in active:
            active[f["attack_id"]]["off"] = (f["off_x"], f["off_y"], f["off_z"])
        elif name == "SetAttackCollSize" and f["attack_id"] in active:
            active[f["attack_id"]]["radius"] = f["size"] * 0.5
        elif name == "ClearAttackCollID":
            active.pop(f.get("value", -1), None)
        elif name == "ClearAttackCollAll":
            active.clear()
        elif name in ("End", "Goto", "Return"):
            break
    emit(frames)
    return out[:frames + 1]


def block_commands(rom, events: dict, start: str) -> list[str]:
    """A block's command texts (events.json "blocks", words from the ROM)."""
    out = []
    for cmd in events["blocks"][start]["commands"]:
        kind, *rest = cmd["at"].split(":")
        if kind == "file":
            fid, off = int(rest[0], 16), int(rest[1], 16)
            data = rom.file_bytes(fid)
            word = lambda k: int.from_bytes(data[off + 4 * k:off + 4 * k + 4], "big")  # noqa: E731
        else:
            addr = int(rest[0], 16)
            word = lambda k: rom.u32_ram(addr + 4 * k)  # noqa: E731
        n = length(word(0)) // 4
        out.append(disassemble([word(k) for k in range(n)], {}))
    return out


def assemble(text: str) -> tuple[list[int], dict[int, str]]:
    """A command's words from its text; pointer words are 0, their targets
    returned by word index (the container writes them)."""
    tokens = text.split()
    if not tokens or tokens[0] not in BY_NAME:
        raise ScriptError(f"unknown command in {text!r}")
    op, spec, remix = BY_NAME[tokens[0]]
    values = {}
    target = None
    rest = tokens[1:]
    i = 0
    while i < len(rest):
        t = rest[i]
        if t == "->":
            target = rest[i + 1]
            i += 2
            continue
        if "=" not in t:
            raise ScriptError(f"{tokens[0]}: field {t!r} has no value")
        key, value = t.split("=", 1)
        values[key] = int(value, 0)
        i += 1
    top_bits = 8 if remix else 6
    words, pointers = [], {}
    for k, spec_word in enumerate(spec):
        if any(b == "ptr" for _, b, _ in spec_word):
            if target is None:
                raise ScriptError(f"{tokens[0]}: no target")
            words.append(int(target[5:], 16) if target.startswith("word:") else 0)
            if not target.startswith("word:"):
                pointers[k] = target
            continue
        word = (op << 24) if (k == 0 and remix) else ((op << 26) if k == 0 else 0)
        bit = 32 - (top_bits if k == 0 else 0)
        for field, bits, signed in _fields_of(spec_word, top_bits if k == 0 else 0):
            bit -= bits
            key = f"{field}{k}" if field.startswith("pad") else field
            value = values.pop(key, 0)
            lo, hi = (-(1 << (bits - 1)), (1 << (bits - 1)) - 1) if signed else (0, (1 << bits) - 1)
            if not lo <= value <= hi:
                raise ScriptError(f"{tokens[0]}: {field}={value} outside {bits} bits")
            word |= (value & ((1 << bits) - 1)) << bit
        words.append(word)
    if values:
        raise ScriptError(f"{tokens[0]}: unknown fields {sorted(values)}")
    return words, pointers


def check(staging: Path, export_root: Path) -> int:
    """F3 over every export's decoded commands."""
    import remix_rom as R
    rom = R.Rom(staging / "ssb64asm.z64")
    slots_cache = {}

    def word_at(key: str, k: int) -> int:
        kind, *rest = key.split(":")
        if kind == "file":
            fid, off = int(rest[0], 16), int(rest[1], 16) + 4 * k
            return int.from_bytes(rom.file_bytes(fid)[off:off + 4], "big")
        return rom.u32_ram(int(rest[0], 16) + 4 * k)

    def target_of(key: str, k: int) -> str | None:
        kind, *rest = key.split(":")
        if kind == "file":
            fid, off = int(rest[0], 16), int(rest[1], 16) + 4 * k
            if fid not in slots_cache:
                slots_cache[fid] = rom.reloc_slots(fid)
            slot = slots_cache[fid].get(off)
            if slot is None:
                return "NULL" if word_at(key, k) == 0 else None
            return f"file:{fid:#x}:{slot[1]:#x}" if slot[0] == "intern" else f"file:{slot[1]:#x}:{slot[2]:#x}"
        value = word_at(key, k)
        return "NULL" if value == 0 else f"ram:{value:#010x}"

    total = bad = 0
    seen = set()
    for exp in sorted(p for p in export_root.iterdir() if (p / "events.json").exists()):
        blocks = json.loads((exp / "events.json").read_text(encoding="utf-8"))["blocks"]
        for block in blocks.values():
            for cmd in block["commands"]:
                at = cmd["at"]
                if at in seen:
                    continue
                seen.add(at)
                w0 = word_at(at, 0)
                n = length(w0) // 4
                words = [word_at(at, k) for k in range(n)]
                spec = (REMIX[w0 >> 24][1] if (w0 >> 24) >= 0xD0 else OPS[w0 >> 26][1])
                pointers = {k: target_of(at, k) for k, sw in enumerate(spec) if any(b == "ptr" for _, b, _ in sw)}
                text = disassemble(words, pointers)
                back, back_ptrs = assemble(text)
                same = all(back[k] == words[k] for k in range(n) if k not in pointers)
                same = same and all(back_ptrs.get(k) == pointers[k] for k in pointers if pointers[k] is not None)
                total += 1
                if not same or text.split()[0] != cmd["op"]:
                    bad += 1
                    if bad <= 10:
                        print(f"{exp.name} {at}: {text}\n  words {[hex(w) for w in words]} back {[hex(w) for w in back]}")
    print(f"F3: {total} commands, {bad} differ")
    return 1 if bad else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--check", nargs=2, type=Path, metavar=("STAGING", "EXPORT_ROOT"))
    ap.add_argument("--words", nargs="+", help="disassemble these words (hex)")
    ap.add_argument("--text", help="assemble this command")
    args = ap.parse_args()
    if args.check:
        return check(*args.check)
    if args.words:
        print(disassemble([int(w, 16) for w in args.words]))
        return 0
    if args.text:
        words, pointers = assemble(args.text)
        print(" ".join(f"{w:08x}" for w in words), pointers or "")
        return 0
    ap.print_help()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
