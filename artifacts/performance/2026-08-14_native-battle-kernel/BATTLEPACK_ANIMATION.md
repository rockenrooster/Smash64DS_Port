# The animation I/O is 15 file reads; the cost is 299 cache HITS

**Date:** 2026-08-14 · **Branch:** `codex/r2-runtime2` · **HEAD `830c13bf809`** (Boundary green).
**Native Battle Kernel slice 1, phases 1–4.** Design: `docs/architecture/RUNTIME2_NATIVE_BATTLE_KERNEL.md`.
**Builds spent: 0.** Phase 1 was answered from the v3 gate-arm capture already on
disk, at higher resolution than the briefed counter build would have given (§1.2).
**UNITS: 2 profile cycles = 1 project tick.** Every table states its window.

---

## 0. Outcome first

1. **In-match animation FAT I/O is 15 file loads in a 1,600-frame match**, on 13
   frames, all 13 inside the 80 that set P95. **Deleting those 13 frames entirely
   moves the 80th-largest frame by 9,874** against a 64,452 requirement. The FAT
   read is not the lever. §2, §3.
2. **The lever is the acquisition path itself: 299 acquisitions, 95.0% of them
   cache HITS**, on **62 of the 80 P95 frames** against **174 of 1,520** body
   frames — **6.8x presence**. A cache hit still copies the whole payload,
   re-registers a loaded file, re-runs finalization, re-normalizes the AObj16
   script region, strips alias status nodes and writes three status entries. §3.
3. **Priced by dose-response**: one cache-hit acquisition costs **+148,969** mean
   ticks over a no-acquisition frame, a second **+77,440** more, a FAT miss
   **+645,225**. Modelled full deletion moves rank-80 by **73,659**. §3.2.
4. **`get_fat`/`f_lseek` are majority BGM, not animation** — at most **38.8%** of
   the P95-set `get_fat` sits on an animation-load frame. This corrects
   `GATE_ARM_OWNERS.md` §5.2's mechanism composition. §2.3.
5. **The complete reachable set is all 301 clips**, from `ftdata.c` — not the
   87-entry warm list, which is one match's observation. §4.
6. **The complete pack is 651,928 B against ~511,904 B of proven RAM**: it does
   **not** fit, and no lossless compaction closes it (dead tails 1.0%, exact
   dedup 4.6%, substring merge 0.004%). §6.
7. **Host equivalence: mismatch = 0**, 297 clips / 5,629 scripts / 77,959
   commands / 71,500 per-track states / 5,629 event callbacks / 105,304 target
   words. Corpus `456834182c5d6f4e26bac43509ea04b066c55f5e2f811fad603ed82798e9b7ea`.
   Two falsifiers demonstrate the test can fail. §7.

---

## 1. Method

### 1.1 The arm

```text
profile     builds/build-c159-profile-bothcpu   NDS_R2_BOTH_CPU=1  NDS_TICK_HUD_DRAW=0
            NDS_TASK37_PROFILE=1, DLDI on, presented frames 438..2038
capture     artifacts/performance/2026-08-14_runtime2-p95-closure/v3-gate-arm/
            arm9-profile.csv, 3.88 GB, 54,564,874 PC rows, 1,601 regions
mask        region 0 excluded (pre-window accumulator); the 80 regions with the
            largest total_cycles - halt_wait   -> threshold 1,174,997 ticks
```

**Every rank-80 figure in this document is the profile arm's, not the tick-HUD
bank arm's.** The bank is `build-c158-gate` (`DRAW=1`) at `WORK-H` rank-80
**1,184,832**; the profile arm's own non-idle rank-80 is **1,174,997**, 0.8%
apart on two different binaries and two different instruments
(`GATE_ARM_OWNERS.md` §4.1). Nothing is transplanted between them.

### 1.2 Why no counter build was spent

The brief and `plan.md` §13 item 2 asked for a per-frame asset-acquisition
counter on the gate arm, to establish whether acquisitions concentrate on the 80
P95 frames. **The v3 capture already answers that, exactly and at finer
resolution.** The profiler is instruction-accurate, so the instruction count at a
function's *entry PC* is exactly its call count (memory:
`entry-PC-gives-exact-call-counts`), and the CSV carries it **per region** —
i.e. per presented frame, for all 1,600 frames, for every function at once.

A counter build would have produced the same per-frame series for a hand-picked
subset of sites, at the cost of one build and one run, on a *different* binary
from the one the P95 set was defined on. `scripts/census-profile-pc-per-region.py`
is that measurement and is committed so it costs nothing again.

**What this modality cannot see**, stated so nobody over-reads it: a function
inlined into its caller has no entry PC — `ndsRelocForceLoadFighterAObj16File`,
`ndsR2AnimWarmLoadOne`, `ndsRelocFindLoadedFileByData` and
`ndsRelocEnsureLoadedAsset` are all absent from the symbol table for that reason,
and the tool reports them as ABSENT rather than as zero calls. It also gives
counts, never bytes; §3.3 says what is still unmeasured because of that.

