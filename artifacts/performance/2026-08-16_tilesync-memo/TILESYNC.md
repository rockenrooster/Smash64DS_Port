# Both counter-gated items already had their counter, and it had already been read. One is real and is now shipped; the other is refuted and re-attributed. C2's byte budget is 5.9x tighter than the estimate that already said no.

**Date:** 2026-08-16 · **Branch:** `codex/r2-runtime2` · **base HEAD `c3f7f048074`**
2 lab builds (`build-c216-tilesync`, `build-c217-tilesync-ship`), 3 whole-match gate runs,
0 gameplay values changed, no default flipped that costs fidelity, no ROM published,
both root ROMs byte-unchanged.
**UNITS: 1 project tick = 1 `cpuGetTiming()` tick = 2 ARM9 cycles.** Every table states
its window.

```text
REQUIREMENT  +82,065 net ticks per presented frame at rank-80.  Basis:
             build-c215-hwmath-ship, rank-80 1,227,392 raw / 1,202,445 net
             against the 1,120,380 gate; SHIPPING renderer (GX_COMPOSE 0),
             bore 0, mode 163 one-minute match, 1,600 samples, frames 439-2038,
             slips=0 (../2026-08-16_hwmath-route/HWROUTE.md section 4).

FOUND FIRST  THE COUNTERS WERE ALREADY WRITTEN AND ALREADY READ.
             NDS_TASK107_RENDER_STATE_CENSUS (nds_renderer.c:5035, Makefile:250)
             publishes gNdsTask107SyncCalls[4] / gNdsTask107SyncUnchanged[4] and
             six bind counters, and it was RUN on 2026-08-15
             (../2026-08-15_renderer-state-redundancy/STATE_REDUNDANCY.md,
             build-c186-statecensus).  Three documents since then say "needs a
             counter first".  Nothing needed a counter; what the item needed was
             for someone to open the artifact directory.

ITEM A1      ndsRendererSyncTextureTile IS REAL WASTE, AND IT IS NOW SHIPPED.
             62.12% of its calls are provably redundant -- 89,511 skips of
             144,105, from this cycle's own counters, IDENTICAL on both route
             arms.  An exact serial memo is in, measured, and kept.
               same-binary route, arm0 -> arm1   paired median  -3,648
                                                 marginal-80    -3,456
                                                 win share       87.1% of 1,600
               complement control  WORK-H -3,648 / WAIT +3,648 / ALL +0
             SECOND, INDEPENDENT CONTROL: Boundary's own 212-frame smoke reads
             binds/vtx/tri 54/2484/828 and ftrTri 132712/p067840/p164872/own424
             IDENTICAL to the last two Boundary runs, for 118,976 FEWER ticks.
             Sections 2, 3 and 3.4.

ITEM A2      THE TEXTURE-BIND COLLAPSE IS REFUTED AS AN ELISION ITEM, AND
             RE-ATTRIBUTED.  The 2026-08-15 census already measured ZERO exact
             locally-redundant bind issues; the 26,769 revisits need draw
             REORDERING and their perfect-ordering ceiling is 2,484 tk/fr.  What
             this cycle adds is where the 13,868 actually goes: 5,754 tk/fr of
             it (45%) is `icache_fill` on 360 bytes of text at 56.8-104.6
             entries a frame.  It is a PLACEMENT item, not a bind-count item,
             and the 268 B of it that is port-reachable fits in the ITCM still
             free on either target.  Section 4.

ITEM B       C2 DOES NOT CONVERT, AND THE MARGIN IS 3x.  The chain's cubic
             evaluator was ALREADY converted to fixed point and is already in
             the binary: ndsR2AnimValueQ, 1,028 B, 370.6 entries/frame, 26,664
             tk/fr of which 21,719 is icache_fill.  THAT IS THE MEASURED
             MARGINAL PRICE OF A HOT KERNEL BYTE IN THIS CHAIN: 21.13 tk/fr,
             corroborated by lbCommonSin 18.66 and lbCommonCos 16.74.
             HWROUTE.md section 7's 3.61 is an AVERAGE over four caller bodies
             and understates an ADDED kernel by 4.6x-5.9x.
               the whole 20,357 prize is spent by     963 bytes  (was 5,640)
               half of it by                          482 bytes  (was 2,820)
             The ring added 3,228 B and the camera inlining 3,032 B: each is
             2.5x-3.4x the WHOLE budget.  Section 5.

NEW BASIS    build-c217-tilesync-ship, rank-80 1,218,752 raw / 1,193,805 net.
             LEVEL +73,425.  Only -3,648 of the -8,640 is attributable to the
             memo; the rest is cross-build placement and is NOT banked.
             Section 3.3.

BYTES        +128 B ITCM, +384 B .main, +96 B .bss.  ITCM free on the tick-HUD
             instrument 640 -> 512; on the proof ROM Boundary grades it is
             2,896.  Quote the target with the number.  Section 6.

INHERITED    ndsR2AnimValueQ pays 21,719 tk/fr of PURE INSTRUCTION FETCH on
             1,028 bytes at 370.6 entries/frame.  Nothing on the board has
             priced it.  It is the largest single fidelity-neutral item found
             this cycle and it is a placement question on an already-converted
             kernel.  Section 5.3.
```

