# The FGM slot cache was a ONE-slot cache, the FindPlanned lever does not exist, and the collision seam has a third hazard

**Outcome, in the order the cycle produced it.** One instrumented build answered
two of the three briefed mechanisms before either was measured on the gate:

- **(c) `ndsAObjEvent32FindPlanned` — REFUTED, and the index was reverted rather
  than shipped.** The inherited "second O(n²) scan, 161,203 iterations,
  ~665 tk/frame" is a mis-attribution. Counted live, the function is entered
  **1,188 times in a whole match**, and the table it scans is reset per script
  and holds 6.4 entries on average. The real cost is **13 tk/frame**, with
  ~302 tk/frame as the extreme upper bound the data allows.
- **(b) the FGM slot cache — the defect is not the slot COUNT, it is the victim
  rule's strict `<`.** Counted live: **188 plays, 38 hits, 150 misses (79.8%),
  and 143 of those 150 evicted a still-resident cue** while at most **5** of the
  8 slots were ever pinned. Four slots share capacity 16,384 and three share
  28,672, and `slot->capacity < best->capacity` can never move `best` between
  equals — so an eight-slot cache behaved like a **one-slot cache per size
  class**. The shipped change is a recency tie-break, not a repartition.
- **(a) the collision ring — NOT WIRED, deliberately**, and §5 records three
  corrections to cycle B's plan that a wiring cycle would otherwise have paid
  for with a build.

Root ROMs byte-identical across the whole cycle, hashed before the first build
and after the last:

```text
smash64ds.nds                       54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds 524448c99c31b62672a63f29914438059d5f9700e10306d147d6342b3223adee
```

No published target was built; every build here is a lab build.

---

## 1. Arms

Repo-local `emulators/melonds/melonDS.exe` (`sha=DE80E46BDCF1FD98`), DLDI **ON**,
`NDS_TICK_HUD_DRAW 1`, `NDS_R2_BOTH_CPU 1`, mode 163 one-minute, `-Samples 1600
-RingDump -AllowRepeatedFrames -NoBuild`, frames 439–2038.

| arm | build | source | ROM SHA-256 (16) | text / bss |
|---|---|---|---|---|
| **A** control | `build-c147-ctl` | HEAD `bf22a37eec3` | `466f736cb6718b99` | 981,588 / 1,471,976 |
| diagnostic | `build-c148-diag` | + FGM counters + plan index | — | — |
| **B** candidate | `build-c149-fgmlru` | + FGM counters + recency tie-break | `712d224ab5db7dc1` | 981,908 / 1,472,040 |
| **A2** falsifier | `build-c150-nolru` | `NDS_AUDIO_FGM_CACHE_LRU=0` | `63aa2f2f34270402` | 981,892 / **1,472,040** |

**A2 is a flag, not a rebuild**, for the reason the ledger-index cycle
established: this build is byte-reproducible and the tick-HUD sampler is
bit-deterministic, so a repeated control brackets nothing. At `LRU=0` the
`last_use` field still exists, is still stamped on every hit and every fill, and
still occupies its bytes in `sNdsAudioFgmCacheSlots` — **B and A2 have identical
bss (1,472,040) and 16 bytes of text between them.** Only the comparison that
reads the stamp reverts.

**The diagnostic arm is also the placement warning.** `build-c148-diag` differs
from A only by counters and a reverted-in-the-end index, and its `WORK-H` P95
read **1,193,024 against A's 1,210,944 — −17,920 for no behavioural change on
the gate path.** That is 3.3x the ±5,376 cross-build floor this project quotes,
and it is why the verdict below is taken against A2 and against the engagement
counters rather than against A alone.

---

## 2. (c) `ndsAObjEvent32FindPlanned` — refuted, no gate run spent

`../2026-08-13_c-ledger-index/LEDGER_INDEX.md` §1 priced this from the c123
per-PC profile: PC range `0x02065cb6`–`0x02065cc2`, 161,203 iterations at
6.61 tk, "1,064,828 tk/match, ~665 tk/frame". The same index shape as slice 51
was built over the plan table and instrumented. Whole one-minute both-CPU match,
`build-c148-diag`:

| counter | value |
|---|---:|
| `gNdsAObjEvent32PlanHashLookupCount` | **1,188** |
| `gNdsAObjEvent32PlanHashHitCount` | 11 |
| `gNdsAObjEvent32PlanHashMissCount` | **1,177** |
| probes per lookup | **1.000** (1,188 / 1,188) |
| `gNdsAObjEvent32NormalizeCommandCount` | **1,177** |
| `gNdsAObjEvent32NormalizeScriptCount` | 183 |
| `gNdsAObjEvent32PlanHashOverflowCount` | 0 |

The miss count is **exactly** the command count, which is the structural check:
`ndsAObjEvent32PlanStream` calls `FindPlanned` once per command and appends on a
miss. 11 hits are the script-internal revisits.

**161,203 iterations cannot happen in 1,188 calls over a table that is reset per
script and capped at 128 entries.** With 1,177 commands over 183 scripts the
table averages 6.4 entries, so the scan it replaces costs
`sum k(k-1)/2 ≈ 3,200` iterations a match — **13 tk/frame**. Even the most
concentrated distribution those two numbers allow (nine scripts at the 128 cap)
reaches only ~73,000 iterations, **~302 tk/frame**, and
`gNdsAObjEvent32NormalizeFailCount = 0` proves no script hit the cap at all.

The profile range is the **`FindNormalized`** scan, which `PlanStream` also
inlines — once per command, over a ledger that reaches 1,177 entries. That is
165,034 ≈ 161,203. Memory *"addr2line names deleted and inlined functions"*
covers this; the new instance is that **two inlined copies of the same loop can
be attributed to two different functions.**

**Reverted.** 256 bytes of bss and its text for at most a P50 crumb on the
normalize frames — which sit at low gate ranks — fails "at equal cost, less code
wins". The refutation is recorded in the source at
`src/import/battleship_sys_objanim.c` so the index is not re-proposed.

### The stale comment the brief asked for, corrected

`battleship_sys_objanim.c` justified `NDS_AOBJ_EVENT32_NORMALIZED_MAX 2048` as
"2x the largest corpus ever measured … 1,029 spare slots". That was true against
the pre-anim-joint-fix corpus of 1,019. The five-minute corpus re-measured at
**1,598** after the fix (`LEDGER_INDEX.md` §4), so the real margin is **450
spare, 1.28x**. Corrected in place; capacity remains the open question at
exactly 1,598 of 2,048.

---

## 3. (b) The FGM slot cache

### 3.1 What the counters found — the instrument BAND_IO_OWNER §5 asked for

`../2026-08-13_c-band-io/BAND_IO_OWNER.md` §5 item 1 recorded that there is no
hit/miss counter on this cache, that `ndsAudioFgmRecordMiss` counts
pack-absent cues rather than slot misses, and that "one `++` pair is the
instrument any future attempt needs first". This is that pair, plus the
working-set masks. Whole match, `build-c148-diag`:

| counter | value | note |
|---|---:|---|
| `gNdsAudioFgmPlayCalls` | 188 | |
| `gNdsAudioFgmCacheHitCount` | **38** | |
| `gNdsAudioFgmCacheMissCount` | **150** | **79.8%**, against the profile's 71.1% inference |
| `gNdsAudioFgmCacheEvictCount` | **143** | 95% of misses overwrote resident data |
| `gNdsAudioFgmCacheNoSlotCount` | 0 | no cue is being dropped today |
| `gNdsAudioFgmCacheMaxPinned` | **5** | of 8 slots; 3 were always free |
| `gNdsAudioFgmMaxActiveHandles` | 6 | |
| `gNdsAudioFgmPlayFailCount` / `ReadFailCount` | 0 / 0 | |

**Pinning is not the ceiling.** BAND_IO_OWNER inferred "with the measured peak
of six live handles only two slots are ever eligible"; the live count says at
most five slots were ever pinned, so at least three were always free. The 143
evictions are not contention — they are the victim rule choosing the same slot
over and over.

### 3.2 The working set, decoded from the masks

`gNdsAudioFgmCacheReqMask{0,1,2}` = 4,148,116,479 / 1,637,809,911 / 654,612, and
`MissMask` is **bit-identical to `ReqMask`** — every cue the match plays missed
at least once. Intersected with the pack's own entry table
(`assets/audio/fgm_phase_pack_ima.bin`):