---

## 2. Phase 1 — the counters, intersected with the 80 frames that set P95

### 2.1 The animation acquisition path

Whole match = 1,600 regions. "presence" = calls per P95 frame ÷ calls per body
frame. Source `io-pc-per-region.csv`.

| function | calls, match | on the 80 | per P95 frame | per body frame | presence | P95 frames touched |
|---|---:|---:|---:|---:|---:|---:|
| `lbRelocGetForceExternHeapFile` (acquisition entry) | **299** | 89 | 1.113 | 0.138 | **8.05x** | **62 of 80** |
| `ndsR2AnimCacheFind` | 299 | 89 | 1.113 | 0.138 | 8.05x | 62 |
| `ndsRelocRegisterLoadedFile` | 299 | 89 | 1.113 | 0.138 | 8.05x | 62 |
| `ndsRelocFinalizeLoadedFile` | 299 | 89 | 1.113 | 0.138 | 8.05x | 62 |
| `ndsRelocRemoveFighterAObj16LoadedAliases.part.0` | 299 | 89 | 1.113 | 0.138 | 8.05x | 62 |
| `ndsRelocNormalizeFighterAObj16File` | **314** | 104 | 1.300 | 0.138 | 9.41x | 62 |
| `ndsRelocApplyInternalPointerFixups` | 314 | 104 | 1.300 | 0.138 | 9.41x | 62 |
| `ndsRelocAssetFindEntry` | 314 | 104 | 1.300 | 0.138 | 9.41x | 62 |
| `sniprintf` (NitroFS path build) | 314 | 104 | 1.300 | 0.138 | 9.41x | 62 |
| `ndsRelocAssetIDForToken` | **1,197** | 357 | 4.463 | 0.553 | 8.08x | 62 |
| `ndsRelocAddStatusNode.part.0` | 885 | 261 | 3.263 | 0.411 | 7.95x | 60 |
| **`ndsRelocAssetLoadIntoZeroedHeap`** (the FAT load) | **15** | **15** | 0.188 | **0** | inf | **13 of 80** |
| `ndsRelocAssetReadHeaderFromFile` | 15 | 15 | 0.188 | 0 | inf | 13 |
| `ndsRelocApplyWordByteSwap` | 15 | 15 | 0.188 | 0 | inf | 13 |
| `fopen` / `_fopen_r` / `fclose` | 15 | 15 | 0.188 | 0 | inf | 13 |
| `nitroromResolvePath` | 15 | 15 | 0.188 | 0 | inf | 13 |
| `strncasecmp` (directory walk) | 947 | 947 | 11.84 | 0 | inf | 13 |
| `nitroromReadIter` | 2,224 | 2,224 | 27.8 | 0 | inf | 13 |

**299 acquisitions, 15 of which missed the cache: a 95.0% hit rate.** The
counters the brief asked for, derived:

```text
asset acquisitions requested per frame   299 / 1,600 = 0.187 mean
                                         1.113 on a P95 frame, 0.138 on a body frame
cache hits / misses                      284 / 15        (hit rate 95.0%)
get_fat / f_lseek / f_read call counts   106,276 / 248 / 5,053   (whole match, ALL consumers)
fopen (NitroFS open = token->file discovery)   15
AObj16 normalizations                    314
ndsRelocFinalizeLoadedFile executions    299
```

`ndsRelocNormalizeFighterAObj16File` runs **314** times against 299 acquisitions:
the extra 15 are the miss path, which normalizes after the load as well as
through the register/finalize sequence the hit path shares.

### 2.2 The distribution, which is the point

```text
acquisitions per frame, the 80 P95 frames : 0 -> 18   1 -> 39   2 -> 20   3 -> 2   4 -> 1
acquisitions per frame, the 1,520 others  : 0 -> 1346  1 -> 139  2 -> 34   3 -> 1
```

**62 of 80 (77.5%) against 174 of 1,520 (11.4%).** This is the shape
`MARGINAL_OWNERS.md` §7 law 1 demands, and the prediction `GATE_ARM_OWNERS.md`
§5.3 wrote down first — "~0 on the two-VBlank frames and non-zero on most of the
80" — is confirmed for the acquisition path and **refuted for the FAT read**,
which touches only 13.

### 2.3 The FAT storm is majority BGM — a correction

`GATE_ARM_OWNERS.md` §5.2 put `get_fat` (+15,058, 10.7x) and `f_lseek` (+9,509,
10.8x) inside the "in-match asset load / file I/O" mechanism. They are file I/O,
but they are **mostly not animation**:

```text
get_fat, whole match                          106,276 calls
  on the 13 animation-load frames              22,178   (20.9%)
  within +/-2 regions of a BGM packet read     56,596   (53.3%)
  on neither, over 63 further regions          33,421   (31.4%)
get_fat on the 80 P95 frames                   57,106
  of which on an animation-load frame          22,178   (38.8%)
```

The driver is arithmetic, not inference: `get_fat / f_lseek` = **428.5** calls
per seek whole match and **391** on the P95 set, i.e. `f_lseek` walks the
cluster chain of the SD-card `.nds` from the start on every seek, ~400 clusters
deep. `f_read` alone cannot produce that ratio. Of the 248 seeks,
`_nitroFS_seek_r` owns 146 and the BGM reader (`ndsAudioBgmReadExact`, 154 calls
over 77 packets) owns most of the rest.

**So slice 1 deletes at most 38.8% of the P95-set `get_fat` and none of the BGM
half.** Sizing slice 1 from the full +15,058 would over-predict by ~2.6x. The
BGM half belongs to slice 48's lane (`docs/HANDOFF.md`: "the FAT lane is BGM"),
which is closed for its own reasons and is not re-opened here.

### 2.4 The free observation — `gNdsRelocAssetOpenFailCount`

`artifacts/verification/2026-08-14_boundary-red/BOUNDARY_RED.md` §6 recorded that
this counter reads ≈1,385–1,402 at `scVSBattleStartBattle` on a *healthy* run,
and that it is `ndsRelocAddStatusNode` (`reloc_backend_assets.c`:2536) declining
a full `LBFileNode` status buffer rather than a file-open failure.

**Confirmed for the in-match window, and the counter is genuinely conflated.**
It has eleven writers: ten in `nds_reloc_assets.c` (a missing asset-table entry
or a failed `fopen`) and one in `reloc_backend_assets.c`:2536 (status buffer
full). In frames 438–2038 `fopen` is entered exactly **15** times and all 15
loads succeed — `ndsRelocApplyWordByteSwap` runs 15 times and only runs after a
successful load — so **no in-match increment can be a file-open failure**. Every
one is the status-node decline, over a population of 885 `ndsRelocAddStatusNode`
executions.

Two consequences, neither of them concluded here:

- **The counter should be split.** One name for "asset could not be opened" and
  another for "status buffer full" is a one-line change that stops a capacity
  signal reading as an I/O failure. Recorded as an item, not done in this cycle
  (out of scope, and it changes a shipped diagnostic).
- **Whether the full buffer costs anything is unmeasured.** A declined node means
  a later `lbRelocGetStatusBufferFile` for that token misses and allocates,
  which is a per-acquisition penalty — but `ndsRelocSetStatusBufferFile` updates
  in place for tokens that already have a node, so only *new* tokens can be hurt.
  The measurement is a gdb read of `sNdsRelocStatusBufferCount` against
  `sNdsRelocStatusBufferMax` on the existing ROM. **It bears directly on slice 1:
  a resident pack removes the status-node path for animations entirely, so this
  is one of the costs slice 1 would delete.**

---

## 3. What an acquisition costs

### 3.1 Dose-response

Per-region ticks against per-region acquisition count, whole population,
FAT-miss frames separated so cache hits are isolated. Source
`io-pc-series.csv`.

| population | n | mean ticks | median | marginal vs 0 |
|---|---:|---:|---:|---:|
| 0 acquisitions, no FAT load | 1,364 | 942,162 | 941,517 | — |
| 1 acquisition, cache hit | 169 | 1,091,131 | 1,074,199 | **+148,969** |
| 2 acquisitions, cache hits | 50 | 1,168,571 | 1,154,384 | +226,409 |
| 3 acquisitions, cache hits | 3 | 1,231,993 | 1,208,666 | +289,831 |
| frames carrying a FAT MISS | 13 | 1,587,387 | 1,534,452 | **+645,225** |

Fitting the first three rows as `fixed + per*n` gives **fixed 71,529** and
**per-acquisition 77,440** — the two-parameter form is not imposed, it falls out:
predicted 3-acquisition marginal 303,849 against a measured 289,831 (4.6% high on
n=3).

**This is a correlational dose-response, not an isolation.** A frame that
acquires an animation is a frame where a fighter changed action, which does other
work too. **Every figure here is therefore an UPPER bound on the acquisition's own
cost.** The `fixed` term is the honest home for whatever the action change costs
independently of the asset; the `per` term is the part that scales with the
number of assets pulled and is the closest thing to the acquisition itself.

### 3.2 The projection, and its ceiling

Ranking all 1,600 regions after subtracting the model, rank-80 on the profile arm:

| removed | new rank-80 | delta |
|---|---:|---:|
| baseline | 1,174,997 | — |
| the 13 FAT-load frames deleted **entirely** (their whole cost) | 1,165,123 | **−9,874** |
| the FAT-miss term only (`per`/`fixed` untouched) | 1,166,030 | −8,967 |
| the per-acquisition term only (77,440 × n) | 1,109,699 | −65,298 |
| the whole modelled acquisition path (`fixed` + `per` + miss) | **1,101,338** | **−73,659** |
| the 62 P95-set acquisition frames deleted **entirely** | 1,127,106 | −47,891 |