---

## 1. The counters existed. Read the artifact directory before writing an instrument.

`POSITION.md` §2, `LADDER.md` §1 and `HWROUTE.md` §9 all carry the same line: these two
rows "need one counter first". They do not, and have not since 2026-08-15.

| | |
|---|---|
| flag | `NDS_TASK107_RENDER_STATE_CENSUS ?= 0` (`Makefile:250`) |
| tile-sync counters | `gNdsTask107SyncCalls[4]`, `gNdsTask107SyncUnchanged[4]`, `gNdsTask107SyncTrackerOverflow` — a full `NDSRendererTileState` + load-tile `set_seen` snapshot per stats object per tile, retired on `ndsRendererInitStats` so stack reuse cannot manufacture a repeat |
| bind counters | `gNdsTask107BindRequests`, `…ZeroNameExits`, `…CurrentNameElisions`, `…Issues`, `…RevisitIssues`, `…NameSetOverflow` |
| run | `build-c186-statecensus`, 1,600 samples, `../2026-08-15_renderer-state-redundancy/` |

Its numbers, quoted so nobody re-measures them:

| tile-sync call site | calls | exact unchanged | |
|---|---:|---:|---:|
| `RecordTextureState` | 76,213 | 74,081 | 97.20% |
| `RecordSetTile` | 50,890 | 30,094 | 59.14% |
| `RecordSetTileSize` | 18,813 | 2,020 | 10.74% |
| `HardwareResolveOrBindTexture` | 305 | 305 | 100.00% |
| **total** | **146,221** | **106,500** | **72.835%** |

That census then **stopped**, on a rule that no longer exists: a "16,000-tick package
floor". `AGENTS.md` now says the opposite — *"Milestone tick targets are directional, not
per-cut discard gates: keep every repeatable correctness-preserving gain and accumulate it
toward the target."* The 2026-08-15 verdict was correct under its rule and is wrong under
today's.

**`[[check-what-the-harness-already-prints]]`, fourth recurrence.** The cheapest
discriminating read in this cycle was `ls artifacts/performance/`.

---

## 2. What `ndsRendererSyncTextureTile` actually is, and why the memo is exact

It is **not** a VRAM tile sync. It republishes nineteen fields of
`stats->texture_tiles[active]` into flat `stats->texture_render_tile_*` /
`texture_tile_size_*` members. It is ITCM-resident (`NDS_R2_DELTA_PATH_CODE`), so it pays
**no instruction fetch at all** — its cost is the store burst:

| | c200 marginal-80 census, `ndsRendererSyncTextureTile` |
|---|---:|
| bytes | 212 |
| entries/frame | 73.5 |
| `write_buffer` | **5,770** |
| `dcache_fill` | 2,417 |
| `issue` | 1,152 |
| `icache_fill` | **1** |
| **total** | **9,469 tk/fr** |

(`GXSTACK_IO_DRAW.md` §4.2's 8,867 came from `perregion3-c181.csv`, a different build and
census; the two agree to 6.8%.)

