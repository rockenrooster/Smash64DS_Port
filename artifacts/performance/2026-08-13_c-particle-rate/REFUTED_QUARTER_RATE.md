# Rung 2 — quarter-rate particles — REFUTED twice over, before a build was spent

**Outcome: STOP at Phase 0.** The contingency ladder's rung 2 ("particle work at
quarter rate, round-robin, never batched") fails on **two independent grounds**,
either of which alone closes it:

1. **Size.** The rate-reducible work is **7,364 tk/frame**, not 17,152. Running
   it at quarter rate prices **−7,493 `WORK-H` P95**; **deleting the whole
   particle/effect update outright, every frame, prices −8,987.** Both are under
   the 16,000 bar and under the 12,000 stop threshold. The family would have to
   be **twice its measured size and deleted entirely** to reach 14,966 — still
   short.
2. **Correctness, and this one is fatal on its own.** The particle and effect
   update draws from **the same single LCG the level-3 CPU AI draws from**
   (`sSYUtilsRandomSeed`, one global, never retargeted during a match).
   `ftcomputer.c` has **65** draw sites; `lbparticle.c` **26**, `efmanager.c`
   **44**, `efground.c` **6**. Changing *when* particle updates run changes how
   many times the seed is stepped before the AI's next draw, so the AI's
   decisions change and the match diverges. That is mechanical divergence, which
   `PROJECT_GOAL.md` forbids outright.

**And rung 2 named the wrong lane.** `MISC` is a **draw-side** bucket by
construction — `taskman_seam.c:5104-5111` computes it as
`DrawTicks − (FTR + STG + BG + HUD) + glFlush`. **No update work is in `MISC` at
all.** The particle/effect update runs inside `ndsRunMarioFoxProofUpdate`
(`taskman_seam.c:4453-4479`), whose bracket is the **`SRC`** bucket. So
"−15.2% of `MISC` buys 16,000" is a true statement about a lane this change
cannot touch.

**No build, no emulator run, no ROM, no runtime source change.** Root ROMs
unchanged: `smash64ds.nds` 54c07fac…6ac68a,
`smash64ds-battle-playable-hwtri.nds` 524448c9…23adee. Boot-headroom price:
**zero bytes**.

---

## 0. Which artifacts this is measured on

Both were already on disk; nothing was produced by running the game.

- `../2026-08-12_c130-fire-gate/c130-gate-rows.csv` — the 1,600-sample
  `BOTH_CPU 1` whole-match gate run, **current code**. Rank-80 `WORK-H`
  **1,220,480** against the 1,120,380 line; gap **100,100**.
- `../2026-08-12_c123-rebank/profile-warm/` — `build-c124-profile`,
  `BOTH_CPU 1`, `NDS_TICK_HUD_DRAW 0`, `regions=1601`, **post-slice-46** and
  therefore the current-world symbol arm (`../2026-08-13_c-band-io/BAND_IO_OWNER.md` §0).
  Alignment: profile region `i+1` ↔ gate row `i`.

`ticks/frame = cycles / (2 × regions)`; a **single** region's ticks are
`cycles / 2` (`../2026-08-13_c-residue/RESIDUE.md` §0).

## 1. What `MISC` actually brackets — the premise correction

`src/port/taskman_seam.c:5104`:

```c
u32 draw_known = gNdsTickHudFighterTicks + gNdsTickHudStageTicks +
    gNdsTickHudBackgroundTicks + gNdsTickHudForegroundTicks;
u32 misc_draw = (gNdsRendererProfileDrawTicks >= draw_known) ?
    (gNdsRendererProfileDrawTicks - draw_known) : 0u;
...
misc_draw += gNdsTickHudFlushTicks;               /* glFlush(GL_TRANS_MANUALSORT) */
```

`MISC` is the **draw pass** minus the four named draw buckets, plus the GX
flush. It is a draw residual. `diagnostics.c:3066` says so in as many words.

The update half is elsewhere. `ndsTask39EffectsUpdate()` and
`scVSBattleFuncUpdate()` (→ `gcRunAll` → every particle/effect GObj proc) both
run inside `ndsRunMarioFoxProofUpdate`, and that function's bracket writes
`gNdsRendererProfileSourceUpdateTicks` — the **`SRC`** bucket
(`taskman_seam.c:4453-4479`). The source keeps the two halves genuinely
separate: `lbParticleStructFuncRun` is a once-per-frame GObj proc
(`lbparticle.c:1416`), `lbParticleDrawTextures` is the display proc
(`lbparticle.c:1448`), and this port preserves that split — its own
`ndsWhispyAOTStructFuncRun` (`src/import/battleship_lbparticle.c:1415`) is a
GObj proc that falls back to `lbParticleStructFuncRun` on route 0, while
`lbParticleDrawTextures` stays at `:3462`.

