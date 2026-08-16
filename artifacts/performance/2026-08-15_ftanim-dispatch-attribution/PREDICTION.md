# Task B — pre-registered prediction, written BEFORE the first build

**Date:** 2026-08-15 · **Branch:** `codex/r2-runtime2` · **base HEAD `1eb6b453803`**
**UNITS: 2 profile cycles = 1 project tick.** Windows stated on every number below.

This file is written and timestamped before `build-c200-trackprof-on` exists. Nothing
below may be edited after the first capture lands; the result goes in a separate file.

---

## The question

`build-c196-trackperf`'s same-ROM A/B (DENSE_RUNTIME.md §5, 1,600 frames, frames
439–2038, DRAW=1, placement floor **zero**) read the dense fighter-animation path as
**tick-neutral to slightly negative**: rank-80 **+128**, P50 +576, trimmed mean
**+1,261 tk/fr**, at **13.51 %** stepped-call coverage (4,898 of 36,255).

Residual arithmetic on that pair: 1,261 tk/fr x 1,600 frames = **2,017,600 ticks**
over **29,095** exchanged calls = **+69.4 tk per exchanged call**, ~1.59x the generic
path. **That is a residual divided by a count, which this campaign has ruled is not a
price** (`a-residual-divided-by-a-count-is-not-a-price`). The v3 replaces it with a
located cost.

Coverage scales **2.45x** from 13.51 % to full Fox.

- If the excess is **steady-state ISSUE** — the dense stepper genuinely executes more
  expensive work per call than the generic parser — the excess scales with coverage:
  full Fox is ~**+3,090 tk/fr** and **the mechanism is REFUTED**.
- If the excess is **compulsory FETCH** — first-touch of the added 3,368 B of code and
  12,244 B of sparsely-read rows, plus eviction collateral on the generic parser that
  still serves 79.8 % of calls — it does not scale with coverage, and **the sign can
  flip** when the generic path stops running for a fighter at all.

**The two predictions differ in SIGN. That is what makes the capture decisive.**

## The instrument

Two `NDS_TASK37_PROFILE=1` builds differing in **one `.data` word**:
`gNdsFtAnimTrackDispatch` (`src/nds/nds_ftanim_track.c:5`, a `volatile u32`
initialised from `NDS_R2_FTANIM_TRACK_DISPATCH`). Nothing else consumes that macro at
preprocessor level, so `.text` must be address-identical. **That identity is a claim I
will verify by comparing symbol addresses, not assert** — if any `.text` address moves,
the pair is a cross-build pair and the +2,266 tk/fr control-arm floor (`K-CLOSE`)
applies and will be stated.

Mask on `total_cycles - halt_wait`, never `total_cycles`. Region alignment
`region = frame - 439`, not `- 438`.

## The split

For each arm, over the same window and the same marginal-80 mask:

- `GEN`  = cycles at the generic parser cluster: `ndsR2FtAnimParseDObjFigatree`,
  `ndsR2FtAnimBuildTrackTable`, `ftAnimGetTargetValue`, `ndsR2AnimRecipSlot`,
  `ndsR2FtAnimAObjToQ`.
- `DENSE`= cycles at `nds_ftanim_track.o` symbols.
- `REST` = `DTOTAL - DGEN - DDENSE`, i.e. everything the change was not supposed to
  touch.

## PREDICTION — I predict **FETCH**, and here are the three falsifiable sub-claims

| # | claim | FETCH predicts | ISSUE predicts |
|---|---|---|---|
| 1 | `|REST| / |DTOTAL|` | **>= 30 %** | <= 15 % |
| 2 | generic parser per-call cost, ON vs OFF | **rises >= 5 %** | flat within +-3 % |
| 3 | dense cost per **early-out** call (24,197 of the 29,095 exchanged = 83.2 %) | **> 25 tk/call** | <= 10 tk/call, with essentially the whole excess on the 4,898 stepped calls |

**Verdict rule, fixed now:** >= 2 of 3 hold -> FETCH -> the 33,951 tk/fr mechanism
survives and the full-coverage arena arm is the priced configuration.
>= 2 of 3 fail -> ISSUE -> full Fox ~ +3,090 tk/fr -> **the mechanism is REFUTED and
the animation lane closes.**

### Why I predict FETCH (the reasoning, so a wrong call is diagnosable)

**Sub-claim 3 is the load-bearing one, and it is an arithmetic argument, not a hunch.**
If the whole +2,017,600 ticks sat on the 4,898 stepped calls it would be **411.9 tk per
stepped call** on top of a generic parser measured at **272 tk/call** (§12c hand-off) —
a 2.5x path for code that *deletes* the 15-way opcode decode, the `command.flags` bit
scan, the `track_aobjs[10]` rebuild, the per-node Q migration, `ftAnimGetTargetValue`
and `ndsR2AnimRecipSlot`. I do not believe a dense stepper can execute 2.5x the
instructions of the parser it replaces.

The alternative distribution — ~69 tk on *every* exchanged call including the 83.2 %
that early-out after a handful of instructions — is only physical if those instructions
are **cold**, which is the fetch signature. 12,244 B of rows spread over 1,980 binds and
4,898 steps cannot stay resident, and each 48 B cursor block is its own line.

**The way I am most likely to be wrong:** if the bind (1,980 calls, one list walk +
per-node Q migration each) is far more expensive than assumed, the excess could sit
there and be neither of my two categories. `gNdsFtAnimTrackBinds` is in the counter set
so the bind is separable, and I will report it as its own row rather than folding it
into whichever category it flatters.

## Configuration, fixed before the run

```text
target   smash64ds-battle-playable-tickhud-hwtri     (lab, never a published name)
builds   build-c200-trackprof-on   NDS_R2_FTANIM_TRACK_DISPATCH=1
         build-c200-trackprof-off  NDS_R2_FTANIM_TRACK_DISPATCH=0
flags    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1
         NDS_R2_FIGHTER_GX_COMPOSE_LAB=1 NDS_TICK_HUD_DRAW=0 NDS_R2_FTANIM_TRACK=1
window   -StartFrame 438 -Frames 1600 -PerFrameRegion $true   (c192's proven recipe)
emulator emulators\melonds-attributor\melonDS.exe
```

Root ROMs before the first build, to be re-hashed after the last:

```text
smash64ds.nds                        54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds  6c939434c53c9b3a76ff016540b810a84f207b1a4e24540b8653b15717367c99
```
