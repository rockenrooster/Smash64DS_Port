#!/usr/bin/env python3
"""Prove `ndsR2S32ToF32Bits` equals `__aeabi_i2f` over all 2^32 inputs.

The cubic kernel's last integer->float call was `(f32)q` inside
`ndsR2FixedToF32`: 60,582 executions, 1,017,778 cycles in the cycle-106
whole-match profile. Replacing it with bit assembly is only legitimate if it is
EXACT, and "exact" here has a sharp edge -- an s32 has 32 significant bits and
an f32 significand holds 24, so every |q| >= 2^24 rounds, and IEEE-754 rounds
to nearest with ties to even. A shift-and-mask that truncates would pass casual
spot checks and drift on precisely the large values a Q12 joint angle reaches.

So this checks all four billion, the way `check_fcmp_exact.py` does for the
comparison predicates: numpy reproduces the C in vector form, and the oracle is
numpy's own int32 -> float32 cast, which is IEEE round-to-nearest-even.

There is no NaN or infinity exclusion to make here -- every s32 maps to a finite
float -- so unlike the comparison checker this one has no documented gap.

Run:  python scripts/check_s32tof32_exact.py
"""

from __future__ import annotations

import sys

try:
    import numpy as np
except ImportError:
    sys.exit("SKIP: numpy not available; cannot run the exhaustive check")

CHUNK = 1 << 24


def model(q: "np.ndarray") -> "np.ndarray":
    """Vector form of ndsR2S32ToF32Bits, returning the f32 bit pattern."""
    q = q.astype(np.int32, copy=False)
    u = q.view(np.uint32)
    sign = (u & np.uint32(0x80000000)).astype(np.uint32)
    neg = q < 0
    m = np.where(neg, (np.uint32(0) - u).astype(np.uint32), u).astype(np.uint32)

    nonzero = m != 0
    # clz for uint32, vectorised: 32 - bit_length
    lz = np.zeros_like(m)
    safe = np.where(nonzero, m, np.uint32(1))
    lz = (np.uint32(31) - np.floor(np.log2(safe.astype(np.float64))).astype(
        np.uint32))
    # log2 on float64 is exact enough for a power-of-two boundary check, but
    # verify and repair rather than trust it: a value one below a power of two
    # must not round up into the next exponent.
    shifted = (safe << lz).astype(np.uint32)
    bad = (shifted & np.uint32(0x80000000)) == 0
    lz = np.where(bad, lz + np.uint32(1), lz).astype(np.uint32)

    exp = (np.uint32(31) - lz).astype(np.uint32)
    m2 = (safe << lz).astype(np.uint32)
    mant = (m2 >> np.uint32(8)).astype(np.uint32)
    rem = (m2 & np.uint32(0xFF)).astype(np.uint32)

    roundup = (rem > np.uint32(0x80)) | (
        (rem == np.uint32(0x80)) & ((mant & np.uint32(1)) != 0))
    mant = np.where(roundup, mant + np.uint32(1), mant).astype(np.uint32)
    carry = (mant & np.uint32(0x1000000)) != 0
    mant = np.where(carry, mant >> np.uint32(1), mant).astype(np.uint32)
    exp = np.where(carry, exp + np.uint32(1), exp).astype(np.uint32)

    out = (sign | ((exp + np.uint32(127)) << np.uint32(23))
           | (mant & np.uint32(0x7FFFFF))).astype(np.uint32)
    return np.where(nonzero, out, np.uint32(0)).astype(np.uint32)


def main() -> int:
    total = 0
    bad = 0
    first = None
    for base in range(-(1 << 31), 1 << 31, CHUNK):
        q = np.arange(base, min(base + CHUNK, 1 << 31), dtype=np.int64)
        q = q.astype(np.int32)
        got = model(q)
        want = q.astype(np.float32).view(np.uint32)
        diff = got != want
        n = int(diff.sum())
        if n and first is None:
            i = int(np.argmax(diff))
            first = (int(q[i]), int(got[i]), int(want[i]))
        bad += n
        total += q.size
    print("checked {:,} inputs".format(total))
    if bad:
        print("MISMATCHES: {:,}".format(bad))
        print("first: q={} got=0x{:08x} want=0x{:08x}".format(*first))
        print("S32TOF32_EXACT=FAIL")
        return 1
    print("S32TOF32_EXACT=PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