| | |
|---|---:|
| distinct cues played | **59** of 88 |
| working-set bytes | **575,760** against a 204,800-byte cache |
| smallest / median / largest cue | 316 / 6,656 / 52,108 |

| cue size class | cues | slots that can hold it (8-slot partition) |
|---|---:|---:|
| ≤ 1,024 | 4 | 8 |
| 1,025 – 2,048 | 7 | 8 |
| 2,049 – 4,096 | 9 | 8 |
| 4,097 – 8,192 | 13 | 8 |
| 8,193 – 16,384 | 15 | 8 |
| 16,385 – 28,672 | 9 | 4 |
| > 28,672 | 2 | 1 |

Residency is impossible (575,760 against 204,800), which confirms
BAND_IO_OWNER's arithmetic on a measured working set rather than on the whole
pack.

### 3.3 Why a repartition was NOT taken, and the tie-break was

The victim rule is

```c
if ((slot->references == 0u) && (slot->capacity >= entry->data_bytes) &&
    ((best < 0) || (slot->capacity < sNdsAudioFgmCacheSlots[best].capacity)))
    best = (s32)i;
```

Capacities are `53,248 / 3 × 28,672 / 4 × 16,384`. For any cue under 16,384 —
**48 of the 59 the match plays** — the scan sets `best` at slot 4 and can never
move it to slots 5, 6 or 7, because 16,384 is not *less than* 16,384. The same
holds for slots 2 and 3 inside the 28,672 class. **The eight-slot cache is a
one-slot cache per size class**, which is exactly what 143 evictions in 150
misses measures.

A repartition of a **fixed** 204,800-byte budget cannot add slots without taking
eligibility from a size class: every table checked here that adds small slots
drops the 8,193–16,384 class from 8 eligible slots to 5 or 6, and the
16,385–28,672 class from 4 to 3, against up to
`NDS_AUDIO_FGM_HANDLE_CAPACITY = 8` handles able to pin. That is a path to
`gNdsAudioFgmCacheNoSlotCount` going non-zero — **a silently dropped sound** —
and it is refused here. The recency tie-break leaves the eligibility SET of
every cue exactly as it is and reorders only the choice within it, so it is
drop-safe by construction; that is why `NoSlotCount` is reported as an invariant
below rather than as a risk.

The class table above is left in this document so a later cycle that wants the
repartition has the measured working set and does not have to re-derive it.

### 3.4 Result — REVERT

| arm | `WORK-H` P50 | `WORK-H` P95 | `ALL` P95 | VBI 2/3/4/5+ max | over-gate | slips |
|---|---:|---:|---:|---|---:|---:|
| **A** control | 924,864 | **1,210,944** | 1,678,720 | 1740/272/13/13 max 26 | 136 | 0 |
| **B** candidate | 927,424 | 1,206,656 | 1,678,656 | 1737/279/11/12 max 26 | 138 | 0 |
| **A2** falsifier | 926,144 | **1,194,368** | 1,678,592 | 1737/273/14/14 max 26 | 139 | 0 |

Computed from the 1,600-row CSVs (P95 = rank 80 of the sorted window, which is
one bucket off the harness's own rounding; both are quoted so neither is
mistaken for the other):

```text
B  - A    P95  -5,056   P50 +2,624    sum excl. top 50  +3,218,368
B  - A2   P95 +11,776   P50 +1,408    sum excl. top 50  +1,047,296
A2 - A    P95 -16,832   P50 +1,216    sum excl. top 50  +2,171,072
```

**The placement floor on this pair, MEASURED, is −16,832 P95.** That is 3.3x the
apparent B−A "win" and **22x the mechanism's own ceiling**. Against its own
layout the candidate is **+11,776 WORSE**, and it is worse at P50 against both
arms and heavier over the whole window on the outlier-trimmed sum. There is no
reading of these three arms in which the mechanism pays.

The brief's rule is "REVERT if placement-ambiguous". This is past ambiguous:
A2, which carries the candidate's bss and 16 bytes less of its text while
executing the CONTROL's victim rule, is the fastest of the three. **REVERTED —
`src/nds/nds_audio_fgm.c` and the `NDS_AUDIO_FGM_CACHE_LRU` flag are restored to
HEAD.** The measurement is what this cycle keeps.

