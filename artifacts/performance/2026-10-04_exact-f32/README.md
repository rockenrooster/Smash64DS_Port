# Exact binary32 chains for the latch walk, 2026-10-04 -- rejected

## Why

After the item replay, the held Beam Sword's attach still runs the source
latch walk every frame: ~9 `func_ovl2_800ED490` composes and ~9
`gmCollisionTransformMatrixAll` locals (late-window lab profile
`artifacts/task37-census/sz-lateprof04`: compose ~2.6K cycles a call with its
63 soft-float calls, local ~1.3K). Late minus early window, the soft-float
calls alone grew 25K cycles a frame.

## Change (measured, then removed)

`src/nds_exact_f32.h`: IEEE-754 binary32 multiply and add, round to nearest
even, on unpacked operands, for zeros and normals within 2^+-50 (no overflow,
no subnormal can arise in a short chain). `src/check-exact-f32.c` compared
them with the host's binary32 arithmetic: 600M iterations (2.4G operations,
sums of products, ties, cancellation, signed zeros), 0 failures.
`src/nds_gm_exact_f32.c`: strong `func_ovl2_800ED490`,
`gmCollisionGetWorldPosition` and `gmCollisionTransformMatrixAll` over the
weak source definitions (battleship_gmcollision.c), each chain in the source's
order, falling back to the source expression outside the domain.

## Result (official gate, `build-gate-1004p`)

| | WORK P50 | WORK P95 | > 1.12M | SRC med | MITM med |
|---|---:|---:|---:|---:|---:|
| `item-replay/gate-routes` (HEAD) | 904,320 | 1,249,152 | 240 | 391,392 | 40,704 |
| exact chains (`gate-xf1`) | 932,928 | 1,317,504 | 329 | 409,280 | 56,128 |

Replay digest IDENTICAL over 1,960 samples (the chains are exact), and every
cycle slower: SRC +18K, MITM +15K. Fully inlined, the three bodies were
2.7 KB, 2.7 KB and 6.1 KB of main-RAM ARM code, called alternately in the
latch walk, so each call refilled its instruction lines; libgcc's
`__aeabi_fadd`/`__aeabi_fmul` live in ITCM. An unpacked add is also ~36
instructions, no fewer than libgcc's whole packed add. Removed; not to be
retried without ITCM room for a compact (looped) body, and even then the
arithmetic saving is ~20% of the compose.