Frames over 1,120,380 fall from **151 to 66**.

**Read these as sizing, not as a result.** Rows 2 and 3 are hard ceilings — they
delete whole frames or a whole measured population and cannot be beaten. Rows 4
and 5 are model-driven and inherit §3.1's upper-bound character. The requirement
is 64,452 at the 80th-largest frame, and the honest statement is: **slice 1 is
the right size for the gate, and it is the only candidate measured on this arm
that is.** It is not yet a measured win, and no cycle may bank it as one until a
candidate ROM exists.

### 3.3 What is still unmeasured

**Bytes.** The profile gives call counts, never byte volume, so "bytes read from
ROM" and "bytes copied from cache into the status heap" are not measured here.
Two of the three are derivable and one is not:

- bytes read from ROM = the payload sizes of the 15 missed assets; needs the
  miss identities, which needs `gNdsR204AnimSeen` off a live run;
- bytes copied on the hit path = Σ payload size over 284 hits; needs the
  per-acquisition asset multiset, which **no existing counter records** —
  `gNdsR204AnimSeen` is a distinct-set bitmap;
- the corpus is exact and is on disk: **703,984 B of payload over 301 files**,
  mean 2,338.8, median 2,352, max 6,224 (`anim-asset-sizes.json`).

A single `gNdsR2AnimCacheHitBytes += cached->size` at
`reloc_backend_assets.c`:7207 closes the second one for a build. It is the one
counter this cycle would still add, and it is named for the next.

---

## 4. The complete reachable animation set

**All 301.** `decomp/BattleShip-main/decomp/src/ft/ftdata.c` — the fighter data
tables `ftMainSetStatus` drives — references **143 of 143** Mario and **158 of
158** Fox animation file-ID symbols. The same 301 are the complete `ll*Anim*FileID`
set across the whole `decomp/src/ft/` tree, so no status table anywhere adds one.

Modality: symbol reference from the status tables, and it cannot see (a) which
statuses a *particular* match can enter, or (b) an animation reached through an
indirection rather than a named symbol. It is therefore an **upper** bound on
reachability and a **lower** bound on nothing.

Against it, the 87-entry `sNdsR204AnimWarmList` is **28.9% of the reachable set**
and is not a set of reachable clips at all — it is one 60-second both-CPU match's
observation, and its own source comment records it drifting three times
(41 → 85 → 87 entries, each re-measured). **That is why assets get requested that
are absent from it: the list is observational by construction.** This match
missed 15 times against it.

The four `AObjEvent32` animations (`0x279` `0x27a` Mario Appear1/Appear2,
`0x309` `0x30a` Fox Appear/Arwing) are reachable but are not AObj16 and are
excluded from the pack by name, not silently.

---

## 5. The pack

Generated by `scripts/generate_battlepack_anim.py` from the **shipped** bank
`builds/build-c158-gate/nitrofs/reloc/reloc_animations` (301 files; the decomp
bank holds all twelve fighters at 1,633 files and the Mario/Fox members are
byte-identical).

```text
clips (AObj16)              297          scripts (entry points)   5,629
raw file bytes          709,952          raw payload bytes      686,192
  pointer table          30,180            script stream        656,012
pack header                  20          clip directory           3,564
pack offset table        22,516          pack stream            625,828
PACK TOTAL              651,928 B   =  95.0% of the raw payload
exact duplicate script runs shared: 512 (30,184 B saved)
sha256  4a066d49f726d1c176a84392d9977f5f23985daefbb194f7711f7000a83b0cea
```

Per-clip: mean 2,195 B, against a raw payload mean of 2,338 B.

**No further lossless compaction exists, and this was measured:**

| candidate | bytes | share |
|---|---:|---:|
| dead bytes past each script's terminator | 6,262 | 1.0% |
| exact duplicate script runs, **already taken by the pack** (512 runs) | 30,184 | 4.6% |
| the same dedup applied after trimming (515 runs) | 30,400 | 4.6% |
| substring merging on top of exact dedup | 28 | 0.004% |

Trimmed **and** deduped, the stream floor is 619,350 B and the pack floor is
**645,450 B**. The command stream is genuinely ~620 KB.

---

## 6. The RAM verdict — it does not fit

```text
proven static headroom (fake_heap_start 0x02260c24, 2026-08-14)     211,936
raw-file anim cache arena, reclaimed by a resident pack             262,144
gSYTaskmanGeneralHeap free-min 70,592 against the 32,768 floor       37,824
                                                                   --------
PROVEN AVAILABLE                                                   ~511,904
complete pack                                                       651,928   (floor 645,450)
SHORTFALL                                                          ~140,024   (~133,546 at the floor)
```