**The exactness argument.** The whole output is a pure function of
`(tile_index, texture_tiles[tile_index], texture_tiles[NDS_RENDERER_LOAD_TILE].set_seen)`.
`texture_tiles[]` has exactly two writers in the tree — `ndsRendererRecordSetTile`
(`nds_renderer.c:7361`) and `ndsRendererRecordSetTileSize` (`:7504`) — plus one `memcpy`
in `ndsFighterDLDrawCopyPersistentRendererState`
(`reloc_backend_renderer_dl.c:6080`). So:

```c
/* both writers, one rule */
if ((tile == stats->texture_render_tile) || (tile == NDS_RENDERER_LOAD_TILE))
    stats->texture_tile_write_serial++;

/* the memo */
if ((stats->texture_tile_sync_serial == stats->texture_tile_write_serial) &&
    (stats->texture_render_tile == tile_index))
    return;
```

`texture_render_tile` is the **last synced index** and only the sync body and that copy
write it, so index equality proves the last publish targeted this tile and serial equality
proves neither input moved since. A write to any other tile cannot change the output for
the tile last published, and if the active tile later moves onto it the index compare
forces a full republish anyway. The initial state is exact too: a `memset` stats has
serial `0 == 0`, index `0 == NDS_RENDERER_RENDER_TILE`, and all republished fields already
zero. `ndsFighterDLDrawResetTransientRendererStats` bzeroes only up to
`offsetof(NDSRendererStats, othermode_h)`, i.e. it preserves `texture_tiles[]`, the
republished fields **and** the two serials together — so the memo survives it.

The serials are deliberately **absent from both state hashes**
(`ndsRendererSemanticSourceStateHash`, `NDS_RENDERER_HASH_STATE_FIELD`) and **present in
the persistent copy**, because that copy carries `texture_tiles[]` and the republished
fields together.

---

## 3. The measurement

Both changes are far under the **≥14,080 rank-80 cross-build floor**, so
`build-c216-tilesync` holds both arms in one binary behind one `.data` word.

| | |
|---|---|
| target | `smash64ds-battle-playable-tickhud-hwtri` |
| config | `NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 NDS_R2_TILESYNC_ROUTE=1`, GX_COMPOSE 0, bore 0, DLDI on |
| window | 1,600 samples, frames 439–2038, `-RingDump`, `slips=0` on every arm |
| route word | `nm` reads `020e72e8 D gNdsR2TileSyncRoute` — **`.data`, not `.bss`** |
| level check | arm 0's rank-80 is 1,216,064 against `build-c215-hwmath-ship`'s 1,227,392 — 11,328 apart, inside the cross-build floor |
| raw | `t0.json`, `t1.json`, `*-rows.csv`, `route-arms.txt`, `ranks.txt` |

**Disassembled before measuring** (`ndsRendererSyncTextureTile` at `0x01ff87c0`, ARM
state, ITCM): the memo compiles to `cmp r3,r1 / beq` on the serials, then
`ldr r3,[r0,#392] / cmp r3,r2 / bne` on the index, then the counter, then the route load
and `bx lr`. On arm 0 the fall-through re-enters the body **after** the two state stores,
so both arms leave identical state and the predicate evolves identically.

### 3.1 Engagement, and the control that could have failed

```text
gNdsR2TileSyncSkips = 89,511      gNdsR2TileSyncRuns = 54,594      arm 0
gNdsR2TileSyncSkips = 89,511      gNdsR2TileSyncRuns = 54,594      arm 1
```

**Bit-identical on both arms**, as the design requires: both arms evaluate the predicate
and both advance the serial, so the only difference between them is the store burst.
Skip fraction **62.12% of 144,105** — 85.3% of the memcmp oracle's 72.835%, which is the
price of a guard that is two compares instead of an 80-byte `memcmp`.

### 3.2 The result, same binary, zero repeat floor

| statistic | arm 0 → arm 1 |
|---|---:|
| paired median, whole run | **−3,648** (87.1% of 1,600 frames improve) |
| paired median, marginal-80 | **−3,456** |
| control ranks 1–40, paired median | −3,264 (7/40 worse) |
| control ranks 41–120, paired median | −3,968 (12/80 worse) |
| `FTR` paired median | −3,456 / −3,840 marginal-80 |
| own rank-80 | **+6,912** |
| own ranks 41–120 band | +1,808 |