### 3.5 Engagement — predicted, then read, and the prediction was WRONG

| counter | prediction | A | **B** | A2 |
|---|---|---:|---:|---:|
| `gNdsAudioFgmCacheHitCount` | 90–130 | — | **47** | 38 |
| `gNdsAudioFgmCacheMissCount` | 60–100 | — | **141** | 150 |
| `gNdsAudioFgmCacheEvictCount` | — | — | 133 | 143 |
| `gNdsAudioFgmCacheNoSlotCount` | 0 | — | **0** | 0 |
| `gNdsAudioFgmCacheMaxPinned` | 5 | — | 5 | 5 |

**A2 reproduces the control's cache behaviour exactly (38 / 150 / 143), which is
the proof that the falsifier flag reverts what it claims to revert** — the arms
are not one run relabelled.

The prediction of 90–130 hits came from "the class goes from 1 effective slot to
4" without dividing by the working set, and that is the error worth keeping. The
≤16,384 class holds **48 of the 59 played cues**. A depth-1 LRU over 48 items
and a depth-4 LRU over 48 items differ by `(4−1)/48 ≈ 6%` of that class's plays,
and 6% of ~150 class plays is **+9**. Measured **+9**. The model was right; the
prediction ignored it.

**This closes the FGM lane by arithmetic, not just the repartition.** Depth 1
gave 38 hits and depth 4 gave 47, so the returns are already flattening at
4 slots; even an implausible depth-16 partition extrapolates to ~60–65 hits,
i.e. ~123 misses against today's 150. At BAND_IO_OWNER §3's price of −12,736
`WORK-H` P95 for eliminating **all** 150,

| change | misses removed | P95-equivalent |
|---|---:|---:|
| this cycle's tie-break | 9 | **−764** |
| an aggressive repartition, extrapolated | ~27 | ~−2,300 |
| every miss eliminated (unreachable — 575,760 B working set) | 150 | −12,736 |

Every row is under the 8,544 measurement floor, let alone the 16,000 bar. **The
in-match FGM I/O lane is closed. Do not re-open it for ticks.** The one thing
that would change this arithmetic is the *unit price*: BAND_IO_OWNER measured
13,159 cycles per seek at 447 FAT-chain steps and noted the chain length tracks
the **ROM image**, not the 917 KB pack — so if `PROJECT_GOAL.md`'s ROM-for-speed
trade grows the image substantially, this lane's price per miss grows with it
and the sizing above must be re-derived rather than inherited.

### 3.6 The invariant pair — identical on all three arms

`gNdsBattleTextHudP0Damage` **0** · `P1Damage` **58** · `P0Stock` **1** ·
`P1Stock` **1** · `gNdsStarKOSparkleCount` **0** · `gNdsDamageSparkScaleCount`
**14** · `gNdsShieldAnimJointAttachCount` **1,344** ·
`gNdsAObjEvent32NormalizedHighWater` **1,177** · `NormalizeFailCount` **0** ·
`NormalizeReuseCount` **1,574** · `NormalizeCommandCount` **1,177** ·
`gNdsAObjEvent32HashHitCount` **1,574** · `HashMissCount` **1,371** ·
`gNdsObjAnimRunawayCount` **0** · `gNdsTaskmanGeneralHeapFreeMin` **70,592** ·
`gNdsTaskmanArenaAllocFailCount` **0** · `gNdsRelocResolveMisalignCount` **0** ·
`gNdsAudioFgmPlayFailCount` / `ReadFailCount` / `PoolExhaustCount` **0 / 0 / 0**.

Byte-identical across A, B and A2, as a transparent cache must be: which slot
holds a cue cannot change which cue plays. **Zero divergence, and none was
budgeted** — unlike the collision mechanism, this one has no flip budget because
it has no decision.

---

## 5. (a) The collision ring — three corrections to cycle B's plan

Not wired. `NDS_R2_COLLISION_FIXED` still defaults to 0 and `--gc-sections`
still drops the whole object. What this cycle did was re-read the seam against
the current control ELF and the decomp source, and it found three things that
change the wiring cycle's work.

### 5.1 `gmCollisionTestSphere` does not return a boolean — it returns an ANGLE

