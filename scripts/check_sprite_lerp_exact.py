#!/usr/bin/env python3
"""Prove the sprite blitter's fast paths are BIT-EXACT, not approximate.

Two changes are covered, both in `src/port/sprite_preview_backend.c`.

R2-07 R0e replaced the VS Results wallpaper's per-pixel loop with a specialized
row that reads one byte per PAIR of destination columns and looks the colour up
in a sixteen-entry palette. Both halves are provable here rather than by
screenshot: the palette is exact because the combine's output depends only on
the 4-bit intensity (`check_nibble_scale` plus `check_lerp_channel`), and the
pairing is exact because of index algebra (`check_i4_pair_mapping`).

R2-07 R0c changed four divisions per blitted pixel, three of which were real
library calls:

  * `ndsSpriteLerpPrimEnv`'s three `/ 255u` became `NDS_SPRITE_DIV255(x)`,
    i.e. `(x * 257 + 257) >> 16`. These WERE `blx __udivsi3` -- measured, and
    gone: the function went 118 -> 100 bytes with `__udivsi3` x3 -> x0.
  * The I4 and IA arms' `(nibble * 255u) / 15u` became `nibble * 17u`. Bit-exact
    and the intent is now explicit, but this removed no library call: GCC had
    already strength-reduced those.

Both are exact rather than close, and this script is the proof, run in CI-style
alongside the other checkers rather than trusted from a comment. It exists
because the exactness of the first one depends on a bound that is easy to break
by accident: `intensity` and `inverse` are complementary, so the numerator tops
out at 255*255 + 127 = 65,152. If someone makes `inverse` independent of
`intensity`, or widens `intensity` past u8, the numerator can reach 130,177 and
`(x * 257 + 257) >> 16` starts returning values one too high -- silently, as a
one-LSB colour error nobody would see in a screenshot diff.

Exits non-zero and prints the first counterexample if any identity fails.
"""

from __future__ import annotations

import sys

MAX_NUMERATOR = 255 * 255 + 127  # 65,152 -- see the module docstring


def div255(x: int) -> int:
    """The C macro, evaluated in Python's unbounded integers."""
    return (x * 257 + 257) >> 16


def check_div255_range() -> list[str]:
    """Every numerator the lerp can produce, exhaustively."""
    for x in range(MAX_NUMERATOR + 1):
        if div255(x) != x // 255:
            return [f"div255 wrong at x={x}: got {div255(x)}, want {x // 255}"]
    return []


def check_div255_first_failure() -> list[str]:
    """Pin WHERE the identity stops holding, so the bound is documented by test.

    This is not a failure mode -- it asserts the cliff is above our range and
    below 2*255*255+127, which is what makes the complementarity argument
    load-bearing rather than incidental.
    """
    x = MAX_NUMERATOR + 1
    limit = 2 * 255 * 255 + 127
    while x <= limit and div255(x) == x // 255:
        x += 1
    if x > limit:
        return ["div255 never fails below 130,178 -- the documented bound "
                "argument is weaker than the comment claims; update the comment"]
    if x <= MAX_NUMERATOR:
        return [f"div255 fails inside the reachable range at x={x}"]
    return []


def check_lerp_channel() -> list[str]:
    """The whole channel expression, over every reachable (colour, env, intensity).

    256*256*256 is 16.7M iterations, a few seconds -- worth it, because the
    composition is what ships, not the isolated division.
    """
    for intensity in range(256):
        inverse = 255 - intensity
        for colour in range(256):
            base = colour * intensity + 127
            for env in range(256):
                numerator = base + env * inverse
                if div255(numerator) != numerator // 255:
                    return [f"lerp channel wrong at colour={colour} "
                            f"env={env} intensity={intensity}"]
                if numerator > MAX_NUMERATOR:
                    return [f"numerator {numerator} exceeds the documented bound "
                            f"{MAX_NUMERATOR} at colour={colour} env={env} "
                            f"intensity={intensity}"]
    return []