**The complement is the control.** `WORK-H −3,648`, `WAIT +3,648`, `ALL +0`. `ALL` is
VBlank-quantised (`[[all-is-a-quantized-gate]]`), so at a fixed presented cadence a
genuine work deletion must appear as work down and idle up by the same amount — and it
does, to the tick. VBI histograms: arm 0 `2:1712 3:299 4:19 5+:8 max:20`, arm 1
`2:1714 3:300 4:16 5+:8 max:20`.

**The +6,912 at rank-80 is a local kink in the order statistic, not a regression**, and
the own-rank curve says so plainly (`ranks.txt`):

```text
rank   20   -7,296      rank  100     -384      rank  400   -4,736
rank   40   -7,104      rank  120     -128      rank  800   -4,160
rank   60   +7,808      rank  160     -576
rank   80   +6,912      rank  240   -3,904
```

Eight of ten sampled ranks improve; two adjacent ones do not. Frame-paired, the marginal
80 improve by a median 3,456 — the frames that set rank-80 in the control all get cheaper,
and the arm's own rank-80 is set by a *different* frame. Two ±134,000 outlier frames
(1500, 996, 957 — cartridge frames that shift by one presented frame between arms) are
what makes the marginal-80 **mean** (−580) disagree with its **median** (−3,456); quote
the median.

### 3.3 The shipping build, and what is honestly bankable

`build-c217-tilesync-ship` is the same source with `NDS_R2_TILESYNC_ROUTE` at its default
0 — no route word, no counters, `nm` finds no `gNdsR2TileSync*` symbol at all.

| | rank-80 raw | net | level | ranks 41–120 | P50 |
|---|---:|---:|---:|---:|---:|
| `c215-hwmath-ship` (basis) | 1,227,392 | 1,202,445 | +82,065 | 1,228,704 | 944,864 |
| **`c217-tilesync-ship`** | **1,218,752** | **1,193,805** | **+73,425** | 1,222,048 | 934,944 |
| cross-build Δ | **−8,640** | | | **−6,656** | **−9,920** |
| paired median / marginal-80 | **−9,216** (93.2% win) | | | | −10,272 |

**Do not bank −8,640.** Cross-build at rank-80 the floor is ≥14,080 and −8,640 is inside
it; the P50 −9,920 clears its ~5,700 floor in sign but not by much. The route says the
memo is worth **−3,648**, and the ~5,600 difference is placement: this pair's own
placement scale is visible as **`SRC +3,136` and `MISC +1,216`**, buckets the memo cannot
touch, and `build-c216`'s memo-OFF arm 0 already sat 11,328 below the c215 basis before
the memo did anything.

```text
BANKED, ATTRIBUTABLE     -3,648  =  0.044x of +82,065     (route, zero-noise instrument)
NEW BASIS LEVEL          +73,425                          (build-c217-tilesync-ship)
```

The basis is quoted as a **level**, because that is what the shipping configuration reads
today on the same target, config and window; it is not a claim that 8,640 was earned.

### 3.4 Boundary is a second, independent equality control — and it could have failed

Boundary builds its own `smash64ds-battle-playable-proof-hwtri` and `nm` on that ELF
confirms it carries the memo (`ndsRendererSyncTextureTile` 0xf8 = 248 B, no
`gNdsR2TileSync*` symbols — the ship form). Its realtime pacing smoke over the same 212
frames, against the last two Boundary runs on this tree:

| | `boundary-c206` / `-c209b` | this cycle | Δ |
|---|---|---|---|
| `binds` / `vtx` / `tri` | `54 / 2484 / 828` | `54 / 2484 / 828` | **identical** |
| `ftrTri` | `132712/p067840/p164872/own424` | `132712/p067840/p164872/own424` | **identical** |
| `frames` / `fps` / `rprof` | `212 / 241/480 / 0` | `212 / 241/480 / 0` | identical |
| `ticks` | 294,482,496 | **294,363,520** | **−118,976 = −561 tk/fr** |

**Same geometry, same binds, same triangles, measurably less time.** The counters that
would move if the memo published a wrong tile state are the ones that did not move at all.
(The −561 tk/fr is the proof ROM's boot/Pupupu scene, not the gate window; it is a
direction check, not a second price.)

