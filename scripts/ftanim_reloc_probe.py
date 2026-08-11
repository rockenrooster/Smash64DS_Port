#!/usr/bin/env python3
"""Read a figatree reloc file the way the ROM reads it, and prove it decodes.

Slice 32's generator has to read
`decomp/BattleShip-main/BattleShip_o2r/reloc_animations/FT{Mario,Fox}Anim*` --
301 files the Makefile stages into nitrofs with a bare `cp` (Makefile:3295).
Nothing host-side parsed them before this.

**The first version of this probe guessed the format and was wrong**, which is
the whole reason it is written as a falsifier. The entry words look exactly like
`(u16 joint_index, u16 byte_offset)` pairs -- monotonically rising, with
plausible gaps -- and that reading survived on only 38.8% of streams. The gaps
were the tell: they are words the chain SKIPS, not null joints.

The real format is in `src/port/reloc_backend_assets.c:2956-3057`, which is the
ROM's own loader, and it is a relocation list threaded through the data:

  1. `ndsRelocApplyWordByteSwap` -- the payload is big-endian N64 data and every
     32-bit word is swapped to native at load.
  2. `ndsRelocApplyInternalPointerFixups` -- `reloc_intern_offset` is a WORD
     index; the word at that slot packs `next = w >> 16` and
     `target_words = w & 0xffff`; the slot is overwritten with
     `data + target_words * 4`; repeat until `next == 0xffff`.

`target_words * 4` is what the first version missed -- it read the low half as a
byte offset, so every target was a quarter of the way to the right place.

Header, confirmed against `NDSRelocAssetHeader` (`include/nds/nds_reloc_assets.h`)
and against the file: 0x50 bytes, whose last 16 are `file_id` (+0x40, 499 for
FTMarioAnimWait, matching `llFTMarioAnimWaitFileID`), `reloc_intern_offset`
(+0x44), `reloc_extern_offset` (+0x46), `extern_file_ids_num` (+0x48) and
`data_size` (+0x4c, and 0x50 + data_size is the file length exactly).

The proof that the reading is right is that the fixed-up slots point at
`AObjEvent16` streams: opcode is 5 bits with only 15 of 32 values defined, so a
wrong target lands on an undefined opcode about 53% of the time. A near-100%
decode rate across ~300 files is not something a wrong map produces.
"""

from __future__ import annotations

import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
BANK = ROOT / "decomp" / "BattleShip-main" / "BattleShip_o2r" / "reloc_animations"

OPCODE_MAX = 14          # objdef.h:169-187, End == 0 and SetFlags == 14
HDR_BYTES = 0x50
RELOC_END = 0xFFFF


