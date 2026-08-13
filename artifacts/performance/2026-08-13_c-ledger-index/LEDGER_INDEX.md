# The shield attach path pays for a linear search, not for its work — 2026-08-13

**KEEP.** An O(1) index over the existing AObj event-32 normalized ledger takes
the gate arm's `WORK-H` P95 from **1,250,368 to 1,210,944 (−39,424)** with P50
flat at −320, on byte-identical gameplay. That claws back **80% of the +49,216**
the 2026-08-13 shield anim-joint fix (`607d3697455` / `cffcea495a6`) added.
Three arms, one falsifier, 12,667 oracle-checked lookups with zero mismatches.

---

## 0. Phase 0 — the price, before any build was spent

Nothing in this section cost a build or an emulator run. It comes off two sets
of artifacts already on disk.

### 0.1 The fix's cost, per attach — the one-variable five-minute pair

`../2026-08-13_c-animjoint-fix/ctl5-c134-rows.csv` (stub + audit) and
`cand5-c135-rows.csv` (fix + audit) are the same 8,448-sample five-minute
both-CPU match with exactly one variable. Summing `WORK-H` over every sampled
frame gives the total work the fix added:

| | control5 | candidate5 | delta |
|---|---:|---:|---:|
| `WORK-H` summed over 8,448 frames | 8,151,908,864 | 8,198,806,208 | **+46,897,344** |
| mean per frame | 964,951 | 970,503 | +5,551 |

Against that arm's `gNdsShieldAnimJointAttachCount` of **9,154**:

```text
46,897,344 / 9,154 attaches      = 5,123 ticks per attach
46,897,344 /   542 install calls = 86,527 ticks per ftCommonGuardInitJoints
```

**A per-frame diff of the two arms is worthless and was tried first.** Frame
index N is a different game tick on each arm (the VBlank histograms differ), so
the per-frame delta distribution runs from −1,057,792 to +4,202,432 on a change
whose real mean is +5,551. Only the SUM over the window is meaningful. Recorded
so the next cycle does not repeat it.

### 0.2 Where the 5,123 goes — the c123 per-PC profile

`../2026-08-13_c-flagsweep/c123-pc-cycles.csv`, `build-c123-profile`,
`BOTH_CPU 1`, `NDS_TICK_HUD_DRAW 0`, `regions=1601`, so `ticks = cycles / 2`
(`../2026-08-13_c-residue/RESIDUE.md` §0). This profile PREDATES the fix — the
shield attach path is absent from it, which is itself the check: entry-PC
instruction count for `lbCommonAddDObjAnimJointAll` is **80 calls executing one
instruction each**, i.e. the `bx lr` stub, on an arm whose
`gNdsShieldAnimJointInstallCalls` is also 80.

Entry-PC counts give exact call counts for free:

| symbol | calls | self cycles | cyc/call |
|---|---:|---:|---:|
| `ndsAObjEvent32NormalizeScript` | 342 | 5,035,795 | 14,724 |
| `ndsRelocResolvePointerFromFileBase` | 8,282 | 448,322 | 54.1 |
| `ndsRelocFindKnownFileContaining` | 5,926 | 241,336 | 40.7 |
| `ndsRelocFindLoadedFileContaining` | 24,123 | 5,174,713 | 214.5 |
| `ndsRelocPointerRangeInLoadedFile` | 29,703 | 2,091,451 | 70.4 |
| `gcAddDObjAnimJoint` | 5,952 | 2,567,371 | 431 |

**95.0% of `ndsAObjEvent32NormalizeScript` is two inlined pointer scans**, and
the per-PC rows show them as two seven-instruction loops:

| loop | PCs | iterations | cycles | cyc/iter | **tk/iter** |
|---|---|---:|---:|---:|---:|
| `ndsAObjEvent32FindNormalized` | `0x02065c34`–`0x02065c40` | 165,034 | 2,654,780 | 16.09 | **8.05** |
| `ndsAObjEvent32FindPlanned` | `0x02065cb6`–`0x02065cc2` | 161,203 | 2,129,657 | 13.21 | 6.61 |