`PROJECT_GOAL.md` allows RAM to be spent aggressively and ROM more so, but
"stability is mandatory" and the GObj-cap law is measured, not theoretical: a
+14 KB bss growth once stopped the ROM booting, and text counts as much as bss.
**A 140 KB overrun is not a rounding error and must not be spent on hope.**

Per `plan.md` §K1 phase 3 the fallback to gameplay-time FAT loading is
**forbidden**, so three lawful moves remain. They are listed in
`RUNTIME2_NATIVE_BATTLE_KERNEL.md` §6, ranked by cost; the cheapest is a gdb read
of the per-status animation `syTaskmanMalloc` volume that a resident `const` clip
makes unnecessary — a quantity **not** in the 511,904 above because nobody has
measured it. The second is the items-off exclusion: 38 of the 301 animations are
item-flavoured, **101,472 payload bytes, 14.4%**, and items are off for P1 — but
that must be proven from the status graph rather than from symbol names before a
byte is dropped, and it is recorded here as a *lead*, not as a saving.

> `BLOCKED(decision: none required yet.)` This is a measurement gap, not a
> fidelity trade. It becomes an owner question only if all three moves fail and
> the choice narrows to spending the RAM anyway or dropping clips.

---

## 7. Phase 4 — host equivalence, mismatch = 0

Both sides are **decoded and executed**, never compared as storage — comparing
bytes would be a tautology, since the pack's stream is the file's script region.
The oracle is slice 32's proven host model (`ftanim_reloc_probe.py` +
`ftanim_script_model.py`), which reproduces the ROM's pipeline and walked 100.0%
of the bank when it was written.

```text
clips decoded          297        scripts compared       5,629
commands compared   77,959        per-track states      71,500
event callbacks      5,629        target words         105,304
corpus hash   456834182c5d6f4e26bac43509ea04b066c55f5e2f811fad603ed82798e9b7ea
MISMATCHES               0        BATTLEPACK_ANIM_EQUIVALENCE=PASS
```

Compared per script: **command stream** (opcode, flags, payload bits, every
per-track target word, in order, with jumps rebased) and **executed semantics**
(per-track state after every command — kind, value_base, value_target, rate_base,
rate_target, length, length_invert, interpolate — plus the wait timeline, the
final `anim_wait`, the DObj flag writes, and the callback/event ordering with its
tags). Duration, interpolation mode, joint/channel target, event ordering,
packed payload bits and end/loop behaviour are all inside that.

**One bug was found and it was in the comparator, not the pack**, and it is
recorded because the failure mode is instructive: the first run reported 5,629
mismatches, all "executed semantics" and none in the command stream, because
`run.callbacks` carries the **absolute PC** of the firing command and the two
sides sit at different base addresses by construction — that is the whole point
of a position-independent pack. Rebasing turns the check into a test of ordering
and tags rather than of storage addresses.

**The test can fail — proven, not asserted:**

| falsifier | result |
|---|---|
| one byte flipped at stream offset 40,000 | 1 mismatch, kind `undecodable` |
| one clip's first script offset shifted by one word | 1 mismatch, kind `command stream` |
| control: unmodified pack | **0 mismatches**, corpus `456834182c5d…` |

The first falsifier originally raised a `ValueError` out of the decoder. That is
detection, but a traceback can be mistaken for a broken tool, so `verify()` now
records an undecodable script as a counted mismatch. Structural, not documented.

---

## 8. Corrections this document makes to the record

1. **`GATE_ARM_OWNERS.md` §5.2/§5.3's "the match is reading animation assets off
   the FAT during gameplay" is true 15 times, not at the volume its symbol table
   implies.** `get_fat`, `f_lseek`, `armCopyMem32` and `memcpy` are in that
   mechanism's cost bucket; ≤38.8% of the P95-set `get_fat` and 89.5% of `f_read`
   sit on an animation-load frame, and the rest is BGM. §2.3.
2. **Its counter prediction is half-confirmed and half-refuted.** Acquisitions
   concentrate exactly as predicted (62 of 80 vs 174 of 1,520); the *file reads*
   touch 13 frames and cannot move P95 by more than 9,874. §2.2, §3.2.
3. **`plan.md` §K1 phase 1's "the current 85-entry warm list" is 87**, and the
   reachable set it is contrasted with is **all 301**, not a larger sample. §4.
4. **`BOUNDARY_RED.md` §6's free observation is confirmed for the in-match
   window** and the counter is shown to be conflated across eleven writers. §2.4.
5. **`plan.md` §3 item 9's newlib finding is corroborated on call counts**:
   `sniprintf` runs 314 times a match — once per `ndsRelocAssetFindEntry` on an
   animation id — and `_vsniprintf_r` 473. Reachable at `NDS_TICK_HUD=0`, as that
   item says. Slice 1 deletes the call site outright (K0: no token→file
   discovery), so the residual it was closed on stops existing rather than
   getting cheaper.

