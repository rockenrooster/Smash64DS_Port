#!/usr/bin/env python3
"""Prove `include/nds/nds_fcmp.h`'s predicates are BIT-EXACT, not close.

Those macros replace calls to the soft-float comparison helpers with integer
tests on the bit pattern. The helpers cost 12,909,690 cycles -- 1.32% of all
non-idle work -- over 1,045,094 calls in the cycle-106 whole-match profile, and
`-ffinite-math-only` does not remove them, so the calls have to go at the call
sites. That is only safe if the replacements agree with IEEE-754 on every input,
which is what this checks.

**Exhaustive over all 2^32 bit patterns**, not sampled. Every float32 is tested
against every predicate, in chunks, with numpy doing the float comparison and
integer arithmetic side by side. That matters more here than a spot check would
suggest, because the failure mode is a single pattern class:

  * `-0.0f == +0.0f` is TRUE in IEEE but the patterns differ, so a naive
    `bits(x) == bits(0.0f)` is wrong for exactly one input out of 2^32. The zero
    predicates shift the sign out instead. A random sample essentially never
    draws -0.0f, and a screenshot would never show it -- but
    `if (payload != 0.0F) { ... 1.0f / payload ... }` taking the wrong branch on
    -0.0f divides by zero.
  * `x < 0.0f` must reject -0.0f, which is why the predicate is `> 0x80000000u`
    and not `>=`.
  * Denormals are ordinary here: their patterns order correctly against each
    other and against zero, and the exhaustive sweep covers all 16,777,214 of
    them without needing a special case.

NaN is the one documented exclusion and is reported separately rather than
skipped silently: IEEE makes every ordered comparison with NaN false, while
these predicates place NaN in the integer order. Gameplay float data never
carries one. The count of NaN disagreements is printed so that the exclusion
stays visible instead of becoming folklore.

Exits non-zero and prints the first counterexample if any identity fails.
"""

from __future__ import annotations

import sys

try:
    import numpy as np
except ImportError:  # pragma: no cover - environment guard
    sys.exit("check_fcmp_exact.py needs numpy")

CHUNK = 1 << 24  # 16,777,216 patterns per pass; 256 passes covers 2^32
ONE = np.uint32(0x3F800000)  # 1.0f


def bits_to_f32(bits: np.ndarray) -> np.ndarray:
    return bits.view(np.float32)


def check() -> int:
    failures = 0
    nan_disagreements = 0
    total = 0

    for base in range(0, 1 << 32, CHUNK):
        bits = np.arange(base, base + CHUNK, dtype=np.uint64).astype(np.uint32)
        vals = bits_to_f32(bits)
        signed = bits.view(np.int32)
        is_nan = np.isnan(vals)
        total += bits.size

        # (predicate name, IEEE result, integer result)
        cases = [
            ("NE0", vals != np.float32(0.0), (bits << np.uint32(1)) != 0),
            ("EQ0", vals == np.float32(0.0), (bits << np.uint32(1)) == 0),
            ("LT0", vals < np.float32(0.0), bits > np.uint32(0x80000000)),
            ("GT0", vals > np.float32(0.0), signed > np.int32(0)),
            ("GE0", vals >= np.float32(0.0),
             (signed >= np.int32(0)) | (bits == np.uint32(0x80000000))),
            ("LE0", vals <= np.float32(0.0),
             (bits >= np.uint32(0x80000000)) | (bits == np.uint32(0))),
            ("GT_C 1.0f", vals > np.float32(1.0), signed > ONE.view(np.int32)),
            ("LT_C 1.0f", vals < np.float32(1.0), signed < ONE.view(np.int32)),
            ("GE_C 1.0f", vals >= np.float32(1.0), signed >= ONE.view(np.int32)),
            ("LE_C 1.0f", vals <= np.float32(1.0), signed <= ONE.view(np.int32)),
            ("EQ_C 1.0f", vals == np.float32(1.0), bits == ONE),
            ("NE_C 1.0f", vals != np.float32(1.0), bits != ONE),
        ]

        for name, ieee, integer in cases:
            bad = ieee != integer
            if not bad.any():
                continue
            nan_bad = bad & is_nan
            nan_disagreements += int(nan_bad.sum())
            real_bad = bad & ~is_nan
            if real_bad.any():
                idx = int(np.argmax(real_bad))
                pattern = int(bits[idx])
                failures += 1
                print(
                    "FAIL {}: bits 0x{:08x} (value {!r}) IEEE={} integer={}".format(
                        name, pattern, float(vals[idx]),
                        bool(ieee[idx]), bool(integer[idx])))
                if failures >= 8:
                    return 1

    print("checked {:,} bit patterns against 12 predicates".format(total))
    print("NaN disagreements (documented exclusion): {:,}".format(
        nan_disagreements))
    if failures:
        return 1
    print("FCMP_EXACT=PASS  every predicate matches IEEE-754 on all non-NaN "
          "inputs, including both signed zeros and all denormals")
    return 0


def main() -> int:
    return check()


if __name__ == "__main__":
    sys.exit(main())