At 8.05 ticks per iteration, 5,123 ticks is **~636 scan iterations per attach**
— consistent with a ledger that reaches 1,177 entries in a minute and 1,598 in
five, scanned from index 0 every time. The other two lookups on the path price
out at ~200 ticks (resolve subtree) and ~174 (`ndsRelocPointerIsFighterAObj16`),
i.e. **under 8% together**. The scan is the cost.

### 0.3 The honest P95 prediction, and it was right

The fix put ~86,527 ticks on ~80 install frames of 1,600 and moved P95 by
+49,216. Reversing the same perturbation on the same frames, to first order,
predicts `−f × 49,216` for a change that removes fraction `f` of it. With
`f ≈ 0.85`: **−41,800 predicted, −39,424 measured** (5.7% under).

---

## 1. What shipped, and why it is not a new cache

`src/import/battleship_sys_objanim.c`. `ndsAObjEvent32FindNormalized` was a
linear scan of `sNdsAObjEvent32Normalized[]`; it is now an open-addressed probe
of a 4,096-slot index over that same array.

**SwitchPlan §3.12 bans keying anything on a pointer that survives a scene
boundary. This introduces no such key.** The ledger is *already* keyed on the
command pointer. The index holds nothing but positions inside it — same array,
same contents, same single discard point (`ndsAObjEvent32ResetNormalizedScripts`,
called only from `ndsRelocResetLoadedFiles`), cleared in the same breath as the
count it indexes. There is no second lifetime to get wrong and no state that can
disagree with the ledger, which is precisely what all five §3.12 incidents had.
The ledger's own `script->u == native_word` re-check is untouched, so the
rejection semantics are unchanged.

| checklist item | this change |
|---|---|
| stable identity | no new key at all; the ledger's existing key, reached faster |
| storage | `u16[4096]` = **8,192 B** fixed bss, compile-time sized, three `_Static_assert`s |
| reset seam | `ndsAObjEvent32ResetNormalizedScripts`, the ledger's only correct one |
| bit-identical | unique keys ⇒ probe returns the scan's index; proven, §3 |
| ledger interplay | insert path untouched; a hit takes the existing reuse path and inserts nothing |

**Slots store `index + 1` so a zeroed `.bss` reads as EMPTY.** That is not
style: the first normalize can precede the first `ndsRelocResetLoadedFiles`, and
with a `0xffff` sentinel an unreset table would be 4,096 occupied slots matching
nothing — a probe that never terminates, i.e. §3.11's freeze class in the one
subsystem whose failure mode is already a freeze. Both loops are additionally
bounded by the slot count, and the lookup falls back to the scan it replaced if
that bound is ever reached (`gNdsAObjEvent32HashOverflowCount`, 0 on every arm).

**Not taken, and sized:** `ndsRelocResolvePointerFromFileBase` (2,144 calls/min)
is ~1.83M of the 46.9M — **3.9%**, ≈ −1,900 P95 equivalent — and memoising it
would need a genuinely new cache with a new lifetime. `ndsAObjEvent32FindPlanned`
is a second O(n²) scan (1,064,828 tk/match on the c123 profile, ~665 tk/frame)
but the shield path never reaches it: every shield attach is a hit at the top of
`NormalizeScript` and returns before `PlanStream` runs. Both are named, not done.

RAM: `check-boot-headroom.ps1 -Build build-c145-cand2` → `fake_heap_start`
`0x0226d904`, **159,488 bytes of proven headroom** (was 167,936; the index costs
exactly its 8,192 plus 24 bytes of counters).

---

## 2. Arms

Repo-local `emulators/melonds/melonDS.exe` (`sha=DE80E46BDCF1FD98`), DLDI **ON**,
`NDS_TICK_HUD_DRAW 1`, `NDS_R2_BOTH_CPU 1`, mode 163 one-minute, `-Samples 1600
-RingDump -AllowRepeatedFrames -NoBuild`, frames 439–2038, `slips=0` on all
three.

| arm | build | source | ROM SHA-256 (12) | text / bss |
|---|---|---|---|---|
| **A** control | `build-c144-ctl` | HEAD `e3944e4126e` | `BB4A034F71F8` | 981,340 / 1,463,752 |
| **B** candidate | `build-c144-ledgeridx` | + index | `B736232B8CBF` | 981,588 / 1,471,976 |
| **A2** falsifier | `build-c145-noidx` | index built, lookup reverted | `346DC2DFEF37` | 981,452 / **1,471,976** |