**Consequence:** every `MISC`-derived figure in `RESIDUE.md` §4 row 6 and §6
rung 2 prices the **draw** half. Rung 2 proposed sub-rating the **update** half.
The two numbers were never about the same work.

`misc-band-split.txt` confirms it empirically — on the 219 frames where
`MISC ≥ 140,000` the premium is +55,956 tk/frame and its owners are draw
symbols (`lbParticleDrawTextures` +3,542, `ndsRendererSubmitWhispyNativeQuad`
+3,506, `ndsRendererAdapterSubmitStageDL` +1,836, `ndsEffectPacketRecord`
+1,496). The update symbols appear only because more particles cost more on
both sides (`ndsWhispyAOTStructFuncRunLean` +2,341,
`lbParticleUpdateStruct.part.0` +973) — they are still charged to `SRC`.

## 2. Size — the exact re-rank, `quarter-rate-rerank.txt`

Method as briefed: scale the rate-reduced family by 0.25 on **every** frame,
subtract from the aligned gate frame, re-take the 80th largest of 1,600.
`PUPD` is the 26-symbol rate-reducible set — the two particle-pool integrators
(`ndsWhispyAOTStructFuncRunLean`, `lbParticleUpdateStruct.part.0`), the
generators, the bytecode/eject/sincos helpers, `ndsTask39EffectsUpdate`, and
every `efManager*ProcUpdate`. `PDRAW` is the 20-symbol draw half, listed for
contrast only — it cannot be sub-rated without strobing.

| family | mean tk/frame | presence | ΔP95 deleted 100% | **ΔP95 at quarter rate** | worst-case pairing, quarter rate |
|---|---:|---|---:|---:|---:|
| **`PUPD`** (update) | **7,364** | 1,600/1,600 frames, 1 run | **−8,987** | **−7,493** | −15,383 |
| `PDRAW` (draw) | 27,758 | 1,600/1,600, 1 run | −30,676 | not eligible | — |
| `PALL` (both halves) | 32,041 | 1,600/1,600, 1 run | **−33,818** | — | — |

Add the spawn path (`lbParticleMakeStruct` and friends, 203 tk/frame) — which a
round-robin must *not* touch, because a gameplay-requested effect has to appear
on the tick that asked for it — and the quarter-rate figure does not move:
**−7,493**.

**How wrong would the measurement have to be?** `PUPD` scaled up and re-ranked:

