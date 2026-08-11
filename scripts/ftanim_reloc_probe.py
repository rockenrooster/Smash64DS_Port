#!/usr/bin/env python3
"""Read the figatree animation files the way the ROM reads them.

Slice 32's AOT generator needs the 301 o2r files in
`decomp/BattleShip-main/BattleShip_o2r/reloc_animations/` that the Makefile
stages into nitrofs with a bare `cp` (Makefile:3295). Nothing host-side parsed
them before this. This module is the front door: it reproduces the ROM's load
pipeline exactly and proves it by decoding every script in the bank.

THE PIPELINE, from `src/port/reloc_backend_assets.c`:

  1. 0x50-byte header. Its last 16 bytes are `NDSRelocAssetHeader`: `file_id`
     (+0x40), `reloc_intern_offset` (+0x44), `reloc_extern_offset` (+0x46),
     `extern_file_ids_num` (+0x48), `data_size` (+0x4c). `0x50 + data_size` is
     the file length exactly, on all 301.
  2. `ndsRelocApplyWordByteSwap` (:2956) -- the payload is big-endian N64 data;
     every 32-bit word is swapped to native.
  3. `ndsRelocApplyInternalPointerFixups` (:2986) -- a relocation list threaded
     through the data. `reloc_intern_offset` is a WORD index; the word there
     packs `next = w >> 16` and `target_words = w & 0xffff`; the slot becomes
     `data + target_words * 4`; repeat until `next == 0xffff`.
  4. `ndsRelocNormalizeFighterAObj16File` (:3310) -- the part that is impossible
     to guess:
       a. the entry table's length is *derived*, not stored: it is the smallest
          resolved pointer, i.e. the table runs until the first script starts;
       b. from there to the end, the two u16 lanes inside every 32-bit word are
          swapped back -- step 2's u32 swap is right for pointers and floats and
          exactly wrong for a stream of u16 commands;
       c. each script is then walked and every COMMAND word (never a payload or
          target word) is re-encoded from the disk bit order to the native one.

**Disk bit order is MSB-first and that is the whole trap.** On disk a command is
`opcode:5, flags:10, toggle:1` from the top -- `opcode = (w >> 11) & 0x1f`,
`flags = (w >> 1) & 0x3ff`, `toggle = w & 1` (:3278-3280). The native C bitfield
`{opcode:5; flags:10; toggle:1}` allocates from the BOTTOM, so `opcode = w &
0x1f`. Decoding disk bytes with the native layout is what eight successive
readings of this format got wrong here, every one of them landing at 38-43%: the
offset map, the word-scaled version of it, the lane swap alone, the exact
per-opcode advance, four variants of that advance, and an AObjEvent32 reading.
None of them moved the number, because none of them was the bug.

Getting the bit order right takes it to **100.0% of every script in all 297
AObj16 files**. The four that remain are not failures: `FTMarioAnim134/135`
(ids 633/634) and `FTFoxAnim135/136` (777/778) are precisely the four
`ndsRelocIsFighterAObj32Asset` names (0x279, 0x27a, 0x309, 0x30a) -- they carry
32-bit scripts and the generator must route them to the other decoder.

The advance rule below is transcribed from `ndsRelocAObj16CommandWords` (:3229)
rather than re-derived, since the ROM already owns it.
"""

from __future__ import annotations

import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
BANK = ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r" / "reloc_animations"

HDR_BYTES = 0x50
RELOC_END = 0xFFFF
OPCODE_MAX = 14                      # objdef.h:169-187; End == 0, SetFlags == 14
OP_END, OP_INTERP, OP_LOOP = 0, 12, 13

# reloc_backend_assets.c:3059-3074. These carry AObjEvent32 scripts.
AOBJ32_IDS = {0x279, 0x27A, 0x309, 0x30A}