Cycle A §6 and cycle B §5.5 both describe the sphere kernel as "`(p − t)·R^-1`,
divide by the same `center`, then `syVectorMag3D ≤ 1`, plus the quadratic sweep
for the non-transfer case". That is the **decision**. It is not the function's
output set. `gmcollision.c:780`'s swept branch also writes, for
`sphit_kind` 0 and 1:

```c
*p_angle = syVectorAngleDiff3D(&copysub, &sp58);      /* kinds 0 and 1 */
syVectorNormCross3D(&copysub, &sp58, argA);           /* kind 1 */
```

`*p_angle` is the shield hit's knockback angle and `argA` its normal — both
**continuous gameplay values**, both consumed by the shield path.

**This breaks the brief's invariant law for a full conversion.** The flip-budget
framing ("every flip inside the 0.0200 bound") is a statement about a *boolean*
disagreeing on a measure-zero set of near-tangent cases. A fixed-point rewrite
of the angle math perturbs `*p_angle` on **every** shield hit, and a flip count
cannot express that. There is nothing to count.

**The correction:** `gmCollisionTestSphere` is a seam **dependency**, not a
saving — it does not appear in cycle B's §3 sizing table at all, and its
1,196 Thumb bytes were counted as a text win, never as a tick win. So convert
its **transform only**: read the fixed frame through the already-proven
`ndsR2CfxWorldToLocal`, convert the three components back to f32, and run the
unchanged float body from there. The angle and cross-product arithmetic stay
bit-identical to the shipped ROM's, modulo the transform's own quantisation,
which the 0.0200 bound already covers. That deletes the largest piece of
unproven new arithmetic in the plan — cycle B called the sphere kernel "the only
genuinely new arithmetic left" — and it removes an unbounded gameplay risk.

### 5.2 The `ftGetStruct` stub hazard is narrower than cycle B stated

Cycle B §5 hazard 1: the stub (`reloc_backend_compat_shims.c:13852`) hands back a
`bzero`'d `FTParts` with `0x5/0x6/0x7` set and `vec_scale` 1.0, so its
`unk_dobjtrans_0x9C` is 64 zero bytes; reinterpreted, "`inv_scale` is zero too,
so the fixed `center` is `size` where the float one is `size + radius/1.0`".

That assumed `inv_scale` comes from the frame. It need not:
`ndsR2CfxTestRectangle` already takes `inv_scale` as a **separate argument**
(`src/port/nds_r2_collision_fixed.c:76`), and the decomp caller passes
`&parts->vec_scale` — which the stub sets to 1.0 — not the inverse matrix. So a
port-side ring that quantises `1/parts->vec_scale` for that argument reproduces
the stub's float behaviour exactly: both paths map every point to the origin
(zero matrix) and both compute `center = size + radius/1.0`. **No identity fill
and no reachability proof is needed** — only the discipline of taking
`inv_scale` from `vec_scale`, which is where the float path takes it from.

### 5.3 `func_ovl2_800EDE5C` must NOT be fused into the frame prepare

Cycle B §5 step 3 folds the row scales into the prepare step: "on
`unk_dobjtrans_0x7 == 0` … build the cofactor frame …; on `0x6 == 0`, write
`vec_scale` from the frame's row scales". `0x6` and `0x7` are **independent
latches**, and `func_ovl2_800EDE5C` reads `parts->mtx_translate` — the float
world matrix, which stays float — not the inverse. A joint can reach `EDE5C`
with `0x7` already 1 and `0x6` still 0; a prepare step gated on `0x7` would then
skip the `vec_scale` write entirely and leave it stale.

`EDE5C` therefore needs no frame at all and can keep its own latch, or be
converted independently with the fixed-sqrt kernel. Its 6,498 tk/frame is
**separable from the seam**, which also means it can be taken without the ring.

### 5.4 The reader oracle, re-run on the current control ELF

Cycle A's §6 claim that `unk_dobjtrans_0x9C` is read by `gmcollision.c` only is
re-verified against `build-c147-ctl`, not inherited:

| symbol | in `build-c147-ctl`? |
|---|---|
| `sNdsFighterPartsPool` / `ndsFighterPartsSyncDObj` / `ndsFighterPartsSetIdentity` | **absent** |
| `ndsGMCollisionTestRectangle` (diagnostic recorders, three call sites) | **absent** |
| `battleship_gmcollision.c:258` | inside the `NDS_R2_COLLISION_L7_ORACLE` block, off |