| family scaled to | deleted outright | at quarter rate |
|---|---:|---:|
| measured (×1.00) | −8,987 | **−7,493** |
| + *every* particle soft-float tick (RESIDUE §3's 1,588) | −10,277 | −8,460 |
| ×1.50 | −11,977 | −9,735 |
| ×2.00 | −14,966 | −11,977 |
| ×3.00 | −21,531 | −16,461 |

The lever needs the family to be **three times its measured size** before
quarter-rating it clears 16,000. Its own alignment-free worst-case-pairing bound
at quarter rate is **−15,383**, so no alignment error rescues it either.

**Why the tail multiplier is ~1.0 and not `SRC`'s 1.63.** `PUPD` is present on
**1,600 of 1,600** frames in **one** run — a per-frame tax, not a burst — and it
is flat where the percentile lives: on the rank-56–115 band it reads mean 8,731
/ median 6,089 / min 3,446, and on the rank-80 frame itself only **3,784**.
A flat component inside a spiky lane pays 1:1; it does not inherit the lane's
proportional multiplier. `RESIDUE.md` §2's `MISC −15.2%` row is a *proportional*
scaling of the whole lane, which nothing in this change does.

## 3. Correctness — the shared RNG, `rng-ownership.txt`

`decomp/BattleShip-main/decomp/src/sys/utils.c:172`:

```c
f32 syUtilsRandFloat(void)
{
    s32 step = (*sSYUtilsRandomSeedPtr * 214013) + 2531011;
    *sSYUtilsRandomSeedPtr = step;
    return ((step >> 16) & 0xFFFF) / 65536.0F;
}
```

One LCG, one seed (`sSYUtilsRandomSeed = 1`). `syUtilsSetRandomSeedPtr` exists
but this tree calls it once, at probe init with `NULL` (= the default seed), and
never during a match; only the diagnostic recorders save/restore the seed value.
So during the gate match there is exactly **one** random stream.

Draw sites, whole-file counts:

| file | sites | who |
|---|---:|---|
| `ft/ftcomputer.c` | **65** | the level-3 CPU AI — and on the gate arm **both** fighters are level-3 CPUs |
| `ef/efmanager.c` | 44 | effect procs, spawn *and* per-frame update |
| `lb/lbparticle.c` | 26 | particle script walk and generator emission |
| `ef/efground.c` | 6 | ground effects |
| `it/itmain.c` | 3 | items (off this milestone) |
| `ft/ftmain.c` | 1 | |

Quarter-rating the update changes the number of seed steps per frame. The AI's
very next draw therefore reads a different value, and from there the match is a
different match. This is the same failure class slice 41 hit when 30 Hz poses
ended a "behaviourally identical" A/B at damage 130/51 versus 33/65.

**Can the draw count be preserved instead?** No, not cheaply. The number of
draws an update makes is data-dependent: it comes from branching on particle
script opcodes (`lbparticle.c:924-1130`) and from how many particles a generator
matures this frame, each emission consuming two or more draws
(`battleship_lbparticle.c:722-731`). To know how many draws a skipped frame
*would* have made you must walk the script — which is the work being skipped.
Preserving the count exactly costs approximately what it saves.

That this is a live invariant and not a theoretical one is already recorded in
the very file rung 2 would edit: `battleship_lbparticle.c:722` keeps a **dead**
`(void)syUtilsRandFloat();` whose only job is to keep the stream aligned after
the port deleted the arithmetic around it.

**Giving particles their own seed does not rescue it either** — today the AI's
draws are interleaved with particle draws, so splitting the streams changes what
the AI reads just as surely as skipping updates does.

## 4. What this closes, and what it does not

**Closed:** rung 2 as written, at any rate (half-rate prices −5,998), by either
leg alone. `PALL` also puts a ceiling on the whole subsystem: deleting **every
particle in the game, update and draw**, pays **−33,818** of a 100,100 gap.

**Not closed by this note:** the *draw* half is 27,758 tk/frame and prices
−30,676 deleted. It is a fidelity question (`PROJECT_GOAL.md` sacrifice-order
item 2), it belongs to the owner, and it is not what rung 2 proposed. Nobody
should read this note as "particles are cheap" — the draw half is 3.8× the
update half and is the only part with a size worth an owner decision.

## 5. Reproduce

```
# one pass over the profile writes the (region, symbol) cache (keep it OUT of the tree)
python scripts/analyze-profile-region-split.py \
  artifacts/performance/2026-08-12_c123-rebank/profile-warm/arm9-profile.csv \
  --census artifacts/performance/2026-08-12_c123-rebank/profile-warm/census.json \
  --gate-csv artifacts/performance/2026-08-12_c130-fire-gate/c130-gate-rows.csv \
  --gate-lane MISC --gate-min 140000 --control-lane MISC --control-max 90000 \
  --check-align --check-controls --head 40 --cache <scratch>/warm-cache.npz   # -> misc-band-split.txt

# the family series, instant off that cache
python scripts/analyze-io-lane-series.py --cache <scratch>/warm-cache.npz \
  --gate-csv artifacts/performance/2026-08-12_c130-fire-gate/c130-gate-rows.csv \
  --group "PUPD=..." --group "PDRAW=..." --group "PALL=..."   # -> particle-family-series.txt
```

`misc-band-split.txt` reports `cache … (reused)` — the `.npz` was already on
disk from `../2026-08-13_c-band-io/`. It is the right one and that is checked,
not assumed: it attributes **3,789,841,877 of 3,791,365,448** cycles, and
3,791,365,448 is `profile-warm/arm9-profile.meta.txt`'s `cycles` exactly
(c123's is 3,816,014,261).

`quarter-rate-rerank.txt` is the 0.25-scaling re-rank and the sensitivity
sweep; its source is inlined in this directory's commit message. The `.npz`
cache stays out of the tree — it is derived, large, and its path carries the
build machine's user directory.

## 6. What this cycle did NOT do

- **No build, no emulator run, no A/B, no screenshots.** Phase 0 refuted the
  lever, so Phase 1 and Phase 2 were never entered and no candidate exists.
- Did not re-profile on current code. `PUPD`/`PDRAW`/`PALL` composition is
  `build-c124-profile` (post-slice-46, pre-fox-gun-overlay); the gate lane
  column and every P95 figure are the current c130 arm.
- Did not measure a per-frame RNG draw count from a running ROM. The stream
  ownership is proved from the source and the single global seed, which is
  sufficient to forbid the change; a counter would only size a divergence that
  is already disqualifying.
- Did not touch the draw half, which is the part with a size.
