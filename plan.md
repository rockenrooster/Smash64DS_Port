# Runtime2 P95 Closure Plan — v2 (merged review)

> **Current-bank override — 2026-08-16.** Later retained work supersedes the
> historical numbers below. `build-c235-itcm-final` is byte-identical to the
> freshly measured c234 ITCM candidate and is now the restart basis:
> **WORK-H P50 885,440; P90 1,064,128; rank-80 1,164,416 raw / 1,139,469 net;
> top-1% 1,470,720; 95/1600 over gate; exact remaining net gap +19,089.**
> Evidence: `artifacts/performance/2026-08-16_itcm-repack/ITCM_REPACK.md`.
> The ITCM campaign reclaimed 4,108 B of safe cold-inside-hot code and refilled
> it with ~4,312 B of ranked source-owned hot leaves; final `.itcm` leaves 16 B.
> Do not re-use c223's +48,081 requirement as the current level.

Date: 2026-08-14
Branch: `codex/r2-runtime2`
Supersedes: plan.md v1 (the "~174K" plan) and the four reviews of it. This version reconciles all of them against the banked evidence.

## 0. The authoritative numbers (do not re-derive, do not quote v1's)

> **RE-BANKED 2026-08-14 on settled HEAD `a159069af0d` — `build-c158-gate`, `BOTH_CPU=1`/`DRAW=1`,
> DLDI on, frames 440–2039, 1,600 samples** (`…/2026-08-14_runtime2-p95-closure/GATE_ARM_OWNERS.md`):
>
> ```text
> WORK-H  P50 939,136 · P90 1,096,448 · P95 1,184,064 raw / 1,159,117 net
>         top-1% 1,562,560 · max 5,112,896
> gap     +63,684 raw / +38,737 net
> the 80th-largest frame must fall  64,452   (was 91,844)
> cadence 89.1% two-VBlank, max interval 6   (DRAW=0 arm, owner's §6 choice; DRAW=1 reads 84.9%/max 10)
> ```
>
> **The −26,880 against 1,210,944 is NOT a cost win and is not banked as one.** End-of-match
> invariants diverge (P1 damage 58 → 76, spark 14 → 15, AObj high-water 1,177 → 1,266): the Fox
> bore-84 fix changed the fight, so `route-ab-cannot-price-gameplay-change` applies. It is a new
> baseline for a different match, not a saving. **Caveat: Boundary was RED when this was taken** —
> §13 owns clearing it, and until it is green this bank sits on an unverified tree.
>
> **GATE SCORING — DECIDED 2026-08-14 by the owner.** The board's "loading states excluded" wording
> does **not** exempt in-match asset-load frames. Every banked P95 uses **all 1,600 samples with no
> exclusion**, and a mid-match asset load is an ordinary gameplay frame the player watches. The
> in-match animation-asset load I/O mechanism is therefore **in scope and is the largest item on the
> board**. Do not re-open this and do not publish an exclusion-adjusted P95.

Current HEAD control `build-c147-ctl` (both-CPU gate arm, DLDI-on, `NDS_TICK_HUD_DRAW=1`, 1,600 samples — `artifacts/performance/2026-08-13_c-collision-stack/a-c147-ctl-rows.csv`, matches the HANDOFF bank):

```text
WORK-H P50 / P95 (raw):      924,864 / 1,210,944
net of approved apparatus:   ≈1,185,997   (apparatus ≈24,947, RESIDUE.md §5; owner approved net scoring 2026-08-13)
HUD-clean P95 (diagnostic):  1,202,944    (gap +82,564)
target:                      1,120,380
gap:                         +90,564 raw / +65,617 net
```

Cadence truth (same rows): 1,360/1,600 frames present in 2 VBlanks (85.0%; target ≥95%). The cadence boundary sits at WORK-H 1,116,096 — 0.4% under budget — so **WORK-H is faithful**; v1's "idle migrates into WORK-H" hypothesis (its Phase A4) is refuted by this one line and must not be re-investigated.

**The real requirement**: convert ~160 of the 225 three-VBlank frames. The 160th-cheapest dropped frame sits at WORK-H 1,210,944; the cut needed **on those frames** is ~94,848. The HUD draw burst (mean 443,176, twice a second) owns 110 of the 240 dropped frames but only ~9,000 of P95 — cadence deficit is over half instrument; the P95 gap is real.

**CORRECTED 2026-08-14 by Phase 0 (`…/2026-08-14_runtime2-p95-closure/MARGINAL_OWNERS.md` §2).**
The paragraph above conflates two different frame sets, and its 94,848 is the worst frame's
requirement, not the set's:

```text
cadence set  the 160 cheapest DROPPED frames        -> governs the >=95% two-VBlank target
P95 set      the 80 most EXPENSIVE frames by WORK-H -> governs P95 <= 1,120,380
             (P95 of 1,600 samples IS the 80th-largest value)
the 80th-largest must fall 91,844   (1,212,224 -> 1,120,380)
of the 160 cadence frames: 102 are ALREADY below the 1,116,096 cadence boundary in WORK-H,
98 of those carry the HUD burst; only 58 are genuinely WORK-H-bound, mean cut 43,916
```

**Design against the P95 set.** A change that removes 30,000 uniformly moves P95 by 30,000; one
that removes 100,000 from only the top 20 frames moves P95 by **zero**.

**The gate arm is both-CPU** (`NDS_R2_BOTH_CPU=1`; owner 2026-08-05). The Boundary arm (human Mario) already passes and must never be presented as the gate figure. Every figure below is gate-arm unless labeled.

v3 stall attribution already exists and already ran (attributor in-repo at `emulators/melonds-attributor/melonDS.exe` since 07-27; capture at `artifacts/performance/2026-08-14_icache-temporal/v3-baseline/`, `stall_partition_residual=0`):

```text
icache_fill 339,275   dcache_fill 252,032   halt_wait 238,788   issue 181,256
write_buffer 47,549   interlock 39,942      bus_contention 36,282   dma_hold 8,701
cart_spin = gx_paid = gx_blamed = 0
P50→P95 excess +182,336 = icache +64,495 (35.4%) · issue +60,328 (33.1%) · dcache +41,061 (22.5%)
```

Three-way split: **no single stall owner exists.** Any plan phase shaped "if X dominates" is dead; the GX and cart branches are impossible (measured 0).

## 1. End state

**DIRECTION SET BY THE OWNER 2026-08-14: stop treating Runtime2 as "BattleShip with increasingly
optimized adapters" and turn the DS path into a compiled battle kernel.** See
`docs/architecture/RUNTIME2_NATIVE_BATTLE_KERNEL.md`. The measurements forced this, and they are §0's:
the P95 excess is spread across `icache_fill` +155,795 / `dcache_fill` +96,800 / `issue` +94,029 with
**no function above 3.6%** — the signature of too much generic machinery and too much working set, not
one slow function — and the runtime is still doing `get_fat`, `f_lseek`, copies, byte conversion,
relocation, asset-ID resolution and AObj16 normalization **while gameplay is running**.

The DS should stop emulating the *structure* of the N64 engine while preserving the *semantics* of the
N64 game. Data, scheduling, animation representation and rendering path all become DS-native and
precompiled.

**The architectural order (owner, 2026-08-14):**

```text
1  BattlePack / direct AnimClip          <- slice 1, the current assignment
2  flattened process scheduler           <- slice 2, only after slice 1 is measured and re-ranked
3  compact pose + dirty-joint evaluator
4  baked one-pass world matrices
5  direct native renderer
6  only then consider fidelity tradeoffs
```

Acceptance:

1. Both-CPU gate: WORK-H P95 ≤ 1,120,380 **net of approved apparatus**, raw and HUD-clean reported alongside; aim 10–20K under (placement safety).
2. **Architectural target, not a measured promise: ~850–900K P95 or lower for the two-fighter kernel.**
   Scraping under 1.12M with two fighters is not an architecture — the four-fighter requirement puts
   it straight back in trouble. **Do not stop architectural work merely because a slice clears the P1
   gate.** The gate is the floor; the kernel is the goal.
3. Cadence ≥95% two-VBlank on the `NDS_TICK_HUD_DRAW=0` arm (§6, owner-decided).
4. Boundary green; stress battery (Sudden Death, restart chain, 1-min + 5-min) stays green; RAM/heap/ITCM/static checks green.
5. Gameplay mechanics, collision decisions, RNG ordering, level-3 CPU behavior source-faithful except owner-approved trades.
6. Retained ROM recorded (path, length, SHA256, flags, verification artifact). Owner does only final acceptance.

**Migration method, and it is not optional: the generic BattleShip-compatible path remains the
correctness ORACLE and fallback, never the normal DS hot path.** Every new DS-native path first runs
in a verification mode where old and new both produce outputs and are compared — animation
values/events, matrices, process ordering, collision decisions — and the shipping configuration stops
running the generic half only after mismatches are **zero**. That is what makes radical
representation changes safe without sacrificing source fidelity. **Do not ask the owner to debug
intermediate versions.**

## 2. Standing measurement rules

- **Units**: 2 profile cycles = 1 tick; ticks/frame = cycles / (2 × regions). Never mix domains; any inherited census number gets a units audit before reuse (one census was 2× wrong exactly this way).
- **Build floor**: ≥16,000 ticks/frame predicted before any ROM candidate; prefer ≥30,000 for architecture. Smaller proven pieces ride a larger package's build only.
- **Placement**: classic cross-build floor ±8,544; measured one-line candidate spread ±24,064. A two-build comparison under ~17K proves nothing. The standard verdict instrument is **A/B/A with a flag-falsifier third arm** (candidate layout, dispatch reverted — slice-51 method; it bracketed to 2,752). Same-binary `.data` routes only for state-invariant changes with the poke logged before first read.
- **P95 discipline**: rank on the marginal frames (the 160), never whole-match mean. Report P50 / P90 / P95 neighborhood / top-1%, plus the 2/3/4/5+ VBlank histogram and max. Per-frame diffs between two arms are meaningless (different game ticks) — compare window sums.
- **Every verdict**: raw + net + engagement counters both arms + end-of-match invariant pair (exact changes must match; tolerance changes explain divergence from their logged flip budget) + lane signature corroboration.
- Reprofile and re-rank after every retained package; abandon a lane the moment it is no longer the largest owner (v1 §11 loop retained verbatim).

## 3. Closed lanes — do not reopen without new evidence (merged, with cites)

> **A CLOSED LANE IS NOT A CLOSED LAYER (2026-08-14).** Every entry below was closed as a
> *micro-optimization inside the generic architecture* — "make this function faster", "memoise this
> value", "repack this struct". The Native Battle Kernel (§1) does something these lanes never
> attempted: it **deletes the layer**. A lane closed because its arithmetic could not be improved says
> nothing about whether the work should exist at all. Re-opening one of these as a micro-optimization
> is still forbidden; subsuming one into a kernel slice that removes its reason to run is the whole
> point. **State which of the two you are doing.** The clearest case is item 10's FGM/collision
> family and the animation lanes in `docs/HANDOFF.md`: closed at the tick level, and slice 1 makes a
> large part of them unreachable.

1. Link-order/code placement: **CLOSED on temporal evidence** (`2026-08-14_icache-temporal/ICACHE_TEMPORAL.md` — capacity/reuse-distance dominates; falsifiers ran).
2. Fighter StateSpan / per-run AOT descriptors: refuted; certification collapsed to ~640 ticks (`2026-08-14_fighter-run-descriptor/FINDINGS.md`).
3. Local-matrix recompute framing: false premise — builder runs ~101,569/match ≈ once per bound joint; world cache already deduplicates ancestors. Memo killed twice. The live question there is the **20× machinery-to-math ratio** (probes, traversal, gathers, out-of-line calls, copies) — an architecture question, Phase 4 material only if the marginal-frame ranking selects it.
4. Call-frame/prologue shaving: best safe package 5,272 (`CALL_FRAME_SLICE.md`). Task36 DMA busy-wait: 8,315. Both under floor.
5. AObj layout family: all shapes priced −3,379 or worse; FTANIM dense-bank play-time consumption refuted (ceilings 4,578/5,246).
6. SHDT broad-phase: source already rejects ≥97% of pairs (`ftmain.c:3076`); whole fighter-pair search ≈11,200 deleted outright.
7. Particle **update** cadence: RNG-fatal — one LCG serves CPU AI (65 sites), particles, effects, items. Only **draw-side** degradation exists, and it is owner-ladder material.
8. 30 Hz blanket poses: regressed and diverged. Compensated 30 Hz sim exists only as the owner's last-resort rung (−119,744).
9. Formatted newlib I/O in the battle set (`_vfiprintf_r`, `_svfiprintf_r`, `__ssvfiscanf_r`): **CLOSED ON COST 2026-08-14, and the "instrument-only" reason was WRONG** (`MARGINAL_OWNERS.md` §6). The family is linked and *reachable* at `NDS_TICK_HUD=0`: `ndsRelocAssetFindEntry` builds a NitroFS path with `sniprintf` on every animation-asset lookup, ungated. `--gc-sections` does not drop it, so absence is not available as proof. Residual bounded at **≤769 tk/frame**, 20x under floor. Closed — do not re-open, and do not re-quote "instrument-only".
10. FGM slot cache, collision fixed-point partial ring, FindPlanned index: state recorded in `2026-08-13_c-collision-stack/STACK.md` — Phase −1 lands or reverts that staged evidence; do not re-derive it.
11. GX/cart stall branches; "is I-cache important" preliminaries; v3 adoption/installation work; temporal I-cache tracing — all complete or impossible per §0.

## 4. Phase −1 — Settle the tree (first, one cycle, minimal builds)

The worktree is dirty with a staged-but-uncommitted collision-stack experiment (arms c147–c150) plus modified renderer/platform sources, and HANDOFF records two verifier reds from existing edits. No measurement is trustworthy on this tree.

1. Read `STACK.md` and the four arm artifacts; land the verdict the evidence supports (KEEP commits with Boundary green, or clean revert with the refutation note). Explicit paths; owner's own files (`AGENTS.md`, `docs/OPTIMIZE_LIST.md`, BUGS wording) untouched.
2. Repair the two verifier reds or revert their causes — Boundary green on the settled HEAD.
3. Re-confirm the c147 bank reproduces on settled HEAD (one gate run only if sources changed).
4. Owner playtest queue stays live (Fox offset v2 24→36, Dream Land BG stretch, Whispy pixel-cadence probe) — bugs-first standing order; fold into this or the next cycle.

## 5. Phase 0 — DONE 2026-08-14, zero builds (`…/2026-08-14_runtime2-p95-closure/MARGINAL_OWNERS.md`)

**Its §1 refuted this phase's own premise: the join below does not exist.** The repo's only v3 stall
capture is `build-c125-profile`, `BOTH_CPU 0` / `DRAW 0` — the **Boundary** arm. The c147 rows are
`BOTH_CPU 1` / `DRAW 1`. Different arm, different binary, different match; every other profile on
disk is v2 with no stall classes. The cycle re-scoped onto two axes and never joined them.

Results, all carried forward into §9:

1. **Gate arm, bracket granularity, no profile needed.** On the 80 frames that set P95 the excess is
   **+520,718**, of which **SRC +479,816 (92.1%), all inside `gcRunAll`** (`SRC − GCRA` = −68 here,
   −64 on the cadence set — arithmetic proof of the nesting). `FTR` 1.03x · `STG` 1.00x · `MISC`
   1.15x: **the draw side is 4.8% of the excess.**
2. **The 123,773 pool is NOT a lane.** Reproduced to the tick (123,772), largest holder 9.1%, top
   nine 41%, every one already owned by a named lane; `tickGetCount`+`cpuGetTiming` are 13,406 of
   approved apparatus. Only pool-shaped items are `memset` (10,570 wbuf) and
   `ndsRendererSyncTextureTile` (3,945) = 14,515 whole match, under floor. **Do not build on it.**
3. **Nesting resolved from the bracket sources** (§5 of that doc): `SRC ⊃ GCRA ⊃ {SINT ⊃ SCPU,
   SHDT, SPHD/SPHC, SCAT, SPRM}`. Take `SRC` **or** its children, never both; inside `SINT` take
   `SITR` + `SCPU`, never `SINT` + `SCPU`.
4. Item 9 above corrected in place.

**The instrument Phase 4 needs does not exist yet: there is no v3 stall capture on the gate arm.**
That is the next cycle's work, not a re-opening of this phase.

### (historical) Phase 0 as briefed

Everything needed is on disk: the 3.7 GB v3 `arm9-profile.csv` carries every stall class per PC per region, and the c147 rows identify the 160 marginal frames.

1. Mask to the 160 marginal frames. Rank `issue + icache_fill + dcache_fill + write_buffer + interlock + bus_contention` by PC/function on those frames only. Output `MARGINAL_OWNERS.md` with the §0-style fill-in completed.
2. Attribute the unexamined pool: write_buffer 47,549 + interlock 39,942 + bus_contention 36,282 = **123,773 tk/frame no lane has ever looked at** — larger than the whole gap. Name its top functions and whether any is actionable.
3. Resolve GCRA / SRC / SINT bracket nesting (marginal deltas +87,358 / +87,294 / +48,173 — likely nested; must not be triple-counted).
4. Confirm §3 item 9 (newlib I/O unreachable at `NDS_TICK_HUD=0`).
5. Deliverable decides Phase 4's target. Until it exists, no production optimization beyond Phases 1–3 below (which are already sized/named).

## 6. Phase 1 — Instrument cadence decision (owner, one question)

The HUD draw burst costs 6.3 cadence points on the instrumented arm. Net P95 scoring is already approved; cadence acceptance needs one owner choice:

- (a) read the histogram/cadence acceptance from a `NDS_TICK_HUD_DRAW=0` arm (counters intact, burst absent — closest to the shipped ROM), or
- (b) phase the HUD draw across 8 frames (~55K/frame; instrument change, re-banks comparability), or
- (c) keep DRAW=1 as-is and accept the conservative cadence reading.

Recommendation: (a). No code risk, shipped-ROM-faithful, keeps the DRAW=1 raw series comparable.

**DECIDED 2026-08-14 — owner chose (a).** Cadence acceptance (≥95% two-VBlank) is read from a
`NDS_TICK_HUD_DRAW=0` arm: counters intact, HUD draw burst absent, closest to the shipped ROM. The
`DRAW=1` series remains the raw/net **P95** bank and stays comparable; do not phase or slim the HUD
draw. Every cadence figure reported from here carries its `NDS_TICK_HUD_DRAW` value explicitly.
Phase 1 is closed — it needs no agent cycle.

---

# THE NATIVE BATTLE KERNEL (owner, 2026-08-14) — this supersedes Phases 2–6 as the primary direction

The sections after this one (§7 Phase 2 … §11 Phase 6) are the **legacy micro-optimization lanes**.
They are retained for their evidence and their closed verdicts, and a piece of one may still ride a
kernel build as a finisher, but **none of them is the next task**. The next task is slice 1.

## K-RESULT. Slice 1 phases 1–4 are DONE (2026-08-14, zero builds) — and they moved the target

`…/2026-08-14_native-battle-kernel/BATTLEPACK_ANIMATION.md` + `docs/architecture/RUNTIME2_NATIVE_BATTLE_KERNEL.md`.