---

## 4. Item A2 — the bind collapse is not an elision item, and the census says what it is

`STATE_REDUNDANCY.md` §3 already settled the elision question and it needs no repeat:

| event | count |
|---|---:|
| requests | 208,327 |
| current-name elisions | 95,934 (**46.05%** — the existing check is already working) |
| actual bind issues | 112,393 |
| issue whose name was used earlier this frame | 26,769 |
| **exact locally redundant issues** | **0** |

The 26,769 revisits are not locally deletable: the currently bound name is *different*, so
dropping the issue changes the texture the following geometry samples. Their
perfect-reordering ceiling is **2,484 tk/fr** and reaching it is a draw-batching task, not
a state-elision one.

**What this cycle adds is the cost breakdown, from the same marginal-80 per-PC census**
(`c2-ledger.txt`):

| symbol | bytes | ent/fr | `icache_fill` | `dcache_fill` | total tk/fr |
|---|---:|---:|---:|---:|---:|
| `ndsRendererHardwareBindTextureName` | 268 | 104.6 | **3,802** | 2,181 | 6,101 |
| `glBindTexture` | 92 | 56.8 | **1,952** | 3,507 | 6,700 |
| **total** | **360** | | **5,754** | 5,688 | **12,801** |

**45% of the 13,868 is instruction fetch on 360 bytes**, at 14.19 and 21.22 tk/fr per byte
— the signature of a small body that does not survive in icache between entries.
`ndsRendererHardwareBindTextureName` is `static` in `nds_renderer.c` and takes
`NDS_TASK82_ITCM_CODE` the same way `NDS_R2_DELTA_PATH_CODE` members do; `glBindTexture`
is libnds (`020be0bc T`) and would need the `--rename-section` route that `BASIS.md` §6
found traps bytes beside live symbols.

**Stated as a ceiling, not a prediction: 5,754 tk/fr, of which 3,802 is reachable
port-side, for 268 B of ITCM (512 free on the instrument, 2,896 on the proof ROM — §6).**
Not built this cycle. The
precedent is `NDS_R2_DELTA_PATH_ITCM`, which bought `FTR` P50 −12,032 for +1,016 B of
ITCM; the counter-precedent is `K-EXCHANGE`, which refuted *main-RAM* placement at a +219
tk/fr ceiling — a different mechanism, since ITCM is zero-wait and never evicted.

---

## 5. Item B — C2's byte ledger

### 5.1 The ledger's first line: most of C2 is already converted

`ndsR2FtAnimParseDObjFigatree` is **already the fixed-point parser in every shipped ROM**.
`Makefile:1680` is `override NDS_R2_CUBIC_FIXED := 1` and `NDS_R2_ANIM_CUT_ROUTE ?= 0`
makes `NDS_R2_ANIM_CUT_ON(bit)` fold to a constant `1`
(`battleship_ftanim.c:99-106`), so `q` is `1u` and every float arm in that switch is
dead-coded. Its evaluator is `ndsR2AnimValueQ` (`battleship_sys_objanim.c:448`,
`__attribute__((noinline, target("arm")))`), and the AObj value slots carry Q in place —
`ndsR2AQStore` is a `__builtin_memcpy` bit-pun into the `f32` field, so the representation
costs **zero** storage.

So the 20,357 tk/fr "prize" is not un-converted arithmetic. It is what **survives** the
conversion: the f32 fields the decomp ABI fixes — `dobj->anim_wait`, `anim_speed`,
`anim_frame`, `aobj->length`, and the `Vec3f` joint outputs — i.e. **the representation
boundary itself**, which is exactly what `EXCHANGE_LEAF.md` measured at R 0.83x–1.00x.