## 9. What this cycle did NOT do

- **No DS runtime path, no `NDSAnimInstance` in the ROM.** Phases 5–9 are
  untouched, as briefed.
- **No build.** §1.2 says why, and states what the modality cannot see. The two
  root ROMs are byte-identical to the start of the cycle (§10).
- **No byte volume measured** on the acquisition path. §3.3 names the one counter
  that closes it.
- **No RAM recovery attempted.** §6 sizes the shortfall and ranks the three moves;
  it does not take one.
- **The items-off exclusion is not proven**, only sized. §6.
- **Nothing is banked as a performance win.** §3.2 is a projection with its model
  stated, on the profile arm, and says so.
- **`gNdsRelocAssetOpenFailCount` was not split**, though it should be. §2.4.

## 10. State and reproduction

Root ROMs **unchanged** across the cycle:
`smash64ds.nds` `54c07fac80c50418949908701f7c2bdbf27512c5f96ac09086fabbb0df6ac68a`,
`smash64ds-battle-playable-hwtri.nds` `2015fbd1f68b81c03626d8c6d473c8bcbcf527a3a26dfe86ff19bd74ecbb1360`.

```powershell
# per-frame call counts for any set of symbols, out of a v3 capture (~2 min)
python scripts/census-profile-pc-per-region.py `
    --profile artifacts/performance/2026-08-14_runtime2-p95-closure/v3-gate-arm `
    --elf builds/build-c159-profile-bothcpu/smash64ds-battle-playable-tickhud-hwtri.elf `
    --symbols lbRelocGetForceExternHeapFile,ndsRelocAssetLoadIntoZeroedHeap,get_fat.isra.0 `
    --marginal 80 --out out.csv --series-out series.csv

# the pack, its sizing, and the equivalence proof (~4 min)
python scripts/generate_battlepack_anim.py `
    --bank builds/build-c158-gate/nitrofs/reloc/reloc_animations --verify `
    --json artifacts/performance/2026-08-14_native-battle-kernel/battlepack-sizing.json
```

Committed alongside: `io-pc-per-region.csv` / `-2.csv` (call counts),
`io-pc-series.csv` / `-2.csv` (per-region series, 1,600 rows — so no future cycle
re-scans 3.88 GB), `anim-asset-sizes.json` (per-asset payload sizes),
`anim-id-names.json` (asset-id ↔ symbol name), `battlepack-sizing.json`,
`battlepack-verify.log`.

---

# Cycle 2 (2026-08-14, later) — the RAM shortfall, measured; and phase 5's premise, corrected

**Builds spent: 0.** Root ROMs unchanged (§10 hashes still hold, re-verified). No
shipped byte changed; the only source edit is a host-side `--exclude-ids` option
on `scripts/generate_battlepack_anim.py`, which the Makefile never invokes.

## 11. Outcome — it still does not fit, and route 1 is 5-11x smaller than briefed

```text
                                   pack        pool        short
full pack (297 AObj16 clips)      651,928     511,904     140,024
items off (259 clips, PROVEN)     553,696     511,904      41,792
  + route 1, 2 figatree heaps     553,696     524,352      29,344
  + route 1, 4 (Sudden Death)     553,696     537,120      16,576
  + matchup lead (245, UNPROVEN)  528,624     524,352       4,272   (-8,496 at 4 heaps)
```

**DOES NOT FIT.** The one row that goes negative spends the entire pool to zero
reserve and half of its exclusion is unproven, so it is not a fit either —
`PROJECT_GOAL.md` requires "sufficient reserve … for reliable operation".

## 11.1 Three corrections to the ledger the brief inherited

**(a) The warm arena is already inside the 511,904; counting it again is a
double count.** `NDS_R2_ANIM_CACHE_ARENA_BYTES` is **262,144**
(`src/port/reloc_backend_assets.c:6377`) and that is the *reservation* §6's table
already banks. `docs/HANDOFF.md`'s slice-46 figure **192,240** is
`sNdsR2AnimCacheArenaUsed` — the bump high-water *inside* that reservation
(`:6470`, published as `gNdsR2AnimCacheArenaUsedBytes`). It is not a second pool
and it is not "1.37x the shortfall".

**(b) The three pool terms are not additive as written, and making them additive
is a coupled three-file change nobody has recorded.** `gSYTaskmanGeneralHeap` is
one `calloc` of `NDS_TASKMAN_ARENA_SIZE = 0x150000` = 1,376,256 B from the libnds
heap (`src/port/diagnostics.c:7750`, `:7791-7849`). The anim arena is a
`syTaskmanMalloc` *inside* it, so releasing it frees **no** libnds heap:

```text
pack in .rodata                 draws on the static headroom only        211,936
pack in the taskman arena       draws on arena-internal free only        299,968
                                  (262,144 reclaim + 37,824 slack)
the 511,904 total exists ONLY if NDS_TASKMAN_ARENA_SIZE is also cut by the
reclaimed arena -- which additionally needs the downward search's 0x130000 floor
lowered (diagnostics.c:7810) AND the Task 36 replay-admission guard taught the
new constant: it blocks unless gNdsTaskmanArenaChosenSize == 0x150000 (legacy) or
>= 0x130000 (NDS_TASK53_REPLAY_ARENA_FIX) -- src/nds/nds_renderer.c:5734-5739.
```

**(c) Route 1 is worth 12,608-25,216 B, not 140,024, and it needs no gdb read.**
The per-status `syTaskmanMalloc` animation volume is `gFTManagerFigatreeHeapSize`
per fighter, which `ftmanager.c:170-206` computes as the **largest single
animation file** over the loaded kinds — a compile-time max over the bank, not a
runtime quantity. The bank's max payload is **6,224 B** (`anim-asset-sizes.json`;
mean 2,338.8), so the heap is at most 6,304 B with the 0x50 O2R header.
`decomp/…/sc/sccommon/scvsbattle.c:199` allocates one per active player (2 in P1)
and `:472` one more per player on the Sudden Death entry, never freed — the
taskman heap is a bump region with no `free`. So 12,608 for the match, 25,216
across Sudden Death. It is real, and it is an order of magnitude short.

## 11.2 The items-off exclusion is PROVEN — from the linked ELF, not from names

Modality: the repo's own oracle (`linked-ELF-is-the-reader-oracle`), on
`builds/build-c158-gate/smash64ds-battle-playable-tickhud-hwtri.elf`. Not a name
grep, not one match's observation.

**Every function that could set an item status in the shipped battle ELF is a
two-byte `bx lr` stub.** `arm-none-eabi-nm -S`:

```text
T 00000002  ftCommonItemThrowSetStatus        ftCommonItemSwingSetStatus
T 00000002  ftCommonItemShootSetStatus        ftCommonItemShootAirSetStatus
T 00000002  ftCommonLightThrowDecideSetStatus ftCommonHammerFallSetStatus
W 00000002  ftCommonItemThrowProcUpdate       ftCommonHeavyThrowProcMap
W 00000002  ftCommonLGunShoot{,Air}Proc*      ftCommonFireFlowerShoot{,Air}Proc*
W 00000002  ftCommonStarRodSwingProcUpdate    ftCommonHarisenSwingProcUpdate
W 00000002  ftCommonHammer{Walk,Turn,KneeBend,Fall,Landing}Proc*
```

`objdump -d` on the cluster at `0x208fc34..0x208fc74` reads `bx lr; nop` for each.
And **there is no item spawner at all**: the entire `it*` surface in the ELF is
`itManagerInitItems` (32 B), `itManagerMakeAppearActor` (16 B),
`itMainGetDamageOutput` (48 B), `itProcessSetHitInteractStats` (208 B),
`itMainDestroyItem` (2 B stub) and `itMainCheckShootNoAmmo` (4 B stub). No item
GObj can exist, and the statuses that would play the item clips return
immediately when asked.

**Priced by re-packing, never by subtracting a mean** — dedup makes a subset's
cost non-linear, which is why `--exclude-ids` re-packs:

```text
651,928 -> 553,696   (-98,232)   38 clips, 728 scripts
equivalence re-run on the subset: MISMATCHES 0
  259 clips / 4,901 scripts / 66,776 commands / 61,121 states / 4,901 callbacks
  corpus c034b342b0760d13152eaeafe60ac204cd39e7905e9ae438d958377429d4982e
  sha256 c7a042de9e96315e6c3773e499d3c8819b2645808e7529c7faab19be7ccc5c2b
artifacts/performance/2026-08-14_native-battle-kernel/battlepack-sizing-itemsoff.json
```

## 11.3 A sized LEAD, explicitly not proven

Fourteen further clips — Mushroom Kingdom pipes (`0x276-0x278`, `0x306-0x308`)
and grabs by fighters absent from the matchup (`0x205`, `0x22d`, `0x22e`,
`0x230`, `0x29c`, `0x2c4`, `0x2c5`, `0x2c7`) — take the pack to **528,624**
(-123,304 from full). Their handlers are *absent* from the ELF rather than
stubbed, and absence is the weakest evidence available (`nm` cannot see an
inlined or differently-named body), so this is a lead and not a saving.
`battlepack-sizing-matchup-lead.json`; not equivalence-verified.

## 11.4 The design answer to route 3 — and it is neither of route 3's options

