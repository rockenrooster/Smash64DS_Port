# P2-2p8: the spline bisection (2026-09-28)

## Where it came from

A whole-match profile of the gate (`builds/p2p8-prof-match`, HEAD `6781d926731`,
frames 100-1900, per-frame regions) split into the 327 frames over two VBlanks
and the rest (`gate-split-over-gate.txt`). Soft float carried ~42K ticks of the
over-gate premium. Counting the BL sites into the soft-float routines in each
class (`callsplit.py`, `gate-float-callers-over-gate.txt`) put
`syInterpGetQuartSum` first: 245 `__aeabi_fmul` calls per over-gate frame
against 44 elsewhere.

`syInterpGetFracFrame` (decomp `sys/interp.c`) reparametrises a TraI spline by
arc length: it bisects [0,1] until the interval is under 1e-5 (seventeen steps),
each step a nine-sample Simpson integral of `sqrt(quartic)`. One call is ~2,700
soft-float operations. In the fighter bank only Samus's rolls carry TraI (the
gate roster is Samus/Fox/Captain/Donkey), and they evaluate it on every roll
tick: 141 calls a gate match, nearly all in frames over the gate. On Sector Z
the Arwing's path evaluates it 2,937 times a match.

## Change (`src/import/battleship_sys_interp.c`)

`syInterpCubic` / `syInterpQuad` keep the source's text; the arc-length step is
the source's own arithmetic, operation for operation, with three lookups that
only skip evaluations whose inputs were already seen:

1. The whole call, keyed on t's bits and a 64-bit hash of every value the call
   reads (kind, point count, length, segment index, the two keyframes around t,
   the segment's five quartic coefficients).
2. The bisection path: each integral covers the left half of a dyadic interval
   and depends only on its two ends and the coefficients, so the previous call
   on the same coefficients answers the nodes the two bisections share.
3. The Simpson samples: after a step left the child's even samples are the
   parent's samples 0-4; after a step right its first sample is the parent's
   last. A sample is reused only when its x bits match.

Linear, unknown kinds, point counts outside [2, 64] and t past the last keyframe
take the source call. Tick-HUD builds carry an oracle (`gNdsInterpFracOracle`)
that also runs the source and counts differing results. Same-ROM A/B word
`gNdsInterpFracMemo`. BSS: 1,024 B memo + 1,072 B paths.

## Measured (same ROM, word 0 -> 1; runs in `2026-09-26_p2-2p8-ftr-item-tail/`)

| Run | P50 | P95 | P99 | calls: memo hits / computed | nodes reused | replay |
|---|---|---|---|---|---|---|
| Gate `ir0/ir1_gate` | 935,360 -> 933,888 | 1,280,832 -> **1,275,904** | 1,481,856 -> 1,468,864 | 49 / 92 | 365 of 1,551 | IDENTICAL |
| Sector Z lab `irl0/irl1_sz_g1` | 1,267,264 -> 1,240,000 | 1,797,184 -> **1,708,864** | 2,146,368 -> 2,027,904 | 25 / 2,912 | 20,184 of 49,435 | IDENTICAL |
| Saffron lab (Samus) `irl0/irl1_fp_g7` | 1,156,032 -> 1,155,520 | 1,573,504 -> 1,573,312 | | 29 / 112 | 451 of 1,870 | IDENTICAL |

Oracle runs (`iro_gate`, `irlo_sz_g1`): 92 and 2,912 compares, 0 mismatches.
Samples reused: 32-33% of computed nodes' samples. The first, whole-call-only
memo (`im*`/`iml*`) hit 26% on the gate and 0.5% on Sector Z: t never repeats
while the Arwing flies, which is why the path and sample reuse exist.

## What is left

Sector Z still computes 29,251 nodes a match (~6 fresh samples each). Its calls
interleave at least two t streams on one descriptor (the sim's TraI and the
draw-side basis at the Arwing's own t), and the single path per coefficient set
serves only the most recent; a small node table shared across streams is the
next step. Sector Z also carries 119 native failures (owed).

## Other findings from the same profile

- Tier stall: `.main` code 36% non-memory stall, 39% memory stall; ~10.6K
  D-cache line fills a frame (`gate-profile-tiers.txt`).
- Top per-frame spikes (`gate-spike-functions.txt`, excess over each symbol's
  median in frames where it exceeds 8K ticks): the hurtbox reject kernel
  (8.1M ticks of excess inside over-gate frames, ~63K per active frame with
  `ndsR2CfxBuildLocal`), pose parse/play, texture resolve (~37K per spike
  frame, 58 frames), lean rebuilds (~200-270K per rebuild frame), shield decode,
  impact-wave and entry-effect submits.
- `ndsR2CfxRowScales` ran three hardware square roots per call even when the
  caller (the hurtbox reject's `ndsP2HbInvSMin`) asks for neither output that
  reads them: volatile register traffic the compiler cannot remove.