`gmCollisionTransformMatrixAll` makes this concrete. Its 489.2 helper calls/frame over
22.5 entries is **21.7 operations per entry** (16 `fmul` + 4 `fadd`/`fsub` + 3 `fcmp`,
counted from `decomp/.../gm/gmcollision.c:29`). A Q rewrite converts 9 inputs
(`rotate`/`scale`/`translate`) and 12 outputs (`Mtx44f`): **21 conversions for 21.7
deleted operations, conv/op = 0.97.** `EXCHANGE_LEAF.md`'s law puts that at R ≈ 1.00x —
the same reading `gmCollisionGetWorldPosition` returned at 18/18. It is a leaf, and it is
worth nothing. Its consumer is `gmCollisionGetFighterPartsWorldPosition`, so making it
*not* a leaf means a Q collision matrix, which is rung 3 and the owner's call.

The ABI blast radius, counted rather than asserted: **593 exact `->anim_wait` /
`->anim_speed` / `->anim_frame` accesses in `decomp/BattleShip-main/decomp/src` and 810 in
`src/`, across 117 files**, plus 157 `->length` / `->length_invert`. `Makefile:2302`
forbids a decomp overlay patch for a new adaptation, so every one of those is a caller
rewrite, not an interception.

### 5.2 The ledger's second line: the price of a byte is 5.9x what was estimated

`c2-ledger.txt`, marginal-80 per-PC census (`c200-off-pc.csv`) attributed to
`build-c200-trackprof-off`'s own symbol table.

> **The attribution has a control and it could have failed.** `__aeabi_fadd` reads
> **3,903.9 entries/frame** here against `BASIS.md` §7's independently published
> "3,903.9 on the marginal-80" — exact — and its issue share reads 96.2% against
> `BASIS.md`'s 96.22%. Different tool, same three significant figures.
> The census's `issue` column is a residual and goes **negative** on three rows; only
> `total_cycles` and `icache_fill` are quoted from those.

| symbol | bytes | ent/fr | `icache_fill` tk/fr | **tk/fr per byte** |
|---|---:|---:|---:|---:|
| **`ndsR2AnimValueQ`** | **1,028** | **370.6** | **21,719** | **21.13** |
| `lbCommonSin` | 56 | 83.8 | 1,045 | 18.66 |
| `lbCommonCos` | 64 | 83.8 | 1,071 | 16.74 |
| `gmCollisionTransformMatrixAll` | 430 | 22.5 | 2,458 | 5.72 |
| `ndsBaseGcPlayMObjMatAnim` | 732 | 85.3 | 2,971 | 4.06 |
| `ndsR2FtAnimParseDObjFigatree` | 3,016 | 93.4 | 12,185 | 4.04 |
| `ndsBaseGcPlayDObjAnimJoint` | 500 | 62.1 | 1,636 | 3.27 |

The split is not scatter. A large caller fetches only its **executed path** per entry; a
kernel is fetched **whole**, on every call, by construction. The three kernel-shaped rows
read 16.74, 18.66 and 21.13. `ndsR2AnimValueQ`'s 21,719 tk/fr over 370.6 entries is 117
cycles of fetch per entry — about twelve cache lines of a thirty-two-line function, i.e.
its lines do not survive between entries at all. The number passes its own physical check.

**HWROUTE.md §7's 3.61 tk/fr per byte is `16,891 / 4,680` — an average over four caller
bodies, three of which are the first kind. It understates the price of an added hot kernel
by 4.6x–5.9x.** Corrected:

```text
                                        HWROUTE section 7      measured here
whole 20,357 prize spent by                   +5,640 B          963 B
half of it spent by                           +2,820 B          482 B
collision ring's added bytes                   3,228 B      =  3.4x the WHOLE budget
camera inlining's added bytes                  3,032 B      =  3.1x the WHOLE budget
```

### 5.3 Verdict, and the item it uncovers

**C2 does not convert. NO.** The rewrite would have to be byte-neutral or negative; every
useful Q kernel in this tree is 1,028–3,228 B and the budget is 963. The two limits
`HWROUTE.md` §7 required it to carry both still hold and both got worse: the chain's own
`issue` cost is a fifth of its total, and `dcache_fill` exceeds it, so it is memory-bound
on both sides and a Q26 joint matrix is the same 32 bits as the `f32` it replaces.