def check_nibble_scale() -> list[str]:
    """(nibble * 255) / 15 == nibble * 17, for the only inputs a nibble has."""
    for nibble in range(16):
        if (nibble * 255) // 15 != nibble * 17:
            return [f"nibble scale wrong at {nibble}"]
    if 15 * 17 != 255:
        return ["nibble scale does not reach full white"]
    return []


def generic_i4_row(width: int, shuf: bool, row: int, row_bytes: int,
                   source: bytes) -> list[int]:
    """The nibble each destination column gets from the generic pixel loop."""
    out = []
    for x in range(width):
        source_x = x ^ 8 if (shuf and (row & 1)) else x
        index = ((row * row_bytes) + (source_x >> 1)) ^ 3
        packed = source[index]
        out.append(packed >> 4 if (source_x & 1) == 0 else packed & 0x0F)
    return out


def fast_i4_row(width: int, shuf: bool, row: int, row_bytes: int,
                source: bytes) -> list[int]:
    """R0e's specialized row: one byte load per PAIR of destination columns."""
    byte_xor = 4 if (shuf and (row & 1)) else 0
    out = []
    for pair in range(width >> 1):
        packed = source[((row * row_bytes) + (pair ^ byte_xor)) ^ 3]
        out.append(packed >> 4)
        out.append(packed & 0x0F)
    if width & 1:
        packed = source[((row * row_bytes) + ((width >> 1) ^ byte_xor)) ^ 3]
        out.append(packed >> 4)
    return out


def check_i4_pair_mapping() -> list[str]:
    """R0e reads the same nibbles in the same order as the loop it replaces.

    The pairing claim is that `source_x ^= 8` cannot touch bit 0, so columns 2k
    and 2k+1 always share a byte and always land high-nibble-then-low. That
    reduces to `^ 4` on the byte index. It is index algebra, so it is provable
    outright rather than argued in a comment: every width the strip loop can
    produce, both row parities, over a source whose every byte is distinct mod
    256. The odd-width tail is the case worth having a test for -- the last
    column of an odd row is even, so it takes the HIGH nibble of byte `pairs`.
    """
    # 320 is the blitter's declared width ceiling; the Results wallpaper is 300.
    source = bytes((i * 7 + 13) & 0xFF for i in range(4 + 320 * 256))
    for width in range(1, 321):
        row_bytes = (width + 1) // 2
        for shuf in (False, True):
            for row in (0, 1, 2, 219):
                want = generic_i4_row(width, shuf, row, row_bytes, source)
                got = fast_i4_row(width, shuf, row, row_bytes, source)
                if got != want:
                    first = next(i for i, (a, b) in enumerate(zip(got, want))
                                 if a != b)
                    return [f"i4 pair mapping wrong at width={width} "
                            f"shuf={shuf} row={row} column={first}: "
                            f"got {got[first]}, want {want[first]}"]
    return []


def main() -> int:
    checks = (
        ("div255 exhaustive over [0, 65152]", check_div255_range),
        ("div255 cliff is above the reachable range", check_div255_first_failure),
        ("lerp channel over all 16.7M reachable inputs", check_lerp_channel),
        ("nibble*255/15 == nibble*17", check_nibble_scale),
        ("R0e I4 pair loop reads the generic loop's nibbles",
         check_i4_pair_mapping),
    )
    failures: list[str] = []
    for name, fn in checks:
        problems = fn()
        print(f"{'FAIL' if problems else 'ok  '}  {name}")
        failures.extend(problems)
    if failures:
        print()
        for line in failures:
            print(f"  {line}")
        return 1
    print()
    # Three, not four: the nibble substitutions below are bit-exact but removed no
    # library call, because GCC had already strength-reduced `/ 15u`. Measured on
    # the ELF -- the blitter's `__udivsi3` count is 2 before and 2 after, and both
    # of those belong to a wallpaper-cache helper that this path never enters.
    print("SPRITE_LERP_EXACT=PASS  three library divisions per pixel removed "
          "and R0e's paired row proven equivalent, both bit-exact")
    return 0


if __name__ == "__main__":
    sys.exit(main())
