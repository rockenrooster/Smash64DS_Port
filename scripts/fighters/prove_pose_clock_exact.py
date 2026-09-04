#!/usr/bin/env python3
"""Prove an integer-only float32 add reproduces the source pose clock exactly.

test_pose_clock_differential.py showed that no fixed-point precision makes
the port's Q clock cross command boundaries on the source's ticks: the source
runs `anim_wait -= anim_speed` / `anim_wait += payload` in binary32, and the
rounding position moves with the wait's magnitude. The only clock that lands
every boundary on the same tick is one that rounds the way binary32 does.

This script is the host proof for that clock before it is written in C. It
implements binary32 addition with nothing but integer operations -- unpack,
align with guard/round/sticky, add or subtract magnitudes, normalise with a
leading-zero count, round to nearest even, repack -- restricted to the operand
class the clock produces (finite normals and zero; no NaN, infinity or
subnormal, which the chain cannot reach), and runs the differential's whole
case set through a chain that uses ONLY that add, asserting bit equality with
the reference float32 chain on every operation, every command boundary, every
End tick and every interrupt threshold.

It is the oracle for the C kernel: the ARM9 port of `exact_f32_add` below has
to agree with it bit for bit, and the ARM9 has CLZ (ARMv5TE), so the
normalise step is one instruction in ARM state.

Exit status: 0 when every case and every operation agrees, 1 otherwise.
"""
import argparse
import pathlib
import struct
import sys

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import test_pose_clock_differential as diff  # noqa: E402

F32_SIGN = 0x80000000
F32_EXP_MASK = 0x7f800000
F32_MANT_MASK = 0x007fffff
F32_HIDDEN = 0x00800000
GRS = 3  # guard, round, sticky bits kept below the 24-bit significand


def bits_of(x: float) -> int:
    return struct.unpack("<I", struct.pack("<f", x))[0]


def float_of(b: int) -> float:
    return struct.unpack("<f", struct.pack("<I", b & 0xffffffff))[0]


def exact_f32_add(a: int, b: int) -> int:
    """binary32 a + b on bit patterns, integer ops only, round to nearest even.

    Operand class: finite normal or zero. The C kernel makes the same
    assumption; the clock's waits are integer frame counts and sums of a
    positive speed, and its speeds are normal floats, so nothing else can
    appear. Zero handling matters: a wait that lands exactly on 0 is the
    boundary the parser keys on.
    """
    if (a & ~F32_SIGN) == 0:
        return b if (b & ~F32_SIGN) != 0 else (a & b)  # -0 + -0 = -0, else +0
    if (b & ~F32_SIGN) == 0:
        return a
    sa, sb = a >> 31, b >> 31
    ea, eb = (a >> 23) & 0xff, (b >> 23) & 0xff
    ma = ((a & F32_MANT_MASK) | F32_HIDDEN) << GRS
    mb = ((b & F32_MANT_MASK) | F32_HIDDEN) << GRS
    # align on the larger exponent; the shifted-out bits fold into sticky
    if ea < eb:
        sa, sb, ea, eb, ma, mb = sb, sa, eb, ea, mb, ma
    d = ea - eb
    if d:
        if d >= 27:
            mb = 1  # entirely below guard: only stickiness survives
        else:
            sticky = 1 if (mb & ((1 << d) - 1)) else 0
            mb = (mb >> d) | sticky
    e = ea
    if sa == sb:
        m = ma + mb
        s = sa
        if m & (1 << (24 + GRS)):          # carry out: renormalise right
            m = (m >> 1) | (m & 1)
            e += 1
    else:
        if ma >= mb:
            m = ma - mb
            s = sa
        else:
            m = mb - ma
            s = sb
        if m == 0:
            return 0                       # exact cancellation is +0 (RNE)
        # normalise left: leading one back to bit 23+GRS
        while not (m & (1 << (23 + GRS))):
            m <<= 1
            e -= 1
    assert e > 0, "subnormal result: outside the clock's operand class"
    # round to nearest even on the GRS bits
    grs = m & ((1 << GRS) - 1)
    m >>= GRS
    half = 1 << (GRS - 1)
    if grs > half or (grs == half and (m & 1)):
        m += 1
        if m == (1 << 24):
            m >>= 1
            e += 1
    assert e < 0xff, "overflow: outside the clock's operand class"
    return (s << 31) | (e << 23) | (m & F32_MANT_MASK)


class Mismatch(Exception):
    pass


def checked_add(a: float, b: float, stats) -> float:
    """The add the exact chain uses: integer kernel, asserted against the
    reference on every call."""
    ref = diff.f32_add(a, b)
    got = float_of(exact_f32_add(bits_of(a), bits_of(b)))
    stats["ops"] += 1
    if bits_of(ref) != bits_of(got):
        stats["op_mismatch"] += 1
        raise Mismatch("%r + %r: ref %08x got %08x" % (a, b, bits_of(ref), bits_of(got)))
    return got


def exact_chain(speed: float, waits, stats, thresholds=(diff.INTERRUPT_BEGIN,)):
    """diff.float_chain, operation for operation, through checked_add."""
    wait = 0.0
    frame = 0.0
    idx = 0
    ticks = []
    thr = {}
    n = len(waits)
    while wait <= 0.0:
        if idx == n:
            return ticks, 0, thr
        wait = checked_add(wait, float(waits[idx]), stats)
        ticks.append(0)
        idx += 1
    if speed <= 0.0:
        return ticks, None, thr
    limit = min(diff.TICK_CAP, int(sum(waits) / speed) + 16)
    for tick in range(1, limit + 1):
        wait = checked_add(wait, -speed, stats)
        frame = checked_add(frame, speed, stats)
        for t in thresholds:
            if t not in thr and frame >= float(t):
                thr[t] = tick
        if wait > 0.0:
            continue
        while wait <= 0.0:
            if idx == n:
                return ticks, tick, thr
            wait = checked_add(wait, float(waits[idx]), stats)
            ticks.append(tick)
            idx += 1
    return ticks, None, thr


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--random", type=int, default=500)
    ap.add_argument("--no-bank", action="store_true")
    args = ap.parse_args(argv)

    probe = None if args.no_bank else diff.load_probe()
    speeds = diff.speed_table()
    notes = []
    streams = diff.real_streams(probe, notes) + diff.synthetic_streams(args.random)
    cases = [(s, st, k) for s in speeds for st in streams
             for k in [diff.pairing_kind(s, st)] if k is not None]

    stats = {"ops": 0, "op_mismatch": 0}
    chain_mismatch = 0
    unfinished = 0
    first_bad = None
    for s, st, kind in cases:
        f = diff.float_chain(s["value"], st["waits"])
        if f[1] is None:
            unfinished += 1
            continue
        try:
            x = exact_chain(s["value"], st["waits"], stats)
        except Mismatch as m:
            chain_mismatch += 1
            first_bad = first_bad or (s["label"], st.get("label", "?"), str(m))
            continue
        if x != f:
            chain_mismatch += 1
            first_bad = first_bad or (s["label"], st.get("label", "?"), "chain %r vs %r" % (x[1], f[1]))

    print("cases %d (unfinished %d skipped); binary32 operations %d; operation mismatches %d; chain mismatches %d"
          % (len(cases), unfinished, stats["ops"], stats["op_mismatch"], chain_mismatch))
    if first_bad:
        print("first mismatch:", first_bad)
        print("POSE_CLOCK_EXACT_FAIL")
        return 1
    print("POSE_CLOCK_EXACT_OK every boundary, End and threshold tick agrees with the float32 source chain")
    return 0


if __name__ == "__main__":
    sys.exit(main())