**And a search trap worth keeping:** `grep -rn --no-ignore 'unk_dobjtrans_0x9C'
. --include=*.c` returns **nothing** in this tree, while the same pattern against
explicit paths returns twenty hits including a *writer* in
`src/port/reloc_backend_fighter_model.c`. A `--include` sweep from the repo root
silently misses the very readers the seam depends on. Name the paths.

---

## 4. The bank, and the tree that ships

**The gate does not move.** No mechanism survived to be banked.

| | raw P95 | net of 24,947 apparatus |
|---|---:|---:|
| banked before this cycle (`build-c144-ledgeridx`) | 1,210,944 | ≈1,185,997 |
| **this cycle's control on current HEAD** (`build-c147-ctl`) | **1,210,944** | **≈1,185,997** |
| gate | 1,120,380 | 1,120,380 |
| **gap — unchanged** | **+90,564** | **+65,617** |

P50 **924,864**. The control on current HEAD reproduces the banked figure to the
tick, on a ROM whose SHA-256 differs from `build-c144-ledgeridx`'s — so the bank
is confirmed on the current tree rather than inherited.

**What is committed changes no instruction.** The tree after the revert differs
from `build-c147-ctl` only in C comments, and `build-c151-final` proves it:
`.text`, `.data` and `.rodata` are **byte-identical** (981,588 / 148,288 /
1,471,976), and the 14 differing bytes in the packaged `.nds` are NitroFS
directory-entry ORDER, not code — the same three offsets appear in both, permuted.
`arm-none-eabi-objcopy --only-section` on each of the three sections reports
IDENTICAL. Boundary was therefore not re-run: it would execute the same bytes
arm A already ran clean (`slips=0`, every invariant in §3.6 nominal). Anything
that *did* change an instruction would not have this proof and must be verified
normally.

**A NitroFS packaging nondeterminism is a finding in its own right.** Two builds
of identical source into different directories produced ROMs differing by 14
bytes of directory order. The ledger-index cycle's "the build is
byte-reproducible … identical ROM SHA-256" holds for the *executable* and is the
claim that matters for A/B/A, but a bare ROM-hash comparison across build
directories can report a false difference. Compare sections, not the `.nds`.

---

## 6. What this cycle did NOT do

- **Did not wire the collision ring**, did not write the sphere kernel, did not
  trim `NDSR2CfxFrame` to 60 bytes, did not add the `#define` renames, did not
  run a map-absence check on a wired build and did not run the differential
  match. §5 is design, not measurement, except for §5.4 which is an ELF read.
- **Did not repartition the FGM cache** — §3.3 states why, with the class table
  a later cycle needs; and §3.5 prices the repartition at ~−2,300 anyway.
- **Did not ship the FindPlanned index** — §2 refuted its premise.
- **Did not ship the FGM tie-break** — §3.4 reverted it on its own falsifier.
- **Did not run the five-minute qualification or Boundary.** Both are gated on a
  KEEP, and nothing was kept; §4 proves the committed tree's `.text` is the
  arm-A `.text`, which is what a verifier would have re-executed.
- **Did not touch `decomp/`.**

## 7. What the next cycle inherits

1. **The collision ring is the only briefed mechanism with size left**, and §5 is
   its corrected plan. Take §5.1 seriously before writing the sphere kernel: it
   is the difference between a bounded change and an unbounded one.
2. **Two lanes closed by measurement**: in-match FGM I/O (§3.5) and
   `FindPlanned` (§2). Neither should be re-briefed.
3. **The placement floor on this ROM is worse than the quoted ±5,376.** Three
   near-identical arms spanned **16,832** of `WORK-H` P95 here, and the
   diagnostic arm spanned 17,920 against the control. Any future change under
   ~17,000 needs its flag falsifier to mean anything at all — a two-build
   comparison at this scale measures the linker.
4. **`gNdsAudioFgmCacheNoSlotCount` does not exist in the shipped tree** (it was
   reverted with the rest). A repartition cycle must re-add it FIRST: today
   "no slot could hold this cue" is folded into `gNdsAudioFgmReadFailCount` and
   a dropped sound is invisible.
