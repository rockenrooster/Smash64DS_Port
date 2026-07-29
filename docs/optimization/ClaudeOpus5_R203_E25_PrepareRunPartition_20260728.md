# R2-03 E25 — PrepareProductionRun has no hot spot, which is why the plan replaces it

**Date:** 2026-07-28
**Phase:** R2-03 (fighter direct draw)
**Verdict:** Measurement. The 42,281/frame is **four roughly equal quarters**, so
no partial optimization reaches it. This is the sizing for the switch plan's
R2-03 bullet, and it argues for that bullet rather than around it.

## 1. Why this measurement

E24 established that the shade's residue is not the action walk, and the census
re-partition put `SubmitPrepTicks` at **42,281/frame** —
`ndsRendererNativePrepareProductionRun`, which is named directly in the switch
plan's R2-03 bullet:

> per-epoch generated submit consuming only baked facts (`poly_fmt`, texture
> slot, palette, matrix binding, corner stream) — no `PrepareProductionRun`
> policy re-checks, no traversal-state/stats dependency, no per-frame texture
> identity proof.

The question before building that is whether one part dominates, in which case a
smaller cut would do.

## 2. Result

The `NDS_R2_FIGHTER_RUN_PROOF=2` instrument already existed from E11 and was
reused rather than rebuilt. 480 presented frames, 62.8 runs/frame:

| phase | ticks/frame | share | per run |
|---|---:|---:|---:|
| entry validate | 8,761 | 17.6% | 140 |
| texture prepare | 13,076 | 26.3% | 282 (46.4 prepares) |
| texture reuse | 1,283 | 2.6% | 79 (16.3 reuses) |
| UV | 12,206 | 24.6% | 194 |
| tail (field writes + batch begin) | 13,753 | 27.7% | 219 |
| **total** | **49,078** | | **781** |

(The total exceeds the census bracket's 42,281 because the two builds carry
different instruments; the ordering is what this is for. E18's ranking-only rule
applies.)

**No hot spot.** Four phases within a factor of 1.6 of each other. Removing any
one of them buys 9–14K, at the noise floor's edge and nowhere near the ~198,000
R2-03 still owes.

## 3. What that implies

Every one of these four phases re-derives a fact that E5 already measured as
**1.9% churn** across frames. The cost is not any individual computation; it is
that ~63 runs a frame each re-establish, from scratch, a description of
themselves that changed for one run in fifty.

That is precisely the shape the switch plan's R2-03 bullet addresses, and it is
why the bullet says *replace* rather than *optimize*. E12 already proved the
approach on the texture quarter alone — a per-run memo worth −32,724 — and the
remaining three quarters are the same trade on the same table.

**The next cut is the full per-run fact memo**: extend `sNdsR2RunTextureMemo` to
carry `poly_fmt`, `scale_s/t`, `origin_s/t`, `offset`, `vertex_flags` and
`textured`, and on a hit skip the entire body of `PrepareProductionRun` down to
the texture bind and `ndsRendererNativeBeginDirectBatch`. E5's 1.9% churn is the
hit rate; the memo already exists and already has a validity protocol.

Expected: most of 42,281, less the bind and batch begin that must still run.

## 3a. The two remaining phases are one problem, not two

`gNdsR2RunTexPrepCount` is **46.4/frame against 62.8 runs** — the full prepare
block runs on 74% of runs — while E12's texture memo reports a **99.5% hit rate**
(`R2_TEXMEMO=1899,9,9,0,0`). Those only reconcile one way: the memo is being
consulted constantly because something keeps clearing `texture_prepare_valid`.

It is the state replay. `ndsRendererNativeApplyStateDelta` calls
`NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE` on every `OTHERMODE`, `COMBINE`,
`TEXTURE`, `GEOMETRY`, `IMAGE` and `TILE` delta, and E20 measured **194.4 delta
applications a frame**. Each one forces the next run in that epoch to redo the
prepare.

So the two largest phases left in the fighter draw —

| phase | ticks/frame |
|---|---:|
| state span replay | 65,026 |
| PrepareProductionRun | 42,281 |
| **combined** | **107,307** |

— are **one coupled mechanism**: the replay's job is to move state, and moving
state is what makes the prepare expensive. Together they are over half of what
R2-03 still owes.

This reframes three earlier results rather than contradicting them. E20/E21
attacked the delta *write* and found it cheap (~280 ticks) — correct, but the
write was never the expensive half; the invalidation it triggers is. E23 and E24
each removed one side of a coupled pair and measured nothing. **Every cut that
optimises one side while the other keeps re-dirtying the state will read as
null**, which is precisely the pattern the last four experiments produced.

It also explains why the switch plan specifies a *generated per-epoch submit with
no traversal-state dependency* rather than a faster prepare or a cheaper replay:
the generator knows each epoch's final state at build time, so neither the replay
nor the re-derivation needs to exist. Breaking the coupling is the phase's
remaining work, and it cannot be done from either end alone.

## 3b. E25c — the cheap variant is ruled out by the effect mix

Before building a value-based validity key to replace the dirty flag, the
question was which effects dominate the invalidations: a handful of scalars a
run can compare cheaply, or the 20-word tile state, which it cannot.

480 frames, per frame, by `NDS_NATIVE_STATE_*`:

| effect | per frame | cheap to compare? |
|---|---:|---|
| COMBINE | 41.7 | yes (2 words) |
| **TEXTURE** | **35.7** | **no — moves tile state** |
| LIGHT_COLOR | 27.4 | (does not invalidate) |
| **TILE** | **23.9** | **no** |
| OTHERMODE | 13.4 | yes |
| **IMAGE** | **10.6** | **no** |
| PRIM | 6.7 | (does not invalidate) |
| GEOMETRY | 2.0 | yes |
| MATERIAL | 0.0 | |

Invalidating total: **127.3/frame**. Of those, **70.2 move the tile state**
against 57.1 cheap scalars.

So a generation counter bumped by TEXTURE/IMAGE/TILE would bump 70 times a frame
across 62.8 runs — more often than there are runs — and every run would miss.
Hashing the tile state per run costs 20 loads against the 781 ticks a prepare
costs, which is a worse ratio than E8's losing memo once the miss rate is
included.

**The memo variant is refuted, not merely deprioritised.** There is no cheap key
for a value that is legitimately rewritten more often than it is read.

That leaves exactly what the plan specifies. The replay writes constantly because
that is what a replay does; the *post-replay state* is what the draw needs, and
E5 measured that stable at 1.9% churn. Baking the resolved per-epoch state and
installing it directly removes the 194.4 applications **and** the 127.3
invalidations **and** the 46.4 re-prepares they force — the whole 107,307 — and
nothing smaller does.

## 4. Two things to carry into that build

- **ITCM is full.** E16 left 1,024 bytes free (31,744/32,768), and
  `NDS_TASK91_DRAW_PHASE_CENSUS=1` plus `NDS_R2_FIGHTER_RUN_PROOF=2` together
  overflow it by 172 bytes. The two instruments can no longer be built into one
  ROM. Measure with one at a time, and put anything new on that call chain
  behind `noinline` outside `.itcm.native_fighter`.
- **E12's memo carries a staleness protocol already** (`entry->ready`,
  `entry->name`, `memo->slot_plus1`). Extending the payload must not weaken it:
  the texture cache entry can rotate underneath a run, which is what
  `gNdsR2TexMemoStaleCount` exists to catch.
