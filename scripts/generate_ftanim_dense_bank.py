#!/usr/bin/env python3
"""Emit the dense fighter-animation bank, and prove it round-trips.

Slice 32's emitter. Reads the 297 AObj16 files in
`decomp/BattleShip-main/BattleShip_o2r/reloc_animations/`, resolves each script
through the semantic model and the bake, and writes a flat binary the runtime
can stream instead of walking 36-byte `AObj` nodes by pointer.

WHY THIS EXISTS, in one number: `ldrb r5,[r4,#5]` -- `aobj->kind` -- is
14,616,804 cycles, **21.5% of `gcPlayDObjAnimJoint`**, at 25.64 cyc/insn over
570,065 executions. With `aobj->next` the bare list walk is 25.3% of the largest
animation symbol in the build. 335 nodes a frame at 36 bytes is 12,060 bytes
through a 4 KB D-cache; cycle 109's contiguous pool bought the second cache line
per node and, as its own comment predicted, could not buy residency. Only a
smaller node does.

**The value fields are exact, which was a measurement result and not a choice.**
`ndsR2AnimTargetValue` scales each disk word by a per-track power of two, so the
stored Q12 values need up to 24 bits (translation) and cannot be s16. Storing
the *authored* s16 word instead, with the group's shift applied at runtime, is
exact -- where re-quantising to a coarser Q would have discarded fractional bits
of every value. `length` is exact too, at s16 Q7.

The record is NOT lossless overall, and `--verify` prints what it costs rather
than letting the relaxed comparison absorb it: `rate_base` at Q16 is off by at
most 7.3e-06 (0.0011% relative) and `length_invert` at Q30 by 4.6e-10
(0.000006%). Both are the widths the shipped runtime already stores these in --
`NDS_R2_AQ_RF` 16 and `NDS_R2_AQ_IF` 30 -- so the dense bank inherits the Q form's
existing error rather than adding a second one.

Nothing here is assumed. Every field is range-checked against its declared
encoding as it is emitted, and `--verify` decodes the blob back and compares it
to the bake record for record. An emitter that silently saturated one joint of
one animation would be invisible in a screenshot and is exactly the failure this
has to make loud.
"""

from __future__ import annotations

import argparse
import importlib.util
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

MAGIC = b"FTAD"
VERSION = 1

# One write record, 20 bytes, 4-byte aligned.
#   s32 rate_base      Q16 -- see below
#   s32 length_invert  Q30 for Cubic/Linear, plain integer for Step -- see below
#   u8  cmd_index      which command of the script applies this write
#   u8  track_kind     track in the low nibble, AObj kind in the high nibble
#   s16 value_base, value_target, rate_target   authored disk words, exact
#   s16 length         Q7   (measured -185..0 across the whole bank)
#   u16 pad            keeps the two s32 fields 4-byte aligned at every stride
#
# `length_invert` was sized at s16 Q8 from its measured range of 0..64, and the
# round-trip immediately rejected that: for a Cubic the field is `1.0 / payload`
# -- a RECIPROCAL -- and 1/17 at Q8 is 15/256, off by 0.4%. Range was the wrong
# question; a rate needs precision. The parser's own comment records the field's
# double meaning: Step stores a FRAME COUNT there, everything else a reciprocal.
# So it is s32 and read per kind, exactly as the runtime does -- `NDS_R2_AQ_IF`
# is 30 and `ndsR2AnimRecipSlot` writes `ndsR2F32ToFixed(r, 30)`.
#
# `rate_base` is the one field that cannot be s16, and the emitter found that
# rather than the design assuming it. It is an authored word for the cubic
# families but `SetVal{,Block}` COMPUTES it as
# `(value_target - value_base) / payload`: 753 of 145,873 records (0.5%) are
# fractional, spanning -614.25..1064.80. s16 Q4 is the only Q that fits that
# magnitude and it carries up to 81% relative error on the small values, which
# is a visibly wrong interpolation rate, not a rounding detail.
#
# So the field is s32 at Q16 -- which is not a new approximation but exactly
# what the shipped runtime already stores: `NDS_R2_AQ_RF` is 16, and the Q arm
# of the parser writes `ndsR2AQStore(r)` with `r` the rounded Q16 ratio.
#
# The record therefore costs 20 bytes, not the 14 the first sizing predicted --
# both widenings were forced by the round-trip, not chosen. It is still 44.4%
# smaller than a 36-byte `AObj`, taking 335 nodes a frame from 12,060 bytes to
# 6,700 against a 4 KB D-cache: 1.64x over rather than 3.0x.
RECORD = struct.Struct("<iiBBhhhhH")
assert RECORD.size == 20

