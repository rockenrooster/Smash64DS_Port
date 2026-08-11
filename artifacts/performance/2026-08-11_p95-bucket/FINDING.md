# The c119 top-80 game+renderer bucket, banded by frame presence

Method per owner direction: exclude one-off transition frames from *attribution*
(not from the gate), then band the game+renderer bucket by how many of the 80
P95-defining frames each symbol actually runs on.

Tooling added for this: `task37_census.py --exclude-regions IDS` and a `frames`
(presence) column on the split table. Presence is the column a flat bucket needs
— a symbol worth 4.2M cycles on ONE of 80 frames tops the mean-delta ranking
while being unable to move the percentile.

**Excluding region 1558 moved the premium 1,341,271 → 1,299,857 (−41,414/frame),
which is itself the proof it was inflating attribution.**

## Bands

| band | cyc/frame | % premium | ticks/frame | symbols |
|---|---:|---:|---:|---:|
| **A ≥40/80 (recurring)** | **309,425** | **23.8%** | **154,496** | 213 |
| B 20–39/80 | 55,455 | 4.3% | 27,689 | 72 |
| C 8–19/80 | 23,840 | 1.8% | 11,903 | 56 |
| D <8/80 (rare) | 1,475 | 0.1% | 736 | 8 |

Band A alone (154,496 tk) exceeds the 138,112-tick gap. Bands C+D together are
12,639 tk — below the 16K bar even if deleted entirely. **Only band A matters.**

## Band A is two causally-linked clusters, both on 80/80 frames

| cluster | cyc/frame | ticks/frame |
|---|---:|---:|
| animation eval (`ParseDObjFigatree` 17,887, `AnimValueQ` 13,065, `gcPlayDObjAnimJoint` 12,149, `ftParamUpdateAnimKeys` 6,423, `gcAddDObjAnimJoint` 5,113, `BuildTrackTable` 5,035, `AnimAObjToQ` 4,686, `AnimTargetValue` 4,320) | **68,678** | **34,290** |
| soft float (`fadd` 24,955, `fmul` 20,836, `udivsi3` 13,614, `clzsi2` 4,604, `fdiv` 4,098) | **68,107** | **34,006** |

These are not independent: the animation evaluator is a named large caller of
the float helpers, so cutting animation work cuts part of the float row too.
Double-counting the two as 68K ticks would be wrong; treat 34–50K as the
realistic joint envelope.

## Call counts — the flat-bucket lever

Entry/epilogue PC execution counts (the epilogue runs once per call):

| symbol | calls (1600 frames) | calls/frame | note |
|---|---:|---:|---|
| `ndsR2AnimValueQ` | **430,671** | **269** | epilogue `pop {r4…pc}` at **13.37 cyc/insn** |
| `ndsR2FtAnimParseDObjFigatree` | — | — | hottest PC `ldr r3,[r3,#4]` ×105,992 at **24.06 cyc/insn** |

Two different defects, and they want different fixes:

1. **269 AObj evaluations per frame.** `gcPlayDObjAnimJoint` walks a DObj's AObj
   list and calls the evaluator per node. This is the "stop doing this N times"
   lever. Note the evaluator itself is already Requirement 4 fixed point at
   ~1.6 cyc/insn — there is no instruction left to delete inside it, only calls
   to avoid ([[flat-function-only-lever-is-not-entering-it]]).
2. **A 24 cyc/insn pointer chase in the parser.** 105,992 executions of one
   `ldr` at 24 cyc/insn is 2.55M cycles, 6.1% of that function, and 24 cyc on a
   load is a cache miss, not arithmetic. That is a DATA LAYOUT problem — the
   walk is chasing pointers through cold memory. Worth ~1,594 cyc/frame mean;
   below the bar alone, but it is the same walk the call-count fix touches.

## Candidates against the ≥16K-tick bar

- **Halve pose evaluation to 30 Hz** — 269 → ~135 calls/frame, roughly halves
  the animation cluster ≈ **17,000 ticks**. Clears the bar. `PROJECT_GOAL.md`
  explicitly permits "skeletal poses to update at 30 Hz" and lists reduced
  animation update rates as an allowed visual compromise — but it also says
  permanent implementation REQUIRES owner approval. **Proposal, not a unilateral
  change.**
- **Contiguous AObj walk** (fix the 24 cyc/insn chase) — sub-bar alone, should
  ride along with whatever touches the walk.
- **Do NOT re-try cycle 117's idle-joint skip on its own**: largest was
  ~1,900 tk/fr, and even at the tail's 1.37× clustering that is ~2,600 — still
  under the ±8,544 cross-build floor.