**(a) "A more compact clip representation" is REFUTED as a *lossless* lever.**
The AObj16 stream is already 16-bit throughout: one u16 command header, one
optional u16 payload, and one or two **s16** target words per selected track
(`scripts/ftanim_reloc_probe.py:197-209`, `struct.unpack_from("<%dh")`). There is
no f32 field to narrow and no value dictionary that can pay — an index over a
16-bit alphabet is not smaller than the alphabet. What remains is a lossy
re-encode (a fidelity trade, `BLOCKED(decision:)` material) or a decompression
step, which re-introduces exactly the per-acquisition work slice 1 exists to
delete. **This also retracts a hypothesis this cycle started with** — that the
105,304 target words were f32 and compactable. They are s16.

**(b) "A deterministic pre-GO arena" creates no RAM.** It only chooses which of
§11.1(b)'s two pools pays, and it forfeits the 211,936 static pool.

**(c) What the evidence favours: spend one named, sized, unspent
`docs/RAM_RECOVERY_PLAN.md` Phase 2 item.** `gSYFramebufferSets[2][230][320]` is
**294,400 B**. The DS never rasterises into it (GX renders to VRAM); its only
reader is the VS Results photo wipe, and `include/sys/video.h:62-72` already
documents the exact span that wipe touches: `base+7,060 .. base+147,819`, i.e.
**231 rows = 147,840 B**. Collapsing the object to that span recovers
**146,560 B**:

```text
proven pool + 146,560 = 658,464   vs full pack 651,928   FITS by 6,536
                                  vs items-off  553,696   FITS by 104,768
```

Blast radius is bounded and that header states it: `scmanager.c` bounds its clear
with `sizeof(gSYFramebufferSets)` so the clear shrinks with the extent, and
`mntitle.c:126-127` hardcodes `[1]`/`[2]` in a scene P1 never runs. It is Phase 2
work with its own gate and the owner's eye on the Results wipe — not this cycle's.

> `BLOCKED(decision: none required.)` Still a measurement/engineering gap, not a
> fidelity trade. It becomes an owner question only if Phase 2 is refused.

## 12. Phase 5's inherited premise is HALF the reason the copy exists

The line this cycle was told to verify before building on it:

> "The adapter copies only because internal fixups write *absolute* pointers …
> Remove those and the reason to copy goes with them."

**Verified as far as it goes, and incomplete.** Reason one is real:
`ndsRelocApplyInternalPointerFixups` writes `data + target*4`, stated at
`reloc_backend_assets.c:6222-6228` and again at `:7199-7201` and `:7264-7266`,
and the pack's word offsets delete it.

**Reason two is independent of the fixups and the pack cannot touch it.**
`decomp/…/ft/ftmain.c:4623-4624` is

```c
lbRelocGetForceExternHeapFile(motion_desc->anim_file_id, (void*) fp->figatree_heap);
fp->figatree = fp->figatree_heap;
```

— the **return value is discarded** and the fighter always animates from its own
heap, so the bytes must physically be at `figatree_heap` whatever the fixups do.
`docs/P1_EXECUTION_BOARD.md:3408-3425` already closed the zero-copy seam on
exactly this line in cycle 108; §3 of the architecture doc does not mention it.
And the port does **not** own that body: `src/import/battleship_ftmain.c` renames
the decomp symbols and `#include`s `ft/ftmain.c` verbatim, wrapping only the
entry point.

**The unblocking move, for the next cycle.** `fp->figatree` is read at `:4628`
(a NULL test) and `:4704` (`lbCommonAddFighterPartsFigatree`) and nowhere assumed
equal to `figatree_heap`, so the one-line patch

```c
fp->figatree = lbRelocGetForceExternHeapFile(motion_desc->anim_file_id,
                                             (void*) fp->figatree_heap);
```

under `scripts/decomp-patches/battleship/` makes the returned pointer
authoritative. **It is inert today for fighter animations** — the
`NDS_IMPORT_BATTLESHIP_FTMANAGER` arm of `lbRelocGetForceExternHeapFile`
(`reloc_backend_assets.c:7294-7305`) returns `file != NULL ? file : heap`, and
`file == heap` on every success — so it changes no behaviour until a const-clip
path returns a different pointer. One caveat to check before landing it: the
generic arm at `:7308-7322` can return a `file != heap` when
`ndsRelocFindLoadedFileByData` misses, which today is discarded; that path is not
fighter-animation and must be confirmed unreachable or handled.

## 13. What this cycle did NOT do

- **No DS runtime path (phase 5), no oracle mode (phase 6).** The premise check
  above moved phase 5's shape, and the RAM verdict is a stop condition for
  sizing the resident set it would point at.
- **No build, no ROM, no gate measurement.** The -73,659 remains a projection.
- **The matchup lead (§11.3) is not proven and not equivalence-verified.**
- **No RAM was recovered.** §11.4(c) names and sizes the recovery; it does not
  take it.
- **`gNdsRelocAssetOpenFailCount` still not split** (§2.4); `gNdsR2AnimCacheHitBytes`
  still not added (§3.3).