**What the ledger uncovered instead is a bigger, differently-shaped item.**
`ndsR2AnimValueQ` — the kernel this campaign already built and already banked — pays
**21,719 tk/fr of pure instruction fetch** for 1,028 bytes at 370.6 entries a frame. That
is 0.265x of +82,065, it is fidelity-neutral by construction (the arithmetic does not
change), and **no document on this board has priced it.** It does not fit in the 512 B of
free ITCM as it stands; the question is whether its executed path does, or whether the
1,028 B can be split so the hot half does. Add `lbCommonSin`+`lbCommonCos` (120 B,
2,116 tk/fr of fetch, 83.8 entries/frame each) and the animation chain's *fetch* bill is
23,835 tk/fr against an arithmetic prize of 20,357 that does not convert.

---

## 6. Bytes

```text
                     .itcm      .main    .main.bss
c215-hwmath-ship     32,096    930,872   1,307,152
c217-tilesync-ship   32,224    931,256   1,307,248
                       +128       +384         +96

ndsRendererSyncTextureTile      212 -> 248   +36     all three are ITCM
ndsRendererRecordSetTile        264 -> 332   +68     members, so the +128
ndsRendererRecordSetTileSize    156 -> 180   +24     is exactly these three
```

**ITCM headroom is per target and the two figures are far apart. State which one.**

```text
region 0x7fe0 = 32,736 B (linker/nds_hot_text.ld:18, minus vectors)

smash64ds-battle-playable-tickhud-hwtri  (the measurement instrument)
   build-c215-hwmath-ship     32,096      640 free
   build-c217-tilesync-ship   32,224      512 free      <- this cycle spent 128

smash64ds-battle-playable-proof-hwtri    (what Boundary builds and grades)
   29,872 / 32,768 reported            2,896 free       <- boundary-c217.log,
                                                           "Task 9 float ITCM passed"
```

The instrument carries tick-HUD code the shipped ROM does not, so **512 B is the tight
figure and it belongs to the lab build only**. §4's bind-placement candidate wants 268 B;
it fits in either, but sizing it against 512 rather than 2,896 is the honest test.

The `.main` +384 and `.bss` +96 are the two new `NDSRendererStats` words rippling through
struct offsets and the static stats objects.

---

## 7. What this cycle did NOT do

- **The bind ITCM candidate was not built.** §4 gives its ceiling (5,754 tk/fr, 3,802 of
  it reachable port-side) and its cost (268 B of 512 free). It is one build.
- **`ndsR2AnimValueQ`'s 21,719 tk/fr of fetch was not attacked.** §5.3 sizes it; nothing
  was tried.
- **No draw reordering.** The 2,484 tk/fr bind-revisit ceiling stands unclaimed.
- **C2 was not built**, which was the point: the ledger answers it without a build.
- **No fidelity decision was taken or asked for.** The memo is exact by construction.
- **No pixel capture** beyond Boundary's own, no ROM published, both root ROMs
  byte-unchanged: `smash64ds.nds` `54c07fac…`, `smash64ds-battle-playable-hwtri.nds`
  `6c939434…` (the bore-84 link, which must not be published), before and after.
- **`build-c205-camtoggle` was not rebuilt.**

---

## 8. Reproduction

```powershell
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c216-tilesync `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1 `
    NDS_R2_TILESYNC_ROUTE=1
foreach ($arm in 0,1) {
  pwsh -File scripts\sample-tick-hud-buckets.ps1 -Build build-c216-tilesync -NoBuild `
      -RingDump -Samples 1600 -StartFrame 438 -TimeoutSeconds 3600 `
      -SetGlobals "gNdsR2TileSyncRoute=$arm" `
      -ExtraGlobals gNdsR2TileSyncSkips,gNdsR2TileSyncRuns `
      -RowsCsv ...\t$arm-rows.csv -JsonOut ...\t$arm.json
}
make TARGET=smash64ds-battle-playable-tickhud-hwtri BUILD=build-c217-tilesync-ship `
    NDS_R2_BOTH_CPU=1 NDS_R2_BATTLEPACK=1 NDS_R2_BATTLEPACK_KEEP_CACHE=1
```

§4 and §5 need **no build at all** — `c2ledger.py` is exact arithmetic over
`../2026-08-15_ftanim-dispatch-attribution/c200-off-pc.csv` and one linked ELF, both
already on disk, and §4's census numbers are `../2026-08-15_renderer-state-redundancy/`.