LENGTH_Q = 7
LENGTH_INVERT_Q = 30             # NDS_R2_AQ_IF, for Cubic/Linear reciprocals
RATE_BASE_Q = 16                 # NDS_R2_AQ_RF
S16_MIN, S16_MAX = -32768, 32767
S32_MIN, S32_MAX = -(1 << 31), (1 << 31) - 1
KIND_STEP_Q, KIND_STEP_F = 5, 1  # NDS_R2_AQ_KIND_STEP / nGCAnimKindStep


def _load(name):
    spec = importlib.util.spec_from_file_location(
        name, ROOT / "scripts" / ("%s.py" % name))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class FieldOverflow(RuntimeError):
    """A value did not fit its declared encoding. Never saturate; fail."""


def _fixed(value, q, what, where):
    scaled = int(round(float(value) * (1 << q)))
    if not (S16_MIN <= scaled <= S16_MAX):
        raise FieldOverflow(
            "%s = %r does not fit s16 at Q%d (%d) in %s -- the measured range "
            "this encoding was sized from no longer holds"
            % (what, value, q, scaled, where))
    return scaled


def _word(value, what, where):
    ivalue = int(value)
    if ivalue != value:
        raise FieldOverflow("%s = %r is not an integer in %s; it was expected "
                            "to be an authored s16 disk word"
                            % (what, value, where))
    if not (S16_MIN <= ivalue <= S16_MAX):
        raise FieldOverflow("%s = %d does not fit s16 in %s"
                            % (what, ivalue, where))
    return ivalue


def encode_write(cmd_index, track, snap, where):
    """One baked snapshot -> one 20-byte record."""
    kind, vb, vt, rb, rt, length, linv, _interp = snap
    if not (0 <= track <= 15) or not (0 <= int(kind) <= 15):
        raise FieldOverflow("track %d / kind %r out of nibble range in %s"
                            % (track, kind, where))
    if cmd_index > 255:
        raise FieldOverflow("command index %d exceeds u8 in %s"
                            % (cmd_index, where))
    rate_base = int(round(float(rb) * (1 << RATE_BASE_Q)))
    if not (S32_MIN <= rate_base <= S32_MAX):
        raise FieldOverflow("rate_base = %r does not fit s32 at Q%d in %s"
                            % (rb, RATE_BASE_Q, where))
    # Step keeps a frame count here; everything else keeps a Q30 reciprocal.
    is_step = int(kind) in (KIND_STEP_Q, KIND_STEP_F)
    linv_raw = int(linv) if is_step else int(round(float(linv) * (1 << LENGTH_INVERT_Q)))
    if not (S32_MIN <= linv_raw <= S32_MAX):
        raise FieldOverflow("length_invert = %r does not fit s32 in %s"
                            % (linv, where))
    return RECORD.pack(
        rate_base, linv_raw, cmd_index, (int(kind) << 4) | track,
        _word(vb, "value_base", where), _word(vt, "value_target", where),
        _word(rt, "rate_target", where),
        _fixed(length, LENGTH_Q, "length", where), 0)


def build(probe, model, bake, on_script=None):
    """Emit every script in the bank. Returns (blob, index, stats)."""
    blob = bytearray()
    index = []                 # (file_id, script_ordinal, offset, count)
    stats = {"files": 0, "scripts": 0, "records": 0, "linear_rate_base": 0}

    for path in sorted(probe.BANK.iterdir()):
        if not path.name.startswith(("FTMarioAnim", "FTFoxAnim")):
            continue
        scripts = probe.scripts_in(path)
        if not scripts:
            continue
        f = probe.load(path.read_bytes())
        stats["files"] += 1
        for ordinal, cmds in enumerate(scripts):
            run = model.run_commands(cmds)
            baked = bake.bake_run(run)
            where = "%s script %d" % (path.name, ordinal)
            start = len(blob)
            for cmd_index, track, snap in baked["writes"]:
                if not float(snap[3]).is_integer():
                    stats["linear_rate_base"] += 1
                blob += encode_write(cmd_index, track, snap, where)
            index.append((f["file_id"], ordinal, start,
                          len(baked["writes"])))
            stats["scripts"] += 1
            stats["records"] += len(baked["writes"])
            if on_script is not None:
                on_script(where, baked, start)
    return bytes(blob), index, stats