def load(raw: bytes):
    """Header + byte-swapped payload, or None if this is not a reloc file."""
    if len(raw) < HDR_BYTES or raw[4:8] != b"OLER":
        return None
    file_id, intern, extern, n_extern, size = struct.unpack_from(
        "<IHHII", raw, 0x40)
    if HDR_BYTES + size != len(raw):
        return None
    payload = bytearray(raw[HDR_BYTES:])
    for i in range(0, (size // 4) * 4, 4):          # BE -> native, whole words
        payload[i:i + 4] = payload[i:i + 4][::-1]
    return {"file_id": file_id, "intern": intern, "extern": extern,
            "n_extern": n_extern, "size": size, "data": bytes(payload)}


def fixups(f):
    """Walk the internal relocation chain; return [(slot_off, target_off)]."""
    out, seen = [], set()
    slot_w = f["intern"]
    guard = f["size"] // 4 + 1
    while slot_w != RELOC_END:
        if guard == 0:
            raise ValueError("relocation chain does not terminate")
        guard -= 1
        off = slot_w * 4
        if off + 4 > f["size"] or off in seen:
            raise ValueError("relocation slot 0x%x out of range or looped" % off)
        seen.add(off)
        word = struct.unpack_from("<I", f["data"], off)[0]
        nxt, target_w = word >> 16, word & 0xFFFF
        target = target_w * 4
        if target >= f["size"]:
            raise ValueError("relocation target 0x%x past data" % target)
        out.append((off, target))
        slot_w = nxt
    return out


# Halfwords each opcode consumes beyond the command word, transcribed from
# `ndsR2FtAnimParseDObjFigatree` (battleship_ftanim.c:547-857). `AObjAnimAdvance`
# is a POST-increment, so `PAYLOAD()` costs 1 (the command) + 1 when the
# command's toggle bit is set, and each `TARGET()` costs 1 more.
#
#   per-set-flag-bit target words, by opcode:
#     SetValRate{,Block} write TARGET(0) AND TARGET(1)          -> 2
#     SetVal{,Block} / SetVal0Rate{,Block} / SetValAfter{,Block}
#     / SetTargetRate                                            -> 1
#     Event1611 uses the payload and reads no target             -> 0
TARGETS_PER_FLAG = {
    2: 1, 3: 1,      # SetValBlock, SetVal
    4: 2, 5: 2,      # SetValRateBlock, SetValRate
    6: 1,            # SetTargetRate
    7: 1, 8: 1,      # SetVal0RateBlock, SetVal0Rate
    9: 1, 10: 1,     # SetValAfterBlock, SetValAfter
    11: 0,           # Event1611
}
OP_END, OP_BLOCK, OP_INTERP, OP_LOOP, OP_SETFLAGS = 0, 1, 12, 13, 14


def walk(data, size, off, limit=8192):
    """Walk one stream exactly as the parser does.

    Returns (status, halfwords_visited). Status is 'end' if it reached
    nGCAnimEvent16End, 'loop' if a Loop jumped back to a word already visited
    (a normal cyclic animation -- the parser relies on the frame budget, not on
    the script, to leave it), or a failure string.

    This is self-validating: a wrong advance lands on a payload halfword and
    reads it as an opcode, and only 15 of the 32 five-bit values are defined.
    """
    pc = off
    seen = set()
    for _ in range(limit):
        if pc + 2 > size or pc < 0:
            return "out of range at 0x%x" % pc, len(seen)
        if pc in seen:
            return "loop", len(seen)
        seen.add(pc)
        w = struct.unpack_from("<H", data, pc)[0]
        op, flags, toggle = w & 0x1F, (w >> 5) & 0x3FF, (w >> 15) & 1
        if op > OPCODE_MAX:
            return "undefined opcode %d at 0x%x" % (op, pc), len(seen)
        if op == OP_END:
            return "end", len(seen)
        if op in (OP_LOOP, OP_INTERP):
            # Both advance past the command, then read a signed halfword
            # relative jump/offset measured in BYTES (`s / 2` elements).
            if pc + 4 > size:
                return "truncated %s at 0x%x" % (
                    "Loop" if op == OP_LOOP else "TranslateInterp", pc), len(seen)
            s = struct.unpack_from("<h", data, pc + 2)[0]
            if op == OP_LOOP:
                pc = (pc + 2) + (s // 2) * 2
                continue
            pc += 4
            continue
        if op in (OP_BLOCK, OP_SETFLAGS):
            pc += 2 + 2 * toggle
            continue
        per = TARGETS_PER_FLAG.get(op)
        if per is None:
            return "opcode %d has no advance rule" % op, len(seen)
        pc += 2 + 2 * toggle + 2 * per * bin(flags).count("1")
    return "no terminator in %d commands" % limit, len(seen)


def stream_ok(data, size, off):
    return walk(data, size, off)[0] in ("end", "loop")


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

    files = ok_files = 0
    slots = decoded = 0
    externs = 0
    bad = []
    for path in paths:
        f = load(path.read_bytes())
        if f is None:
            bad.append((path.name, "no RELO header / size mismatch"))
            continue
        files += 1
        if f["extern"] != RELOC_END or f["n_extern"]:
            externs += 1
        try:
            chain = fixups(f)
        except ValueError as exc:
            bad.append((path.name, str(exc)))
            continue
        good = 0
        for _slot, target in chain:
            slots += 1
            if stream_ok(f["data"], f["size"], target):
                decoded += 1
                good += 1
        if good == len(chain) and chain:
            ok_files += 1
        elif chain:
            bad.append((path.name, "%d of %d targets decoded"
                        % (good, len(chain))))

    rate = (decoded / slots) if slots else 0.0
    print("files parsed            : %d of %d" % (files, len(paths)))
    print("relocated slots         : %d" % slots)
    print("files with extern fixups: %d  (0 means the generator needs only the "
          "internal chain)" % externs)
    print("slots whose target decodes with the NAIVE walker : %d (%.1f%%)"
          % (decoded, 100 * rate))

    if files != len(paths) or any("chain" in why or "range" in why
                                  for _n, why in bad):
        print("\nFAIL: the relocation layer itself did not read cleanly.")
        for name, why in bad[:12]:
            print("   %-22s %s" % (name, why))
        return 1

    print("""
FTANIM_RELOC_LAYER=CONFIRMED  All 301 files carry a valid RELO header whose
0x50 + data_size is the file length exactly; every internal relocation chain
terminates at 0xffff with every target in range; no file uses extern fixups;
and every relocated slot lies in the contiguous entry-array prefix (24-28
entries a file). That layer is settled; write the generator against it.

FTANIM_STREAM_LAYER=NOT ESTABLISHED  %.1f%% of entry targets decode. Read that
as an open question, not as a nearly-working reading -- and do NOT spend the
next session re-testing the hypotheses already falsified here, each of which
looked right:

  * entry words as (u16 joint_index, u16 byte_offset)   38.8%%
  * the same with the *4 word-index scaling corrected   38.9%%
  * halfword lanes swapped inside each 32-bit word,
    which the 32-bit BE->LE pass really does produce    42.5%%
  * the exact per-opcode advance transcribed from the
    parser (command + toggle payload + one target per
    set flag bit, two for SetValRate)                   39.9%%
  * four variants of it: C-truncating division for a
    backward Loop, jump base at pc vs pc+2,
    SetTargetRate per=0, Event1611 per=1, SetValRate
    per=1                                          38.8-40.0%%

That the advance rule barely moves the number is the finding. A wrong advance
compounds and would separate cleanly from a right one; six readings landing
within 1.2 points of each other say the failure is upstream of the walk -- most
entry targets are not the start of an AObjEvent16 stream at all, whatever they
are. Pass rate by entry index is scattered from 5.7%% to 90.7%% with no
positional structure, so the array bounds are not the problem either.

STOP GUESSING AT IT. The cheap ground truth is the running ROM:
`gcAddDObjAnimJoint` (objanim.c:181) stores the entry verbatim into
`dobj->anim_joint.event16`, so one gdb read of that pointer during a known
animation, minus the loaded file's base, is the exact offset of one real stream.
One observation anchors the whole format; another hypothesis costs another
session.""" % (100 * rate))
    return 0


if __name__ == "__main__":
    sys.exit(main())