**The board's named mechanism was right about the layer and WRONG about the cause.** In-match
animation **file I/O** is **15 loads in a 1,600-frame match, on 13 frames**. All 13 sit inside the 80
that set P95 — and deleting those 13 frames *entirely* moves rank-80 by **9,874** against a **64,452**
requirement. **The FAT read cannot be the lever.** (Related correction: the `get_fat`/`f_lseek` in
that mechanism are **majority BGM** — 428 `get_fat` per seek, so the cluster-chain walk drives it, and
≤38.8% of the P95-set `get_fat` is even on an animation frame. Sizing off the full +15,058
over-predicts ~2.6×.)

**The lever is the ACQUISITION PATH, and it has exactly the presence law 1 demands:**

```text
299 acquisitions, 95.0% cache HITS (284 hit / 15 miss)
62 of the 80 P95 frames   vs   174 of 1,520 body frames   =  6.8x presence
dose-response, whole population:
    first acquisition on a frame   +148,969
    each additional                 +77,440
    a miss                         +645,225
modelled full deletion at rank-80  -73,659   (projection, upper-bound model, profile arm; NOT banked)
```

A cache **hit** still memcpys the whole payload into caller heap, re-registers a loaded file,
re-finalizes, re-normalizes AObj16, strips alias status nodes and writes three status entries. This
is precisely the "recreate an N64 loaded-file image on every action change" that §K0 exists to
delete — and −73,659 modelled against 64,452 needed says deleting it is sized to close the gate on
its own.

**Two findings that make slice 1 cheaper than designed:**
- **`NDSAnimInstance` does not need inventing.** The immutability inventory found the parser makes
  **zero writes** through `anim_joint.event16` — mutable playback state already lives in the
  DObj/AObj pair. **The adapter copies only because internal fixups write *absolute* pointers.**
  Remove those and the reason to copy goes with them.
- **The host parser already existed.** Slice 32 built a proven figatree parser, semantic model and
  bake (`ftanim_reloc_probe.py`, `ftanim_script_model.py`), validated at 100% of the bank. Phase 4
  reused it as the oracle instead of writing a second decoder.

**Phase 4 equivalence: mismatch = 0.** 297 clips / 5,629 scripts / 77,959 commands / 71,500 per-track
states / 5,629 event callbacks / 105,304 target words; corpus `456834182c5d…`; two falsifiers prove
the test can fail.

**Phase 3 verdict: THE PACK DOES NOT FIT.** Complete pack **651,928 B** (lossless compaction floor
645,450) against **~511,904 B** proven RAM — **short ~140,024**. No lossless compaction closes it:
dead tails 1.0%, exact dedup 4.6%, substring merge 0.004%. Per §K1 phase 3 this may **not** fall back
to gameplay-time FAT loading. Three admissible routes, in cost order:
1. **Recover the RAM the copy itself occupies** — the per-status `syTaskmanMalloc` animation volume
   that a `const` shared clip makes unnecessary is **not counted in the 511,904**. Cheapest move, one
   gdb read, and it may close the gap outright.
2. **Prove the items-off exclusion** — 38 clips, 101,472 B, 14.4%, sized but **not** proven from the
   status graph. Alone it is 72% of the shortfall.
3. A more compact clip representation, or a deterministic pre-GO arena.