def decode_write(blob, offset):
    (rb, linv, cmd, tk, vb, vt, rt, length,
     _pad) = RECORD.unpack_from(blob, offset)
    kind = tk >> 4
    linv_v = float(linv) if kind in (KIND_STEP_Q, KIND_STEP_F)         else linv / (1 << LENGTH_INVERT_Q)
    return (cmd, tk & 0xF, kind, vb, vt, rb / (1 << RATE_BASE_Q), rt,
            length / (1 << LENGTH_Q), linv_v)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=pathlib.Path)
    ap.add_argument("--verify", action="store_true",
                    help="decode the blob back and compare to the bake")
    args = ap.parse_args()

    probe = _load("ftanim_reloc_probe")
    if not probe.BANK.is_dir():
        print("SKIP: %s absent -- run scripts/fetch-battleship-reference.ps1"
              % probe.BANK)
        return 0
    model = _load("ftanim_script_model")
    bake = _load("ftanim_bake")

    mismatches = []
    worst = {"rate_base": (0.0, 0, 0, ""),
             "length": (0.0, 0, 0, ""),
             "length_invert": (0.0, 0, 0, "")}

    def check(where, baked, start):
        if not args.verify:
            return
        for i, (cmd_index, track, snap) in enumerate(baked["writes"]):
            got = decode_write(blob_ref[0], start + i * RECORD.size)
            is_step = int(snap[0]) in (KIND_STEP_Q, KIND_STEP_F)
            linv_q = (float(int(snap[6])) if is_step else
                      round(float(snap[6]) * (1 << LENGTH_INVERT_Q))
                      / (1 << LENGTH_INVERT_Q))
            want = (cmd_index, track, int(snap[0]), int(snap[1]), int(snap[2]),
                    round(float(snap[3]) * (1 << RATE_BASE_Q))
                    / (1 << RATE_BASE_Q),
                    int(snap[4]), float(snap[5]), linv_q)
            if got != want and len(mismatches) < 5:
                mismatches.append((where, i, want, got))
            # Report the quantisation against the MODEL separately, so the loss
            # the encoding does accept is visible rather than absorbed by the
            # comparison that was just relaxed to permit it.
            for name, exact, coded in (("rate_base", snap[3], got[5]),
                                       ("length", snap[5], got[7]),
                                       ("length_invert", snap[6], got[8])):
                err = abs(float(exact) - float(coded))
                if err > worst[name][0]:
                    worst[name] = (err, float(exact), float(coded), where)

    blob_ref = [b""]
    try:
        blob, index, stats = build(probe, model, bake)
    except FieldOverflow as exc:
        print("FAIL: %s" % exc)
        return 1
    blob_ref[0] = blob
    if args.verify:
        build(probe, model, bake, on_script=check)

    aobj_bytes = stats["records"] * 36
    print("files            : %d" % stats["files"])
    print("scripts          : %d" % stats["scripts"])
    print("records          : %d" % stats["records"])
    print("blob             : %d bytes (%.2f MB)"
          % (len(blob), len(blob) / (1024 * 1024)))
    print("same as 36-byte AObj records would be : %d bytes (%.2f MB), so %.1f%% "
          "smaller" % (aobj_bytes, aobj_bytes / (1024 * 1024),
                       100 * (1 - len(blob) / aobj_bytes)))
    print("index entries    : %d" % len(index))
    print("non-integer rate_base (Linear, computed not authored) : %d"
          % stats["linear_rate_base"])

    if args.verify:
        print("verify           : %d mismatches" % len(mismatches))
        print("quantisation against the model (the loss this encoding "
              "accepts):")
        for name in ("rate_base", "length", "length_invert"):
            err, exact, coded, where = worst[name]
            rel = (100 * err / abs(exact)) if exact else 0.0
            print("   %-14s max abs %.3e  rel %.6f%%  (%r -> %r, %s)"
                  % (name, err, rel, exact, coded, where or "none"))
        for where, i, want, got in mismatches:
            print("   %s write %d\n      want %s\n      got  %s"
                  % (where, i, want, got))
        if mismatches:
            print("FAIL: the emitted blob does not decode back to the bake.")
            return 1

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        header = struct.pack("<4sIII", MAGIC, VERSION, len(index),
                             stats["records"])
        table = b"".join(struct.pack("<IHII", *e) for e in index)
        args.out.write_bytes(header + table + blob)
        print("wrote %s (%d bytes with header and index)"
              % (args.out, len(header) + len(table) + len(blob)))

    print("\nFTANIM_DENSE_BANK=OK  every record fits its declared encoding, the "
          "value fields carry the authored s16 words exactly, and the two "
          "fixed-point fields inherit the widths the runtime already uses")
    return 0


if __name__ == "__main__":
    sys.exit(main())