def load(raw: bytes):
    """Header + payload with the u32 big-endian swap applied (pipeline 1-2)."""
    if len(raw) < HDR_BYTES or raw[4:8] != b"OLER":
        return None
    file_id, intern, extern, n_extern, size = struct.unpack_from(
        "<IHHII", raw, 0x40)
    if HDR_BYTES + size != len(raw):
        return None
    payload = bytearray(raw[HDR_BYTES:])
    for i in range(0, (size // 4) * 4, 4):
        payload[i:i + 4] = payload[i:i + 4][::-1]
    return {"file_id": file_id, "intern": intern, "extern": extern,
            "n_extern": n_extern, "size": size, "data": payload}


def fixups(f):
    """Pipeline 3: walk the threaded chain, returning {slot_off: target_off}."""
    out, slot_w = {}, f["intern"]
    guard = f["size"] // 4 + 1
    while slot_w != RELOC_END:
        if guard == 0:
            raise ValueError("relocation chain does not terminate")
        guard -= 1
        off = slot_w * 4
        if off + 4 > f["size"] or off in out:
            raise ValueError("relocation slot 0x%x out of range or looped" % off)
        word = struct.unpack_from("<I", f["data"], off)[0]
        nxt, target = word >> 16, (word & 0xFFFF) * 4
        if target >= f["size"]:
            raise ValueError("relocation target 0x%x past data" % target)
        out[off] = target
        slot_w = nxt
    return out


def normalize(f, fx):
    """Pipeline 4a-4b: derive the table bound, then unswap the script lanes."""
    table = min(fx.values())
    d = f["data"]
    for i in range(table, f["size"] - 3, 4):
        d[i:i + 2], d[i + 2:i + 4] = d[i + 2:i + 4], d[i:i + 2]
    return table


def command_words(opcode, flags, toggle):
    """ndsRelocAObj16CommandWords (:3229), verbatim."""
    if opcode in (OP_LOOP, OP_INTERP):
        return 2
    words = 1 + (1 if toggle else 0)
    flagged = bin(flags).count("1")
    if opcode in (2, 3, 6, 7, 8, 9, 10):
        words += flagged
    elif opcode in (4, 5):
        words += flagged * 2
    return words


def walk(data, size, off, limit=8192):
    """Walk one script in DISK bit order. 'end' or 'loop' means well-formed."""
    pc, seen = off, set()
    for _ in range(limit):
        if pc < 0 or pc + 2 > size:
            return "out of range at 0x%x" % pc
        if pc in seen:
            return "loop"                 # a cyclic animation; the parser's
        seen.add(pc)                      # frame budget ends it, not the script
        w = struct.unpack_from("<H", data, pc)[0]
        opcode = (w >> 11) & 0x1F
        flags = (w >> 1) & 0x3FF
        toggle = w & 1
        if opcode > OPCODE_MAX:
            return "undefined opcode %d at 0x%x" % (opcode, pc)
        if opcode == OP_END:
            return "end"
        if opcode == OP_LOOP:
            if pc + 4 > size:
                return "truncated Loop at 0x%x" % pc
            s = struct.unpack_from("<h", data, pc + 2)[0]
            half = -(abs(s) // 2) if s < 0 else (s // 2)   # C truncates to zero
            pc = pc + 2 + half * 2
            continue
        pc += 2 * command_words(opcode, flags, toggle)
    return "no terminator in %d commands" % limit


def decode_script(data, size, off, limit=8192):
    """Decode one script into structured commands.

    Yields dicts with the fields the semantic model needs, which is strictly
    more than `walk` needs: the payload word when `toggle` is set, and the
    per-track TARGET words. Those targets are the part `ftanim_script_model.py`
    originally approximated with the payload -- real commands carry a separate
    value per selected track, and `SetValRate{,Block}` carries two.

    Raises on a malformed script rather than returning a partial decode; the
    bank is proven well-formed, so a raise here means the reader broke.
    """
    out, pc, seen = [], off, set()
    for _ in range(limit):
        if pc < 0 or pc + 2 > size:
            raise ValueError("script ran off the end at 0x%x" % pc)
        if pc in seen:
            out.append({"pc": pc, "op": OP_LOOP, "flags": 0, "payload": None,
                        "targets": [], "jump": pc, "cyclic": True})
            return out
        seen.add(pc)
        w = struct.unpack_from("<H", data, pc)[0]
        op, flags, toggle = (w >> 11) & 0x1F, (w >> 1) & 0x3FF, w & 1
        if op > OPCODE_MAX:
            raise ValueError("undefined opcode %d at 0x%x" % (op, pc))

        cmd = {"pc": pc, "op": op, "flags": flags, "payload": None,
               "targets": [], "jump": None, "cyclic": False}
        if op == OP_END:
            out.append(cmd)
            return out
        if op in (OP_LOOP, OP_INTERP):
            s = struct.unpack_from("<h", data, pc + 2)[0]
            half = -(abs(s) // 2) if s < 0 else (s // 2)
            cmd["jump"] = pc + 2 + half * 2
            out.append(cmd)
            if op == OP_INTERP:
                pc += 4
                continue
            pc = cmd["jump"]
            continue

        p = pc + 2
        if toggle:
            cmd["payload"] = struct.unpack_from("<H", data, p)[0]
            p += 2
        per = 2 if op in (4, 5) else (1 if op in (2, 3, 6, 7, 8, 9, 10) else 0)
        if per:
            for bit in range(10):
                if not (flags >> bit):
                    break
                if (flags >> bit) & 1:
                    vals = struct.unpack_from("<%dh" % per, data, p)
                    cmd["targets"].append((bit, vals))
                    p += 2 * per
        out.append(cmd)
        pc = p
    raise ValueError("no terminator in %d commands" % limit)


def scripts_in(path):
    """Every decoded script in one bank file, or [] for an AObj32 file."""
    f = load(path.read_bytes())
    if f is None or f["file_id"] in AOBJ32_IDS:
        return []
    fx = fixups(f)
    if not fx:
        return []
    normalize(f, fx)
    return [decode_script(f["data"], f["size"], t)
            for t in sorted(set(fx.values()))]


def main() -> int:
    if not BANK.is_dir():
        print("SKIP: %s absent -- run scripts/fetch-battleship-reference.ps1"
              % BANK)
        return 0

    paths = sorted(p for p in BANK.iterdir()
                   if p.name.startswith(("FTMarioAnim", "FTFoxAnim")))
    if not paths:
        print("FAIL: no FTMarioAnim*/FTFoxAnim* under %s" % BANK)
        return 1

    scripts = clean = 0
    aobj16 = aobj32 = 0
    failures = []
    for path in paths:
        f = load(path.read_bytes())
        if f is None:
            failures.append((path.name, "no RELO header / size mismatch"))
            continue
        if f["file_id"] in AOBJ32_IDS:
            aobj32 += 1
            continue
        aobj16 += 1
        fx = fixups(f)
        if not fx:
            failures.append((path.name, "no relocated entries"))
            continue
        normalize(f, fx)
        for _slot, target in sorted(fx.items()):
            scripts += 1
            status = walk(f["data"], f["size"], target)
            if status in ("end", "loop"):
                clean += 1
            elif len(failures) < 12:
                failures.append((path.name, status))

    rate = (clean / scripts) if scripts else 0.0
    print("AObjEvent16 files : %d" % aobj16)
    print("AObjEvent32 files : %d  (ids %s -- routed to the other decoder)"
          % (aobj32, ", ".join("0x%x" % i for i in sorted(AOBJ32_IDS))))
    print("scripts walked    : %d" % scripts)
    print("scripts well-formed : %d (%.1f%%)" % (clean, 100 * rate))
    if failures:
        print("\nfailures (%d shown):" % len(failures))
        for name, why in failures[:12]:
            print("   %-20s %s" % (name, why))

    if rate < 1.0:
        print("\nFAIL: every AObj16 script in this bank decoded before. A "
              "script that no longer does means the reader or the bank moved.")
        return 1
    print("\nFTANIM_RELOC_FORMAT=SOLVED  header, u32 big-endian swap, threaded "
          "internal fixups, derived table bound, script-region lane unswap, and "
          "MSB-first command bits -- every script in all %d AObj16 files walks "
          "to End or a Loop. The generator can read the bank." % aobj16)
    return 0


if __name__ == "__main__":
    sys.exit(main())