**A2 had to be a flag, not a rebuild.** `build-c145-ctl2` is HEAD rebuilt into a
fresh directory and its ROM SHA-256 is **byte-identical to `build-c144-ctl`** —
the build is reproducible and the sampler is bit-deterministic, so a repeated
control cannot bracket anything. `NDS_AOBJ_EVENT32_LEDGER_INDEX=0` builds the
index, its counters and its 8,192 bytes of bss exactly as the shipping arm does
and reverts only the lookup: **the candidate's placement with the control's
behaviour**, which is the only arm that can separate the two.

`build-c145-cand2` is the candidate source *with the falsifier flag added*, and
its ROM is byte-identical to `build-c144-ledgeridx` — so adding the flag changed
nothing in the shipping arm and arm B's run stands against the committed source.

---

## 3. Result

| arm | `WORK-H` P50 | `WORK-H` P95 | `ALL` P95 | VBI 2/3/4/5+ max |
|---|---:|---:|---:|---|
| A control | 925,184 | 1,250,368 | 1,678,656 | 1700/310/15/13 max 26 |
| **B candidate** | **924,864** | **1,210,944** | 1,678,720 | **1740/272/13/13 max 26** |
| A2 falsifier | 921,728 | 1,253,120 | 1,678,592 | 1700/306/18/14 max 26 |

```text
B - A   = -39,424 P95   (-320 P50)
B - A2  = -42,176 P95
A2 - A  =  +2,752 P95   <- the placement floor, MEASURED on this pair
```

**A2 brackets A to 2,752 across 8,224 bytes of bss and 112 bytes of text.** The
win is **14–15x that floor**. It is not placement.

Lane signature — the saving lands where the fix's cost landed, and nowhere else:

| lane | B − A P95 | A2 − A P95 |
|---|---:|---:|
| `SINT` (fighter interrupt proc, where `ftCommonGuardInitJoints` runs) | **−23,936** | −576 |
| `SRC` | **−19,648** | +5,248 |
| `GCRA` | **−19,584** | +5,440 |
| `FTR` | −384 | −192 |
| `STG` | +2,816 | −448 |
| `SHDT` | −64 | −640 |

The fix charged `SRC` +21,760 when it landed; the index gives back 19,648 of it.
**P50 flat at −320 while P95 moves −39,424** is the exact inverse of the fix's
own signature (+1,536 P50 / +49,216 P95): work that clusters on the ~80 install
frames, removed from the same frames. **40 frames move from 3 VBlanks to 2.**

### Engagement — predicted before the runs, then read

| counter | prediction | A | **B** | A2 |
|---|---|---:|---:|---:|
| `gNdsAObjEvent32HashHitCount` | ≈ ReuseCount | — | **1,574** | — |
| `gNdsAObjEvent32HashMissCount` | 1,600 ± 400 | — | **1,371** | — |
| probes / lookup | 1.15–1.35 | — | **1.227** (3,613/2,945) | — |
| `gNdsAObjEvent32HashOverflowCount` | 0 | — | **0** | 0 |

`HashHitCount` equals `gNdsAObjEvent32NormalizeReuseCount` **exactly** on both
lengths — every index hit is a ledger reuse, which is the engagement proof from
the consuming side. On A2 the three lookup counters have no compiled writer and
`--gc-sections` drops them; the sampler refuses the run by name rather than
reading 0, which is the 2026-08-13 lesson working as intended.

### The invariant pair — identical on all three arms

`gNdsShieldAnimJointInstallCalls` **80** · `AttachCount` **1,344** ·
`NullCount` **800** · `gNdsAObjEvent32NormalizedHighWater` **1,177** ·
`NormalizeFailCount` **0** · `NormalizeReuseCount` **1,574** ·
`NormalizeScriptCount` **183** · `NormalizeCommandCount` **1,177** ·
`gNdsObjAnimRunawayCount` **0** · `gNdsRelocResolveMisalignCount` **0** ·
`gNdsTaskmanGeneralHeapFreeMin` **70,592** · `ArenaAllocFailCount` **0**.