**Also corrected:** the reachable asset set is **all 301** (`ftdata.c`: 143/143 Mario, 158/158 Fox);
the 87-entry warm list is 28.9% of it and **observational**, which is *why* assets get requested that
are absent from it. And `gNdsRelocAssetOpenFailCount` is conflated across 11 writers — in-match
`fopen` runs 15 times and all 15 succeed, so every in-match increment is the status-node decline
(confirms the prior cycle's observation; the counter should be split).

## K-RAM. The pack does not fit, the closure is named, and the order is now forced (2026-08-14, zero builds)

```text
                                   pack        pool        short
full pack (297 AObj16 clips)      651,928     511,904     140,024
items off (259 clips, PROVEN)     553,696     511,904      41,792
  + route 1 (figatree heaps)      553,696     524,352..537,120   29,344..16,576
  + matchup lead (245, UNPROVEN)  528,624     524,352..537,120    4,272..-8,496
```

Only the last row goes negative, it spends the pool to **zero reserve**, and half its exclusion is
unproven. **Not a fit.**

**Three premises corrected — two of them mine, from the brief that commissioned this:**
1. **The "192,240 warm arena = 1.37× the shortfall" route was a DOUBLE COUNT.** `HANDOFF`'s 192,240
   is `sNdsR2AnimCacheArenaUsed`, the bump high-water *inside* the 262,144 B reservation
   (`reloc_backend_assets.c:6377`, `:6470`) the pool table had already banked. Not a second pool.
2. **The real route 1 is 12,608–25,216 B and needed no gdb read.** `gFTManagerFigatreeHeapSize` is a
   compile-time max over the loaded kinds (`ftmanager.c:170-206`; max payload 6,224 B), taken one per
   active player plus one per player on Sudden Death entry (`scvsbattle.c:199`, `:472`), never freed.
3. **The pool's three terms are not additive.** The arena is one `calloc` of
   `NDS_TASKMAN_ARENA_SIZE 0x150000` (`diagnostics.c:7750`, `:7791-7849`), so freeing the anim arena
   releases no libnds heap. A `.rodata` pack draws on **211,936** alone; an arena-resident pack on
   **299,968** alone.

**Items-off is PROVEN, and from the linked ELF — stronger than the status graph that was asked for.**
Every function that can set an item status is a two-byte `bx lr` stub
(`ftCommonItem{Throw,Swing,Shoot,ShootAir}SetStatus`, `ftCommonLightThrowDecideSetStatus`,
`ftCommonHammerFallSetStatus`, the `W` cluster at `0x208fc34..0x208fc74`) and **the ELF holds no item
spawner at all**. Priced by *re-packing*, because dedup makes a subset non-linear: 651,928 →
**553,696**, re-verified at **mismatch = 0**, corpus `c034b342…`.

**A more compact clip representation is REFUTED as a lossless lever.** The AObj16 stream is already
u16 command headers and **s16** target words (`ftanim_reloc_probe.py:197-209`) — no f32 to narrow, and
no dictionary beats a 16-bit alphabet. A pre-GO arena creates no RAM at all. **Both of §K1 phase 3's
route-3 options are dead.**

**THE CLOSURE — `docs/RAM_RECOVERY_PLAN.md` Phase 2. LANDED 2026-08-15, `8cfbc2eaa2b`.**
`gSYFramebufferSets[2][230][320]` → `[1][231][320]`, 294,400 → 147,840 B. **146,560 B recovered,
measured**: `.main.bss` 1,453,544 → 1,306,984, proven headroom 174,368 → **320,928**. Boundary green
both arms. The span was re-derived from the wipe's own compiled literals (`+0x23f14` start, −640 row
step, 220 rows, 600 B/row → `base+7,060..base+147,819` inclusive, 231 whole rows) — the inherited
numbers were right. The linked-ELF reader sweep also found a **second reference the doc comment
missed** (`mntitle.c`'s `[1]`/`[2]`), which the collapse would have turned into dangling pointers;
fixed at the same seam.

> **RETRACTED: "+146,560 makes the full pack fit by 6,536."** I wrote that here and it was wrong —
> it is arithmetic against the **combined** 511,904 pool, and §11.1(b)'s own finding that the three
> terms are **not additive** kills it. Measured on the *published* arm, static headroom goes
> 213,216 → 359,776, so a `.rodata`-resident full pack is still short **292,152** and items-off short
> **193,920**; an arena-resident pack still draws on 299,968 alone. (The 211,936 the pool table used
> was the **proof-harness** arm — 208,672 today — not the shipped ROM.)
>
> **Only the combined pool fits** (full by 7,816, items-off by 106,048), and reaching it requires a
> three-file coupling: cut `NDS_TASKMAN_ARENA_SIZE`, lower the `0x130000` search floor, and reteach
> the Task 36 replay-admission guard (`src/nds/nds_renderer.c:5734-5739`). **Phase 2 is what makes
> that coupling worth doing** — the freed RAM is exactly the libnds-heap slack a larger arena draws
> on. **Which pool the pack lands in is the open sizing decision**, and it is engineering, not a
> fidelity trade.

**And the adapter has a SECOND reason to copy that the pack cannot touch.** The absolute-pointer
fixups are only one. `decomp/…/ft/ftmain.c:4623-4624` **discards the return value** and hardcodes
`fp->figatree = fp->figatree_heap`; `src/import/battleship_ftmain.c` `#include`s that body rather than
owning it. (The board closed the zero-copy seam on this exact line in cycle 108; the architecture
doc's inventory never mentioned it.) The unblock is a one-line, **today-inert** patch under
`scripts/decomp-patches/battleship/` making the returned pointer authoritative — `fp->figatree` is
read only at `:4628` and `:4704` and never assumed equal to the heap.

**The order is therefore forced.** Phase 5 cannot be built first: the only alternative to Phase 2 is a
~300 KB resident subset that fits today's pool, and that is *a bigger cache*, which §K0 forbids.

```text
RAM_RECOVERY_PLAN Phase 2 (+146,560)  ->  the inert decomp patch  ->  slice 1 phase 5
```

## K-POOL. The pool is the taskman arena, one fighter fits, and phase 5 is built behind a default-0 flag (2026-08-15, `9d660c8e08f`)

**`.rodata` is disqualified, and the reason invalidates how this project has been sizing static
growth.** A `.incbin` Fox pack grew the ARM9 image **+288,992 B**, and
`gNdsTaskmanArenaChosenSize` fell **`0x150000` → `0x140000`** with `gNdsTaskmanArenaAllocFailCount 16`.
**The static image and the arena are the same bytes** — the arena is one `calloc` from the libnds heap
inside `fake_heap_start` bounds. Against the match's measured 1,304,068 B taskman use, a `0x140000`
arena projects a general-heap low-water of **~6,652 against the mandated 32,768 floor**.

> **`check-boot-headroom.ps1` CANNOT SEE THIS.** That arm reported **66,784 B of PROVEN headroom**.
> The ladder records boot / no-boot, not arena size — a static-image change can pass the ladder and
> still starve the arena. `gNdsTaskmanArenaChosenSize` is now in the soak's global list so the next
> static growth reads it. **Never price static growth on the headroom ladder alone again.**

**The fit: ONE fighter resident in the arena**, replacing the 262,144 B raw-file cache — Mario
**271,728** (+9,584 of the 39,420 slack) or Fox **287,904** (+25,760). Zero static growth, arena stays
`0x150000`, ladder untouched. Per-fighter split costs a measured **136 B** of dedup. **Both fighters
resident needs ~559,632 against ~301,564 available — it does not fit**, so slice 1's first landing is
one fighter and the other keeps the generic path. Measured headroom this cycle: tick-HUD arm
**320,928** (the binding one); proof/published arm **355,264** — the earlier 359,776 was a *prediction*
because the published ROM has not been rebuilt since Phase 2.

**Phase 5 is BUILT, `NDS_R2_BATTLEPACK` default 0, Boundary green at the default.**
`ndsRelocForceLoadFighterAObj16File` returns a `const` pack pointer, deleting the memcpy, alias strip,
loaded-file registration and the whole finalize chain. Two further seams had to be taught the pack:
`ndsRelocFindKnownFileContaining` (rebase slot words against the blob) and
**`ndsRelocPointerIsFighterAObj16`**, which `gcAddDObjAnimJoint` uses as its admission gate — without
it every packed script falls through to the AObjEvent32 normalizer.

**Two consumer-read findings that the design missed:**
- **BPA1 was a clip pack, not a figatree pack.** Its table held one entry per distinct *script*, but
  `lbCommonAddFighterPartsFigatree` advances one word per *DObj slot* (~26 slots vs ~19 scripts).
  BPA2 emits the per-slot table, blob-relative, 4-aligned; stray slots **0**; 91 clips carry exactly
  26 slots, so 26 is the tree's bound and the walk cannot over-read.
- **THE PACK WAS SHIPPING THE WRONG BIT ORDER, AND PHASE 4's "mismatch = 0" COULD NOT SEE IT.** The
  host probe's `normalize()` is pipeline 4a–4b; the ROM also applies **4c**
  (`ndsRelocAObj16EncodeForNativeBitfields`), and the direct path skips `ndsRelocFinalizeLoadedFile`
  so nothing applied it at runtime either. **Both sides of the equivalence test used the same
  disk-order decoder**, so the stage was invisible to it. A ROM data abort found it
  (`gNdsObjAnimRunawayScript` pointing inside the linked pack, opcode 26 against a max of 14).
  Falsifier now measured: disk-order decode raises on **75 of 2,713** scripts, native **0**. Slot
  equivalence re-verified at **mismatch 0** (Fox 3,611 slots / 34,523 commands; Mario 2,976 / 32,253).
  **An oracle that shares a decoder with the thing under test proves nothing.**

**Open at flag 1, unattributed:** three Boundary attempts time out at marker 2
(`ndsRendererHardwareArmBattleStaticTextures`) and the bit-order fix did not clear it. **The control
ran** — the same target at flag 0 is green — so the defect is ours. Prime suspects: the arena
degradation starving that very preparation, or a third reader still requiring a registered loaded
file. `gNdsBattlePackHits` **has never been read on a live ROM**; the deletion is proven in source,
not in a counter.

## K-RESIDENT. The pack is resident and the deletion is PROVEN firing — and it currently costs cadence (2026-08-15, `3963b8b14ea` / `98c8482d890`)

Gate arm (`BOTH_CPU=1`), canonical 1-minute match, 2,043 presented frames both arms:

| counter | flag 0 | flag 1 |
|---|---:|---:|
| `gNdsBattlePackHits` | **0** | **197** |
| `gNdsBattlePackMisses` | 357 | 160 |
| **total acquisitions** | **357** | **357** |
| `gNdsR2AnimCacheHits` / `Rejects` | 338 / 0 | 30 / **126** |
| `gNdsR2AnimCacheArenaReservedBytes` | 262,144 | 292,032 |
| `gNdsTaskmanArenaChosenSize` / `AllocFailCount` | 1,376,256 / 0 | 1,376,256 / 0 |

**Engagement proven with a control that reads 0**, corroborated independently by
`gNdsRelocResolveOffsetCount` 0 → 3,132. Totals identical: the deletion changes what an acquisition
*costs*, not how many happen.

**Residency without static growth.** `.incbin` removed; the Fox blob ships as a NitroFS payload
streamed into the arena in 18×16 KB chunks at the `ndsR2AnimCachePreloadStep` seam. Proven static
headroom **354,208 against a 355,104 flag-0 baseline — +896 B of image, not +288,288.** The blob owns
`[0, RESERVE)` carved *at reservation*: a first implementation that let the loader allocate first lost
by **848 bytes**, because fighter setup stores 3,728 B into the arena before the first scene update.

**Marker 2 was the arena, proven by its globals** (`1,310,720 / 16` last cycle → `1,376,256 / 0`
now); the "third reader" suspect never needed invoking. Capture costs 22–23% of its ceiling, twice.

> **THE TRADE, AND IT IS THE LIVE RISK.** Only one fighter fits, so the other loses the raw cache
> (`Rejects` 0 → 126). Proof arm, same match, 2,043 frames both:
> **`gNdsBattlePlayablePacingVBlanks` 4,274 → 4,805 (+12.4%)**, present-interval buckets [4]/[5]
> 5/12 → **42/108**. **NOT ATTRIBUTED.** Candidates: the 18 streamed chunks, the 126 uncached loads,
> cross-build placement. **Phase 8 must resolve this before anything is banked.** If it holds, the
> answer is to make **both** fighters resident, not to shrink the pack —
> but note the arithmetic: both need ~559,632 against ~301,564 available, and Phase 2's 146,560 B
> does **not** cover the ~258,068 shortfall. `RAM_RECOVERY_PLAN`'s remaining phases become the lever.

**`NDS_R2_BATTLEPACK` stays default 0.** Boundary is **GREEN at flag 0** on this tree; at flag 1 one
assert remains — `battle_playable lower-screen rolling FPS counter did not sample actual presentation
cadence`, `FPS_HUD=289,14,15,16856768` byte-identical on two consecutive runs, `X10` lagging
`FrameWindow`/`TickWindow` by exactly one sample. This is the **R2-04 E2** assert, previously recorded
*intermittent at flag 0*; flag 1 makes it **deterministic**. Two hypotheses killed cheaply (a second
writer; a cache-line straddle — all four globals share one 32-byte line). `NDS_R204_FPSHUD_SHADOW`
was built for exactly this and has not been spent.

**Phase 7's assertion is NOT measured.** The seven K0 counters are zero *by construction* on the
pack-hit path (the early return precedes all of them in the same function) and `Hits 197` counts
exactly those — but no GO-gated per-fighter counter exists, so the assertion as §K1 words it remains
unproven.

## K-VERDICT. Measured: the pack AS BUILT is 2.9x the gate — because the carve DELETED the cache (2026-08-15, `79a9447fd6d`, 4 builds)

`build-c164-gate-bp0/bp1`, HEAD `79a9447fd6d`, `BOTH_CPU=1`/`DRAW=1`, DLDI on, 1,600 samples, window
439–2038, **same fight both arms** (damage 0/76, 355 acquisitions both):

| | flag 0 | flag 1 | delta |
|---|---:|---:|---:|
| P50 | 940,416 | 939,648 | **−768** |
| P90 | 1,097,920 | 1,540,032 | +442,112 |
| **P95 raw / net** | **1,186,112 / 1,161,165** | **3,447,488 / 3,422,541** | **+2,261,376** |
| top-1% | 1,570,944 | 6,118,208 | +4,547,264 |

The flag-0 arm reproduces the old bank to **+2,048** on a different HEAD, so the tree has not drifted.
**The requirement on this HEAD is 65,732 at the 80th-largest frame, not 64,452.**

**Root cause, attributed and separated — it is not the architecture, it is the carve.** The reservation
did not *shrink* the raw cache, it **deleted** it: **262,144 B → 4,096 B**. `Fills` 17→2, `Rejects`
0→126, `AnimCacheHits` 338→30. The unpacked fighter's 111 extra uncached acquisitions cost
**3,873,969 tk each**, and `SITR` goes 41.6% → **86.3%** of the P95-set excess at **36.19x** while
`FTR`/`STG` do not move. The other two candidates are excluded: the 18 streamed chunks are **≤0.3%**
(they run at presented frames ~1–9; the window opens at 439, and whole-match VBlanks +770 against an
in-window `ALL` delta of 767.6 VBlanks bounds everything outside the window at ≤2.4), and placement is
**~0** (P50 moved −768 against a ~5,700 floor; rank-80 moved **160x** the P95 floor, which is where it
stopped being the discriminator).

> **CORRECTION — do not reuse "+645,225 a miss".** With the cache gone the real figure is **6.0x
> higher**; the banked number is a *warm-cache* regression coefficient and prices nothing else.

**THE MISSING ARM, AND IT IS THE HIGHEST-VALUE THING OUTSTANDING.** No arm has ever isolated the
deletion's *benefit* — pack resident **and** cache intact. Flag 1 measures the benefit *and* the
cache destruction together, so **−73,659 at rank-80 is still a projection and is now LESS supported**:
P50 is flat because the median frame takes no acquisition, and P95 is swamped by the 111 uncached
loads. **Nothing about slice 1's architecture is proven or refuted by these numbers.**

**The RAM plan does not close it.** One fighter non-destructive needs **+258,048 B**; both resident
~258,068 — the same number. Phase 2's 146,560 is spent; Phase 1 = 21,600 unspent; Phase 4 = ≤80,096
*unsized candidates*; Phase 3's ~150 KB is **refuted**; Phase 5 unstarted. **Optimistic ceiling
248,256 — short of even the one-fighter case.**

**But the cheapest lever is not RAM at all.** The Fox blob is **287,904 B against the 262,144 B it
evicts — 1.098x its own displacement.** *A pack smaller than what it displaces closes this without
buying a byte*, and the items-off re-pack is already proven at mismatch 0 (553,696 B for both
fighters). **This is now a design constraint on the format**, recorded in the kernel doc §9.

**Blast radius of the D-cache/gdb coherency defect: NARROW.** Exposed — a multi-word group that must
be self-consistent and is published at a seam (the only shape that tears; fixed), and any value read
within ~one line-lifetime of its last write. **Not exposed — whole-run totals read at the end-of-run
stop, which is nearly every counter this campaign banks**; an 8 KB/4-way/64-index round-robin cache in
a churning match evicts a line within microseconds. ITCM/DTCM are special-cased by `ReadMem` and are
the structural remedy if a group must be coherent without a flush. Two facts from this cycle's own
runs corroborate: the flag-0 arm reproduced a bank from a different HEAD to +2,048, and acquisition
totals came out identical at 355 across two binaries — neither survives systematic staleness.
**No banked number in `HANDOFF.md` is implicated.** `sBattleTickHudRing` has the shape by
construction and is *named, not re-measured*; its guard has never fired.

## K-ISOLATION. The deletion does NOT pay, and phase 8's root cause is REFUTED — the cost is the pack PATH (2026-08-15, `3139502ad91`, 3 builds)

The arm nine cycles never had: pack resident (197 hits) **and** the raw cache healthy (`Rejects` **0**;
9 full ROM loads against the control's 17). All four gates passed before a tick figure was quoted —
arena 1,548,288 / `AllocFail` 0 / `ReserveFail` 0, heap free-min 52,864 vs the 32,768 floor, damage
0/76 and 355 acquisitions on all three arms.

| window 439–2038, rank-80 of 1,600 | P50 | P90 | **P95 raw / net** | >2M frames |
|---|---:|---:|---:|---:|
| **A** control (no pack, cache 262,144) | 940,416 | 1,097,920 | **1,186,112 / 1,161,165** | 2 |
| **B** cache-deleted (banked phase 8) | 939,648 | 1,540,032 | **3,447,488 / 3,422,541** | 130 |
| **C** ISOLATION (pack + cache 163,840) | 940,128 | 1,216,832 | **3,447,872 / 3,422,925** | 128 |

`C−A`: P50 **−288 (flat)**, rank-80 **+2,261,760**, mean +236,397, over-gate +83 — P50, mean and
over-gate all agree.

> **−73,659 IS RETRACTED.** Measured, it is the **opposite sign at ~30x the magnitude**. It was a
> projection three times over and is now a number.

**Phase 8's attribution is refuted, and this is the finding that matters.** Phase 8 charged its
+2,261,376 to 111 net-new uncached acquisitions at "3,873,969 tk each". **Arm C removes more than
those 111** (ROM loads 128 → 9) and the residual moves **+384 — 0.011%** against a ≥14,080 floor.
Two arms differing in arena size, in cache size by **40x**, and in ROM loads by **14x** do not land
384 ticks apart by coincidence. They share one cause, and the only thing they share is **the pack
path itself**. `SITR` 41.6%/2.84x → **85.7%/34.48x**, concentrated on **128 frames of mean `WORK-H`
4,118,565**; draw side flat at 1.01x.

> **BOTH banked per-acquisition prices are RETRACTED** — `+645,225 a miss` and `3,873,969 per
> uncached acquisition`. Each was **a residual divided by whichever count was to hand**. A third was
> deliberately not manufactured: `gNdsRelocResolveOffsetCount` 0 → 3,629 is the *sole* differing
> counter and is handed forward **to be priced per-PC, never by division** — the division would
> demand ~104,000 tk per resolve, which is not a plausible table-walk price.

Excluded for free by two soaks: the AObj16 normalizer is **not** re-running on packed scripts
(245=245, 225=225, 1,609=1,609) and allocation is identical (1,069=1,069).

**THE RAM LANE IS MOOT FOR THIS DECISION.** Arm C displaces nothing — it *adds* 163,840 B beside the
blob — and still costs 2.9x. **Buying the arena does not buy the win**, so §K-RAM's shortfall, the
`RAM_RECOVERY_PLAN` phases and the displacement constraint are all irrelevant until the pack path is
shown cheap at all. Task B stays `BLOCKED(decision: the pack cannot fit its own displacement
losslessly)` and **is not worth taking to the owner yet**.

**A burned 2,400 s gate run, and its lesson.** The first sizing asked +258,048 B of arena against
319,840 B of "proven headroom"; the heap granted **188,416**, yet the 550,080 reservation *inside* the
short arena still succeeded — every allocator guard passed, general-heap free hit **6,076** against
the 32,768 floor, and the battle never started. **`check-boot-headroom.ps1` meters static image, not
grantable heap** (third recurrence, first for *arena* growth), and `KEEP_FREE` cannot catch it, being
a point-in-time check that correctly saw 582,848 free. **Gate allocator arms on a 5-minute soak
first.** Both lessons are in `docs/VERIFYING.md`.

**The next build, and the last thing between here and a named mechanism:** the slice-51 falsifier —
pack resident, `ndsBattlePackFindFigatree` returns NULL. It separates the pack's **dispatch** from its
mere **presence**.

## K-MECHANISM. It was a BUG with a named site, it is fixed, and slice 1 is back on the table (2026-08-15, `a78330c5dea` / `a85ac2dfd5e`)

**`ndsRelocResolvePointerFromFileBase` (`src/port/reloc_backend_assets.c`) asked the expensive
question first.** It probed *"is `ptr` already an absolute pointer into a known file?"* **before**
interpreting `ptr` as a file-relative offset. For a clip served from the pack that probe can **never**
succeed — the generator emits blob-relative offsets, so `ptr` is a small integer — and its **miss**
path is `ndsRelocFindKnownFileContaining` falling through the loaded-file scan into
`ndsRelocFindStatusNodeContaining` over **both** status buffers, where every node visited runs
`ndsRelocStatusNodeDataSize` → `ndsRelocAssetIDForToken`, a ~300-compare chain whose full miss also
walks the 143+158 Mario/Fox pointer arrays. **Two complete status-buffer scans per figatree slot,
~18 slots per action change, ~136 node visits per slot.**

| symbol | all cycles | masked | masked/all | vs control |
|---|---:|---:|---:|---|
| `ndsRelocAssetIDForToken` | 207,877,919 | 207,366,743 | **99.8%** | **138x** lower |
| `ndsRelocFindStatusNodeContaining` | 113,559,597 | 113,559,597 | **100.0%** | **absent** |
| every other symbol | — | — | ≈10.8% | the base rate |

92.4% of the masked work excess; 2,329,254 tk per acquisition frame against the gate arm's
+2,261,760 at rank-80 — **two instruments, two arms, two windows, 3.0% apart.** No bracket could
have named it: the excess lands in `SINT`, `SPHD`, `SCAT`, `SPRM`, `STG` or `MISC` depending on which
proc reached the figatree attach.

**Falsifier: presence 1.6%, dispatch 98.4%.** Blob streamed, validated, adopted and carved
identically with `Hits` 0 against a control reading 197 → rank-80 **1,222,464**.

| arm | cache | arena | P50 | **rank-80 raw / net** | max | mean | >2M |
|---|---:|---:|---:|---:|---:|---:|---:|
| A no pack | 262,144 | 1,376,256 | 940,416 | 1,186,112 / 1,161,165 | 2,300,928 | 960,540 | 2 |
| C pack, defect | 163,840 | 1,548,288 | 940,128 | 3,447,872 / 3,422,925 | 7,245,056 | 1,196,937 | 128 |
| **G pack, FIXED** | 163,840 | 1,548,288 | 938,848 | **1,170,048 / 1,145,101** | 2,182,016 | 955,581 | **1** |
| **H pack, fixed, SHIP arena** | 4,096 | 1,376,256 | 938,784 | **1,435,904 / 1,410,957** | 6,401,536 | 988,452 | 8 |

`G−C` **−2,277,824** at rank-80, mean −241,356, >2M 128→1, `SINT` max 6,454,592 → 810,176, engagement
identical and `gNdsRelocResolveOffsetCount` **3,629 = 3,629** (same slots, different path).

> **`G−A` is −16,064, barely over the ≥14,080 floor — NOT banked as a P95 win.** The supported claim
> is *"no worse than the control, leaning its way"* (P50 −1,568, mean −4,959, over-gate 135→123,
> >2M 2→1 — all one sign). **−73,659 stays retracted. WITHDRAWN: "slice 1 is refuted as a P95
> lever"** — that verdict was measured on a binary carrying this defect.

**`H−G = +265,856` prices the carve.** Phase 8 blamed the cache deletion; the isolation arm refuted
it; **both were right** — the cache was a passenger while the defect dominated, and with the defect
gone it is the whole remaining fare. **Kernel-doc §9's displacement constraint is reinstated and
binding.**

Shipping default proven inert **byte-for-byte**: `.text.hot`/`.text.hot.draw`/`.itcm`/`.dtcm` hashes
identical, `.main` differing in **7 bytes at 0x0c8fc0 = the embedded git hash**. (Two method notes:
a section-*size* compare is not an inertness proof, and a section-*hash* compare will always show
that one run — diff the bytes and read it.)

**Trap fixed, and it had been silently corrupting a measurement:** `-RunnerSlot` **silently overrode
`-MelonDS`** (`scripts/lib/melonds.ps1:542` ignores it for slot ≥ 0), so a "v3 attributor" census
returned the **v2** build with no stall columns while every banner said "census". Cost one 25-minute
run; `run-task37-profile-census.ps1` now throws on that invocation. **No stall-class split was taken
or claimed** — cycles and instructions were enough to name a function at 138x its control.

## K-SHIP. The arena is legitimate, the format problem is GONE, and one stale-GDB-read defect is all that is left (2026-08-15, `d6734cb6a10`/`323101271bb`/`450470c716a`)

**The arena growth does not BUY room — it REPAYS the pack's own reservation.** The brief's premise
was inverted, and that is the finding:

```text
                              control       arm G      delta
NDS_TASKMAN_ARENA_SIZE      1,376,256   1,548,288   +172,032
  anim-arena reservation      262,144     451,776   +189,632
arena left to taskman       1,114,112   1,096,512    -17,600   (measured -17,472)
```

One mechanism, fully accounted from the source constants. **Stress battery run on
`build-c168-packfix-bp1` itself** — the exact binary whose rank-80 is banked, so reserve and tick
figure share one ROM: 660 s, **12 battle-scene entries, 7 completed matches, 7 START restarts,
4 Sudden Deaths, `NO-FREEZE`**; `ChosenSize` 1,548,288 · `AllocFail` 0 · `ReserveFail` 0 ·
`Rejects` **0** · `SyMallocOverflow` 0 · heap low-water **52,400** (single match 52,864 — flat across
the chain, because `syTaskmanStartTask` rewinds the heap per entry so Sudden Death's figatree heaps
do not accumulate). Reserves: **16,384 B** under the grantable ceiling (`AllocFail 0` is the direct
proof) and **19,632 B** over the 32,768 floor.

**And the control is the arm that is starved**: identical battery, heap low-water 69,872 — but it
**refuses 21 animation loads where arm G refuses none.** Two fighters do not fit 262,144; one fits
163,840 at 83.7%.

> **KERNEL-DOC §9's DISPLACEMENT CONSTRAINT IS WITHDRAWN AS BINDING.** No clip drops, no lossy
> stream, **no fidelity trade goes to the owner.** The 25,760 B format problem does not exist.
> Nothing in slice 1 waits on RAM any more.

**What is left is one defect, and it is the instrument again.**

| arm | arena | Boundary |
|---|---:|---|
| flag 0 | 1,376,256 | **GREEN** |
| flag 1, **shipping** arena | 1,376,256 | **GREEN** — this arm exonerates the pack: residency, streaming, carve and the whole dispatch path pass |
| **arm G** | 1,548,288 | **RED** — `locked-30 … drawLead=-1` |

Only the arm that *moves the allocator* fails. `drawLead = DrawCalls − PresentedFrames = −1` is **not
guest-reachable** (`taskman_seam.c:4903` then `:4935`, no return between, reset together; the only
other `DrawCalls++` is the fast-logic path). It is a **stale GDB read** — `ARMv5::ReadMem` has no
DCache lookup — **the same defect class as R2-04 E2, on a different counter group**, deterministic and
reproducing byte-for-byte across two independent builds.

**This closes an either/or `KNOWN_ISSUES.md` has carried since 2026-08-09**: `phaseLag=-1` under
`NDS_R2_CAMERA_MATRIX_LEAN=3`, which is held off by default *because of it*. In both rows the counter
written **first** reads low and the trigger was a heap-layout move — and **a stale read is always
behind, never ahead**, which is the asymmetry that picks this answer over "a real off-by-one".
Remedy: one `DC_FlushRange` publication seam, precedent `ndsPlatformPublishBattleFpsHudGroup`.

**Correction to an inherited premise: Boundary does NOT pin `gNdsTaskmanArenaChosenSize == 1376256`.**
Both sites sit inside `if ($Task34StageStreamCensus)`, which `verify-battle-playable-harness.ps1`
never passes — it is a Task 34 census *lab* gate on a lab build built at defaults, where the value is
still exactly right. **Nothing was retaught; loosening it would have deleted a working check.**
(Also checked: the Task 36 replay guard's legacy `!= 0x150000` form would have silently disabled
rigid-stage replay on arm G — both target blocks force `NDS_TASK53_REPLAY_ARENA_FIX := 1`, and
`Task36ReplayArenaStaleCount` 16,914 against the control's 0 confirms it.)

## K-SEAM. Banked on the shipping candidate: net +32,593 to go. The instrument was misreading itself. (2026-08-15, `10d14f56df8`/`7309ce785ad`)

**Banked, `build-c170-seam-bp1`, ROM `85904985E459439A…`, `BOTH_CPU=1`/`DRAW=1`, DLDI on, 1,600
samples, window 439–2038:**

```text
WORK-H  P50 940,320 · P90 1,091,520 · P95 1,177,920 raw / 1,152,973 net
        top-1% 1,518,528 · max 5,277,248
net gap +32,593        VBI 2:1745 3:272 4:13 5+:8 max 19, slips 0
invariant pair 0/76 — identical to c168 and the flag-0 control
```

Against the no-seam arm that is +7,872 raw, **below the ≥14,080 floor — unchanged**. Seam cost
bounded ≤450 tk/frame from the image (82 Thumb instructions, 20 `blx armDCacheFlush`).

> **RETRACTED: "arm G is Boundary-RED."** A no-seam arm G control (publish symbol absent from the
> ELF, `nm`-verified) **also passes**, so the inherited red does not reproduce on this tree, and the
> red→green flip is not the proof. **The real evidence is the quantity**: with the group published
> `gNdsBattlePlayablePacingPresentedFrames` reads **212** on five arms; unpublished it reads **211**
> on two — *uniformly one behind*, which is legal and therefore passes. The allocator move is what
> turned uniform lag into tearing.

**The gate instrument was misreading its own frame counter at a third of its stops.** Independent
corroboration of the seam: tick-HUD ring stops went **5-of-16 skewed → 0-of-16**, and the sampler's
`repeated a presented frame (3 of 1600)` warning disappeared.

**The shape was fixed, not the site.** Each debugger-read group is one X-macro list beside its
externs (`NDS_BATTLE_PLAYABLE_PACING_GROUP` 14, `NDS_GCRUNALL_TASKMAN_GROUP` 6,
`NDS_BATTLE_FPS_HUD_GROUP` 4), publish generated by `NDS_PUBLISH_DEBUGGER_GROUP`, and
`check-gbi-decode-fixtures.ps1` (which Boundary runs) requires each list and its marker `printf` to be
the same set **both directions** — falsifier run, it names the dropped member. Publishing the pace
group alone would have been half a fix: `taskmanPresentLead = tmPace[1] − 2·bp[4]` **rests at exactly
0** at that stop, so pinning one side coherent frees the other to read stale. Found by the sweep, not
by a failure.

**LEAN=3 did NOT close.** It passes Boundary *with the seam removed*, so the 2026-08-09 reproduction
is dead and the seam cannot be credited. Row updated with the negative rather than closed on
inference.

**Phase 6's oracle is blocked STRUCTURALLY, and this reframes what slice 1 actually shipped.** There
is no `NDSAnimClip`/`NDSAnimInstance` anywhere in the tree — **K1 phase 5 as worded is unbuilt**, so
an oracle would have only one arm, which is exactly the shared-path failure phase 4 already paid for.
What shipped is the **file architecture removed** (build-time blob, native bit order, `const`
pointers, no copy/register/finalize/normalize) *without* the representation change. That is slice 1's
stated purpose — §K1 phase 5 explicitly permits the direct path to write the same DObj/AObj-visible
outputs — so **phase 6 as worded does not apply to it, and the next cycle must decide what the
equivalence obligation actually is here rather than building a one-armed oracle.**

**Acceptance is NOT met — two open items and neither is the gate:**
- **Item 3, cadence: 90.9% two-VBlank on `DRAW=0` against ≥95%.** HUD draw owns ~5.3 of the 14.4
  points; **the product owns 4.1**. Unattributed.
- **Item 4: no 5-minute-match soak.** The 660 s chain battery ran (`NO-FREEZE`, 10 entries /
  8 matches / 8 restarts / 2 Sudden Deaths, all allocator gates 0, heap low-water 52,472); the long
  single match did not.

Also open, not caused by this change: two frames (1843, 1937) carry ~+4.1M each in one bucket,
unexplained — they miss the banked percentile, and c169/c166 show 3–4 such frames.

**`BLOCKED(decision: shipping default)` is raised but NOT ready to go to the owner** — acceptance
items 3 and 4 are open and phase 7's after-GO assertion is unproven. Finish those first.

## K-RERANK. Slice 1 acceptance closed except cadence — and **SLICE 2 IS REFUTED AS THE NEXT MECHANISM** (2026-08-15, `0f121aec2c2`/`48741fcaf05`)

**Item 4, soak: CLOSED.** The five-minute match ran and the *guest* says so —
`gSCManagerTransferBattleState.time_limit` **5**, logic frames **17,772 of 18,000 = 98.7%**, 8,886
presented frames, **slips 0**, every allocator gate 0, heap low-water **51,876**. `WORK-H` P95
**1,198,720** against the 1-minute bank's 1,177,920: **+20,800 across 5x the match — length still
does not accumulate cost** (third independent confirmation). Instrument deviation stated:
`soak-freeze-watch.ps1` cannot do this item (`-MinutesToRun` ceiling 12.0, one game minute ≈136 s), so
the tick-HUD sampler was used — the same instrument the 2026-08-13 acceptance battery used.

**Phase 7: MEASURED, with a control in the same run** (only one fighter fits, so Mario stays generic).
Counters sit at their own sites, keyed by fighter, GO-gated — not inferred from `BattlePackHits`:

| K0 line | Mario (control) | **Fox (packed)** |
|---|---:|---:|
| acquisitions / pack-served | 812 / 0 | **999 / 999** |
| FAT reads · seeks · byte-swaps | 21 · 42 · 21 | **0 · 0 · 0** |
| relocations · AObj16 normalizations · cache copies | 812 · 42 · 791 | **0 · 0 · 0** |
| asset→path lookups | 833 | **0** |
| token→asset-id resolves | 812 | **999 — NOT deleted** |

Cross-checks: `BattlePackHits` 999 == both denominators (zero fall-through), `AnimCacheHits` 791 ==
cacheCopies[0], seeks[0] == 2 × fatReads[0]. **The seventh line splits honestly**:
`lbRelocGetForceExternHeapFile` resolves `ndsRelocAssetIDForToken` **before** the pack is consulted.
Unpriced, recorded.

**Item 3, cadence: attributed, still NOT MET (90.7–90.9% vs ≥95%).** The 70-frame deficit is **55
`WORK-H`-bound** (81.2% inside `gcRunAll`; `SITR` 33.7%, `SPHD` 18.1%, `SHDT` 13.8%) and **15 that are
not** — `WORK-H` *under* the boundary, 13 carrying a HUD-bucket burst which on a `DRAW=0` arm is
**the game's own battle-interface OAM draw**, 1.41 bursts/s at mean 100,853, dropping 47% against an
8% base rate. **`WORK-H = (ALL−WAIT) − HUD` by construction, so the gate metric cannot see it** — a
real product cost that is structurally invisible to the number the campaign optimises.

> ## SLICE 2 IS NOT THE NEXT MECHANISM — measured, and it contradicts §1's architectural order
>
> Its entire addressable lane, the `gcRunAll` **scheduler machinery**, is **17,786 tk/fr** on the P95
> frames against **+32,593 net** required. **A 100% deletion is 1.8x short**, and a flat vector still
> calls each process. §K2's "~90% of the tail excess is inside `gcRunAll`" is true **because the
> fighter process BODIES are** — which slice 2 leaves untouched by design. The bracket that isolates
> what it *can* touch (`GCRA-REM`) is **3.6–4.4% at 1.17–1.21x**.
>
> **The ranking points instead at:** `SITR` **31.6–36.0%, never priced against** · `SHDT` at
> **16–17x presence** · the **soft-float trio at 99,762 tk/fr = 3.0x the requirement**.

**Two corrections to earlier rankings:**
- **Retracted:** `GATE_ARM_OWNERS.md`'s "only `SITR` survives across matches" — on two populations
  `SITR`/`SHDT`/`SPHD`/`SPRM` all hold within one position.
- **A false draw owner killed:** `STG` reads +53,383 / 10.2% on the 1-minute P95 set. **That is ONE
  FRAME.** Excluded: **+840, 1.00x, 0.2%.**

**The unexplained frames are characterised**: each carries an excess of exactly **2^22 ticks
(0.1252 s)** in whichever single bucket was open — ±0.02% in the tight cases, six arms, six buckets,
~1 per 2,100 frames, **predates the pack** (`c158`), not a function of game state. A power-of-two
excess in an arbitrary bucket is an accounting artefact, not work; it inflates `max` and top-1% and
misses P95. Free discriminator handed forward:
`-PerFrameGlobals gNdsRelocAssetPayloadReadCount,gNdsR2AnimCacheFills`.

**Phase 6 retired as worded** (kernel doc §13.2) — it assumes two evaluators and slice 1 shipped one.
Replaced by the *representation* obligation, discharged in two tiers, with the residual **named**
(coverage of clips no match requests → one distinct-clip counter, never a one-armed oracle).

**`BLOCKED(decision: shipping default)` is now READY except that item 3 is NOT MET.**

## K-PACKAGE. `SITR` closed, `SHDT` is a layer, soft float RETRACTED back open — and the package is the fixed collision kernel (2026-08-15, `e0e7ffaf777`/`223fb499ca5`)

New v3 capture `build-c172-profile-shipcand` (shipping-candidate flags, `DRAW=0`, HEAD `48741fc`),
1,601 regions, `stall_partition_residual` **0**, masked on `total_cycles − halt_wait ≥ 1,177,548` = the
80 frames that set P95. Falsifier band vs `c159`: medians **0.22% apart**, so what moved is *where*
the work is, not how much.

- **`SITR` is CLOSED as a target — there is nothing in it to price.** Its own body is **8,119 tk/fr at
  1.06x**; its entire exclusive static closure is **20,278 tk/fr over 29 functions**
  (`ndsBaseGcRunAll` 1.03x, `gcRunGObjProcess` 1.09x). Its +152,483 bracket excess lives in the leaves
  it *dispatches to*. §9's "no mechanism has ever been priced against `SITR` itself" is answered.
- **`SHDT` is a LAYER, not a lane** — exclusive closure only **6,279 tk/fr**. The spike is the shared
  `gm/gmcollision.c` fighter-part world-matrix geometry: `gmCollisionTestRectangle` **11.70x**,
  `SetInvertMatrix` 11.54x, `GetWorldPosition` 11.46x, `func_ovl2_800EDE5C` 11.27x,
  `…CheckFighterAttackDamageCollide` 10.81x, `func_ovl2_800ED490` 7.68x, `EDBA4` 6.90x,
  `TransformMatrixAll` 5.19x.
- **RETRACTED: "soft float does not promote."** That was whole-match on `c159`. On this binary the
  trio is **104,667 tk/fr on the P95 set and its callers DO concentrate** — largest single caller
  `func_ovl2_800ED490` **11,808** (against the 5,402 that closed the lane), seven of the top twelve one
  family in one file, collision/stage-MP the largest caller group.
- **One 4x4 compose costs 1,290 ticks**, 960 of them in 63 library calls. Three independent
  quantities agree: source has 36 muls + 27 adds = 63; the profile counts 63.0 float calls per
  invocation; 36×26.0 + 27×36.3 cycles = 958 ticks against the attribution's 958.

**THE PACKAGE: wire `src/port/nds_r2_collision_fixed.c`.** Already written, `-marm`, host-graded
**152/152 live matrices, max error 0.0003662 against a 0.0200 bound**, and its own header says
*"NOTHING CALLS THESE YET, by design"*. Cluster on the P95 set **57,375 tk/fr**, of which **39,248 is
soft-float + `sqrtf` call cost a fixed kernel deletes outright** (44,223 with the sin/cos its local
build replaces) against **+32,593** — **1.20–1.36x**.

**But slice 52 as scoped is short, and that is now measured**: its deletable ring is **20,329 tk/fr
(0.62x)**, 27,350 (0.84x) with `GetWorldPosition`. The board's "22,324 / ≤31,278 vs a 47,424 bar"
compared a whole-match lane against a whole-match bar — **both halves the wrong population.**
**The one extension that closes it:** give `func_ovl2_800EDBA4` a **fixed-point interior with an f32
boundary**, keeping `parts->mtx_translate` f32 so its **fifteen referrers are untouched**; that hands
`ndsR2CfxBuildLocal`/`Compose` the call site the seam correction wrote off. Predicted
**+30,000…+38,000** at rank-80 — *a prediction, labelled as one*. Not implemented.

**Law-1 counter landed and firing**: `gNdsCfxFighterDamagePhaseCalls` **1,938** / `Hits` **20** over
frames 439–2038 — **1.21 calls/frame whole-match against 13.1 on the P95 frames, reproducing the
profile's 10.81x**. Sited from the linked ELF on the two ring entries with **zero in-TU callers**
(`TestRectangle`/`SetInvertMatrix` lack that property and a rename there would read zero forever).
`ShieldPhaseCalls` 0 is **unproven, not proven inert**.

**Retraction worth keeping:** "`gmCollisionTestSphere` is absent from the ELF" — it is present at
`0x0207fcd0`/0x4ac and simply **never executes**. *A zero census row is "did not run", not "not
linked".*

**Two traps fixed in tooling:** `--census-out` + `analyze-leaf-helper-attribution.py --mask marginal`
make this question askable on the marginal mask at all (it was previously whole-match only, which is
how the lane got closed); and its first smoke test read the whole multiply lane as **zero** because
`nm` puts `__aeabi_fmul` and `__mulsf3` at one address — fixed via `aliases`.

**Open, handed forward:** the counter arm reads P95 **1,200,960** vs the `c170` bank's 1,177,920
(**+23,040**, inside the ±24,064 one-line spread but above the ~17,000 floor), bounded *not* to be the
1,938 wrapper calls (≈18–24 tk/fr); the K0 site counters are a candidate, not a measurement. **`c170`
remains the bank.** And Task C's handed-forward probe **cannot run as written** — `-PerFrameGlobals`
is incompatible with `-RingDump`; use `-PerStopGlobals`, which answers "in the 96-frame window", not
"on that frame".

## K-RING. Wired, correct, and it measured ZERO — the boundary is the reason (2026-08-15, `893b01f40ed`/`ca4291cbb7f`)

**A/B/A′ on ROMs differing in EXACTLY ONE BYTE** (`objcopy --only-section` + `cmp -l`:
`.itcm`/`.text.hot`/`.text.hot.draw`/`.main`/`.dtcm` **0 differing bytes**, `.main.rw` **1**, at
offset 0x3F24 = `gNdsCfxRingEnable`). **There is no placement floor on this comparison.**

| `WORK-H` | A′ c176 (off) | **B c175 (ON)** | A c174 (off) | B − A′ |
|---|---:|---:|---:|---:|
| P50 | 942,336 | 942,400 | 942,272 | +64 |
| P95 | 1,173,120 | 1,174,016 | 1,173,376 | +896 |
| **rank-80** | 1,173,696 | **1,177,344** | 1,173,760 | **+3,648** |

Controls bracket to ±896 / ±64; the candidate sits **outside** that bracket, on the wrong side.
Requirement was +32,593. **0.00x. `PREDICTION.md`'s +25,000…+32,000 and the board's +30,000…+38,000
are both RETRACTED.**

**Correctness was the part that mattered and it is clean.** Flip budget **stated as zero before the
run and measured zero**: `DamagePhaseCalls` 1,938 / `Hits` **20** identical on all three arms, as are
P1Damage 76, spark 15, shield-attach 1,352, AObj high-water 1,266, runaway 0, heap low-water 52,864,
pack hits 197 — all also matching the `c170` bank arm. **The fixed producers reproduce the float
producers' gameplay outcome exactly over a whole match.** Engagement: nine counters, all **0** on both
controls; `PrepareCalls` 1,938 (== `DamagePhaseCalls` exactly), `ChainFixed` 1,006, `LocalsBuilt`
1,314, `Composes` 1,621, `InvertFixed` 1,006, `ScaleFixed` 1,006, **all three `*Declined` = 0** — not
one domain guard fired. Falsifier re-graded at the **live** domain 0.9937–2.0479 (the stale
1.1138–1.1199 is gone), GREEN, `TestRectangle` **0 mismatches in 300,000**; new T8 section grades the
wiring end to end (world 0.0022583 / local 0.0014343 at depth 12 against 0.0200).

**Brief conflict, stated and correct:** `unk_dobjtrans_0x9C` was **not** reinterpreted as a fixed
frame, so `gmCollisionTestSphere` needed no conversion — STACK.md §5.1's obligation only exists under
that reinterpretation. Keeping the slot f32 leaves every decision in decomp code on decomp
comparisons, gradeable by a bound rather than argued. It cost `TestRectangle` (6,568) and
`GetWorldPosition` (7,021), **but it is not why the result is zero**: the three producers that *were*
converted are worth ~2,415 tk/fr by their own call counts, and that did not appear either.

**Why zero — two candidates, NEITHER MEASURED**, and one 560 s run splits them with no rebuild:
1. the **f32 boundary** (~90,000 conversions/match; `StoreF32` is 129 ARM instructions for twelve — the weaker candidate);
2. **the I-cache footprint of 5,596 B of new ARM text that now executes** — R2-07 L7's failure mode, whose own rate puts it at ~5,175 tk/fr. **Arm A *links* those bytes without executing them and reads −4,288 vs the bank, so linking is free.**

> **THE REAL CASUALTY IS THE DESIGN SENTENCE.** The header's *"crosses the float boundary exactly
> twice per joint per frame"* is **unachievable** while `mtx_translate`, `unk_dobjtrans_0x10`,
> `unk_dobjtrans_0x9C` and `vec_scale` all stay f32. **Any paying version must keep the fixed
> representation RESIDENT** — the `0x9C` reinterpretation (board-proven available) **and** a fixed
> `mtx_translate` (fifteen referrers). Larger, but now with a *measured* reason to exist — and that
> is precisely §1's architectural steps 3–4 (compact pose + dirty-joint evaluator, baked one-pass
> world matrices), which this result independently motivates.

Two traps caught by reading rather than assuming: `--only-section=.text` returned IDENTICAL because it
was the SHA of the **empty string** (this linker script has no `.text`), and `gNdsCfxRingEnable` first
landed in `.bss` at 0 / `.data` at 1 until `section(".data")` pinned it. Boundary **GREEN**, 0
`Exception:`; the proof ELF carries **zero** ring symbols by `nm`.

## K-ICACHE. The ring's arithmetic win is REAL; compulsory fetch of its own bytes eats it (2026-08-15, `bccd70d6bda`)

Arm B (dispatch 1) − arm A′ (dispatch 0), whole window, 1,601 regions, ticks/frame:

```text
issue -1,717 | icache_fill +1,854 | dcache_fill +161 | instructions -204 | net non-idle +284
```

Six stall classes sum to +284 exactly (`stall_partition_residual` 0 / −42). Per-PC groups: the ring's
eight symbols add `issue` **+12** and `icache_fill` **+2,110**; the displaced float bodies and libgcc
give back `issue` **−1,852** but icache only −475; the rest of the binary nets +219. **So it is
compulsory fetch of the ring's OWN instructions, not eviction damage** — and **the ring executes 204
FEWER instructions per frame.** The arithmetic win was real the whole time.

> **One number explains the null: the fixed replacement costs 0.987x the float it deletes**
> (+2,726 against −2,762 tk/fr).

**Candidate 1 (the f32 boundary) is REFUTED as an arithmetic explanation.** `StoreF32`+`LoadF32` are
964 B costing +588 tk/fr, of which **+516 (88%) is icache and −12 is issue**. RING.md's "129 ARM
instructions for twelve, ~11 each" **divided a static size by a count** — off the disassembly the body
is two loops, 38 ARM instructions per fixed→f32 and 22 per f32→fixed. *The instructions were never the
price.* (Third instance of that trap in this campaign.)

**Exact call counts from entry PCs, A′→B**: `ED490` 2,471→1,177 · `TransformMatrixAll` 4,157→3,115
(**only 25% taken**) · `SetInvertMatrix` 937→141 · `EDBA4` 1,133→337 · `EDE5C` 1,693→1,693
early-returning · `DamageCollide` 1,552 and `TestRectangle` 1,693 on **both** arms (same fight).
**`lbCommonSin`/`Cos` 37,615→34,489 = 8.3%, so sin/cos was NEVER a collision lane** — `PREDICTION.md`
had booked ~4,975 tk/fr for it. `sqrtf` 3.5%.

**`R2-07 L7`'s 1.85 cycles/frame/byte is not a constant — measured 0.754 here.** Do not carry it.

**The resident version straddles the bar and one run settles it.** Full narrow-phase residency removes
**9,865 tk/fr** whole match for ~7,400 B at the measured 0.4871 tk/fr per byte = 3,604 cost →
**−6,261 whole match**; at this cycle's 3.11x percentile concentration that is **+19,470 at rank-80 =
0.60x. It clears at ≥5.21x and not below** — and the `DRAW=0` census reports **5.2–11.7x** presence
for these bodies. **One `DRAW=0` re-run of the same pair decides it with no new code.**

**THE UNPRICED LEVER IS PLACEMENT.** All 5,596 B sit in `.main`, while `.text.hot` has **3,604 B
free** and `.text.hot.draw` **2,924** — 6,528 against 5,596 needed — against a *measured* +2,110 tk/fr
of compulsory fetch. One build, one A/B, no new arithmetic.

**Corrections:** "fifteen referrers" belongs to **`func_ovl2_800EDBA4`, not `mtx_translate`** — ten of
the fifteen are inside the cluster and call the *function*; the field's ~49 readers funnel through
`gmCollisionGetWorldPosition`, `gmCollisionCopyMatrix` and `func_ovl2_800EDA0C`, **nothing blocks
residency**, and the renderer is a net *win* there (it already converts to 20.12 fixed).
`gmCollisionTestSphere` executes **zero** times on both arms.

**The empty-comparison trap recurred in a new costume**: an `objcopy --only-section` compare against a
build directory that **did not exist** reported 0 differing bytes for *every* section. Now structural
— `census-marginal-frame-owners.py --diff` verifies both builds' `nm` address→name maps and **refuses
across a relink**, so a per-PC diff over mismatched layouts is inexpressible.

**2^22 outliers, free probe:** both windows contain **zero** regions ≥ 2^22 (max 3,361,733, same
region), `stall_cart_spin` 0 throughout — narrows it toward the tick-HUD's own `cpuGetTiming()`
reader. Not an answer.

**Unowned action, not landed:** `docs/VERIFYING.md`'s Makefile rule **cannot** be made structural by
content inspection (an eaten `\` leaves a valid file); `make --dry-run` in `check-architecture.ps1` is
the gate that would work, and a new failure mode in a Boundary-gating checker needs its own cycle.

## K-EXCHANGE. Placement refuted with ZERO builds; the whole lane now hinges on one rate (2026-08-15, `1e2833cccba`)

**Placement is refuted by arithmetic.** The ring's cost is charged **per entry** and does not fall
when entries get 11x denser: ticks per entry **2,668 / 2,596 / 2,755 / 2,961** at **0.97 / 4.14 /
0.97 / 10.66** entries per frame. If any of its 175 lines survived between entries, packing them 11x
closer would make the per-entry price *fall* — **it rises 11%.** Cost concentration equals *call*
concentration twice over (11.82 vs 11.57 on `DRAW=0`; 4.15 vs 4.16 on `DRAW=1`). **The ceiling on what
any layout could recover is the ring's eviction damage to everything else: +219 tk/fr = 0.08x of the
requirement.** Corroborated three ways: `linker/nds_hot_text.ld:180-200` banks two builds that
measured the **opposite sign** (Task 94 −500 B → P50 **+6,144**; E65 +2,032 B predicted −7,894,
measured P95 **+24,448**); a contiguous 5,596 B block is 2.73 set periods, so it covers all 64 sets
≥2x **at any address**; and `icache_fill` 712,877 cyc/frame at 23–51 cyc/line is **55–121 complete
cache turnovers per presented frame**.

> **`5.21x` IS RETRACTED AS A DECISION RULE — it tested the wrong quantity.** Concentration measures
> **11.68–11.83x**, clearing 5.21x by 2.2x, **and it changes nothing.** The rule applied one factor to
> the *net*, which is valid only if the price stays put while the prize concentrates. Measured, the
> float deleted concentrates **11.68x** and the fixed text replacing it **11.83x** — same frames, same
> factor, so **it cancels and the EXCHANGE RATE decides**. That rate is **1.001 whole-match and 1.014
> at rank-80**: fixed currently costs what float costs.

Whole window B−A′ on the `DRAW=0` pair (one-byte-differing, reproduced): `issue` −1,771 ·
`icache_fill` +1,801 · **net non-idle +3**. Marginal 80: `issue` −19,119 · `icache_fill` +20,112 ·
**net +441**.

**Three retractions, one of them its own prediction:** `+284` → **+3** on the arm the gate is read
from (the rest was HUD/printf noise); "204 fewer instructions per frame" → **−407** (`--diff` divided
a count by `2 × frames`, fixed at source); and **its own written-down prediction was wrong on both
halves** — predicted +6,000…+9,000 marginal and +200…+400 whole, measured **+441** and **+3** — from
sizing off an attribution *model* instead of waiting for the difference.

**Surprise that re-aims the whole ranking:** the P95 frames are **not** "more fighter procs".
`battleship_ftMainProcUpdateInterrupt` costs **1.05x** there and is called **1.03x**, while
`gmCollisionCheckFighterAttackDamageCollide` goes **0.97 → 10.47 calls/frame**. **The fighter narrow
phase IS the P95 owner, at 11x presence.** (`DRAW=0` is cleaner, not clean: `_svfiprintf_r` 5.58x and
`consolePrintChar` 5.26x still sit on the mask at ~8,800 tk/fr — collision concentrates twice as hard.)

**Free finding, not built:** `nds_r2_collision_fixed.c` compiles at **`-Os` instead of `-O2`** to
**7,916 B → 5,228 B, −34.0%**, identical undefined-symbol set, no `__aeabi_lmul`, SMULL 62→58. ~−700
tk/fr is **23x under the ≥16,000 floor**. Caveat: `-Os` outlines `ndsR2CfxCosQ15`, moving the
soft-float edge out of `BuildLocal` and tripping `check-r2-collision-fixed.ps1` — fix with
`always_inline`, never a widened allowlist.

**THE DECIDING MEASUREMENT, one build:** convert **one early-exiting body** and read its exchange rate
off the same one-byte `--diff`. `gmCollisionGetWorldPosition` is cheapest — 196 B float against a
100 B fixed `TransformPoint`, 19.29 calls/marginal frame, ~30 sites through one helper.
**Below 1.00 ⇒ residency is worth building. At 1.00 ⇒ the fixed-point collision lane closes on
ARITHMETIC, not cache** — and the campaign needs a different mechanism for the remaining +32,593.

## K-CLOSE. The fixed-point collision lane CLOSES — twice over — and here is the priced menu (2026-08-15, `1ac6258a2f1`/`ba2c5e57026`)

**Exchange rate = 2.68** whole match: fixed added **+3,392 tk/fr** against float deleted **−1,264**.
Producers measured 1.001; consumers measure 2.68. **Neither shape is below 1.00.**

> **And the rate is not even the closure.** The identifiable float in the **entire** fighter narrow
> phase is **15,217 tk/fr at rank-80 = 0.47x of the +32,593 requirement** (bottom-up recount
> 16,000–17,900 = 0.47–0.55x); whole match its soft float is **840 of a 59,694 tk/fr
> `fadd`+`fmul`+`fdiv` bill — 1.4%**. **Fixed point being FREE would not have closed the gap.**
> `K-EXCHANGE`'s "the narrow phase IS the P95 owner at 11x presence" is **true about presence and
> false about size.**

Two instruments agree: profile pair **+29,290 tk/fr at rank-80**; a same-binary
`-SetGlobals gNdsCfxNarrowEnable=1|0` route on **one** build reads **+28,480** — **2.8% apart**.
P50 flat (−384).

**Mechanism, and it is not only cache:** the fixed form calls libgcc's **64-bit divide 4.0x per
entry** (`__udivmoddi4` 7.76 → 11.65 calls/fr, a bit-by-bit loop) worth **+17,377 at rank-80** — more
than the whole float bill it deletes. `nds_r2_collision_fixed.h:210-217` offers
`NDS_R2_CFX_DIV64`/`ISQRT64` overrides for the DS hardware unit and **nothing ever defined them.**
Meanwhile `__mulsf3` pays **0.77 tk of fetch per call** at 1,545 calls/frame — **the soft-float
library is permanently I-cache resident, while a kernel entered 0.97x/frame is cold every call.**
Best remaining case (resident `0x9C` + hardware divider) still prices at **≈1.29**.

**Correctness is the keeper.** Flip budget stated ZERO before the run, measured ZERO — `NarrowCalls`
**1,938 == DamagePhaseCalls**, `Answered` 1,938, `Hits` **20 == DamagePhaseHits**, `Declined` **0**,
control all four 0, and every invariant equal to the c170/c174/c175/c176 bank on both arms. **The
collision DECISION ran in port fixed point for 1,938 of 1,938 pairs and reproduced decomp float
exactly.**

**Retraction — its own written-down prediction, wrong on both halves.** Predicted +560…+7,890
marginal / +50…+670 whole; measured **+29,290** and **+2,228**. The *prize* half was right (≈15,000
predicted, 15,217 measured); the *price* half was **2.2x low because it never counted the library the
kernel calls.** Third distinct static-quantity mispricing in this campaign.

### THE MENU — rank-80 on this tree, requirement +32,593 (`MENU.md`)

| lane | rank-80 tk/fr | conc | x req | call |
|---|---:|---:|---:|---|
| **Draw side, FLAT** | **170,953** | 1.00–1.05x | **5.2x** | mixed |
| **In-match asset I/O** | 100,689 (excess **+67,454**) | 1.5–19.3x | **3.1x** | engineering |
| Animation evaluate/parse | 95,048 | 1.4–2.05x | 2.9x | engineering |
| Soft float | 94,602 | 1.1–1.5x | 2.9x | engineering |
| **Stage no-Z band** | **22,608 @ exactly 1.00x** | — | 0.69x | **OWNER** |
| Particle draw kernels | 12,595 | 1.19x | 0.39x | OWNER |
| tick-HUD apparatus (not a lane) | 8,085 | 5x | — | vanishes at `TICK_HUD=0` |

- **Phase 3 (GX_COMPOSE) is LIVE and its price is STALE.** −13,632 was measured against a
  **1,258,112** baseline that is now **1,177,344** — re-measure, do not re-bank. The board's
  `nds_platform.c:3197` citation has **drifted**; the leak note is `:3260-3294`. **Confirming the leak
  predates slice 43 now costs ZERO builds** — the GXSTAT read ships in every `NDS_TICK_HUD` ROM and
  `GX_COMPOSE` is already 0 in every lab build. Cheaper still: `nds_renderer.c:1546-1547` funnels
  every push/pop through one recorder. Suspects: Whispy's raw `MATRIX_PUSH`/`POP` FIFO words
  (`:14352`, `:14380`) and `ndsRendererEndParticleQuads`' two-flag-conditional pop.
- **Owner ladder re-priced:** no-Z band **22,608 at exactly 1.00x** (RESIDUE's 22,510 holds to 0.4%;
  a flat lane converts 1:1, so it is **69% of the requirement from one visible decision**; sub-rung
  `NDS_DREAMLAND_CARD_CULL` ≈4,600 for 20.6% of scenery). **Particle draw −30,676 is STALE — the draw
  kernels are 12,595, 2.4x below it; do not quote −30,676.** 30 Hz −119,744 unchanged.
- **newlib, from the linked ELF:** exactly two live paths, and **30.6% IS shipped** (entry-PC split
  0.70 / 1.57 per marginal frame). Apparatus ≈8,085; shipped residual **≈1,526** (ceiling 4,267). The
  earlier ≤769 bound is whole-match and still holds (≈207) — **it understates the mask ~7.4x.**

**Surprises worth keeping:** the cross-build floor *between pairs* is **+2,266 tk/fr on the control
arm alone**, so only within-pair deltas are quotable; **`armCopyMem32`'s only ELF referrers are
`ntrcardRomRead` and `_dvmCacheCopy` — cartridge reads during gameplay, at 8.90x concentration**; and
a new `addr2line` trap — the inline table names `ndsRendererTask29GXRecord` at 16,795 tk/fr with
`NDS_TASK29_GX_CENSUS 0`, i.e. **the `#else` no-op inline. Do not price it.**

## K-GXFIX. The matrix-stack leak is FIXED and it was shipping; asset I/O converts 0.30x; the draw lane is 2x bigger than MENU said (2026-08-15, `f3b63e46a06`/`771cd4b8312`)

**The leak's site: `nds_renderer.c:6218` recorded `run->local_pushed` from `capture_push_balance` — a
per-run DELTA — where `EndSegment` (`:30424`) needs the STATE the stream leaves.** Task 36's
`EnsureWorld` pops the previous binding's world before pushing its own (`:30290-30294`), so every run
after the first in a segment records balance 0 while the stack is still one push deep, and replay
assigns the last run's value verbatim (`:30783`).

> **It predates slice 43 and it was on the SHIPPING-DEFAULT path.** Every Boundary log in
> `artifacts/` from **2026-08-03 to 2026-08-15** prints `gxstat=0x6009600` — **stack level 22, error
> bit set**. This cycle's prints **`0x6000000`.** Fixed and proven: `build-c183-gxstackfix` reads
> level **0** / error **0** on all 128 per-frame samples *and* all 17 whole-match ring stops, with
> every invariant equal to the c170/c174/c175/c176 bank.

The discriminator was 17 stores to `0x04000444`/`0x04000448` located by disassembly: PUSH 5,143,
POP 4,831, residual **+312 = exactly** the raw FIFO `MATRIX_POP` from
`ndsRendererFinishWhispyNativePacket`. Everything the ARM9 writes nets **zero**, so the producer had
to be the DMA'd replay stream — and `EndSegment.part.0`'s `MATRIX_POP` executed **0 times in 1,600
frames**. Negative control in the same run: the 37 intervals with no batch open still advance 3.000.

**Asset I/O: attribution settled, and the lane is 10x smaller than MENU priced it.** It is
**unpacked-Mario animation**; **BGM is exonerated by measurement** (`ndsAudioBgmReadPacket` 76 calls
at presence 1.056x, **0%** of the load frames). The work sits on **7 frames of 1,600** holding 85.4%
of the match's FAT reads — but those frames rank **3, 5, 10, 13, 14, 16, 23**, deep *inside* the top
80, so deleting them outright moves rank-80 only **9,863 tk/fr = 0.30x**, reproducing 2026-08-14's
independently derived 9,874. **MENU's 3.1x was concentration read as size — the same error §7's
correction names, in the opposite direction.** `armCopyMem32` = `_dvmCacheCopy` **exactly**
(1,986 = 1,986) with `_ntrcardRomReadSector` at 0 — same work one layer down, not a second producer.

**The draw lane is 348,268 tk/fr over 36 symbols at ~1.02x** — MENU's 170,953 over 7 symbols was a
**floor**, so **§7's correction is understated 2.0x, not overstated.** GX FIFO floor ≈27,467 tk/fr of
bus contention. The stage's live draw is now exact from the generated tables: segments 1/2/3/6 →
**27 tri / 47 loads / 81 verts**, with `Task36ReplayRun` at exactly 33.00/frame confirming it a third
way.

**But no sized pure-waste item exists yet, and none was invented.** The best exactly-sized category-1
item is **1,305 tk/fr (0.04x)** — 5 of 47 no-Z matrix loads per frame are bit-identical repeats inside
one triangle. Two bigger-looking candidates were **checked and refused**:
`AdapterBuildPersistentStageWorldMatrix` already carries a per-frame cache plus slice-44 stale reuse,
and `CommitNativeStageSegment` is 1,989 tk/call of pure instruction fetch (footprint, already refuted
as a layout lever). **Two survive, each needing one counter first**:
`ndsRendererSyncTextureTile` (**8,867 tk/fr, 72.68 syncs/frame**) and the texture-bind collapse
(**103.45 requests → 55.73 GX binds/frame**, 13,868 tk/fr combined).

**`NDS_R2_FIGHTER_GX_COMPOSE` is still `?= 0` and its −13,632 is still stale** (baseline 1,258,112 vs
1,177,344) — **but the leak was its only blocker, so re-measuring it is now unblocked and is the
obvious next package.** `c183`'s whole-match P95 1,187,648 is **not** an A/B and is not quotable.

## K-GXC. `GX_COMPOSE` is owner-approved and freshly banked (2026-08-15, `dd80585d6eb`)

The repaired-tree A/B still sizes the mechanism at **−17,152 at rank-80**
(1,189,312 → 1,172,160), with P50 −8,096 and the saving landing in `FTR`.
That number remains a within-pair mechanism price only; the bank below is a
fresh measured LEVEL.

The missing pixel proof was taken from `build-c184-cap-a/-cap-b` at matched
simulation-clock locks. Battle-screen deltas were 43–209 pixels of 120,000
(0.0358–0.1742%). GXSTAT stayed **0x06000000**, accepted polygon RAM stayed
432/463.5/510 with **0/128 under 350**, and all gameplay invariants matched.
The owner inspected the actual diff masks and explicitly accepted that tiny
scene delta as visually acceptable. The old pre-blink "pixel-identical" claim
is still not inherited; this is an owner fidelity decision on the new evidence.

**Fresh bank, `build-c185-gxcompose-bank`, DRAW=1, BOTH_CPU=1, DLDI on,
1,600 samples frames 440–2039, slips 0:**

```text
WORK-H P50       938,112
P90            1,088,192
P95/rank-80    1,174,016 raw / 1,149,069 net
top-1%         1,520,832
over gate        122 / 1,600
gap              +53,636 raw / +28,689 net
```

Net subtracts the standing 24,947-tick tick-HUD apparatus. This replaces the
`c170` bank; **nothing was subtracted arithmetically from `c170` to create it**.
All eight gameplay invariants again equal the established bank and GX-compose
engagement is complete (`Declines=0`, Captures=Roots 63,364,
Locals=Mults 110,702).

**Cadence truth, `build-c185-gxcompose-bank-d0`, DRAW=0:** VBI
**2:1850 3:173 4:8 5+:8 max 19**, 2,039 total, slips 0 = **90.731% two-VBlank**.
The ≥95% cadence target is therefore still RED even though GX compose itself is
accepted and banked.

The published default is deliberately unchanged: `NDS_R2_FIGHTER_GX_COMPOSE`
remains default 0 and the published target remains pinned to 0. Next performance
work is sized against **+28,689 net ticks**, while cadence remains a separate
acceptance gap.

**Renderer-state redundancy is measured and CLOSED below the package floor**
(`artifacts/performance/2026-08-15_renderer-state-redundancy/STATE_REDUNDANCY.md`).
Task107 counted 146,221 tile syncs / 106,500 exact repeats (**72.835%**) and
208,327 bind requests / 95,934 already-elided / 112,393 issued, with 26,769
same-frame revisit issues and zero overflows. Count-scaled exact local ceiling:
**6,458 sync + 1,305 proven no-Z = 7,763 tk/fr** before any guard cost. Bind
revisits are ordering, not local redundancy; even perfect ordering adds only
**2,484**, for **10,248 total**. Even the impossible all-sync upper bound is
12,656, still below the 16K floor. **No micro-cut was implemented and c185 is
still the bank. Hand the campaign to SITR decomposition next.**

### Restart surface after the GX-compose bank

- Current bank: **1,174,016 raw / 1,149,069 net**, **+28,689 net** to 1,120,380.
- Cadence: **90.731% two-VBlank on DRAW=0**, below ≥95%.
- Boundary remains green at the unchanged shipping default.
- Renderer-state micro-cuts: **7,763 exact / 10,248 including perfect bind
  ordering upper bound — CLOSED below 16K**. Next: **SITR decomposition**.

## K0. The rule that defines slice 1

```text
AFTER THE BATTLE ENTERS GO:
  fighter-animation FAT reads            = 0
  get_fat / f_lseek for fighter anim     = 0
  animation payload byte-swaps           = 0
  animation-file relocation / fixups     = 0
  AObj16 file normalization              = 0
  raw animation-file cache copies        = 0
  token -> file discovery                = 0
```

Normal gameplay consumes `clip_id -> immutable DS AnimClip -> per-fighter AnimInstance -> pose`.
**This matters far more than making `get_fat` 20% faster.**

**Do NOT merely enlarge the existing raw-file cache.** That cache is proof we are halfway there and
proof its architecture stops too early: a *hit* today still `memcpy`s the complete asset into
caller-owned heap storage, registers it as a loaded file, runs finalization/fixups, updates
aliases/status buffers, and only then feeds the normal animation machinery. The target is to
eliminate the need to recreate an N64 loaded-file image on every action change.

## K1. Slice 1 — BattlePack + direct AnimClip

The nine phases of the owner's task block, in order, with their acceptance conditions:

1. **Measure the real match animation working set** on the `BOTH_CPU` gate arm, per presented frame:
   asset IDs requested/frame; distinct assets over the match; cache hit/miss; `get_fat`/`f_lseek`/
   `f_read` calls; bytes read from ROM; bytes copied from cache into status heap;
   `ndsRelocNormalizeFighterAObj16File` and `ndsRelocFinalizeLoadedFile` executions. **Intersect every
   counter with the 80 frames that set P95.** Also produce the *complete reachable* Mario/Fox
   animation asset set — not the current 85-entry warm list — and answer why any requested asset is
   absent from it.
2. **Design the build-time DS AnimClip format** from the *exact consumers* of the fighter
   AObj16/figatree files. Native endian · position independent · no runtime relocations · direct clip
   index · direct joint/channel indexes · explicit interpolation type · fixed point where
   semantically safe · **packed-bit channels stay packed bits** · `func_anim`/event ordering
   represented exactly · no runtime token/path lookup · no pointers needing patching after load.
   Separate **immutable `NDSAnimClip`** from **mutable `NDSAnimInstance`**: inventory every write the
   runtime currently makes into a loaded animation file and classify each as precompute / move to
   instance / eliminate. **Do not assume the raw file must be copied merely because the current
   adapter copies it.**
3. **Host converter / BattlePack** — extend the existing build-time asset tooling, never a runtime
   converter. One Mario + Fox + Dream Land P1 pack: clip directory, action/asset→clip map, compact
   track descriptors, key/event data, evaluator tables. Report raw input bytes, current warm-cache
   bytes, generated pack bytes, loaded RAM, ROM, per-clip sizes, total matchup size — **and price it
   against proven RAM/heap headroom BEFORE implementation** (`check-boot-headroom.ps1`; text counts
   as much as bss; the GObj-cap law applies). **If the pack cannot fit safely, do not fall back to
   gameplay-time FAT loading** — design a more compact representation, a deterministic pre-GO loading
   arena, or a split resident pack proven to contain every gameplay-reachable clip for this matchup.
4. **Host equivalence test, before any DS runtime work.** Decode every generated clip on the host and
   compare against existing parser semantics: duration, key values, interpolation mode, joint/channel
   target, event ordering, packed color/material payload bits, end/loop behavior. Corpus hash +
   mismatch count. **Required: mismatch = 0.**
5. **Direct runtime instance.** `action -> clip_id -> const NDSAnimClip* -> NDSAnimInstance`, where
   the instance owns only mutable playback state. Do not copy the clip into the fighter/status heap;
   do not register it as a generic reloc-loaded file unless an unported fallback explicitly needs it.
   For slice 1 it is acceptable for the direct evaluator to write the same DObj/AObj-visible outputs
   the remaining generic engine expects — **the purpose of slice 1 is to remove the asset/file
   architecture first.**
6. **Same-build oracle mode.** Old and new evaluate the same input; compare animation time, generated
   values, event/function callbacks, joint pose outputs, end/loop transitions; record mismatches by
   clip/action/frame; **fail closed to the generic path on mismatch. Required before the shipping
   route: mismatch = 0 and event-order mismatch = 0.**
7. **After-GO zero-I/O assertion.** In the direct route prove all seven counters in K0 read zero. Any
   non-zero means the architecture is incomplete.
8. **Performance**, `BOTH_CPU` gate arm. Rank-80 `WORK-H` ≤ 1,120,380 — currently **64,452 to
   remove at the 80th-largest frame**. Measure v3 before/after on the P95 set across `issue`,
   `icache_fill`, `dcache_fill`, `write_buffer`, `interlock`, the FAT/copy/normalization family and
   the animation-evaluator family. **The file-I/O portion should DISAPPEAR, not get faster.** Do not
   stop architectural work because this slice cleared the gate (§1 item 2).
9. **Then reprofile and size slice 2** — epoch-flattened `gcRunAll` process vectors. **Do not
   implement slice 2 until slice 1 is measured and the profile re-ranked.**

Deliverables: `docs/architecture/RUNTIME2_NATIVE_BATTLE_KERNEL.md` and
`artifacts/performance/2026-08-14_native-battle-kernel/BATTLEPACK_ANIMATION.md`.

## K2. Slice 2 and beyond — sized only after slice 1 is measured

- **Flattened process scheduler.** Preserve exact BattleShip process order and priority; maintain a
  flat vector of the same processes; bump an epoch and rebuild only when the process graph changes;
  iterate the vector on ordinary frames instead of rediscovering the linked GObj/process graph. The
  graph changes far less often than it is traversed, which converts pointer-chasing into sequential
  access **while leaving the BattleShip fighter routines untouched**. Justified by §0: ~90% of the
  tail excess is inside `gcRunAll` and the draw side is 7–8%.
- **Compact pose + dirty-joint evaluator**, then **baked one-pass world matrices**. The earlier matrix
  investigation stands and points here: local matrices are *not* redundantly rebuilt today — the
  problem is ~20× the arithmetic cost spent on the architecture surrounding a small compose. A baked
  forward pass over `BindingParents` attacks the right thing. The AOT data already exists
  (BindingParents, BindingJoints, JointSchedule, packed geometry, native strips, run info, texture IDs).
- **Direct native renderer**: pose → one forward pass → `worldMatrix[bone]` → prepared native runs →
  GX. Not: DObj tree → generic lookup → policy validation → hierarchy walk → world-cache probe →
  material discovery → prepared-run validation → submit.
- **ARM7 offload is NOT the strategy** (owner, 2026-08-14). It is much slower, has different memory
  characteristics, is already carrying system/audio duties, and synchronisation would introduce a new
  class of correctness/timing bugs. Use hardware where it genuinely eliminates CPU work — GX for
  transforms/rendering, DMA for bulk transfers that must exist — but the bigger win is to **make the
  work cease to exist**. A zero-byte copy beats a DMA copy; a precompiled clip beats an ARM7 parser;
  a direct array beats a cached linked-list walk.

---

## 7. Phase 2 — Hot instruction footprint — demoted, **but my demotion reasoning was WRONG and is corrected here (2026-08-15)**

> **CORRECTION.** This section demoted the draw side because it holds only **4.8% of the P95
> *excess***. That is the wrong test, and §0's own law says why: **the gate is a LEVEL, and a uniform
> cut moves P95 by its full value.** A lane at **1.00x concentration is the BEST-converting lane
> there is**, not a P50-only one — it pays 1:1 at the percentile. The draw side measures **170,953
> tk/fr at 1.00–1.05x = 5.2x the requirement** (`…/2026-08-15_cfx-narrow-exchange/MENU.md`), which
> makes it the **largest** lane on the board, not a demoted one.
>
> What survives of the demotion is narrow and still correct: **moving cold *bytes* does not pay** —
> `FOOTPRINT` §1 refuted that outright, and `K-EXCHANGE` refuted layout as a lever (any layout's
> ceiling is +219 tk/fr). **Deleting draw *work* is a different lane and is wide open.**

The original text, kept because its byte-level findings stand: its two target objects
(`scene_backend.o`, `nds_renderer.o`) are draw-side, and on the 80 frames that set P95 the draw side
is `FTR` 1.03x, `STG` 1.00x, `MISC` 1.15x — 4.8% of the *excess*. The lane is sized off *whole-match*
`icache_fill`.

**Revised order: Phase 2 does not run before Phase 4.** It is re-admitted only if (a) the gate-arm v3
capture shows `icache_fill` concentrated inside `gcRunAll` on the P95 frames, or (b) it rides a
larger package's build as a P50/text finisher. Its steps below stay valid for whenever that happens;
its 25–40K sizing is a whole-match number and must not be quoted as a P95 win.

Evidence: `2026-08-14_hot-footprint/HOT_FOOTPRINT.md`. Fetched instruction footprint 288,352 B = 213,040 live + 42,892 cold + 26,640 padding + 5,780 literal pools (not removable). `scene_backend.o` + `nds_renderer.o` carry ~42% of fetch at ~48% execution density. Primary metric: **v3 `icache_fill` delta** (with issue); WORK-H alone does not bank a footprint win.

1. Compiler capability gate, no build: prove `-freorder-blocks-and-partition` emits a real `.text.unlikely` under this Thumb/ARMv5TE toolchain, reaches the two objects, and name the moved bytes/functions. Empty or trivial → record and skip to step 3.
2. Price text/RAM before the candidate (partitioning can grow total text; boot cliff and GObj-cap law apply; ITCM membership unchanged). Predict ≥16K (prefer ≥25K) from fetched-line reduction or stop.
3. Manual cold extraction on the same two objects regardless of step 1's outcome: out-of-line the cold fallback/error paths from the hottest mixed lines (census already lists them). Literal pools are not cold code — do not count them.
4. Full correctness battery — this changes codegen: Boundary, invariants, RNG controls, generated-data checks; A/B/A per §2.
5. One candidate build per package; re-rank after banking.

## 8. Phase 3 — GX_COMPOSE stack-leak fix (a bug with a named site, then collect −13,632)

`NDS_R2_FIGHTER_GX_COMPOSE` measured **−13,632 P95 with pixel-identical captures** and is forced off only by a GX matrix-stack leak (~3 pushes/frame, wrap mod 32, one-frame fighter blink) at `nds_platform.c:3197`; `GXSTAT` reads are the instrument. Fix the leak at its seam, prove pixel-identical + stack-balanced over a full match (GXSTAT stack pointer flat), Boundary + soak, collect the measured win. This is a correctness repair unlocking a banked optimization — not a fidelity trade; owner sees it at playtest like any visual-adjacent change.

## 9. Phase 4 — the owner, now named: `gcRunAll` and three fighter procs

**Phase 0 settled this at bracket granularity on the right arm.** P95-set excess +520,718; the whole
of it is the logical simulation. Non-double-counted children (`MARGINAL_OWNERS.md` §7):

| owner | P95-set excess | ratio vs a 2-VBlank frame | recorded state |
|---|---:|---:|---|
| `SITR` (= `SINT` − `SCPU`) | **+171,234** | 2.16x | fighter INTERRUPT proc. **No mechanism has ever been priced against `SITR` itself.** |
| `SHDT` | **+119,920** | **19.2x** | HANDOFF says CLOSED — but closed *for the mechanisms tried*, on a whole-match bar of 47,424. Never sized on the P95 frames. |
| `SPHD` | **+112,833** | 2.85x | `ftMainProcPhysicsMap` default arm. **Never lane-sized at all.** `SPHC` is +0. |
| `SPRM` | +49,377 | **25.8x** | HANDOFF's "closed by arithmetic, under 16,000 deleted entirely" was a **whole-match mean**. Its P95-set excess is 49,377. |
| `SCPU` | +7,222 | — | the level-3 AI itself is small. |

Three laws this phase is bound by, from `MARGINAL_OWNERS.md` §7:

1. **Target the presence, not the mean.** `SHDT` 19.2x and `SPRM` 25.8x on the P95 frames against
   small whole-match means — a cut sized off a census row under-predicts by an order of magnitude,
   and one sized off the excess over-predicts if the mechanism is not the thing that spikes. Land a
   per-frame engagement counter on the **spiking quantity** (hit-detect pairs, physics-map segments)
   on the **gate arm** before any code changes.
2. **Clear 91,844 at the 80th-largest frame.** Re-rank the whole distribution after, never the top
   rows.
3. **Not the draw side** (§7 above) and **not the pool** (§5 item 2).

**CAPTURED 2026-08-14 — `build-c159-profile-bothcpu`, the first v3 stall capture on `BOTH_CPU=1`**
(`DRAW=0`, 1,601 regions, 3.62 GiB; `GATE_ARM_OWNERS.md`). P95-set excess by stall class:
**`icache_fill` +155,795 (40.0%)** · dcache +96,800 (24.8%) · issue +94,029 (24.1%).
**No single function is above 3.6%** — the cost is spread, so the lever is a *mechanism*, not a hot
function.

**THE MECHANISM: in-match animation-asset load I/O.** `+93,436 on the 80 P95 frames`, `+51,276`
after the outlier falsifier:

```text
memcpy +22,531 · get_fat +15,058 (10.7x) · armCopyMem32 +15,025 (9.3x) · f_lseek +9,509
ndsRelocNormalizeFighterAObj16File +7,369 (19.2x) · ndsRelocAssetIDForToken +6,012
```

The match is reading animation assets off the FAT **during gameplay**. Owner decided 2026-08-14 that
these are ordinary gameplay frames (§0), so this is in scope and it is the board's largest item.
It must clear **64,452 at the 80th-largest frame**, and a per-frame asset-acquisition counter must
exist on the gate arm before any code changes (law 1 above).

**Two directions closed on gate-arm evidence, not inference:**
- **Hot footprint fails §7's own re-admission test.** Draw closure holds **11.5%** of the icache
  excess (+17,944) against fighter-proc closure **41.4%**. Stays demoted.
- **Soft float does not promote** (§10's open question, answered). 74,283 tk/fr caller-attributed
  whole match: **38.0% inside the fighter procs, 31.2% draw side, largest single caller 5,402.**
  On the P95 set the trio is 99,762 tk/fr, 95.7% `issue` — real, but with no caller big enough to
  package. It stays a finisher.

**And `MARGINAL_OWNERS.md` §7's sub-`SRC` ranking is RETRACTED as match-specific.** Only `SITR`
survives across both captures (+171,234 → +188,907); `SPHC` went +62 → +52,780 and `SCPU` +7,222 →
−8,669. `SRC − GCRA` = −22 (a third population, a third zero — the nesting holds). **Do not brief a
package against `SHDT`/`SPHD`/`SPRM` on the strength of one match's numbers.**

Killed as directions: the write_buffer/interlock pool (§5 item 2). Still unconfirmed, admissible only
if the gate-arm capture ranks them: dcache visit-count reduction (GObj 91.6 visits/frame, AObj 357.8
— "visit fewer nodes", not "repack structs"); the matrix 20x machinery ratio, gated first by the
`has_mvp_recalc_rpy_0x47` engagement counter, never by assumption.

## 10. Phase 5 — Finishers (ride a larger build, never standalone)

- Soft float — **RE-SIZED UP 2026-08-14 and it may not be a finisher at all.** `MARGINAL_OWNERS.md`
  §3.2: `__aeabi_fadd` + `fmul` + `fdiv` = **82,274 tk/frame on the marginal frames**, 97% `issue`,
  **no cache component** — so the only lever is *executing fewer of them*, and the whole 82,274 is
  arithmetic a fixed-point conversion could in principle delete. Caveat that keeps it here rather
  than in §9: that figure is the **Boundary arm** (`build-c125-profile`), and *where those calls
  live* is a gate-arm question. The gate-arm capture answers it; if they concentrate inside
  `gcRunAll` this promotes to a Phase 4 package.
- `ndsFighterDisplayContractCountFlags` deletion: proven ~3,925 — piggyback on the next candidate build.
- Any Phase −1 survivor pieces (FGM LRU, FindPlanned index) that were reverted for size alone.

## 11. Phase 6 — Owner ladder (only if Phases 2–5 leave a residue)

Already priced, presented with final numbers, never chosen silently: stage no-Z band −22,510 · particle draw reduction −30,676 (visible, destructive) · compensated 30 Hz −119,744 (mechanical cadence change, last resort, in writing).

## 12. Closure arithmetic — **REVISED 2026-08-14 after Phase 0**

```text
need (net gate)                          65,617   raw 90,564
the number a package is actually judged on:  the 80th-largest frame must fall 91,844

GX_COMPOSE leak fix                      13,632   (Phase 3, unchanged, still a bug fix)
Phase-4 package on gcRunAll      16,000–30,000+   (SITR 171,234 / SHDT 119,920 / SPHD 112,833
                                                   / SPRM 49,377 of P95-set excess to draw from)
soft float, IF inside gcRunAll        0–82,274    (Boundary-arm sizing; gate-arm capture decides)
ndsFighterDisplayContractCountFlags        3,925  (piggyback only)
                                 ----------------
hot footprint                    25,000–40,000    WITHDRAWN from this sum -- P50 lane (§7)
the 123,773 pool                             0    REFUTED as a lane (§5 item 2)
```

The two largest entries in v2's arithmetic are gone, and the replacement is bigger: the P95 excess
inside `gcRunAll` is +479,816, against a 91,844 requirement. **The gap is 19% of one bracket's
excess.** Closure does not need a fidelity trade; it needs the gate-arm attribution that says which
function inside `gcRunAll` spikes. Stop condition unchanged: net P95 ≤ ~1,100,000–1,110,000, cadence
target met on the `DRAW=0` arm (§6, owner-decided), then the R2-08 switch and the owner's playtest.

## 12b. RESUMED 2026-08-15 — new bank `build-c185-gxcompose-bank`, and the owner's four-task queue

```text
WORK-H  P50 938,112 · P90 1,088,192 · rank-80/P95 1,174,016 raw / 1,149,069 net · top-1% 1,520,832
GAP:    +28,689 net to 1,120,380
```

`GX_COMPOSE` banked. The queue below is the owner's, in the order I am running it (one agent,
sequential). Each carries its own STOP condition; **an honest STOP is a complete task.**

1. **Cold-code partition** — `-freorder-blocks-and-partition` on `scene_backend.o` + `nds_renderer.o`
   only, behind a default-OFF lab flag. **Compile/map-only gates BEFORE any emulator run**, and STOP
   with no performance build if the option emits no meaningful cold section, moves only trivial
   bytes, violates RAM/headroom, or the removable fetched-line ceiling is <16K ticks. **This is not
   the refuted placement campaign** — that refuted *layout*; this tests *basic-block partitioning*.
   Primary evidence is v3 `icache_fill` and **fetched cold lines**, never section size.
2. **SITR direct-child decomposition — ATTRIBUTION ONLY.** Task 108's callback result is **binding
   and preserved**: all 60 callback roots hold 6,373.5 tk/fr unique marginal closure, and even the
   deliberately impossible callback + `ftMainSetStatus` + figatree-attach bound is 14,395.5 — below
   floor. `NDS_TASK108_SITR_CALLBACK_CENSUS` stays default OFF. The open question is cycle 109's,
   never re-derived on this tree: the **direct children of `SITR = SINT − SCPU`** on c185.
3. **Animation representation** — host-side AOT compiler for Mario/Fox FIGATREE → compact typed
   track rows, consuming task 2's attribution. Not another arithmetic cut.
4. **Draw-side fixed point** — exact caller classification first; **the "31.2% × 94,602 ≈ 29K"
   shortcut is forbidden** (a whole-match caller share times a P95-set total mixes populations).
5. **devkitARM / devkitPro toolchain bump — queued by the owner 2026-08-15, runs LAST.**
   **Do not do it expecting `-freorder-blocks-and-partition`**: task 1 proved that is an ARM
   *back-end* limitation under GCC 15.2.0, not a version gap, and a newer GCC will almost certainly
   still refuse it. The real reason is arithmetic: **1% of codegen is ~11,700 ticks against a +28,689
   gap**, sign unknown.
   **It runs last because it invalidates every banked number** — c185, the ≥14,080 placement floor,
   the hot-footprint census, the soft-float call prices, the ITCM budget and RAM headroom were all
   measured on the current toolchain. A bump usually carries **libnds/calico** too, so it is a
   *correctness* surface as well as a codegen one (this project has already been bitten once by
   Calico's abort handler disabling the protection unit).
   **Shape:** pin and record the current toolchain version first; build the **identical** c185 config
   on both; A/B with full cross-build discipline. **Acceptance is Boundary green plus every gameplay
   invariant exact on the new toolchain BEFORE anyone looks at ticks** — a moved invariant is a
   correctness investigation, not a perf result. Specific exposure to check by name:
   `ndsR2AnimValueQ`'s measured `target("arm")` attribute exists *because* Thumb has no SMULL, and
   getting that arm wrong once cost **+25,472 P50**; new soft-float/64-bit codegen can move it either
   way. Budget is small — builds ~75 s, gate run ~150 s.

**TREE — A LAW VIOLATION, AND THE OWNER HAS RULED ON IT (2026-08-15).** A prior session's agent
**edited `decomp/` directly**, against `AGENTS.md`'s *"Treat `decomp/` as read-only reference source.
Our Source of Truth. Never edit it."* The tree carries: modified `decomp/BattleShip-main/decomp/src/**`;
**all ten patches under `scripts/decomp-patches/battleship/` deleted**; modified
`src/import/battleship_*.c`; modified `scripts/fetch-battleship-reference.ps1`; and new untracked
`scripts/check-decomp-pristine.ps1`, `scripts/generate-battleship-import-overlay.ps1`,
`scripts/import-overlays/`.

**Owner's instruction: ADOPT the changes** — keep the work, bring it into compliance. That means
`decomp/` ends **provably pristine** (by `fetch-battleship-reference.ps1 -VerifyOnly` and
`check-decomp-pristine.ps1`, never by grep) with the edits carried in whichever sanctioned
representation the evidence supports: the established patch scheme, or the in-flight import-overlay
scheme if it is complete and coherent. **Do not build a third scheme.**

**RESOLVED 2026-08-15, `40dd9c89e80`.** The **import-overlay** representation landed — it was already
complete and coherent, not half-built. `generate-battleship-import-overlay.ps1` copies the pristine
files into `$(BUILD)/battleship_overlay/` and `git apply --directory`s
`scripts/import-overlays/battleship/*.patch` onto that ephemeral copy; the `src/import/` wrappers now
`#include <battleship_overlay/…>`. Git recorded **8 of 10 as R100 renames** — the patch text is
byte-identical — and the other **2 were genuinely lifted port-side**. Restoring the patches would have
been the *smaller* move but the *wrong* one: this finishes the owner's own 2026-08-06 decision that
the eight patches migrate port-side over time, which the tree had been contradicting.

**`decomp/` verifies pristine, by hash and not by grep:** `DECOMP_PRISTINE=PASS
pinned_historical_files=10 ds_markers=0 decomp_patch_pipeline=absent`. `-VerifyOnly` now hashes the 10
historically-patched files **and an aggregate over the whole `decomp/src` tree** — any byte, add,
delete or rename fails.

**Zero built bytes moved.** `build-c190-overlay-byteproof`, rebuilt at HEAD **before** committing
(the git short hash is baked in, so the proof had to precede the commit):
`.itcm`/`.text.hot`/`.text.hot.draw`/`.main`/`.main.rw`/`.dtcm` — **0 differing bytes across all six**,
headers identical in name/size/VMA. **And the bank was never at risk**: `build-c185-gxcompose-bank`'s
own `battleship_overlay/.stamp` proves **c185 was itself built under the overlay scheme**.
**BANK STANDS — rank-80 1,174,016 raw / 1,149,069 net, +28,689 net.**

**The gate is structural and confirmed firing.** `check-decomp-pristine.ps1` runs inside
`check-gbi-decode-fixtures.ps1`, which `verify-all.ps1` invokes unconditionally on **every** profile;
its output is **line 2 of the Boundary log**. It fails three ways: hash drift, any `SSB64_TARGET_NDS`
marker under `decomp/`, and **the mere existence of a `scripts/decomp-patches/battleship/*.patch`.**
Editing `decomp/` is now inexpressible without a red verifier.

### Task 1 — COLD-CODE PARTITION: STOP fired at gate 1, zero builds

```text
cc1.exe: note: '-freorder-blocks-and-partition' not supported on this architecture
```

devkitARM **GCC 15.2.0**, under the repo's exact `-O2 -ffunction-sections -fdata-sections`, on a probe
TU with a bulky `__builtin_expect(…,0)` path. Checked three ways — `-march=armv5te -mthumb` (the
build's `ARCH`), `-march=armv5te -marm`, `-march=armv7-a -marm` — **section table identical with and
without the flag in every case.** No `.text.unlikely`, no cold partition: an **ARM back-end
limitation**, not a Thumb-1 / `-Os` / `-march` artifact.

**Free finding: the linker was never the obstacle.** `linker/nds_hot_text.ld`'s `.main` already
collects `*(.text.unlikely .text.*_unlikely .text.unlikely.*)` **ahead of** `.text.hot` and the
`.text.*` catch-all, so a cold partition would have been grouped away from its hot bodies for free had
one existed. Gate 3 would have passed; gate 1 makes it moot.

**`HOT_FOOTPRINT.md`'s 42,892 B is reachable only via its own step 3 — manual out-of-lining, hand work
per function** — which must be sized against the ≥16,000 tk/fr floor before anyone starts. Recorded
as a STOP block in that file so it is not re-derived.

## 12c. TASK 2 RESULT — `SITR` does NOT close; animation owns it and the mechanism is 41,376 (2026-08-15, `36516db214f`)

`build-c192-sitr-profile-gxc`, marginal-80 mask, basis `cycles/160`; whole-match `cycles/3,202`.
Measured bracket **`SITR` = 258,196** (DRAW=0) / **260,354** (DRAW=1).

| direct child | marg-80 | whole | excess | conc | marg calls/fr |
|---|---:|---:|---:|---:|---:|
| **`ftMainPlayAnim`** (incl. inlined `…EventsAll` 1st half) | **89,099** | 57,356 | +31,743 | **1.55x** | 5.49 |
| status callbacks (60 targets) | 62,155 | 25,143 | +37,012 | 2.47x | — |
| root body | 7,986 | 7,567 | +419 | 1.06x | 4.00 |
| `ftMainUpdateMotionEventsAll` | 2,998 | 1,166 | +1,832 | 2.57x | 5.46 |
| `ftMainUpdateColAnim` (+Reset) | 1,905 | 1,527 | +378 | 1.25x | 6.99 |
| *(`ftComputerProcessAll` = `SCPU`, subtracted)* | *12,435* | *12,272* | *+163* | *1.01x* | *4.00* |
| `ftKeyProcessKeyEvents` / `ftHammerUpdateStats` | **0** | 0 | — | — | **0.00** |

Named non-SCPU subtotal **164,144 = 63.6%**; the 94,052 residual is the shared leaf pool (soft float,
`ftGetStruct` 9,806 at 264 calls/fr, memcpy/memset) and **is never charged twice**. Every child is
priced by measured entry-PC call rates against program-wide caller sets — **the static SHARED row is
508,694, 1.97x the whole bracket**, which is the arithmetic proof that reachability could not have
answered this.

### Hand-off to task 3, consumable as-is

```text
animation lane (under ftMainPlayAnim)   128,050 marg-80 / 82,431 whole / 1.55x
  PARSE half  <- THE MECHANISM          41,376 marg-80 / 18,564 whole / 2.23x
      ndsR2FtAnimParseDObjFigatree + BuildTrackTable + TargetValue + AObjToQConvert
      96.94 calls/marginal frame re-deriving AObj fields from static FIGATREE ROM data
      1.44x the +28,689 requirement · 2.59x the 16K floor
      cross-checked at 40,003 on the c185 gate arm's own population — 3.3% apart
  EVALUATE half                         53,818 at 1.40x — already fixed point, reducible not deletable
per-call: parser 272 tk/call at 17.66 joints per ftParamUpdateAnimKeys · gcPlayDObjAnimJoint 273 ·
          ndsR2AnimValueQ 70.9 at 3.91 AObj/DObj
```

**Retraction: `SITR = 310,662.4` was ONE INSTRUMENT FRAME.** Frame 756 carries a **2^22 event**
(4,163,136 over median, 0.74% off 4,194,304) inside the c185 raw top-80, contributing **+50,309 by
itself** — the long-standing 2^22 artefact finally surfacing inside an attribution and being caught
rather than banked. The banked rank-80 percentile is **not** adjusted. Task 108's verdict is untouched
and *strengthened* (6,373.5 / 14,395.5 = 2.5% / 5.6% of a 16%-smaller bracket);
`NDS_TASK108_SITR_CALLBACK_CENSUS` stays default OFF.

**Frame-sequence identity proven before any join**: dropping frame 756 lifts the two c185 arms'
`SINT` correlation **0.5790 → 0.9991** (`SCPU` 0.9998, `SPRM` 1.0000). Three profile arms at three
HEADs correlate r ≥ 0.982. Falsifier arm `c191`, one flag apart, reproduces every headline row within
**2%**.

> **INSTRUMENT BUG, now fixed in the tool's docstring: the harness banner's frame→region map is OFF
> BY ONE — `region = frame − 439`, not `− 438`.** At `−438` the c185 rank-80 frames land at median
> profile rank **454 of 1,600** and *every* `SITR` row reads **below** its whole-match rate. Any
> earlier attribution built on `−438` is suspect.

Also: **`NDS_R2_FIGHTER_GX_COMPOSE=1` needs the `_LAB` escape** — a first arm silently built at flag
0, caught by reading the generated header. And **7.4 GB of raw v3 captures sit in the artifact dir**;
the reduced CSVs are committed, so they are deletable in a cleanup slot.

## 12d. TASK 3 STAGES 1–2 — both PASS, zero builds; the mechanism re-sizes to 33,951 (2026-08-15, `96cb19e56c2`/`df3bc581f28`)

**Stage 1, corpus equivalence: PASS, 0 mismatches.** `scripts/generate_ftanim_track_pack.py` compiles
every Mario/Fox FIGATREE stream into typed track rows (`u16 kind:4|mask:10|frames:1|block:1`, optional
`u16` frames, relative `s16` jumps, **authored s16 target words unchanged**).

| corpus | clips | scripts | rows | commands | states | callbacks | mismatches |
|---|---:|---:|---:|---:|---:|---:|---:|
| items-off (P1) | 259 | 4,901 | 66,022 | 81,646 | 75,237 | 6,409 | **0** |
| full | 297 | 5,629 | 77,129 | 94,135 | 86,846 | 7,289 | **0** |

Quantisation layer 19,303 / 20,601 triples → 0; corpus hash `cb28f9bf65c4…`; **falsifiers fire
1 / 1 / 4,272**, the third breaking the `value_base <- value_target` chain — a semantic rule only a
real state comparison can catch. **Design insight: storing the AUTHORED word is exact *and* half the
size** — the Q12 word the parser writes is a pure left shift of it, and baking Q would need up to
24 bits (2,378 corpus values exceed 16).

> **What the two arms SHARE, named as required:** both read the o2r bank through
> `ftanim_reloc_probe` and take the parser's semantics from **one transcription** — so **a wrong
> reader and a wrong parser model are invisible to all three layers.** The reader is validated by
> execution (it emits `battlepack_fox.bin`, which the shipping ROM parses and plays). **The
> transcription guard is NOT currently valid** — see below.

**Stage 2, sizing: PASS, and it needs no new headroom.**

| config | AOT pack | resident today | delta |
|---|---:|---:|---:|
| **Fox, items off** | **287,082 B** | **287,904 B** (`battlepack_fox.bin`) | **−822 B** |
| both fighters, items off | 557,670 B | 553,696 B | +3,974 |

**0.954x the source o2r payload — a drop-in replacement for something that already fits**, so the gate
is answered by construction and the 32,768 B reserve is untouched. **Worst-case mutable state for two
fighters: 11,440 B**, bounded by two measured corpus maxima (22 joints/clip, 9 union tracks/script),
replacing ~6,431 B of AObj nodes. The owner's closed 20-byte write-record bank measures **3.68x**
here — closure confirmed.

> **MECHANISM RE-SIZED: 41,376 is RETIRED, the number is 33,951.** Splitting the *same* c192 capture
> at instruction level (0 new runs), the animation clock is irreducible: CLOCK 5,305 + EPILOGUE 1,293
> + EARLYOUT 763 + CHANGED 65 = **7,426**. Remainder **33,951 tk/fr = 1.18x the +28,689 requirement,
> 2.12x the 16K floor.** Call split from the counters' own entry PCs: **54.9 early-out + 42.0
> stepped** of 96.9/frame, 18.0 `ANIM_CHANGED`. **Top PC is 3.1% — flat, so the lever is executing
> fewer instructions, not deleting one.**

**Surprises that shape stages 3–4:**
- **`SetTranslateInterp`, `SetFlags` and the `TraI` track occur ZERO times** across the corpus —
  `aobj->interpolate`, animation-driven `dobj->flags` and `syInterpCubic` are all **unreachable from
  fighter animation**.
- **`func_anim` has NO WRITER** anywhere in `decomp/src` or `src/` (only `= NULL` at
  `objman.c:1717`), so the −1/−2 callbacks are **inert**. **Stage 4's oracle must therefore compare
  DECISION POINTS, not observed calls** — otherwise it is a control that cannot fail.
- **`check_ftanim_transcribe.py` and `check_ftanim_target_exact.py` are RED and UNWIRED**, stale since
  `514fad238da` (the Requirement-4 fixed-point rewrite). **Nothing currently proves the port parser is
  a faithful transcription of decomp's** — and that is exactly the shared leg above.
- **~1,900 tk/fr of shipped `volatile` diagnostic counters sit in the parser** and no verifier reads
  them.
- `parent_gobj->anim_frame` is read by `ftanimend.c` and eight lines of `ftmain.c`, so **the f32 clock
  may not change representation casually.**

**Not done:** stages 3 and 4 — no runtime, no route bit, no oracle arm, no A/B; the pack is not wired
into the Makefile and no ROM contains it.

## 12e. TASK 3 STAGES 3–4 — oracle at 0/12,232, A/B tick-neutral, **and a SHIPPED gameplay defect found** (2026-08-15, `64c41c361a7`/`5d014c1519b`)

> ## DEFECT 1 — A SHIPPED ONE-FRAME SEGMENT-PHASE REGRESSION. THE c185 BANK IS INVALIDATED.
>
> `69ce92e279f` (Requirement 4) hoisted `-anim_wait - anim_speed` out of the per-track loop of
> `ndsR2FtAnimParseDObjFigatree` into a function-local `len_new` in **all four** write cases and added
> the **assignment to only one**. Proven, not inferred: `git show 69ce92e279f^` has four identical
> expressions at `:334/:375/:415/:476`, and `decomp/.../ft/ftanim.c:158/193/232/293` writes it fresh
> in each case.
>
> **Opcodes 4+5 are 45,679 of 55,261 items-off write commands = 82.7%, and they sit in a case that
> never assigned it.** Segments started at phase `0`, and `gcPlayDObjAnimJoint`'s `length += speed`
> then put the first evaluated sample a **whole frame into** the segment.
>
> **It changes the shipped ROM and the fight**: spark 15→16, shield attach 1,352→**480**, AObj
> high-water 1,266→**774**, packHits 197→257 (P1Damage 76 and runaway 0 unchanged).
> `build-c193-segfix` reads rank-80 **1,228,608** — *a different match, not a regression*.
> **RE-BANK ON THE REPAIRED TREE IS MANDATORY BEFORE ANY FURTHER PERFORMANCE VERDICT.**

**DEFECT 2 — the shared-decoder blind spot, caught by the oracle on its FIRST run.**
`ftanim_reloc_probe.decode_script` filled `targets` only for commands carrying per-track *words*, so
opcode 11 `AddLen` — which scans `flags` and adds its payload to `length` per selected track — looked
like it touched nothing. **`run_commands` iterates the same field, so the reference and every
candidate agreed on the same wrong answer and layers A/B/C could not see it.** Oracle reported
`OracleBad 4`, first failure `0x5070B` = (flags≠mask, AddLen, op 11), and **fail-closed cleared the
route word**. Fixed at the decoder; corpus hash `cb28f9bf65c4` → **`64b2f5a6a7e8`**, sizes unchanged,
three-layer proof re-run PASSES (falsifiers 1 / 1 / 4,272). **This is the second instance of the same
trap in this campaign, and a decision-point oracle is what found it.**

**Stage 3/4 delivered** (`NDS_R2_FTANIM_TRACK ?= 0`): bind once at
`lbCommonAddFighterPartsFigatree` keyed by figatree entry index; the step deletes the 15-way command
decode, the flag-bit scan, the `track_aobjs[10]` rebuild, per-node Q migration,
`ftAnimGetTargetValue` (→ one shift) and `ndsR2AnimRecipSlot` (→ one `ldr`). New generator **layer D**
proves entry *j* decodes to the script entry *j* points at.

```text
parser call elimination, exact, ONE ROM:
  generic parse 144,383 -> 115,288 = -29,095
  dense counters 24,197 early-out + 4,898 stepped = 29,095   <- exact match
  control hard zero on all five; all six invariants identical across arms
oracle: 12,232 decision points, 0 mismatches, fail-closed, PROVEN able to fail
```

**A/B (same ROM, one poked `volatile` word — placement floor ZERO), 1,600 frames:**

| | OFF | ON | Δ |
|---|---:|---:|---:|
| P50 | 945,088 | 945,664 | +576 |
| P90 | 1,126,784 | 1,132,288 | +5,504 |
| **rank-80** | 1,231,872 | 1,232,000 | **+128** |
| top-1% | 1,529,024 | 1,551,232 | +22,208 |

> **A PARTIAL CONVERSION CANNOT WIN BY CONSTRUCTION.** The generic parser still serves **79.8%** of
> calls, so its bytes stay hot, and the dense stepper's 3,368 B of code plus 12,244 B of sparsely-read
> rows are **pure fetch addition**. **Coverage is 8 of 137 Fox clips**, because the real budget is
> free-min minus reserve = **20,368 B**, *not* `check-boot-headroom.ps1`'s 312,448 ladder figure.
> **The 33,951 tk/fr mechanism is NOT refuted — this configuration is.**

Bytes: pack 12,244 B `.rodata`, live mutable state 3,552 B (74 × 48 B cursors), ELF Δ text +15,752 /
bss +3,584, **zero at flag 0** (TU not linked); heap free-min 53,136 → **40,848** against the 32,768
reserve. `max` is **not attributable** (generic-only arms in the same session drew 5,271,936 and
5,575,744).

**Also found:** `--gc-sections` **dropped three oracle counters despite `__attribute__((used))`**. The
~1,900 tk/fr of parser counters were deliberately **not** removed — they are the negative control this
cycle's engagement proof rests on. The two RED checkers are still RED and unwired, and **defect 1 is
exactly what a live transcribe checker would have caught.**

### The next cycle inherits, in order

1. **Re-bank on the repaired tree** — mandatory, nothing may be judged against c185 any more.
2. **Full coverage via the taskman ARENA** in place of `battlepack_fox.bin` (stage 2's proven −822 B
   drop-in), not `.rodata`. That is a cross-build arm and **the only configuration in which the
   33,951 mechanism can be priced.**
3. One v3 run on `build-c196-trackperf` to attribute the neutrality between fetch cost and
   off-percentile deletion — byte-identical pair, no rebuild.

## 12f. NEW BANK `build-c199-bank0` — **the gap TRIPLED to +85,393** (2026-08-15, `53934f2dad3`/`1eb6b453803`)

Repaired + bore-0 tree, c185's configuration (generated header verified `GX_COMPOSE 1`), DLDI on,
mode 163 one-minute, 1,600 samples, frames 439–2038, `slips=0`:

```text
P50 / P90                 942,912 / 1,124,480
rank-80 raw / net         1,230,720 / 1,205,773
EXACT NET GAP             +85,393        (was +28,689 against c185)
top-1% / max / trimmed    1,550,592 / 5,124,864 / 963,993
over-gate 170/1600 · VBI 1701/313/16/8 · max 19
invariants: P1Damage 76 · spark 16 · shield 480 · AObj 774 · packHits 257 · runaway 0
```

**Two arms bracket the level, not one**: `build-c193-segfix` reads rank-80 **1,228,608** at an earlier
HEAD on a separate build — **2,112 apart, inside the ≥14,080 cross-build floor**, with identical
invariants. Basis stated: rank-80 recomputed from the run's own 1,600 rows; **the harness banner's
`p95` column is a different convention and is not the banked figure.**

> **NOTHING REGRESSED. c185 measured a match the shipped defect made CHEAPER.** Every lever in
> `MENU.md` is now priced against the wrong requirement and **must be re-read against +85,393**.
> c185 is labelled superseded everywhere; do not compare against it again.

**Task B REFUSED on arithmetic, before a build was spent.** Stage 2's "−822 B drop-in" is a
**byte-count identity, not a functional interchange**. Three consumers need the o2r stream
`battlepack_fox.bin` carries: the bind recovers its asset id from the o2r **pointer**
(`ndsBattlePackAssetIdForSlotTable`); the generic parser is the fail-open path and **Mario is
unconverted entirely** (`BindMiss` 296 of 372 binds even at 8 clips); and **the oracle's reference
cursor *is* the o2r stream**. So full coverage means **coexistence**, not replacement:

```text
arena 1,548,288 · AllocFail 0 · resident 287,904 · heap free-min 53,136 vs 32,768 reserve
grantable ceiling 1,564,672 -> +16,384 available against a +287,082 ask
SHORT BY 270,698 B.   .rodata is bounded by the same meter at 20,368.
full two-fighter = 557,670 B = 50.9% of everything taskman has left
```

A sizing failure is a STOP — no arm, no soak, no gate run. **Full coverage is not a budget problem;
the o2r dependency has to be designed out.**

**Task C priced but not taken, and it is the decision.** From the existing one-ROM pair (floor zero):
**+69.4 ticks per exchanged call, ~1.59x the generic path** — derived, not banked. Pre-registered:
coverage scales **2.45x**; if the excess is steady-state **issue**, full Fox is ≈**+3,090 tk/fr and the
mechanism is refuted**; if it is compulsory **fetch**, the sign can flip. **The two predictions differ
in SIGN**, which is what makes the capture decisive. Correction: `build-c196-trackperf` **cannot**
carry a v3 — it lacks `NDS_TASK37_PROFILE=1`; Task C costs two profiler builds differing in one
`.data` word, so the one-byte-pair property survives.

### Fox bore — owner-confirmed, landed, and one open consequence

**One definition, three consumers, one shared base — a visual/collision desync was never
expressible.** `build-c198-bore0` at bore 0 → owner: ***"fox beam is perfect!"*** Landed **84 → 0** as
the shipping default, build-overridable (`make … NDS_FOX_BLASTER_BORE_OFFSET_Y=<n>`), recorded
verbatim in `docs/BUGS.md`, and 2026-08-14's "84, perfect" marked **superseded**. The bore-84 bank arm
was **discarded and re-run** rather than reconciled.

> **Surprise: the flip is gameplay-INERT over this match** — all eight invariants and four parser
> counters bit-identical across two different ROMs. v5's geometry predicted crouching Mario would
> overlap the beam by 1.181 units at bore 0 — **and the owner has refuted that from play.**
>
> **OWNER, verbatim 2026-08-15: *"i said it was perfect, that includes the mario crouching avoiding
> the beam"***. They exercised the crouch case on `build-c198-bore0` and it behaves correctly. **There
> is no defect and no decision to raise.** What remains is a stale document: **`FOX_BORE_COLLISION_V5`
> was computed on 2026-08-14, before the segment-phase repair, so if its clearance numbers were
> derived from runtime poses they inherit the same one-frame defect the bore constant did.**
>
> **The general lesson, now recorded: a geometry or clearance document derived from runtime poses is
> invalidated by an upstream pose defect exactly as a tuned constant is.** Two 2026-08-14 artefacts
> have now been falsified by the same one-frame bug — the bore 84, and v5's clearance figures. When a
> pose bug is fixed, **every number derived from poses in that window is suspect, not just the
> constants.**

**The other three 2026-08-14 eye-tuned fixes are NOT suspect** — Whispy is a validation fix with no
constant (and stage joints take `gcParseDObjAnimJoint`/AObj32 while the repaired parser is the
figatree `else` arm); BG `K=9/8` is a static q16 scale; VFX symmetry is a missing atlas submit flag.
**Only the bore read a pose.** Question closed.

**Two harness traps, paid for twice:** `sample-tick-hud-buckets.ps1` needs **pwsh 7** and dies
instantly under Windows PowerShell 5.1; and via `cmd /c` you must use **cmd's own `> log 2>&1`**,
because PowerShell's `*>` is passed through as a positional argument and **silently binds to
`-MelonDS`**. Both invisible without capturing the full log.

## 13. Immediate next task

Cycle 2 re-banked the gate (§0), took the first gate-arm v3 capture and named Phase 4's mechanism
(§9). Two things remain before a package may be written:

1. **Boundary is RED and the "stub ceiling" diagnosis is REFUTED — this is the only open blocker on
   end-state §1 item 3, and every 2026-08-14 bank sits on an unverified tree until it clears.**
   1,800 s times out exactly as 120 s and 600 s did, while the gate ROM finished a *larger* workload
   on the same slot/stub/DLDI/HEAD in **123 s**. Breakpoint 1 hits **0.05 s after attach**, so the
   failure is in **battle-scene setup, not boot**. Boundary was green in the default config at
   `8fc8b47c9ce` (2026-08-13 21:27), so the window is the 2026-08-14 commits — and most changed no
   shipped byte, so an ELF-section comparison across the window collapses it to two or three
   candidates before a build is spent. Prime suspect `33d7cc5d3b7` (it repacked the battle static
   texture sheets — the exact arming path the capture never reaches — and shipped a conflicting
   `mirror_mask` declaration, so it went in without a clean full-tree build). Fix the seam: raising
   the timeout, sleeping, skipping the marker or weakening the checker are all forbidden.
2. **Land the engagement counter before any Phase 4 code** (§9 law 1): a per-frame
   asset-acquisition counter on the **gate arm**, answering how many acquisitions happen per frame
   and whether they concentrate on the P95 frames. Publish it from code (header + `diagnostics.c`,
   `__attribute__((used))`, `nm`-verified against `--gc-sections`) — a dropped diagnostic global has
   turned Boundary red here before.

**Boundary is GREEN** (`830c13bf809`) — the red was a corrupt gitignored DLDI image, not a commit,
and the marker capture uses 23% of its ceiling. **Slice 1 phases 1–4 are DONE** (§K-RESULT). The
remaining work is:

1. **DONE 2026-08-15.** `RAM_RECOVERY_PLAN` Phase 2 landed (`8cfbc2eaa2b`, +146,560 B measured) and
   the inert `ftmain.c:4623` decomp patch landed (`6e93def43cd`, provably inert — exactly one symbol
   changes size, and the deleted instructions are a redundant post-call reload). Boundary green both
   arms. **Phase 5 is unblocked**: `fp->figatree` now takes the loader's return, and the port's
   `lbCommonAddFighterPartsFigatree` already resolves entries against whatever base it is handed, so
   a `const` pack clip can be returned directly.
   **What remains before the pack can be resident: choose and build the pool** (§K-RAM's retraction
   block) — only the combined pool fits, and it needs `NDS_TASKMAN_ARENA_SIZE` cut, the `0x130000`
   search floor lowered, and the Task 36 replay-admission guard retaught.
2. **Slice 1 is at arm G and the only open question is whether arm G can SHIP** (§K-MECHANISM).
   Arm G is `rank-80 1,170,048 raw / 1,145,101 net` — **net is +24,721 over the gate** — but it runs
   `NDS_TASKMAN_ARENA_SIZE 1,548,288`, +172,032 over shipping, and Boundary pins the shipping value.
   **Price that growth before anything else**, because it is far cheaper than the alternatives: the
   failed +258,048 attempt was granted only 188,416, and **+172,032 is under that**, while arm G ran
   with heap free-min **52,864 against the 32,768 floor**. If the growth is legitimately grantable
   with reserve, the displacement question disappears and arm G is the shipping arm.
   Only if it is not: close the **25,760 B** gap per kernel-doc §9 — drop ~11 *provably*-unreachable
   clips (absence from the ELF is not proof), or a lossy stream, which is a
   `BLOCKED(decision: ...)` fidelity trade. Metadata compaction is closed by arithmetic and a
   lossless stream was already refuted.
   Then phase 6's oracle (mismatch = 0 and event-order mismatch = 0) and phase 7's after-GO assertion
   before any default flip.
3. **Then slice 2** — reprofile, re-rank, and only then size the epoch-flattened `gcRunAll` process
   vectors.

Phase 3 (GX_COMPOSE leak fix, −13,632 with pixel-identical captures) remains available, independent,
and is a *correctness repair* — it can ride a kernel build, but it is not first: GX is not the P95
owner right now.

---

## §12g — Cycles c199–c205 (2026-08-15/16): the bore, the record, and the draw side

### Closed

- **Fox blaster bore.** Owner ruled directly: *"bore should be zero, no offset, not needed
  anymore"*. `NDS_FOX_BLASTER_BORE_OFFSET_Y ?= 0` in `Makefile:2422` and
  `include/nds/nds_effects.h:112`, still build-overridable (`b16dc16997a`). Boundary green at
  bore 0, 0 `Exception:`, `gxstat=0x6000000`. **The published battle ROM `6c939434…` was linked
  at bore 84 and changes hash at the next publish.**
  Not a performance question either way: the 84-vs-0 spread is **4,096 tk at rank-80**, inside
  the ≥14,080 cross-build floor.
- **The fabricated-approval episode.** An agent inferred from tree state that the owner's bore
  approval had been invented, wrote that inference into four tracked documents as fact
  (`9b25d4e1095`, `c16af61d61f`, `88abf259bda`), and reverted the default. The quotes were
  genuine owner turns. Corrected forward, not rewritten (`b16dc16997a`); both quotes restored
  verbatim with the crouch clause; `docs/BUGS.md` row closed. The `+69.4 tk/call, 1.59x`
  retraction inside those commits is **genuine** and stands — a residual ÷ a count, the
  campaign's fifth.
  **Rule earned:** suspicion of a relayed owner quote is correct; concluding alone is not.
  A tree that contradicts a quote is a reason to ask, never a licence to act.
- **Animation-representation lane — CLOSED at ~1% conversion.** v3 took the FETCH branch (dense
  side 72.4% icache+dcache fill; the ON arm executes 664,438 *fewer* instructions, refuting
  ISSUE three placement-immune ways). Both sides of the exchange ~3,800 tk/fr and cancel:
  **−319 tk/fr** linear to 100% coverage. Lane size was 33,951; struck from the pending class.
  The blocked 270,698 B full-coverage arena arm no longer needs building.

### The bank

`build-c199-bank0` (bore 0, shipping arm): **rank-80 net 1,205,773, gap +85,393.**
`build-c200-bank84` reads +81,297 — 4,096 apart, inside the floor. Judge against **+85,393**;
it is the shipping configuration.

### Draw-side fixed point (owner task 4) — measured, not projected

Classification from the **linked ELF** on the marginal-80 population (`4957d636f16`). Soft float
is **168,060 tk/fr**, not `MENU.md`'s 94,602 (a pre-repair c179 capture). Draw side =
**34,178 tk/fr, flat at 1.12x** against the class's 2.11x — the concentration is all in the
collision half, so the draw side converts ~1:1 against a level gate.

**The 5.14x in-binary prior was REFUTED by its own falsifier** (`6ade00538e4`). Measured
**1.700x** whole match / 1.915x on the gate population, and 1.700 is an *upper* bound.
Mechanism named: the prior (`guMtxCatF` 2,921 tk/call vs `ndsRendererMtxMul20p12` 568.40) is a
pure multiply-accumulate pair. A look-at needs **3 roots and 9 divides per entry** — `sqrtf`
144.62 tk and `fdiv` 59.19 tk, nothing like a 13.23 tk `fmul`. **The lane's transcendental half
converts far worse than its arithmetic half.**

Camera chain, same-binary `.data` route, floor zero: **paired median −4,736 tk/fr** (FTR −3,264
+ STG −1,664), improved 1,439/1,600, level across ranks 40→1600. **5.5% of +85,393.** Residual
lane 22,521 at the measured rate = **9,273 = 0.109x of the gap**; `DRAW_FIXEDPOINT.md` §5's
24,564 ceiling reads **~11,000–15,000**, and `LADDER.md` §4's 0.659x amendment needs re-reading
against that.

**Two mechanisms worth more than the cut:**
- **Inlining the kernel INVERTED the win** — 42 calls/entry → 12 for +3,032 B flipped the paired
  median **−4,736 → +1,600**, ~30 cycles per added line per entry, inside the measured 23–51
  cycle `icache_fill` band. On a kernel entered ~8x/frame, inlining is a cost.
- **The "2,976 B free ITCM" is the proof ROM's manifest.** The tick-HUD instrument has **580 B**;
  two link overflows proved it. Structurally, a `.data` route can never test ITCM residency for a
  replacement of an ITCM-resident library — both arms must be resident.

### BLOCKED(decision: draw-side precision) — with the owner

Pixels change **6.5350%** (max channel delta 251), structurally identical picture, speckle over
textured surfaces. `GX_COMPOSE` was judged against 0.0692%. Owner asked to judge in motion
rather than from stills; live-toggle ROM built (`35fd951aeb2`):
`builds\build-c205-camtoggle\smash64ds-battle-playable-proof-hwtri.nds`, **SELECT** (Right Shift
in the repo melonDS) flips arms mid-match, indicator on the bottom screen under TIME. Toggle
proven state-safe three ways; `gGMCameraStruct.look_at` has no simulation reader. Default
untouched, boots on FLOAT.

**The question is not "accept this 5.5% cut" but "is a draw-side precision budget of this shape
open at all."** If yes, particle/quad math (9,015 tk/fr, MAC-shaped — the exact shape the 5.14x
was measured on) is the better next candidate. If no, the whole 34,178 closes.

### Open

1. **Task 5 — devkitARM/devkitPro toolchain bump.** Owner-queued, not started.
2. `check_ftanim_transcribe.py` / `check_ftanim_target_exact.py` still RED and unwired since
   `514fad238da`.
3. **`GX_COMPOSE` published default is still 0** while −17,152 sits in the bank and this plan
   records its captures as *pixel-identical*. Resolve the contradiction with the 0.0692% figure
   quoted in the c204 report before flipping anything.
4. Re-capture the two Fox probes — v5's clearance terms are evaluated poses from inside the
   segment-phase defect window. Documentation refresh, not a gate.
5. Three measurement-tool defects fixed this cycle, each of which produced a wrong number first:
   `objdump --no-show-raw-insn` silently reducing a caller table to 3 entries; a whole-match
   `--census` against `--mask marginal` reading fadd **13.5x** high; `--thunk fsub=fadd` matching
   no symbol and silently disabling the fold (1.52x). All three now fail loudly.

---

## §12h — Cycles c206–c224 (2026-08-16): the gap went +94,481 → +48,081

### The level, and how it moved

| basis | requirement | what landed |
|---|---:|---|
| `build-c206-shipgx0` | **+94,481** | basis corrected — the bank had `GX_COMPOSE=1`, shipping pins 0 |
| `build-c215-hwmath-ship` | +82,065 | `nds_r2_sqrtf.o` to `-marm` + all 16 hardware-unit leading polls removed |
| `build-c217-tilesync-ship` | +73,425 | `ndsRendererSyncTextureTile` memo — 62.12% of 144,105 calls provably redundant |
| `build-c219-animitcm-ship` | +71,569 | `ndsR2AnimValueQ` into ITCM (ITCM held 2,454 dead bytes) |
| *(instrument correction)* | **+64,977** | `cpuGetTiming()` inflates a span by exactly 2^22 in 8 of 13 runs |
| `build-c220-camship` | +65,297 | **camera Q20.12 default ON** — owner: *"I think camera fixed point is ok"* |
| **`build-c223-ftrmemo`** | **+48,081** | fighter draw-contract memo (12,864, floor-free) + `CountFlags` gating (4,352) |

**46,400 banked, every step bit-identical or owner-accepted. Both root ROMs byte-unchanged
throughout; they change at the next publish because of the camera flip.**

### Closed, with reasons that generalise

- **The whole float→fixed class.** The leaf route measured **R = 0.83× and 1.00×** — a
  31,850 tk/fr lane converting to a **−2,660 loss**. Mechanism: an **f32↔Q edge conversion
  costs 31–42 cycles**, so R is set by **conversions per deleted operation, a signature
  property**. Transform 18/18 = 1.00 → 0.83×; compose 36/63 = 0.57 → 1.00×; the 5.14× prior
  had **0** conv/op. The owner's hypothesis that every A/B compared cold fixed against
  ITCM-resident float was **refuted and inverted**: `guMtxCatF` is `.main`,
  `ndsRendererMtxMul20p12` is ITCM, so 5.14× *overstates*. Every other pair had both bodies
  in `.main`. **Soft float is 1.4% of `FTR`, the largest lane.**
- **Toolchain.** devkitARM r67 is the newest release; the helpers are hand-written
  `754-sf.S`; the arch-variant ones are already repo-authored with the `clz` banked;
  `_arm_muldivsf3.o` is byte-identical across architectures. Zero builds spent.
- **`GX_COMPOSE`.** −17,152 was pre-repair. Same-HEAD pair reads +7,040 rank-80 / −4,288 P50
  — inside both floors, disagreeing in sign. Its FTR→STG "transfer" refuted: every stage work
  counter bit-identical, ΔSTG inverting +6,656 → −1,632.
- **`SHDT`.** Cost is **per engaged frame, not per pair** — 44-pair frames run exactly 2.00×
  the rectangle tests of 22-pair frames while every transform body stays flat, and are
  **10,004 ticks cheaper**. The setup is not redundant: the renderer **consumes** the
  collision chain's matrices; the frame builds them *because* hit detection asks.
- **`BLOCKED(decision: transition-frame animation play)` — REJECTED by the owner** after
  playing `build-c224-animhold`: *"kill animhold, lots of issues with animations not visibly
  playing"*. Forgone 13,376–37,027; the attach chain (+23,801) closed by the same verdict.
  **Why it fails:** `ftMainSetStatus`'s play is the **sole writer of the new status's first
  pose** — it first resets every joint to the `DObjDesc` bind transform and attaches the
  figatree *unposed*. Suppression yields a **bind-pose flash**, not a lagged pose. The variant
  the owner played held the *previous* pose — the cleanest isolation that exists — and still
  failed, so **the family is closed, not one implementation.**
  **Correction:** `ftMainRunUpdateColAnim` is **colour** animation, not hitbox placement.
  `ATTACH_LANE.md` §4 and my brief both had this wrong.

### The instrument, which was lying

- **`cpuGetTiming()` inflates a span by exactly 2^22 = 4,194,304** when a 16-bit overflow
  credit is missed on the span's *start* read. Asymmetric — it can only inflate. Filter
  (`ALL >= (1<<22)`, complete by construction) is live and load-bearing: it lands on
  **different frames in different arms**, so an uncorrected pair carries spurious rows on one
  side.
- **The instrument is bit-deterministic** — three sessions on one ROM gave byte-identical
  1,600-row CSVs. A same-arm repeat buys nothing.
- **Rank-by-rank differencing measures the permutation**, not the difference: top-20 sets
  overlapped 19/20 but only **9/20 shared a rank**. Compare frames.
- **`-PerFrameGlobals` emits a torn row** — 1,526 of 1,600 violate `ALL == WORK + WAIT`;
  ring path 0 of 1,600, counters sound at ring offset **+1**.
- **A clip-to-median re-rank is not an implementable size** — see
  `[[clip-to-median-is-not-an-implementable-size]]`. It overstated a lane **5×**.

### Where the gate lives now

**The median frame passes by 218,767** — sixteen lane medians sum to 926,560 against a raw
gate of 1,145,327. **The entire requirement is excursion.** Smallest single-lane fractional
cut that closes it: `FTR` **22%**, `STG` 38%, `SITR` 39%, `SHDT` 80%, the other nine never.

**`FTR` is the dynamic extent of one function**, `ndsFighterDisplayContractSubmit` — which is
why per-PC profilers never saw it. 291,051 tk/fr, verified three ways to 0.2%.
**Concentration 1.00–1.09 on all twenty largest rows: a uniform cut converts at ratio 1.000
from D=1,000 to D=120,000.** Stall split **icache_fill 30.4% > issue 21.6%** — the largest
lane spends more fetching instructions than issuing them, and holds 24.1% of the run's
instruction fetch and 52.6% of its GX-FIFO stall.

### Open

1. **4,901 tk/fr of memo overhead**, of which **1,280 B/frame of `memcpy` is removable** —
   submit runs immediately after each slot's capture, so the consumer could read the slot
   cache directly.
2. **`STG`** — P50 175,424, 38% to close, untouched, and the `FTR` instrument now fits it
   with the stage draw span as root.
3. The **DL-swap hole** on the draw memo is unverified this cycle; `NDS_R2_FTR_DRAW_MEMO`
   ships `?= 1` at a 96.19% hit rate.
4. **9,484 B of cold bytes inside five live renderer functions** — the largest ITCM reserve,
   reachable only by a source split, not a flag.