Every one byte-identical across A, B and A2. An exact index cannot alter
gameplay, and it did not.

---

## 4. Qualification — five minutes, oracle armed

`build-c146-oracle5` (`NDS_R2_BOTH_CPU 1`, `NDS_R2_SOAK_MATCH_MINUTES 5`,
`NDS_AOBJ_EVENT32_HASH_ORACLE 1`), 8,448 samples, frames 439–8886, `slips=0`.
**This build is a correctness arm and can never be a gate figure** — the oracle
restores the entire linear scan on every lookup, which is why its `WORK-H` P95
reads 1,246,336, i.e. back at the un-indexed cost. That agreement is itself a
third confirmation that the scan was the cost.

| | value | required |
|---|---:|---|
| `gNdsAObjEvent32HashOracleRuns` | **12,667** | non-zero — the oracle is armed |
| `gNdsAObjEvent32HashOracleMismatch` | **0** | 0 |
| `gNdsAObjEvent32HashOverflowCount` | **0** | 0 |
| `gNdsObjAnimRunawayCount` / Mask | **0 / 0** | 0 |
| `gNdsAObjEvent32NormalizeFailCount` | **0** | 0 |
| `gNdsTaskmanGeneralHeapFreeMin` | **70,000** | ≥ 32,768 |
| `gNdsTaskmanArenaAllocFailCount` | **0** | 0 |
| `gNdsRelocResolveMisalignCount` | **0** | 0 |

The oracle runs the replaced linear scan beside every probe and compares the two
answers: **12,667 paired lookups, zero disagreements.** That is the exactness
claim measured rather than argued — the uniqueness of the ledger's keys is a
property of the *insert* path, so a lookup-side proof by inspection would not
have covered it.

Against `cand5-c135` (the fix, same arm, same length) every state counter is
byte-identical: `InstallCalls` 542, `AttachCount` 9,154, `NullCount` 5,420,
`NormalizedHighWater` 1,598, `NormalizeFailCount` 0, `RunawayCount` 0,
`RelocResolveMisalignCount` 0, heap free-min 70,000, `ArenaAllocFailCount` 0.
Index engagement over five minutes: hits **10,794** (= `ReuseCount` 10,794),
misses 1,873, **1.515 probes per lookup**, 1.830 per insert.

### Ledger margin, re-read — and the brief's expectation was wrong

`gNdsAObjEvent32NormalizedHighWater` is **1,598 of 2,048**, margin **450 spare,
1.28x** — *unchanged*. The index was expected to reduce fresh normalizes; it
cannot, and the reason is worth keeping: the ledger already deduplicated them.
`ReuseCount` was 1,574 in a minute and 10,794 in five *before* this change. The
scripts were never being re-normalized — they were being re-**found**, one
linear scan at a time. Capacity is therefore still the open question cycle 13
left, at exactly the number it left it, and nothing here moves it.

---

## 5. Verifier and ROMs

`scripts/verify-boundary.ps1 -DelaySeconds 3 -RunnerSlot 2` — see `boundary.log`
beside this file, scanned in full for `Exception:` and `=FAIL`. Boundary is the
right width: the change is battle-only and touches no normal or shared startup
path.

**Root ROMs byte-identical across the whole cycle**, hashed before the first
build and after the last:

```text
smash64ds.nds                       54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a
smash64ds-battle-playable-hwtri.nds 524448c99c31b62672a63f29914438059d5f9700e10306d147d6342b3223adee
```

No published target was built; every build here is a lab build.

---

## 6. The bank

| | raw P95 | net of 24,947 apparatus |
|---|---:|---:|
| banked before this cycle (`build-c136-animjoint`) | 1,260,096 | ≈1,235,149 |
| this cycle's control on current HEAD (`build-c144-ctl`) | 1,250,368 | ≈1,225,421 |
| **candidate — new bank** | **1,210,944** | **≈1,185,997** |
| gate | 1,120,380 | 1,120,380 |
| **gap** | **+90,564** | **+65,617** |

P50 **924,864**. The 2026-08-13 anim-joint fix's +49,216 now costs **+9,792**
net of this change, and the correctness it bought is untouched: 144 silent
animation misreads and 50 dropped-joint faults per five minutes, still zero.
