# P1 Execution Board

Updated: 2026-08-05 (cycle 80). Boundary: `battle_playable_realtime`, mode `163`.

This board was rewritten from a 10,207-line append log into a queue. Every
verdict, baseline, and instrument note carried forward unchanged; the full
pre-rewrite text is `docs/optimization/archive/P1_EXECUTION_BOARD_pre-cycle79.md`.
Closed work goes to that archive (append a dated section), not back onto this
board. The charter is `docs/Smash64DS_Runtime2_SwitchPlan.md`; measurement and
workflow rules live in `docs/VERIFYING.md`.

## THE MATCH LENGTH RULE (owner, 2026-08-05) — read before banking anything

> "the soak was only meant to catch freezes, boundary and both cpu gates should
> be the 60 sec match"

**Both gate arms run the one-minute match. LANDED, cycle 80.** The `time_limit
= 7` is gone from `scene_harness.c`'s `NDS_R2_BOTH_CPU` branch, and the soak's
long match now lives on `NDS_R2_SOAK_MATCH_MINUTES` (Makefile, default 0 =
canonical one-minute match). `soak-freeze-watch.ps1` derives that value from its
own `-MinutesToRun` instead of a constant, so the match can no longer be shorter
than the run watching it, and it *reads the seeded value back out of the guest*
at end of run rather than trusting the flag. Proven end to end: resolved 2 →
generated header `NDS_R2_SOAK_MATCH_MINUTES 2` → in-guest `time_limit = 2` with
`gNdsVSResultsStartCount 0` over 2,705 presented battle frames, i.e. the soak
spent none of its run on a Results screen.

Both arms were re-banked on the corrected seed and **now measure identically**:
match timer 1 min, clock 52 s → 0 s, coverage 86.7%, logic:presented 2.000.

**A window is "whole match" only if its coverage was measured against the match
clock.** Coverage is part of a baseline's identity now, not a footnote. The
conversion: the sim runs 60 Hz and presents 30 Hz, ratio **measured at exactly
2.000**, so 1,600 presented = 3,200 logic = **53.3 s**.

## Banked baselines — BOTH ARMS RE-BANKED ON THE CORRECTED SEED (cycle 80)

1,600 samples, frames 441–2040, `dldi=ON`, git `34091054`+reseed,
`sample-tick-hud-buckets.ps1 -RingDump` (stride 96), `slips=0` on both.
Builds `builds/build-c80-gate-bothcpu` and `builds/build-c80-boundary`.
**Never take a gate reading on a 128-frame window** — it reads the cheapest 6%
of the match (P95 understated ~306,000, over-gate rate 5×).

| arm | role | coverage | `WORK-H` P50 | P95 | over gate | gap to gate |
|---|---|---|---:|---:|---:|---:|
| **both-CPU** `NDS_R2_BOTH_CPU=1` | **THE GATE** | **86.7%** of 60 s | 1,094,464 | **1,624,064** | 704/1600 (44.0%) | **503,684** |
| **Boundary** mode 163 | shipped configuration | **86.7%** of 60 s | 1,082,112 | 1,476,672 | 673/1600 (42.1%) | 356,292 |

VBI 2/3/4/5+ (max): both-CPU 1127/808/91/14 (20); Boundary 1194/784/51/11 (20).

**The corrected gap is 503,684, not 485,060** — the early-match window was
optimistic by 18,624, exactly as predicted. Every superseded both-CPU figure is
now replaced above; the old ones are in the cycle-79 archive section.

**THE WINDOW RUNS PAST THE BUZZER ON BOTH ARMS — 43 frames, and they are cheap.**
Frames 1998–2040 are post-buzzer GAME SET on *both* arms (contiguous, `SRC` <
50,000 against medians of 355,328/300,736; gate-arm mean `WORK-H` 711,751). They
are 2.7% of the window and they drag P50/P95 *down*. Gate arm, gameplay-only
(441–1997): P50 1,100,096, P95 1,631,936 — 7,872 above the full-window P95, just
outside the ±5,376 floor. Both arms carry the identical tail, so cross-arm
comparison is unaffected; a single-arm figure should say which it quotes.

Boundary's own numbers: it trails the gate by 356,292. **Its `WORK-H` P95 reads
+13,568 against the older banked `f24f0cc1` figure of 1,463,104, and that move
predates this row** — this cycle's Boundary run reproduces the cycle-79 run
*bit-identically* on every shared bucket (`ALL` P50 1,119,872 / P95 1,680,192 /
mean 1,399,603, `named` 1,149,672, `SRC` P95 547,648), so the reseed did not
touch it. The delta sits between `f24f0cc1` and the c79/c80 builds and is
unattributed; treat 1,476,672 as the current Boundary baseline. The shipped ROM
remains
`smash64ds-battle-playable-hwtri.nds` (Boundary, mode 163). Label every figure
with its arm AND its coverage; never present a both-CPU figure as the Boundary
number (`Makefile:305-308`). Loading states are excluded from the gate by the
owner's stated rule: drop frames with `SRC` > 2× that arm's own `SRC` median.

Noise floors: `WORK-H` P95 cross-build ±5,376; per-bucket placement ≥8,544 —
buckets locate, `WORK-H` decides. 1.85 cycles of `FTR` mean per byte of added
ARM text: a change that adds text must beat its own footprint.

### The load-frame exclusion does NOT select loading states — do not apply it (cycle 81)

The banked figures above are correct **because they are taken with the
exclusion OFF**. `scripts/analyze-load-frame-exclusion.ps1` audits the rule
("drop frames with `SRC` > 2× that arm's own `SRC` median"); artifacts
`artifacts/performance/2026-08-05_c81-load-exclusion-audit-{bothcpu,boundary}.json`.
Four findings, all from banked CSVs, no new run:

- **The rule is circular for SRC.** It thresholds on the very bucket whose
  excursion is being attributed, so it shrinks SRC's share whether or not a
  load happened: applying it moves gate-arm `SRC` 68.9% → 52.3% and `MISC`
  25.7% → 39.6%. **Never rank SRC with it on.**
- **It is fragile.** Gate-arm gap by threshold: OFF 503,684, 1.5× 163,332,
  2× 237,956, 2.5× 309,252, 3× 384,964, 4× 484,292 — a **3.08× swing on the
  knob alone**.
- **The dropped frames are not loads.** 122 frames form 110 runs, **100 of
  them singletons**, longest 4, spread evenly (43 early / 42 mid / 37 late)
  and **0 overlap the GAME SET tail**. A loading state is a contiguous
  multi-frame event. Their other buckets are ordinary — `FTR` 1.01×, `STG`
  0.99×, `MISC` 1.04× — and only `SRC` is elevated (2.80×).
- **Cross-arm asymmetry settles it.** Both arms run the same stage, fighters
  and assets, so a real loading filter must bite similarly. It swings the gate
  arm 3.08× and Boundary only **1.09×** (356,292 → 327,236) — precisely
  because the gate arm's tail *is* `SRC`. The rule tracks the tail, not loads.

**Consequence: the gate is 503,684, not 237,956**, and the owner's intent
(exclude genuine loading) needs a rule keyed on an actual load signal — the
anim-cache warm step or the Task 75 asset-load counter — not on `SRC` itself.
That instrument does not exist yet; see the inherited row below.

## The diagnosis the lane was built on — **BOUNDARY-ONLY, re-priced 2026-08-05**

**Every figure in this section is a Boundary-arm figure.** It was banked without
an arm label, and cycle 79 measured it on the both-CPU arm the owner's gate
actually reads (G2a, commit `62fe823d`): the prize is 4–9× smaller there. Do not
quote these numbers as gate-arm numbers, and do not rebuild G3's case from them.

- **Effect DObj submits are the tail** *(Boundary)*: 99.3% of the `MISC`
  excursion, 359,717 ticks/frame on over-gate frames, **0** on clean ones. Net
  recoverable ~315,000 (part displaces `FTR`).
  **On the both-CPU gate arm: 71.5% of the `MISC` excursion, and `MISC` is only
  16.9% of the WORK-H excursion — so effect submits are ~12.1% of it. Measured
  recoverable 33,699–75,264, not ~315,000.**
- **The cost is a per-list constant** *(Boundary)* (~102,730 Exec ticks/list,
  1,360 lists/match, 16.1 tris/list): exact nine-phase partition — generic DL
  interpreter 65.57% (77,440/list), texture resolve 21.41% (25,289/list),
  Matrix 6.93% (8,179/list), everything else ~5.8%.
  **The constant does not hold on the gate arm: 527–563 lists/match at 83,632
  ticks/list (44,073,856 total, versus Boundary's ~139,714,000) — 41% of the
  lists and 81% of the per-list cost.**
- **The interpreter is honestly generic**: every list terminates at `G_ENDDL`
  (1,360/1,360, none at the 8192 cap), 160.1 commands/list at **626
  ticks/command**. No overrun to fix — **the precompiled-packet path is the
  answer, not a workaround.**
- **Dead, do not re-derive**: projectiles (44 ticks/frame median); particles
  (flat ~47K, a P50 lever, never the gate); texture thrash (1 upload/1,408
  frames — `Tex` is cache-*hit* key/hash/lookup cost); `Find` 0.44%; `Material`
  0.25%; `FTR` as the gate (anti-correlated with the tail); the `Tex`
  (dl-pointer, bind-ordinal) memo (built as approved: 4.56% hit rate, `Tex`
  went *up* 20% — reverted, flag deleted); L7 fixed-point collision (+534 won
  vs 6,481 lost to its own text); asset loads as the tail owner (refuted three
  times); Task 56 strips (ROM hangs the present loop — never completed a run).
- **The ROM has 96 BYTES of proven headroom — not 1.4 KB (corrected cycle 82).**
  The old "+1,408 boots, +2,208 does not" was a **delta over a datum build, and
  the datum moved every time the tree grew**: the tree had already spent 1,312
  of that 1,408 by cycle 80, so quoting the band on a later tree overstates the
  room by ~14x. Price it in **absolute `fake_heap_start`** (end of `.bss`, hence
  the heap base; address delta == text+data+bss delta, verified to the byte).
  Highest address ever proven to boot **`0x02294804`**; lowest proven to fail
  **`0x02294b24`**; the 800-byte band between them has never been bisected. The
  current gate-arm control links at **`0x022947a4`**. **Text counts as much as
  bss.** `scripts/check-boot-headroom.ps1 -Build <dir>` reads any build's ELF and
  returns OK / UNPROVEN / OVER CLIFF (exit 1) in under a second — run it after
  every lab build, then the 8-sample `-StartFrame 60` boot probe (~50 s) only if
  it says UNPROVEN. A failing arm never reaches presented frame 1 and the
  harness reports a timeout, which reads exactly like a hung emulator.

## THE GATE LANE — in order, one row live at a time

### G1 — MEASURED (cycle 79). Mechanism proven, gate unmoved. Not shipped.

`sNdsRendererStageTextureSites` (`nds_renderer.c:11086`), enabled in
`ndsRendererProfileSetOwner` (`nds_renderer.c:28819` — the old `29241-29247`
reference was stale by ~450 lines and is corrected here). Mode 9 now joins the
4/7/8 list behind `gNdsG1SiteCacheRoute`; **route 0 is the default and
reproduces shipped behaviour exactly**, so nothing about the shipped ROM
changed.

**The memo works, and the ~175-key working-set fear was wrong.** Whole match,
both-CPU, one binary, both routes (`builds/build-c79-g1-bothcpu`):

| | route 1 (on) | route 0 (off) |
|---|---:|---:|
| `Tex` per list | **7,203** | 20,780 |
| hit rate | **78.06%** (2,305/2,953) | — |
| overwrites / occupancy | **0** / 26 of 128 | — |
| WORK-H P50 | 1,089,024 | 1,095,552 |
| WORK-H P95 | 1,615,872 | 1,612,032 |
| over gate | 682/1600 | 710/1600 |
| `MISC` P95 | 396,096 | 473,536 |

`Tex`/list falls **65.3%** and `Exec` total falls **9,591,232 ticks/match**
(~5,994/frame mean). `MISC` P95 falls 77,440 — 9.1x the ≥8,544 bucket floor.

**But WORK-H P95 moved +3,840, INSIDE the ±5,376 floor: the gate did not
move**, and this closes none of the 485,060 gap. Buckets locate; WORK-H
decides. `ALL` P95 (+128) and the VBI histogram are unchanged, so pacing is
unchanged.

Two findings the next cycle should not re-derive:

- **The both-CPU gate arm exercises this path ~3.5x LESS than Boundary** —
  2,953 consults over 563 lists, against Boundary's 10,336 over 1,360. The
  banked 21.41% / 25,289-per-list `Tex` figure is a Boundary-config number.
  The gate arm is the *worst* case for any effect-texture lever, which is
  worth knowing before G3 is sized against it.
- **The refuted `(dl-pointer, bind-ordinal)` memo's failure does not
  transfer.** It took 471 hits on 10,336 with 7,517 evictions of 7,525 fills;
  this key takes 78% with **zero** evictions and 26 live slots. The site
  address points into static source display-list data and is stable across
  frames; the dl pointer was not.

**Open before this can ship:** the owner's visual gate on shield / revival
platform / impact wave / reflector (not run — see Inherited), and the byte
cost. The measured **+1,924 bytes (text +1,796, bss +128)** is the
*instrumented* build and sits INSIDE the +1,408/+2,208 cliff band; it is
dominated by six census counters inlined at several sites, not by the flip
(one condition). **The shipping cost is unmeasured** — G2 must measure the
flip alone, because G3's packet builder spends from the same budget.

### G2 — footprint map DONE (cycle 79). Failing allocation NOT yet named; 32 KB NOT yet demonstrated.

**EXIT MET, cycle 84 — ≥32 KB demonstrated at 4.2×.** `gSYZBuffer` gave back
**134,400 bytes**; proven static headroom went **96 → 134,496 bytes**
(`build-c84-zbuffer` links at `0x02273aa4` against `0x02294804` highest-booting).
Boot probe PASS (frames 60–67, 8 samples, slips 0), Boundary green, root ROMs
unchanged, `decomp/` verified untouched.

**Report the two shortages separately — they are coupled through the heap.**
Freeing bss lowers `fake_heap_start`, which enlarges the heap the arena callocs
from, so the arena ate **131,072** of the 134,400 and its deficit is now closed:
`gNdsTaskmanArenaChosenSize` 1,245,184 → **1,376,256** (its full `0x150000`
request, first time ever) and `gNdsTaskmanArenaAllocFailCount` **32 → 0**. That
is the predicted engagement to the digit. **Consequence for the next spender:**
static headroom and arena health are the same 134,400 bytes seen twice — an
instrument that adds N bytes re-starves the arena by ~N (quantised to `0x1000`)
long before it reaches the boot cliff. The SRC ring buckets cost 1,040, so they
now fit with ~129× margin *and* leave the arena at full request.

Authoritative, from the **shipped** ROM's matching ELF pair
(`smash64ds-battle-playable-hwtri.elf`, Aug 4 20:33, pairs with the published
`.nds`). Sections: **text 891,836 / data 147,712 / bss 1,709,640**.

Top `.bss`, which is where the budget actually is (symbol total 1,709,401 of
1,709,640 — so the ranking is essentially complete, not a sample):

| symbol | bytes | share of bss |
|---|---:|---:|
| `gSYFramebufferSets` | 441,600 | 25.8% |
| `sNdsAudioFgmCache` | 204,800 | 12.0% |
| `sNdsRelocSceneFileBuffer` | 185,696 | 10.9% |
| **`sOriginalSpritePreview`** | **153,600** | **9.0%** |
| **`sOriginalSpriteDisplayPreview`** | **153,600** | **9.0%** |
| `gSYZBuffer` | 140,800 | 8.2% |
| `sNdsRendererHardwareTextureScratch` | 32,768 | 1.9% |
| `sNdsRendererTask36ReplayOwner` | 30,880 | 1.8% |
| `sNdsRelocLoadedFiles` | 29,184 | 1.7% |
| `sNdsFighterDLAllDrawStates` | 27,136 | 1.6% |

The top six are **74.9% of all bss**. Top `.text`:
`ndsResetStartupDiagnostics` 33,260, `__dldi_start` 16,384, `categories`
14,328, `ndsRendererHardwareResolveOrBindTexture` 10,944,
`ndsRendererPrepareNativeStageOwner` 10,888, `ndsOpeningRoomRenderDLPreview`
8,756. Top `.data` is `gNdsParticleScriptBank` 10,912 — note `nm` reports
`__sp_usr` at 184,600,960, which is an absolute stack address and **not** a
size; exclude it from any ranking.

**Leading candidate, NOT yet verified removable.** The two sprite preview
buffers total **307,200 bytes (300 KB, 18% of bss)** — nearly 10x the 32 KB
exit on their own, and `sOriginalDLPreview` (13,824) plus
`sOriginalDLDisplayPreview` (7,776) add 21,600 more. All four are declared
**unguarded** in `src/nds/nds_platform.c:114/196/202/205`, so they are
allocated in every configuration including the shipped battle ROM, even if
the battle scene never populates them. `src/port/port_probe.c:53` says the
"original asset previews now own the top-screen visual signal", which is a
**dev preview** role.

**MEASURED AND REFUTED (same cycle, zero build).** Read deep in battle
(frame 1200) on the existing tick-HUD ROM:

```
gNdsOriginalSpritePreviewReady = 1      gNdsOriginalDLPreviewReady = 0
```

**`sOriginalSpritePreview` is populated and in use during battle**, so the
153,600-byte buffer — and by association its 153,600-byte display twin — is
**not** a free deletion. The 300 KB headline is dead. This is exactly why the
flag was read instead of trusting the symbol name and the "dev preview"
comment: deleting on the name would have removed something battle actively
uses, which is the name-driven-logic failure mode in its purest form.

**What survives as a candidate: 21,600 bytes.** `gNdsOriginalDLPreviewReady`
is **0** in battle, so `sOriginalDLPreview` (13,824) and
`sOriginalDLDisplayPreview` (7,776) are not populated there. That is 21,600
bytes — **below the 32 KB exit on its own**, so G2 needs at least one more
source. Note a ready flag is a *state*, not proof of never-use: it shows the
buffer is unpopulated at that moment, not that no configuration ever fills
it. Confirm with a config-level trace before removing, and remember `.bss` is
static — the lever is deleting or `#if`-guarding the buffer out of the battle
configuration, never freeing it at runtime.

**ALL FOUR LARGE TARGETS NOW TRACED (cycle 83). Two refuted, two live, nothing
freed yet.** Trace = who writes it, who reads it, in which configuration.

- **`gSYFramebufferSets` 441,600 — LIVE, do NOT delete.** Three N64 software
  framebuffers (`[3][230][320]` u16, `include/sys/video.h:55`). It **is
  dereferenced on DS**: `decomp/…/src/lb/lbtransition.c:228` reads through
  `gSYSchedulerCurrentFramebuffer`, and `src/import/battleship_lbtransition.c:47`
  deliberately points that at `&gSYFramebufferSets[0]` when NULL — the photo wipe
  into **VS Results, which is in P1 scope**. Also fully cleared by the patched
  `scmanager.c:855-865` loop (NDS arm bounded by `sizeof`, N64 used
  `end = 0x80400000`). **Open question, and it is a FIDELITY question, not a free
  deletion:** the DS renders through GX into VRAM, so the readback may be
  sampling the cleared black — exactly the failure libultraship documented in
  `decomp/…/port/bridge/framebuffer_capture.h:19`. Answer that before sizing it.
- **`gSYZBuffer` — FREED, 134,400 bytes, cycle 84.** An N64 *software* Z-buffer
  on hardware whose depth buffer is in VRAM. Reduced 140,800 → 6,400 bytes
  (`320*10`, the border its own `SYVIDEO_ZBUFFER_START` arithmetic names) in
  `src/import/battleship_sys_zbuffer.c` + the matching extern
  `include/sys/video.h:54`. **No decomp patch was needed** — the import file was
  a one-line `#include` of decomp's text, so the port already owned the TU;
  `fetch-battleship-reference.ps1 -VerifyOnly` confirms `decomp/` untouched.

  **The verdict rests on measurement, not on "no dereference found".** Gate-arm
  control, frames 600–607, DLDI on
  (`artifacts/performance/2026-08-05_c84-zbuffer-liveness.json`): `gSYZBuffer`
  sampled at **9 points across all 70,400 halfwords — every one 0**, untouched
  `.bss` deep in gameplay. **The `−6,400` offset is explained, and it was the
  real hazard:** `gSYFramebufferSets` ends at `0x02211cd0`, *exactly*
  `gSYZBuffer`'s address, so `SYVIDEO_ZBUFFER_START` = `gSYZBuffer − 6400` points
  into the tail of `gSYFramebufferSets[2]` — a **live** buffer feeding the VS
  Results photo wipe. Those 3 border samples still read **1**, so nothing writes
  through the start pointer either. **Control, same run, same instant:**
  `gSYFramebufferSets[0][0][0]`, `[1][115][160]`, `[2][0][0]` all read **1** —
  the clear loop's `GPACK_RGBA5551(0,0,0,1)`. The probe demonstrably sees writes
  (0→1) in the neighbouring bytes and sees none here.

  Consumers, all enumerated and classified: `SYVIDEO_ZBUFFER_START` computes;
  `video_bootstrap.c:19` and the imported scene setups **store**; `syVideoInit`
  **stores** into `gSYVideoZBuffer`; `video_bootstrap.c:34` **compares**.
  **Zero dereferences**, and nothing takes `sizeof(gSYZBuffer)` — which is what
  makes a size reduction safe. Engagement proof that the surviving consumer
  still works: `gNdsVideoBootstrapResult` reads `0x56494430`
  (`NDS_VIDEO_BOOTSTRAP_PASS`) after the change, and that check can fail
  (`0xBAD00001`).
- **`sNdsRelocSceneFileBuffer` 185,696 — REFUTED as free.** Already
  harness-guarded (`reloc_backend_assets.c:352-358`): in harness builds the union
  is sized exactly `CASTLE_STATIC + BANK_113_STATIC` and serves as the **battle**
  static-asset staging store. This is the 2026-07-16 PORTING optimization,
  already banked; there is no second helping.
- **`sNdsAudioFgmCache` 204,800 — REFUTED without an owner decision.** A live
  8-slot FGM sample cache; capacities `{53,248, 3×28,672, 4×16,384}`
  (`nds_audio_fgm.c:440-443`) sum to exactly 204,800. Shrinking it trades audio
  fidelity — charter §7, the owner's call, not this row's.

### The failing boot-time allocation IS NAMED (cycle 83), and it gives G2 a real ceiling

`sNdsTaskmanArenaBytes = calloc(1, arena_size + 0x10)` at
`src/port/diagnostics.c:7646`. It requests `NDS_TASKMAN_ARENA_SIZE` = `0x150000`
(1,376,256) and walks **down** in `0x1000` steps to a floor of `0x130000`. Read
off the booting gate-arm control (`build-c80-gate-bothcpu`, 8 samples, frames
60–67, artifact `artifacts/performance/2026-08-05_c83-arena-ceiling.json`):

```
gNdsTaskmanArenaChosenSize     = 1,245,184   (0x130000 -- the FLOOR)
gNdsTaskmanArenaAllocFailCount = 32
```

1,376,256 − 1,245,184 = **131,072 = exactly 32 steps of 0x1000**, matching the
fail count to the digit. **The arena is pinned at the bottom rung of its own
ladder, 128 KB short of what it asks for, on a ROM that boots.** It degrades
instead of freezing only because the ladder exists; below it sit
`0x100000/0xc0000/0x80000/0x40000` and then the §3.11 `syMallocSet` spin.

**This is a better G2 instrument than the boot probe, because it is continuous
rather than binary.** Free N bytes of static footprint and
`gNdsTaskmanArenaChosenSize` should rise by ~N (quantised to 0x1000) while
`gNdsTaskmanArenaAllocFailCount` falls by ~N/4,096 — a zero-ambiguity engagement
proof for any candidate, readable with `-ExtraGlobals` in ~50 s and **no build**.
Pair it with `check-boot-headroom.ps1`; the arena number says how much the free
was *worth*, the headroom check says whether the build still boots. Note the
first 131,072 bytes freed are absorbed by this deficit before the request is even
met, so **G2's ≥32 KB exit buys arena, not slack** — the boot cliff and the arena
starvation are two separate shortages and both are real.

**Not done this cycle:** the +2,208 failing allocation is **not named** — that
needs a `fake_heap_start` build plus a gdb probe for the `syMallocSet` spin,
and naming it by inference was explicitly out of scope. No headroom freed, no
32 KB demonstrated, no build made for this row.

### G2 (original row) — RAM headroom before any new code lands

The boot cliff blocks every candidate that adds text or data (it is what
actually killed the Tex memo arm). Produce the authoritative footprint map:
rank `.text`/`.data`/`.bss` and fixed pools from the map file; identify which
boot-time allocation fails at +2,208 (the failing arm dies before frame 9, so
it is a boot/scene-entry peak, not steady state); free or defer the cheapest
candidates. **Exit: ≥32 KB static headroom demonstrated by the same
`fake_heap_start` probe**, so G3's builder text plus arena bookkeeping fit with
margin. No performance claim — this row is measured in bytes, not ticks.

### G3 census — TAKEN ON THE CORRECTED WINDOW, BOTH ARMS (cycle 87). The arena is 8 templates.

**The sizing input existed only as an instance count until now.** Every G3 figure
on this board counts list *instances*; the arena is sized by *unique templates*,
and nobody had counted those. Measured with `gNdsEffectDLCensus*`
(`reloc_backend_renderer_dl.c`, all behind `#if NDS_TICK_HUD`), whole match,
1,600 samples, frames 442–2041, **86.7% coverage, DLDI on, exclusion OFF**.
Builds `builds/build-c87-census-{boundary,bothcpu}`; artifacts
`artifacts/performance/2026-08-05_c87-census-{boundary,bothcpu}.json`.

| | **both-CPU (gate)** | **Boundary (shipped)** |
|---|---:|---:|
| **unique templates** | **8** | **8** |
| **command total over the 8** | **669** | **669** |
| **max commands in one template** | **336** | **336** |
| overflow / state variants / cmd variants | 0 / 0 / 0 | 0 / 0 / 0 |
| instances / match | 581 | 1,366 |
| reuse factor | 72.6x | 170.8x |
| commands / instance | 111.2 | 159.5 |
| Exec ticks / instance | 80,394 | 101,359 |
| texture ticks / instance | 19,406 (24.1% of Exec) | not read this run |
| triangles / instance | 13.65 | 16.07 |
| vertices / instance | 40.95 | 48.2 |
| terminated at `G_ENDDL` | 581/581 | 1,366/1,366 (0 at cap) |

**THE TEMPLATE SET IS ARM-INDEPENDENT.** Uniques, command total and command max
are *identical* on both arms while instances differ 2.35x. The 8 templates are a
property of the owner-approved effect model set, not of match dynamics — so a
fixed arena's size does not depend on how hard the match is, which is what makes
it safe to fix at build time.

**The pointer is a COMPLETE key: `StateVariants` 0 and `CommandVariants` 0 on
both arms.** No template was ever submitted under a different entry
`othermode_l`, and none ever produced a different command count. One packet per
unique template suffices; no state multiplexing, no per-instance variation to
encode. This was the open question that decided whether the arena is sized by 8
or by 8 x (number of entry states).

**The interpretation ratio is 325.7x on Boundary** (669 distinct commands
executed 217,920 times) and 96.6x on the gate arm.

Cross-checks, all independent: the run reproduces Boundary's banked per-list
constants (160.1→159.5 commands, ~102,730→101,359 ticks, 16.1→16.07 tris,
1,360→1,366 lists, all within 1.3%), and **the gate-arm figures the cycle-80
caveat flagged as 12.6%-window artefacts largely HOLD** — 527–563 lists → 581,
83,632 ticks/list → **80,394** (−3.9%). Vertices are exactly 3.000x triangles on
both arms: every triangle is an independent 3-vertex submit, no strip reuse.

Instrument cost **+3,488 bytes** (text +384, data 0, bss +3,104), headroom
130,976 → **127,488** (36.5x margin), `fake_heap_start` 0x02274864 → 0x02275604
— the address delta equals the section delta to the byte. Boot probe PASS
(frames 61–68, 8 samples, slips 0); arena `ChosenSize` 1,376,256 → 1,372,160 and
`AllocFailCount` 0 → 1, i.e. exactly one 0x1000 step, the predicted re-starve.
**The shipped ROM pays none of it** — every part sits inside
`diagnostics.c`'s `#if NDS_TICK_HUD` (3014–3150) and the published target builds
`NDS_TICK_HUD 0`; the `NDS_TICK_HUD=0` configuration was link-checked via
`smash64ds-battle-playable-proof-hwtri`.

Both census arms are identity-checked against their banked baselines: Boundary
WORK-H P95 1,480,576 vs 1,476,672 (+3,904) and gate 1,621,696 vs 1,624,064
(−2,368) — both inside the ±5,376 floor, so the instrument does not move what it
measures. **Neither is a new baseline; 1,624,064 still stands.**

**THE ARENA CONSTANT IS MEASURED (cycle 87, step 0). 83 triangles / 249
vertices / 669 commands over the 8 templates — identical on both arms.**
Artifacts `artifacts/performance/2026-08-05_c87-geomcensus-{boundary,bothcpu}.json`.

| | gate | Boundary |
|---|---:|---:|
| tris over the 8 templates | 83 | 83 |
| verts over the 8 templates | 249 | 249 |
| `GeomVariants` / instances | 2 / 581 | **0** / 1,366 |

`hardware_triangle_count` is POST-CULL, so per-instance geometry can vary and a
packet must encode the template's whole content — the census therefore records
the **max** per template, not a first sighting. `GeomVariants` is its confidence,
and at 0 (Boundary) and 2 of 581 (gate) the geometry is frame-invariant, so the
maxima are exact rather than a lower bound. Verts are exactly 3x tris on both
arms, consistent with the per-instance 3.000x ratio.

**The inference this replaced was 20% low** (~67 tris / ~200 verts from the 1.91x
command skew), which is why §3.11 requires the arena constant to be measured: an
undersized arena is a freeze, not a slowdown.

Packet-byte estimate on the measured basis: 249 verts x 16 B (VTX_16 two words +
TEXCOORD + COLOR) = 3,984, plus <=83 BEGIN_VTXS (332), plus per-template state
(matrix 4x3, polygon format, texture params, colours ~80 B x 8 = 640), plus
packed-command-byte overhead — order **6 KB for the entire effect set**. A 16 KB
fixed arena is 2.6x margin; 32 KB is 5.2x. Against 125,248 bytes of proven
static headroom this is not a constraint.

**8 remains a LOWER bound on templates**: the census counts templates actually
submitted, so the builder must enumerate the closed effect-model set at match
load and build eagerly. Lazy discovery would be gameplay-time allocation, and
§3.11 makes that a freeze. Overflow policy: size at build time with a
compile-time assert, and fall back to the interpreter for any list not in the
prebuilt set — correctness-preserving and allocation-free.

### G3 step 1 — THE PACKET DESIGN IS REFUTED AS BRIEFED (cycle 88). Effect geometry is the per-instance data, not the invariant.

**The row assumed geometry is template-constant and that matrix + colour are the
per-frame patch. Measured, it is exactly inverted.** Boundary arm, frames
900–907, 645 effect list instances over 7 templates, DLDI on, build
`builds/build-tick-hud-buckets`; artifact
`artifacts/performance/2026-08-05_c88-effect-packet-stream-boundary.json`.
The instrument hashes the captured GX word stream per list in three classes and
compares each against that template's first sighting:

| | matches | variants |
|---|---:|---:|
| **geometry** (VERTEX16 + TEX_COORD + BEGIN/END/POLY_FMT) | **0** | **638** |
| colour | 638 | 0 |
| matrix | 638 | 0 |
| geometry **word count** | — | **0** |

638 = 645 − 7, i.e. **every comparable instance varied, 100%**, while the stream
LENGTH never varied once. Colour and matrix words are byte-identical across
instances and are the positive control: the comparator demonstrably detects a
difference (638 times) and demonstrably reports agreement (1,276 times), so
"geometry always differs" is a reading and not a broken hash.

**What it means.** If the hardware were transforming, every instance of a
template would emit identical model-space vertices and the MATRIX would carry
the per-instance difference. The matrix is constant and the vertices move, so
**the transform is already baked into the vertex words** — effect lists take the
CPU-projected submit shape (`ndsRendererHardwareClipVertex` divides x/w and y/w
and emits screen-space v16). A packet captured from that stream pins the effect
to the position and camera of the capture instance. **This is R2-02 E3's "smear
of specks" failure, measured on the effect layer for the first time.**

Patching "the matrix and the dynamic colour words" therefore patches the two
things that were already constant, and leaves all ~235 geometry words per list —
the actual per-instance payload — unpatched. **The briefed design cannot work on
the current submit path**, and no arena, capacity or overflow policy changes
that.

**The precondition this creates, and it is the real G3 row now:** effects must
first be moved from the projected submit shape to the raw one (load the
composed matrix into GX once per list, emit model-space vertices). Then geometry
becomes template-constant and the packet design works with the matrix as the
per-instance patch, exactly as briefed. `ndsRendererHardwareClassifySubmit`
(`nds_renderer.c`) returns `PROJECTED_NO_Z` on its **first** predicate when
`source_zbuffered == FALSE`, before any range or matrix-compatibility check, so
effects plausibly never reach the raw path at all — that attribution is read
from the code and is NOT yet measured; the class histogram is the next probe.

**The mechanism G3 wants already exists: Task 36 replay.** `NDS_TASK36_HW_COMPOSE
== 2` is already on in the tick-HUD ROM. `NDSRendererTask36ReplayOwner`
(`nds_renderer.c`) is a fixed 4,608-word static arena with per-run word offsets,
a segment admission mask, capture/replay states and a real packed DS display
list — `ndsRendererTask36ReplayOpcode` encodes FIFO opcodes with parameter
counts and deliberately drops the state classes its BeginRun re-issues live.
Its `NDS_TASK36_REPLAY_SEGMENT_MASK` comment already states the rigid-versus-
dynamic law this measurement just confirmed for effects. **Do not build a second
packet arena; admit effects to this one once their stream is rigid.**

Instrument: `gNdsEffectPacket*`, all behind `#if NDS_TICK_HUD`, hooked into the
GX record funnel in `nds_renderer.c` and compared beside the census in
`reloc_backend_renderer_dl.c` (same key, same population). Cost **bss +5,032,
text ~+912** (nm-measured); headroom 114,272 proven, `check-boot-headroom.ps1`
OK. **The shipped ROM pays nothing** — the `NDS_TICK_HUD=0` link check
(`smash64ds-battle-playable-proof-hwtri`) contains **0** `EffectPacket` symbols.
Engagement is exact rather than plausible: capture count 645 equals
`gNdsEffectDLSubmitCount` 645, templates 7 equals `gNdsEffectDLCensusUnique` 7,
and the capture's own VERTEX16 command total **31,941 equals the renderer's
independent `gNdsEffectDLVertexTotal` 31,941** — the capture sees the effect
layer's complete vertex stream and nothing else. Verts/tris is exactly 3.000,
reproducing the census.

### G3 step 2 — THE DECIDING PREDICATE IS NAMED (cycle 89). It is the FIRST one, and it is load-bearing.

**100.000% of effect triangles are refused the raw path by
`source_zbuffered == FALSE`, the first test in
`ndsRendererHardwareClassifySubmit`.** Boundary arm, frames 900–907, DLDI on,
build `builds/build-tick-hud-buckets`; artifact
`artifacts/performance/2026-08-05_c89-effect-submit-class-boundary.json`.
Binned at each return site, so the index names the *predicate*, not merely the
resulting class (the two sites returning `PROJECTED_RANGE_OR_MATRIX` are split
by hand into bins 3 and 4 for that reason):

| bin | predicate | effect triangles | share |
|---|---|---:|---:|
| **0** | **`source_zbuffered == FALSE`** | **10,647** | **100.000%** |
| 1–4 | decal / prim depth / range reject / matrix reject | 0 | 0% |
| **5–6** | **RAW current + RAW snapshot (the raw path)** | **0** | **0%** |
| 7 | cross-matrix | 0 | 0% |

**Not one effect triangle reaches the range or matrix checks.** The exclusion is
one predicate deep, and the range/matrix conditions are untested rather than
failing — so nothing is known about whether effect geometry would satisfy them.

Engagement is exact: `gNdsEffectSubmitTotal` **10,647 == `gNdsEffectDLTriangleTotal`
10,647**, the renderer's own independent effect triangle count, so the histogram's
population is precisely the effect triangles and nothing else. The cycle-88
result reproduces on this separately linked ROM (SHA `1EA9CE6E` vs `1E06AFAF`):
645/645 captures, 638 geometry variants, 638 colour matches, 31,941 vertices.

**Why the predicate is load-bearing, and this is the part that decides the row.**
`source_zbuffered` is the source display list's own `G_ZBUFFER` geometry-mode bit
(`nds_renderer.c`, `stats->geometry_mode & NDS_RENDERER_GEOM_ZBUFFER`) — a
BattleShip asset property. The port's own comment states the constraint:
**"The DS cannot disable depth testing per polygon."** So for non-Z geometry it
reproduces N64 painter-order by handing every triangle its own monotonically
descending depth (`sNdsRendererHardwareProjectedDepth`, step 6) and **emitting
that z explicitly** — which requires the CPU to own the vertex, hence the
projection, hence cycle 88's per-instance geometry. The projected path is not an
oversight to be flipped; it is how draw order is reproduced on hardware with no
per-polygon depth-test disable.

**Consequence: there is no raw path for non-Z geometry today.** Bin 0 is not a
gate that effects fail, it is a gate they are not eligible for. Making effects
raw means giving them hardware-computed depth in place of the port's synthetic
painter order — a **visual** change to layering, and therefore the owner's call.
See the escalation below; do not route effects to the raw path without it.

### G3 step 3 — OPTION B IS SMALL, AND THE COST IS GENUINELY THE PROJECTION (cycle 90)

Boundary arm, frames 900–907, 645 instances, DLDI on, build
`builds/build-tick-hud-buckets`, ROM `573F4F41`; artifact
`artifacts/performance/2026-08-05_c90-effect-exec-split-boundary.json`. Three
spans measured inside the existing Exec bracket, armed by the cycle-88 flag;
**traversal is DERIVED** as `Exec − TexInExec − Vtx − Tri`, so a negative
residual would disprove the nesting. Residual **+16,802,176, non-negative.**

| span | ticks | share of Exec | per instance |
|---|---:|---:|---:|
| **Tri** (classify + clip divides + painter depth + GX emit) | 63,044,032 | **70.95%** | 97,743 |
| **Traversal** (derived: walk, dispatch, state, in-Exec texture) | 16,802,176 | **18.91%** | 26,050 |
| **Vtx** (G_VTX transform) | 9,006,080 | 10.14% | 13,963 |
| Exec | 88,852,288 | 100% | 137,755 |

**The bias correction closes on the banked figure to one tick, which is the
cross-check that makes this usable.** The brackets charge a timer read to the
span they wrap, and Vtx/Tri fire ~33x per list against Exec's once — so Exec
reads 137,755/instance against the banked **101,359**. Charging the whole
36,396 excess to Tri+Vtx gives 75,310, and 75,310 + 26,050 = **101,360 vs
101,359 banked.** Traversal is unaffected by the correction because it is
derived.

**Option B's recoverable, both ends measured:** **low 19,167/instance (18.91%
of banked Exec)**, **high 26,050/instance (25.7%)**. So B removes at most about
a quarter of Exec and **74.3% of the cost is per-vertex and per-triangle work B
must still pay every frame** — because cycle 88 proved the vertex words are the
per-instance payload. B is real but small, and it is small for the decisive
reason rather than an incidental one.

Engagement exact: `gNdsEffectPhaseTriCount` **10,647 == `gNdsEffectDLTriangleTotal`
10,647**; capture count 645 == `gNdsEffectDLSubmitCount`.

**Instrument defect, stated rather than papered over:** `gNdsEffectPhaseTexInExecTicks`
read **0**. There are three texture-resolve entry points charging
`gNdsEffectPhaseTexTicks`; only one was given the in-Exec twin and it never
fires while armed. In-Exec texture resolve is therefore folded into the derived
traversal residual. That is *correct for B's accounting* — texture resolve is
template-invariant, so a packet removes it — but it means the texture sub-share
is not separately reported, and `gNdsEffectPhaseTexTicks` (18,086,656) spans
work outside Exec so it cannot be substituted.

**THE GATE ARM IS NOW RUN (cycle 91), AND THE TEXTURE TWIN CHANGES THE
COMPOSITION — not B's size.** Whole match, both-CPU, 1,600 samples, frames
439–2038, **86.7% coverage**, DLDI on, exclusion OFF. Two runs: the as-built
`build-c90-split-bothcpu` (ROM `1A91A4A4`, artifact
`artifacts/performance/2026-08-05_c91-effect-split-bothcpu.json`) and, after the
`TexInExec` fix, `build-c91-slots-bothcpu`
(`...c91-painter-slots-bothcpu.json`). Figures below are the fixed build.

| span | ticks | share of Exec | per instance |
|---|---:|---:|---:|
| **Tri** | 41,091,968 | **70.98%** | 70,726 |
| **TexInExec** (was 0) | 12,506,624 | **21.60%** | 21,526 |
| **Vtx** | 5,142,656 | 8.88% | 8,851 |
| spans sum | 58,741,248 | 101.47% | — |
| **derived traversal residual** | **−848,384** | **−1.47%** | — |
| Exec | 57,892,864 | 100% | 99,643 |

**The residual is NEGATIVE, which is the instrument's own designed failure
signal, and it fires for the right reason.** With `TexInExec` honest, Tri + Vtx
+ Texture alone exceed Exec by 1.47%: every span charges its own timer reads and
Tri/Vtx/texture each fire many times per list against Exec's single bracket, so
the raw shares are biased high. The bias correction is mandatory, not optional.
**Consequence: the 18.91% "traversal" the Boundary run reported was mostly
texture resolve.** True walk/dispatch/state traversal is ≈ 0.

**Option B on the gate arm = `Exec − Tri − Vtx`** (a packet removes traversal
*and* the template-invariant texture resolve): **11,658,240 ticks/match,
20.14% of Exec**, i.e. **16,189–20,066 ticks/instance** (low end = that share of
banked Exec/instance 80,394; high end = measured). Boundary was 19,167–26,050.
Over 1,600 presented frames that is **5,879–7,286 ticks/frame — 1.2%–1.4% of the
503,684 gap.**

**And the prize is one G1 already tried.** `TexInExec` is 21,526 ticks/instance;
G1 measured route-0 `Tex` at 20,780 per list and cut it **65.3%** — and WORK-H
P95 moved +3,840, **inside the ±5,376 floor**. Two independent instruments price
the same cost, and a two-thirds cut of it has already been measured to be worth
nothing at the gate. **B is not a gate lever on the arm the gate reads on.**

**Neither run is a baseline.** The phase instrument perturbs what it measures:
WORK-H P95 reads 1,639,872 (c90) and 1,669,632 (c91) against banked 1,624,064 —
+15,808 and +45,568, both far outside the ±5,376 floor. **1,624,064 still
stands.** Engagement is exact on both: `gNdsEffectPhaseDLCount` 581 ==
`gNdsEffectDLSubmitCount` 581 == the c87 census instance count, and
`gNdsEffectDLCensusUnique` 8. One honest mismatch: `gNdsEffectPhaseTriCount`
7,946 vs `gNdsEffectDLTriangleTotal` 7,930, **+16 (0.20%)** — the two closed
exactly on Boundary (10,647 == 10,647), so the gate arm has 16 triangles the
phase bracket counts and the census does not. Unattributed, and too small to
move any figure above.

**The `TexInExec` defect is FIXED (cycle 91).** Three texture-resolve entry
points charged `gNdsEffectPhaseTexTicks`; only `ResolveResidentTexture` had the
in-Exec twin and it never fired. `ndsRendererHardwareBindTexture` and
`ndsRendererHardwareResolveStageSourceFrameTexture` now carry it too. It reads
**12,506,624 == `gNdsEffectPhaseTexTicks` 12,506,624, i.e. 100% of effect
texture-resolve time is inside the Exec bracket.** That equality is informative
rather than tautological: `sNdsEffectPacketArmed` wraps only the
`ndsRendererExecuteDisplayListWithVertexCache` call
(`reloc_backend_renderer_dl.c:9492/9508`) while `gNdsEffectPhaseActive` wraps the
whole tree walk (9856/9861), so the twin's condition is strictly narrower and
*could* have differed.

### G3 step 4 — STATIC PER-LAYER WORLD Z CANNOT REPRODUCE THE CURRENT ORDER (cycle 90)

The owner's proposal — bake a fixed distinct world-Z per effect layer and let
the depth test reproduce painter order by construction, giving A's win without
A's fidelity cost. **Verdict: no, not as stated, and none of the fighter-Z
measurements is what kills it.** Read from the specification, no build:

- **The scheme orders per PRIMITIVE, not per layer.**
  `ndsRendererHardwareNextProjectedDepth` decrements by `STEP` and returns
  `counter / STEP`, giving **every no-Z primitive its own depth slot in
  submission order**. Its comment records the exact failure a per-layer
  constant would reintroduce: *"Subtracting one here made six consecutive no-Z
  triangles share a depth after division, allowing an earlier stage triangle to
  reject a later grass/bush draw."* Sharing a depth across primitives is a
  **known, already-observed rendering bug**, not a hypothetical.
- **The order is dynamic and scene-dependent.** The depth a given effect
  receives depends on how many painter primitives preceded it *that frame*, and
  on whether the first source-Z triangle has flipped the counter via
  `ndsRendererHardwareEnterProjectedForeground` into its foreground range. A
  static constant cannot reproduce an order defined by submission position.
- **Precision budget, from the constants — BOTH NUMBERS IN THIS BULLET WERE
  WRONG. Corrected and measured in step 5 below; do not quote this bullet.**
  The reserved band is **128 slots per endpoint, not 4,096** (the `0x1000` is
  the v16 representation of clip-space 1.0, not a slot count; the literal `128`
  in `FOREGROUND_START` is the band width, and
  `ndsRendererHardwareSourceDepthToV16`'s comment says so outright). And effects
  consume **~3.9 primitives/frame, not 1,331** — that figure divided
  `gNdsEffectDLTriangleTotal`, which is cumulative from boot (one write site, a
  `+=`, no reset anywhere), by an 8-frame window. The two errors run in opposite
  directions; the conclusion "the slots are already substantially spent" is
  **refuted by measurement** in step 5.

**Consequently fighter-Z constancy (the owner's premise) is not the binding
constraint and was not measured.** It would matter only if the target were
per-object ordering; the specification is per-primitive. Reporting that the
premise is untested is the honest form — it may well be true and still not help.

**A variant does survive and is worth pricing, but it is a hypothesis, not a
result.** Bake each template's per-triangle ordering into its model-space Z
(the template's primitive set is fixed — 83 triangles over 8 templates), and
carry the per-instance base depth in the **patched matrix's Z translation** —
which is the one patch the original G3 design already called for. That
reproduces per-primitive order by construction *if* base offsets are assigned
in submission order. **ANSWERED, cycle 91 — see step 5. The precision budget is
comfortable and the variant fails anyway, for a structural reason that no amount
of depth resolution fixes.**

### G3 step 5 — DEPTH PRECISION IS NOT THE CONSTRAINT, AND THE VARIANT IS DEAD ANYWAY (cycle 91)

**Verdict: NO.** Not on precision — on mechanism. The A-branch is closed.

**Measured first, because it refutes the stated reason for doubt.** New census
`gNdsPainterSlot*` (`nds_renderer.c`, all `#if NDS_TICK_HUD`), derived from the
depth counter itself rather than incremented in
`ndsRendererHardwareNextProjectedDepth`, because the M3 replay path decrements it
in bulk (`triangle_count * STEP`) without calling the accessor. Folded once per
renderer hardware frame. Gate arm, whole match, 1,600 samples, frames 439–2038,
86.7% coverage, DLDI on, build `builds/build-c91-slots-bothcpu`; artifact
`artifacts/performance/2026-08-05_c91-painter-slots-bothcpu.json`.

| | measured | band | worst-frame use |
|---|---:|---:|---:|
| background slots, max frame | **72** | 128 | **56.2%** |
| foreground slots, max frame | **107** | 128 | **83.6%** |
| frames over either band | **0 / 2,044** | — | — |
| background / foreground mean | 71.8 / 57.8 | — | — |

Engagement: `gNdsPainterSlotFrames` **2,044** tracks the 2,038 presented frames,
so the fold ran once per hardware frame across the whole match. **Zero over-band
frames in 2,044 folds**, and 21 free slots in the tighter band at the worst
frame of the match. The budget is not spent.

**Where the ordering primitive comes from, and why that is fatal.** Painter
primitives are emitted through `ndsRendererHardwareClampS64ToV16(projected_z)`
(`nds_renderer.c`) — the slot **integer goes straight into the vertex z with no
perspective divide** — and the projected path loads **identity for both
projection and modelview**, so clip `w` is exactly 1.0 and the clip test
`|z| ≤ w` becomes `|z_v16| ≤ 4096`. That is the whole mechanism: integer slots,
uniform spacing, no perspective crowding, and the DS is in **Z-buffering** mode
(`glFlush(GL_TRANS_MANUALSORT)`, bit 1 clear).

**The packet and the painter order are mutually exclusive.** The integer depth
slot exists *only* because the CPU owns the vertex on the projected-identity
path — and CPU-owning the vertex is exactly what makes effect geometry
per-instance and un-packetable (cycle 88, 638/638). Moving effects to the raw
path to make geometry template-constant replaces identity with `RAW_COMPOSED`, a
real perspective matrix: the emitted z is then divided by a per-vertex `w`, so
"one integer slot per primitive" ceases to exist. A baked model-space Z and a
matrix Z-translation preserve *order* (the map is monotonic in view Z) but not
*spacing* — the separation two baked offsets produce depends on the instance's
distance from the camera, which a build-time bake cannot know. Solving for the
offset per instance per frame is precisely the per-frame CPU work the packet
existed to remove.

**So the deciding question was mis-aimed, and both of its inputs were wrong**
(step 4 bullet 3, now corrected): the budget is 128 per band rather than 4,096,
and demand is ~3.9 effect primitives/frame rather than 1,331. Fixing both makes
the precision picture *better*, and the variant still fails.

**Not measured, and not needed:** whether a single effect list ever straddles the
`ndsRendererHardwareEnterProjectedForeground` switch. Background use is
essentially constant (mean 71.8, max 72) so the switch fires at a stable point
every frame, but a static intra-template Z could not express that discontinuity
anyway. **Also still true and still the owner's call** (cycle 89): routing
effects to the raw path at all is a visual change to layering.

**Consequence for the lane:** G3's A-branch is closed. B is priced and small
(step 3). The remaining G3 question is whether anything is worth doing here at
all, given `SRC` is 8.6x the `MISC` lever on this arm.

**Actionable, found in passing (cycle 91):** `/artifacts/` is gitignored
(`.gitignore:24`), and **no artifact this board cites from cycles 85–90 is
actually tracked** — `git ls-files artifacts/performance` matches none of them.
The 197 JSON and 44 CSV files that *are* tracked predate the rule. So every
"artifact `artifacts/performance/...json`" citation on this board is a path that
does not exist in a fresh clone, which makes the evidence unreproducible for
anyone but the machine that ran it. Either force-add cited evidence (`git add
-f`) or stop citing paths as if they were committed; do not leave it ambiguous.
Cycle 91's four artifacts are on disk and uncommitted, matching current practice.

### G3 — RE-PRICED ON THE GATE ARM (cycle 79). The prize is 4–9x smaller than this row claims.

**Every number below this heading is Boundary-derived and carries no arm
label. The gate reads on both-CPU, and on that arm they do not hold.**
Measured on `build-c79-g1-bothcpu`, route 0 (shipped), whole match, 1600
samples, frames 441–2040, stride 96, DLDI on:

| | Boundary (banked, unlabelled) | **both-CPU (gate arm)** |
|---|---:|---:|
| effect display lists / match | 1,360 | **527–563** |
| Exec ticks / list | ~102,730 | **83,632** |
| effect submits as share of `MISC` excursion | 99.3% | **71.5%** |
| recoverable on WORK-H P95 | ~315,000 | **33,699 – 75,264** |

**CAVEAT, cycle 80 — the gate-arm column came off the 12.6% window.** It was
measured on `build-c79-g1-bothcpu`, which still seeded the 420-second match, and
was labelled "whole match". The same applies to G1's *2,953 consults over 563
lists*. Counts **per window** are unaffected (both are 1,600 presented frames),
but "per match" is the wrong denominator, and the effect density of an opening
minute is not that of a full match with its KO-heavy endgame. **RESOLVED, cycle
87 — re-measured on the corrected window (see the G3 census section above), and
the gate-arm column largely HELD: 581 lists/match at 80,394 ticks/list, against
527–563 at 83,632.** Already re-derived: the
`MISC` share of the WORK-H excursion is 25.7% (was 29.1%), and the `MISC` lever
is 48,002 (was 58,240) — still inside the bracket below.

The recoverable is a bracket, both ends measured on this arm: 33,699 charging
each ring stop's effect ticks uniformly across its 96 frames, 75,264 charging
all of them to that stop's most expensive frames (concentration-favourable
upper bound). **Removing 100% of effect DObj submits leaves WORK-H P95 at
1,536,768–1,578,333 against a 1,120,380 gate — a residual gap of
416,388–457,953.** G3 cannot close the gate on the arm the gate reads on.

**RETRACTED (cycle 79, same author): "`OTHR` owns 48.3% of the gate-arm
excursion" was wrong.** `OTHR` is not a region's cost, it is an accounting
remainder — `taskman_seam.c:5137` computes it as `ALL - named`, and `named`
does **not** include `WAIT`, so `OTHR` still contains the VBlank idle that
Task 66 later broke out separately. The retracted table ranked `OTHR` while
excluding `WAIT` as untargetable, double-counting the same idle time; their
excursions differed by 0.04%.

The exact identity, verified frame-by-frame with **max error 0** over 1600
frames:

```
WORK-H = (FTR + STG + BG + AUD + SRC + MISC) + (OTHR - WAIT)
```

`OTHR - WAIT`, the true unattributed work, is **flat ~19,159 ticks/frame**
(P50 19,136, P95 19,776, range 17,984-20,352) and contributes **89 ticks** to
the hot-vs-clean excursion. It is a P50 constant and is not a lever in any
form. **`OTHR` needs no further attribution; this closes it.**

**What actually owns the tail — the two arms are INVERTED, and cycle 80 CONFIRMED
it is not a window artefact.** Mean on over-gate frames minus mean on clean
frames (the metric that separates gate levers from P50 levers). Owners sum
exactly to the WORK-H delta on both arms. Both columns below are now the
**corrected 60-second match at 86.7% coverage**, same window, same method:

| owner | **both-CPU** (gate) | **Boundary** |
|---|---:|---:|
| **SRC** | **216,083 (68.9%)** | 91,350 (27.8%) |
| **MISC** | 80,642 (25.7%) | **232,263 (70.6%)** |
| AUD | 12,075 (3.8%) | 7,602 (2.3%) |
| STG | 8,161 (2.6%) | 2,146 (0.7%) |
| `OTHR-WAIT` | 149 | 158 |
| BG | 23 | 2 |
| FTR | −3,442 (−1.1%) | −4,393 (−1.3%) |
| WORK-H hot−cold | 313,690 | 329,127 |

**The inversion survived a 6.9× change in window size.** Gate-arm `SRC` share
across three windows: 69.6% (12.6% of match), **68.9%** (86.7%), 67.4%
(gameplay-only, 441–1997). `MISC`: 29.1%, **25.7%**, 26.7%. The shares are
stable, so the two arms genuinely have different primary owners and the
two-track scope stands on measurement rather than on an artefact.

Method note: `scripts/analyze-tick-hud-excursion.ps1` computes this and **fails
closed** — it verifies the per-frame identity `WORK-H = (FTR+STG+BG+AUD+SRC+
MISC) + (OTHR−WAIT)` (max error 0 over 1,600 frames on both arms) and refuses to
print a ranking whose owners do not sum to the WORK-H delta. It was validated by
reproducing the cycle-79 both-CPU table and this Boundary table to the digit
before being trusted on new data.

**G3's lane was built on Boundary, where `MISC` genuinely is the tail at
70.6%. The gate reads on both-CPU, where `SRC` is the tail at 69.6% and
`MISC` is secondary.** `SRC` is inflated 1.54x at P95 by the stress config
(P95 547,648 → 842,816) but is **not** a config artefact: it is still 27.8%
of Boundary's excursion. FTR is anti-correlated on both arms, independently
reproducing the existing Parked note.

**Levers priced on the gate arm**, counterfactual "bucket never exceeds its
own clean-frame mean" (an **upper bound** per lever — it assumes the entire
hot-frame excess is removable, which for `SRC` it is not, since some excess
is genuine extra AI work):

**RE-DERIVED, cycle 80, on the corrected 86.7% window** (clean-frame means
`SRC` 309,210, `MISC` 112,830):

| | WORK-H P95 | delta | over gate | residual vs 1,120,380 |
|---|---:|---:|---:|---:|
| baseline | 1,624,064 | — | 704 | 503,684 |
| **SRC capped** | 1,209,050 | **415,014** | 202 | 88,670 |
| `MISC` capped | 1,576,062 | 48,002 | 504 | 455,682 |
| **SRC + MISC** | 1,085,504 | **538,560** | 64 | **−34,876** |

The `MISC` figure (48,002) still falls inside the independently-derived
33,699–75,264 effect bracket, which cross-validates the method. **`SRC` is now
worth 8.6x the `MISC` lever** (was 6.8x on the bad window), and the two
together still put the gate arm inside budget — but by only 34,876, not
57,788, so the combined lane has less margin than the superseded figures
promised.

The combined figure is **super-additive** — 415,014 + 48,002 = 463,016, but
capping both moved P95 by 538,560. That is P95 being a position in a sorted
list rather than a sum. It is not an arithmetic error; do not "correct" it
into an addition.

### The SRC split — MEASURED, cycle 85. Hit detection is NOT the owner.

The instrument booted and both arms ran. `SBAS` (the decomp sim path) owns
`SRC`, not hit detection, and **the ratio is arm-independent** — which is what
makes it a structural finding rather than a config artefact. Whole match, 1,600
samples, frames 442–2041, stride 96, DLDI on, 86.7% coverage, `slips=0` on both.
Builds `builds/build-c85-src-bothcpu` and `builds/build-c85-src-boundary`;
artifacts `artifacts/performance/2026-08-05_c85-{gate-bothcpu,boundary}-*.json`.
Both identities close with **max per-frame error 0**.

| sub-owner | both-CPU excursion | % of SRC | % of WORK-H | Boundary excursion | % of SRC |
|---|---:|---:|---:|---:|---:|
| **SBAS** decomp sim path | **184,316** | **87.1%** | **61.5%** | **80,768** | **89.0%** |
| `SHDT` hit detection | 27,389 | 12.9% | 9.1% | 10,019 | 11.0% |
| `SWRM` anim warm | 12 | 0.0% | 0.0% | 6 | 0.0% |

Lever prices, same counterfactual as the table above (cap the bucket at its own
clean-frame mean — an **upper bound**, it assumes every hot-frame excess is
removable). `artifacts/performance/2026-08-05_c85-src-lever-prices.json`:

| lever | both-CPU delta | residual vs gate | Boundary delta |
|---|---:|---:|---:|
| **SBAS capped** | **315,456** | 177,092 | 73,685 |
| `SHDT` capped | 55,104 | 437,444 | 5,824 |
| `SWRM` capped | 0 | 492,548 | 65 |
| `SBAS` + `MISC` | 454,280 | **38,268** | 369,442 |

`SHDT` + `SBAS` capped reproduces `SRC` capped to the digit (391,552 on the gate
arm), which is the cross-check that the split is exhaustive: `SWRM` is inert, so
the two remaining sub-owners are all of `SRC`.

**Consequence for the lane.** `SBAS` + `MISC` leaves the gate arm 38,268 over,
where `SRC` + `MISC` lands 47,384 under — the difference is exactly `SHDT`. So
the gate needs the sim-path residual, the packet path, **and** a slice of hit
detection; no two of the three suffice.

**`SBAS` is a residual, not a target.** It is everything in
`ndsTask39EffectsUpdate` + `scVSBattleFuncUpdate` except the hit search and the
warm step: the fighter animation/event interpreter, physics and status
transitions, CPU AI, particle bytecode, map collision, camera. **Splitting it is
the next instrument row** — it cannot be optimised as one thing, and the same
mistake that made `MISC` a campaign is available here.

### The SBAS split — MEASURED, cycle 86. `SGCO` owns it; `SCPU` is a P50 lever.

**The composition above was VERIFIED before instrumenting, and it was wrong in
one load-bearing way.** `SRC` brackets `ndsTask39EffectsUpdate` +
`scVSBattleFuncUpdate` (`taskman_seam.c:4442-4466`, its *only* writer, run twice
per presented frame). But decomp's scene update is a one-liner
(`sccommon/scvsbattle.c:75`) calling `ifCommonBattleUpdateInterfaceAll`, whose
`game_status` switch reaches `ifCommonBattleGoUpdateInterface`, which **ends in
`gcRunAll` (`ifcommon.c:2970`)**. So `gcRunAll` is the SOLE gateway to the whole
simulation inside `SRC`, and it already had a port wrapper
(`battleship_sys_objman.c:75`) — **no `decomp/` edit was needed.** The fighter
proc chain is `ft/ftmanager.c:858-863`: six procs per fighter, priority 5→0
(`UpdateInterrupt`, `PhysicsMapDefault`, `PhysicsMapCapture`, `SearchCatch`,
`SearchHitAll` = `SHDT`, `Params`).

**Measured dead, do not re-nominate:** all seven fighter-loop branches in the
port's `scVSBattleFuncUpdate` (`battleship_scvsbattle.c:347-384`) read **0** on
the gate arm — `gNdsFighterNaturalMotionRunAllCount` and all six siblings. The
`gcRunAll` call at `reloc_backend_movement.c:12227` is in that dead chain.
Bracketing it — the obvious reading of the source — would have measured nothing.

Whole match, 1,600 samples, frames 443–2042, stride 96, DLDI on, **86.7%
coverage**, exclusion OFF, `slips=0`. Build `builds/build-c86-sbas-bothcpu`;
artifacts `artifacts/performance/2026-08-05_c86-gate-bothcpu{,-rows,-excursion}`.
Identity closes with **max per-frame error 0** and both derived residuals are
non-negative on every frame.

| sub-owner | excursion | %SRC | %WORK-H | clean mean | hot mean | **hot/clean** |
|---|---:|---:|---:|---:|---:|---:|
| **`SGCO`** unattributed in `gcRunAll` | **153,291** | **73.6%** | **52.0%** | 256,011 | 409,303 | 1.60x |
| `SHDT` hit detection | 25,995 | 12.5% | 8.8% | 4,470 | 30,464 | 6.81x |
| `SPRM` `ftMainProcParams` | 14,774 | 7.1% | 5.0% | 2,011 | 16,785 | **8.35x** |
| `SCPU` `ftComputerProcessAll` | 14,196 | 6.8% | 4.8% | 42,504 | 56,700 | **1.33x** |
| `SCAT` `ftMainProcSearchCatch` | 83 | 0.0% | 0.0% | 1,522 | 1,605 | 1.05x |
| `SWRM` anim warm | 2 | 0.0% | 0.0% | 597 | 600 | 1.00x |
| `SOUT` outside the sim | −100 | −0.0% | −0.0% | 4,490 | 4,389 | 0.98x |

**Boundary arm, same window/method** (`builds/build-c86-sbas-boundary`,
`WORK-H` P95 1,476,352 against the banked 1,476,672 — **320 apart**, a near-exact
reproduction). Identity max per-frame error **0**; `MISC` is still Boundary's tail
at 68.7% and `SRC` is 29.3%, so the two-track scope is unchanged:

| sub-owner | excursion | %SRC | %WORK-H | clean mean | hot mean | **hot/clean** |
|---|---:|---:|---:|---:|---:|---:|
| **`SGCO`** | **77,384** | **81.3%** | 23.8% | 254,300 | 331,683 | 1.30x |
| `SHDT` | 10,797 | 11.3% | 3.3% | 4,007 | 14,804 | 3.69x |
| `SPRM` | 5,338 | 5.6% | 1.6% | 1,819 | 7,156 | 3.93x |
| `SCPU` | 1,754 | 1.8% | 0.5% | 20,132 | 21,885 | **1.09x** |
| `SCAT` | 37 | 0.0% | 0.0% | 1,396 | 1,432 | 1.03x |
| `SWRM` | −1 | −0.0% | −0.0% | 585 | 584 | 1.00x |
| `SOUT` | −143 | −0.2% | −0.0% | 4,443 | 4,300 | 0.97x |

**`SGCO` owns `SRC` on BOTH arms — 73.6% and 81.3%.** Like the cycle-85 ratio,
that arm-independence is what makes it a structural finding rather than a stress
-config artifact. `SCPU` is flat on both (1.33x / 1.09x), so it is a P50 lever on
the gate arm and nothing at all on the shipped one.

**Engagement proof for the `SCPU` bracket, predicted before it was measured.**
The gate arm runs BOTH fighters as CPU and Boundary runs one, so the bracket must
read ~2x. Clean-frame means: **42,504 (gate) vs 20,132 (Boundary) = 2.11x.** The
span is measuring what it claims, and this is the negative control the flat
reading needed before it could be trusted.

**READ THE hot/clean COLUMN BEFORE NOMINATING ANYTHING.** `SCPU` is the trap:
72,512 ticks at p50 mid-match makes the level-3 CPU AI look like the prize, and
on the both-CPU arm it runs twice. But it is **1.33x** hot-vs-clean — a large
FLAT owner, i.e. a P50 lever that cannot move the gate, exactly like the
particles (flat ~47K) that already cost this campaign time. It is 6.8% of the
SRC excursion. Conversely `SPRM` is small in absolute terms yet switches
**8.35x**, the same bimodal shape that makes `SHDT` real. Absolute size and
switching behaviour are different questions and only the second owns a tail.

**What `SGCO` actually is, and what a split would have to bracket.** `SGCO` =
`GCRA − SCPU − SCAT − SHDT − SPRM`, i.e. everything inside `gcRunAll` that the
four bracketed spans do not claim:

| content | port-side wrapper? |
|---|---|
| `ftMainProcUpdateInterrupt` less its AI | **NO** — decomp name, unwrapped |
| `ftMainProcPhysicsMapDefault` | **NO** — decomp name, unwrapped |
| `ftMainProcPhysicsMapCapture` | **NO** — decomp name, unwrapped |
| `mpProcessUpdateMain` (map collision) | YES — `battleship_mpprocess_live_bridge.c:150` |
| camera / effect / item / weapon / interface GObj procs | mixed; not enumerated |

**The three unwrapped procs are why this split stopped where it did, and it is
NOT a `decomp/` question.** `linker/nds_hot_text.ld:207-209` pins all three into
ITCM **by exact symbol name** under a size ASSERT. The port's own rename-on-
include pattern (`src/import/battleship_ftmain.c:47-70`) could wrap them without
touching `decomp/` — but renaming them breaks those linker-script matches and
moves the code out of hot text, changing the very cost being measured. **A next
cycle that wants them must move the `.ld` entries in the same edit**, and should
expect `mpProcessUpdateMain` to be nested inside the physics procs (so it is an
overlay, not a disjoint owner — do not subtract it blindly).

**Instrument cost and health.** +2,368 bytes (four ring buckets) on top of
cycle 85's +1,152; `fake_heap_start` `0x02273f24` → **`0x02274864`**, leaving
**130,976 bytes proven headroom** (`check-boot-headroom.ps1` OK). Boot probe
PASS (frames 60–67, 8 samples, `slips=0`), and the arena stayed at its full
request (`ChosenSize` 1,376,256, `AllocFailCount` 0) — the instrument did not
re-starve what G2 freed. `named` read 959,960 on the boot probe, exactly
FTR+STG+BG+AUD+HUD+SRC+MISC, proving the four new buckets are excluded from
`named`; had they been included it would have read 1,226,456. **The shipped ROM
pays nothing** — every bucket, global, bracket and name is behind
`#if NDS_TICK_HUD`, and the published `-hwtri` target builds `NDS_TICK_HUD 0`.

**A liveness lesson worth keeping: `SCPU` reads 0 at frames 60–67.** The CPU AI
genuinely is not called during the pre-match countdown, so the boot probe alone
would have looked exactly like a dead counter. It was proven non-zero (72,512
p50) by an 8-sample probe at frame 600 **before** the whole-match runs were
spent — standing rule 3 doing its job.

**Do NOT re-bank the gate baseline on c85 (cycle 86).** Three whole-match
gate-arm `WORK-H` P95 readings on identical terms (both-CPU, 86.7% coverage,
1,600 samples, DLDI on, exclusion OFF): c80 **1,624,064** (banked), c85
**1,612,928** (−11,136), c86 **1,625,088** (**+1,024**). c86 carries *more*
instrument than c85 yet lands inside the ±5,376 floor of the banked figure, so
c85's dip is not reproduced and its attribution to G2's z-buffer free was
premature. **1,624,064 stands.**

**`SWRM` is measured INERT over the gate window, and that is a negative result
worth keeping.** It was designed as the honest load signal the circular
`SRC > 2x median` rule needs. It cannot serve: the 41-entry warm list is
exhausted long before the window opens at frame 442, so `SWRM` is a flat ~772
ticks/frame constant (hot 783, clean 772, p50 768 on both arms). A load-signal
rule for this window needs the Task 75 asset-load counter instead.

**Owner decision 2026-08-05: both tracks are in scope. G3 is NOT parked.**
These numbers force it — SRC alone leaves 97,243 over gate, `MISC` alone
leaves 433,412, and only both together land inside. SRC is necessary but not
sufficient. G3's disposition reads **"required, second in order, and primary
for the shipped configuration"** — not "refuted". What was refuted is only
the claim that it alone closes the gate. The second, independent reason to
keep it: **the shipped ROM is Boundary**, and on Boundary `MISC` is 70.6% of
the excursion against SRC's 27.8%, so the packet path is the *dominant*
lever for the configuration that actually ships. Two arms, two different
primary owners, both legitimate. G2's ≥32 KB headroom is therefore back on,
because it funds G3.

**SRC is not a charter §7 question yet and must not be escalated as one.**
`PROJECT_GOAL.md` requires exhausting specialization, approximation,
precomputation, lower-frequency processing, interpolation, event-driven
updates, simplified representations and DS-specific implementations *first*,
and it explicitly encourages fighter- and move-specific native code,
precomputed hitbox trajectories, large lookup tables, and compile-time
baking. `decomp/` is read-only as a **tree**, not as an **algorithm**: a
mechanically equivalent DS-optimized port-side equivalent is wanted, not a
compromise. Rate reduction and simulation-rate change are the LAST resort
and the owner's call.

**No longer provisional (cycle 80).** These shares were re-measured on the
corrected 60-second match at 86.7% coverage and moved by less than a
percentage point, so the two-track scope rests on measurement, not on the
window. The superseded 12.6%-window figures are in the cycle-79 archive.

### The SGCO split — MEASURED, cycle 92. `SITR` owns it on BOTH arms.

`SGCO` was 52.0% of the gate arm's WORK-H excursion and the campaign's largest
remaining lever. It is no longer a residual. The three per-fighter procs cycle 86
could not bracket are now bracketed, and the non-fighter GObjs fall out as a
derived remainder:

```
SGCO = SITR + SPHD + SPHC + SOBJ          (identity, checked per frame)
SITR = SINT - SCPU                        interrupt proc less its AI
SOBJ = GCRA - SINT - SPHD - SPHC - SCAT - SHDT - SPRM
```

Whole match, 1,600 samples, stride 96, **86.7% coverage, DLDI ON, exclusion
OFF**, `slips=0` both arms. Builds `builds/build-c92-sgco-{bothcpu,boundary}`;
artifacts `artifacts/performance/2026-08-05_c92-{gate-bothcpu,boundary}{,-rows,-excursion}`.
Identity closes with **max per-frame error 0** on both arms; both derived
residuals non-negative on every frame; the `SGCO` partition identity holds.

**Gate arm (both-CPU), frames 440–2038:**

| sub-owner | excursion | %SRC | %WORK-H | clean | hot | **hot/clean** |
|---|---:|---:|---:|---:|---:|---:|
| **`SITR`** interrupt less AI | **95,136** | **48.5%** | **31.8%** | 103,329 | 198,465 | **1.92x** |
| `SPHD` physics/map default | 25,828 | 13.2% | 8.6% | 71,725 | 97,553 | 1.36x |
| `SHDT` hit detection | 23,170 | 11.8% | 7.7% | 3,998 | 27,169 | 6.79x |
| `SOBJ` non-fighter GObjs | 19,967 | 10.2% | 6.7% | 80,099 | 100,066 | 1.25x |
| `SCPU` CPU AI | 19,240 | 9.8% | 6.4% | 38,588 | 57,828 | 1.50x |
| `SPRM` params | 12,853 | 6.5% | 4.3% | 1,765 | 14,618 | **8.28x** |
| `SCAT`/`SPHC`/`SWRM`/`SOUT` | 89 / 33 / 8 / −91 | ~0% | ~0% | — | — | ~1.0x |
| `SGCO` roll-up | 140,964 | 71.8% | 47.1% | 255,770 | 396,734 | 1.55x |

**Boundary arm (shipped), frames 438–2038:**

| sub-owner | excursion | %SRC | %WORK-H | clean | hot | **hot/clean** |
|---|---:|---:|---:|---:|---:|---:|
| **`SITR`** | **49,452** | **53.8%** | **13.6%** | 105,715 | 155,168 | 1.47x |
| `SOBJ` | 23,594 | 25.7% | 6.5% | 79,068 | 102,661 | 1.30x |
| `SHDT` | 9,290 | 10.1% | 2.5% | 4,176 | 13,466 | 3.22x |
| `SPHD` | 5,298 | 5.8% | 1.5% | 67,474 | 72,772 | 1.08x |
| `SPRM` | 4,721 | 5.1% | 1.3% | 1,929 | 6,651 | 3.45x |
| `SCAT`/`SPHC`/`SWRM`/`SOUT`/`SCPU` | 89 / 36 / 6 / −128 / **−419** | ~0% | ~0% | — | — | ~1.0x |
| `SGCO` roll-up | 78,380 | 85.3% | 21.5% | 252,899 | 331,279 | 1.31x |

**`SITR` owns `SGCO` on both arms — 67.5% of it on the gate arm, 63.1% on
Boundary.** Like the cycle-85 and cycle-86 ratios, that arm-independence is what
makes it structural rather than a stress-config artifact. It is also the highest
switching ratio of any large owner (1.92x), so it is a gate lever and not only a
P50 one.

Cross-checks: the `SGCO` roll-up's clean mean is **255,770 (gate) / 252,899
(Boundary) against banked 256,011 / 254,300 — 0.09% and 0.55% apart**, so the
instrument does not move the cost it subdivides. `SCPU`'s two-CPU control
reproduces: clean 38,588 (gate) vs 20,524 (Boundary) = **1.88x**, against the
expected ~2x.

**Three negative results, do not re-derive.** `SOBJ` is **flat on both arms**
(1.25x / 1.30x) — the camera, effects, items, weapons and interface GObjs inside
`gcRunAll` are **not** the tail, which closes the largest open hypothesis about
`SGCO`'s content. `SPHC` is ~0 on both arms while its bracket demonstrably runs
(clean mean 618/642) — the mutually exclusive capture arm costs nothing in a
match without grabs, exactly as predicted from ftmain.c:1918-1937. `SCPU` on
Boundary is **negative** (−419, 0.98x), confirming it is not a gate lever on the
shipped arm at all.

**`mpProcessUpdateMain` was NOT bracketed, deliberately.** It is the one seam
here with an existing port wrapper, but decomp has ~20 call sites across
`mp/mpcommon.c`, `it/itmap.c` and `wp/wpmap.c` — so it is an **overlay** spanning
`SPHD`/`SPHC` *and* `SOBJ`, not a disjoint owner, and the identity accounts for it
by not naming it (fighter-side cost inside `SPHD`/`SPHC`, item/weapon cost inside
`SOBJ`, no double count). Its call frequency is an order of magnitude above every
existing bracket, and the cycle-90/91 phase instrument already showed a
high-frequency bracket moving WORK-H P95 +15,808/+45,568. Measure it with a call
counter or on the fighter entry points only — never a timer on the shared leaf.

**The ITCM problem cycle 86 stopped at is solved, and the fix has two mirrors.**
The three procs were pinned by decomp symbol name at `linker/nds_hot_text.ld`.
They are renamed with the port's existing `#define`-before-`#include` pattern
(`src/import/battleship_ftmain.c`) and the three pins were rewritten **in place**
to `.text.battleship_ftMainProc*`. In place matters — the Task 94 note on that
file records the list is a curated 8 KiB working set whose members re-address
each other when the order changes. **Verified in the linked ELF, not assumed:**
`.text.hot` spans `0x020013c0`–`0x02002460` and all three renamed bases sit inside
it at their original positions (`0x16b8`/`0x1700`/`0x1748`), with the shared
`ftMainProcPhysicsMap` still pinned and the wrappers in `.main`. **No `decomp/`
patch was needed.** Two things must move with any future rename here:
`scripts/check-gbi-decode-fixtures.ps1:2098-2109` mirrors the pin list and is the
only guard that catches a half-done rename (an unmatched linker input-section
pattern fails **silently** — the member just drops out of hot text), and the
sampler's `$bucketNames` must move in the same commit as the enum.

**Instrument cost and health.** +1,832 bytes (text +232, bss +1,600) against
**111,584 proven headroom** — 61x margin, `check-boot-headroom.ps1` OK,
`fake_heap_start 0x02279424`. `named` = 1,112,522 = FTR+STG+BG+AUD+HUD+SRC+MISC
exactly, so the three buckets are excluded from `named` and the identity is
byte-identical to every banked measurement. **The shipped ROM pays no instrument
bytes** — the `NDS_TICK_HUD=0` link check contains **0** of the new symbols
against **3** in the lab ELF (a control that can find them). It does pay **one
extra call indirection per proc**, because the rename is unconditional: guarding
it on `NDS_TICK_HUD` would give the lab and shipped ROMs different ITCM layouts
and the measurement would no longer be of the shipped program. That is the same
cost `SCAT`/`SHDT`/`SPRM` have carried since cycle 86.

**NOT a new baseline. 1,624,064 stands.** These runs read WORK-H P95 1,650,240
(gate) and 1,589,056 (Boundary); the instrument perturbs and both runs used
`-AllowRepeatedFrames` (5/1600 gate, 3/1600 Boundary), which is valid for a
ranking and invalid for a per-presented-frame percentile. See `docs/VERIFYING.md`
for the flag's exact scope and for the stop-aligned repeat mechanism.

**INHERITED — the next row is NOT pricing `SITR` (owner, cycle 92).** The
excursion method is structurally blind to flat cost, and the gate arm's P50
already sits within ~26K of the 1,120,380 gate. So the next row measures the
**flat** buckets against their charter budget lines, which is §7 rung 1 and
**not** owner-gated. **DONE, cycle 93 — see "The flat buckets" below**: the
overage is 166,416 (31.4% of the gap), `FTR` owns 81.6% of it, and the row that
follows is the `FTR` phase split, because `FTR` is the only major bucket that
has never been partitioned. `SITR` pricing (specialization, precomputation,
event-driven status updates, fighter/move-specific native code, large LUTs)
remains available behind it, with
`docs/optimization/OPTIMIZATION_IDEAS.md` as the ideas store.

### The flat buckets — MEASURED, cycle 93. `FTR` owns 82% of the flat overage, and it is the one bucket never partitioned.

§7 rung 1, both arms, whole match, 1,600 samples, frames 441–2040, **86.7%
coverage, DLDI ON, exclusion OFF**, `-AllowRepeatedFrames` (3/1600 on each,
valid for a ranking), `slips=0` on both. Builds
`builds/build-c93-flat-{bothcpu,boundary}`, git `8770a246`; artifacts
`artifacts/performance/2026-08-05_c93-flat-{bothcpu,boundary}{,-rows.csv,-excursion.json}`.

**Both arms agree to within 1.2% on both buckets** — as expected for rendering
owners that do not depend on match dynamics, and the cross-check that makes
these usable.

| bucket | arm | mean | P50 | P95 | **clean mean** | hot mean | hot/clean | line | **over line** |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| **FTR** | **gate** | 383,930 | 394,944 | 398,080 | **385,814** | 382,216 | 0.99x | 250,000 | **+135,814** |
| FTR | Boundary | 383,693 | 394,112 | 397,504 | 381,085 | 386,354 | 1.01x | 250,000 | +131,085 |
| **STG** | **gate** | 214,338 | 210,240 | 218,496 | **210,602** | 217,735 | 1.03x | 180,000 | **+30,602** |
| STG | Boundary | 215,005 | 210,816 | 218,688 | 210,919 | 219,173 | 1.04x | 180,000 | +30,919 |

**Ranked flat overage, gate arm: `FTR` +135,814 (81.6%), `STG` +30,602 (18.4%),
total 166,416** — **31.4% of the gate arm's 529,860 gap**, and it is the part
the excursion method cannot see. Both are genuinely flat: `FTR` spread
P95/P50 **1.01**, `STG` **1.04**.

**The premise holds, and it is tighter than the brief assumed. Gate-arm
clean-frame P95 is 1,115,072 against the 1,120,380 gate — 5,308 of margin**
(Boundary 1,108,480, 11,900). The frames that are *not* excursions are already
at the budget, so every flat tick removed passes to P95 one-for-one.

**Correction to §7: the charter understates `STG`.** It says "`STG` at ~195K
against 180K"; measured clean mean is **210,602**, so the overage is **30,602,
not ~15,000 — 2.0x**. Its `FTR` figure is good (~389K stated, 385,814 measured).

**Two STG candidates REFUTED from the same run, no extra build.**
`-ExtraGlobals` on the Boundary arm, whole match (2,041 frames):

- **Stage-prepare rebuild is not happening.** `gNdsR2StagePrepareReuseCount`
  **2,039** against `gNdsR2StagePrepareBuildCount` **2** — the R2-02 E1a reuse
  key hits **99.9%**. There is nothing to win by making it hit.
- **Task 103's generic-emit lever is dead as a per-frame cost.**
  `gNdsStageGCDrawAllLoopDObjDrawCallbackCount` = **8 for the whole match**, so
  the traversal/run-loop site (`reloc_backend_movement.c:13625`) fires 8 times,
  not per frame — the Task 36 replay is armed and the loop only runs during
  capture. The archived 2026-07-27 figures ("21 generic runs, 103 triangles,
  63,903 ticks/frame", `ClaudeOpus5_Task103_TheStageIsNotWhereWeLooked`) no
  longer describe the steady state. **Do not restart that lever from that
  document.**

**What is left in STG is the display-commit site.**
`gNdsStageGCDrawAllLoopCapturedDisplayCount` = **56,178 = 27.5/frame**, i.e.
`ndsRendererAdapterCommitNativeStageDisplay` (`:13521`) is the only
high-frequency STG accumulation site remaining. `ndsRendererAdapterFinishNativeStageOwner`
(`reloc_backend_renderer_dl.c:8376`) is an **empty function** in this
configuration, so the `:14044` site is timer overhead only. The
Prepare-versus-Display split is **not** measured on the current build.

**FTR IS THE ROW, AND IT IS THE ONLY MAJOR BUCKET WITH NO PHASE PARTITION.**
`SRC` has three generations of splits (c85, c86, c92); `STG` has Task 103; `FTR`
has none on the tick instrument. Its bracket
(`reloc_backend_renderer_dl.c:14604–14678`) contains exactly two calls per
fighter — `ndsFighterDisplayContractCapture` then
`ndsFighterMarioFoxDLAllDrawForSlot` — at `gNdsFighterMarioFoxDLAllDrawCount`
**3,955 = 1.94 draws/frame** and `gNdsFighterDisplayContractSubmittedCount`
**63,534 = 16.1 contract events per fighter per frame**, i.e. **~198,900 ticks
per fighter draw per frame**.

A split already exists but **cannot be used**: `m2_contract_capture_ticks`
(`:14613`) is gated on `NDS_RENDERER_PROFILE_LEVEL == 1 &&
NDS_RENDERER_M2_DETAILED_LEDGER`, and the renderer benchmark path asserts
`TICK_HUD=0`, so it cannot run on the whole-match gate instrument. **Next row:
fork the two existing timestamps into `#if NDS_TICK_HUD` counters at those two
sites** — the Task 103 precedent exactly, which added *no* new timer reads
because every site already computed the stamp it needed. Cost is order tens of
bytes against **111,584** proven headroom
(`build-c93-flat-bothcpu` links at `0x02279424`), so the boot cliff does not
bind. Until that split exists, any FTR price is a guess: the honest bracket on
the lever is **0 to 135,814**, and naming a narrower one without the partition
would be the self-time-is-not-a-subsystem-budget mistake again.

### `FTR` is 86% draw / 14% capture — MEASURED, cycle 94. And the phase census below it is ITCM-blocked.

**Correction to the row above: `FTR` is NOT "never partitioned".** Task 91
(`NDS_TASK91_DRAW_PHASE_CENSUS`, default 0) is a complete partition of the draw
half — walk / reset / validate / owner-prep / matrix-prep / material-prep /
inputs / execute / total, plus five matrix sub-phases. **It no longer links.**
`build-c94-ftr-bothcpu` died at `region 'itcm' overflowed by 908 bytes`: its
timer sites sit in ITCM-pinned code (`ndsRendererExecuteNativeFighterOwnerProduction`
at `0x01ffe43c`), and ITCM is `0x7fe0` holding 101 text symbols. *That* is why
`FTR` has no partition on the whole-match instrument — not the absence of one.
Do not brief "add FTR brackets" without this: the brackets exist.

**The top-level split was taken a different way, with zero instrument.**
`NDS_R2_DRAW_SUPPRESS_MASK=3` (existing default-off lab flag, R2-03 E13) returns
from `ndsFighterMarioFoxDLAllDrawForSlot` at its first statement while the `FTR`
bracket still spans `ndsFighterDisplayContractCapture`, so residual `FTR` **is**
capture. Gate arm, whole match, 1,600 samples, frames 441–2040, 86.7% coverage,
DLDI ON, exclusion OFF, `slips=0`; artifact
`artifacts/performance/2026-08-05_c94-ftrsplit-bothcpu.json`.

| | build | `FTR` mean | `FTR` P50 | share |
|---|---|---:|---:|---:|
| control | `build-c93-flat-bothcpu` | 385,814 (clean) | 394,944 | 100% |
| draw suppressed | `build-c94-ftrsplit-bothcpu` | 54,342 | 55,360 | 14.1% |
| **draw (derived)** | | **331,472** | **339,584** | **85.9%** |

**Engagement is exact, not plausible:** `gNdsFighterMarioFoxDLAllDrawCount`
3,955 → **0**, `gNdsFighterDisplayContractSubmittedCount` 63,534 → **0**, and
both `gNdsFighterDLAllDrawP{0,1}HardwareTriangleCount` → **0**. Cross-build, so
it pays placement noise — but `FTR`'s own spread is **1.02** and the delta is
**60x** the ±5,376 floor, so the split is not in question.

**A CEILING, NOT A CANDIDATE.** Deleting both fighter draws outright moves
gate-arm `WORK-H` P95 **1,650,240 → 1,297,984 (−352,256)** and P50 1,128,192 →
777,216. Read it the way Task 106's −119,744 is read: the most the whole
fighter-draw lever can ever be worth, with no fighters on screen. VBI went
2:965 → 2:1704.

**REFUTED: the source display proc is not the cost.** Capture runs
`gmCameraLookAtFuncMatrix` (once per *fighter*, on the same camera),
`ndsFighterDisplayContractCountFlags`, and the decomp
`ndsBaseFTDisplayMainProcDisplay` over the fighter's DObj tree — **all of it
together is 54,342**, 14.1% of `FTR` and 4.5% of a clean frame. The duplicated
per-fighter camera matrix is therefore bounded above by a fraction of that and
is **not worth a build**. Spend the next row on the draw half, where 331,472 is.

**Next row is the Task 91 census made buildable**, and the cheap form is a
sub-flag excluding the five `gNdsTask91Mtx*` sites that live in ITCM-resident
code, then re-try. Do **not** un-pin ITCM symbols to make it fit: that moves hot
text and changes the very cost being measured (`linker/nds_hot_text.ld`, Task 94
note).

### Why the census cannot link — ATTRIBUTED, cycle 95. **ITCM is 99.1% full**, and that is a standing constraint, not a Task 91 problem.

Measured from the linked ELF of `build-c93-flat-bothcpu` and an object-level
diff against `build-c94-ftr-bothcpu` (the two differ **only** by
`NDS_TASK91_DRAW_PHASE_CENSUS`, both `NDS_R2_BOTH_CPU=1`):

```
.itcm section  32,448 bytes      ITCM region  0x7fe0 = 32,736
FREE                288 bytes    census needs      +308   (nds_renderer.o .itcm 17,620 -> 17,928)
```

**The census was never the problem; the region is full.** `.itcm` is 99.1%
occupied by 99 text symbols, and *no* timer-based partition of the draw half can
be added without evicting something. This is the durable answer to "why has
`FTR` never been partitioned", and it generalises: **treat ITCM as closed to new
instrument code.**

**The five biggest residents ARE the draw half** — 20,696 bytes, **63.8% of all
ITCM**:

| symbol | bytes |
|---|---:|
| `ndsRendererScanList` | 7,728 |
| `ndsRendererExecuteNativeFighterOwnerProduction` | 3,628 |
| `ndsRendererNativePrepareProductionRun` | 3,348 |
| `ndsRendererSubmitHardwareTriangle` | 3,304 |
| `ndsRendererHardwareSubmitVertex` | 2,688 |

That is a *code-size* ranking, not a time ranking — do not read it as an
attribution of the 331,472. It does say where the hot path lives.

**Honest gap: the arithmetic does not fully reconcile.** Free 288 against +308
predicts a 20-byte overflow; the linker reported **908**. The extra 888 is
**unattributed** — most likely `--gc-sections` retaining symbols the census
newly references, or alignment shifting between contributions, neither of which
is measurable without a successful link. It does not change the conclusion in
either direction: at 288 bytes free the census does not fit even on the
optimistic reading.

**Consequence for the next row — suppression, not timers.** The zero-byte
technique that produced the 86/14 split is the only one immune to this, and it
already has levers in the tree: `NDS_R2_FIGHTER_SHADE_SKIP`,
`NDS_R2_FIGHTER_STATESPAN_SKIP` (`Makefile:570,575`) alongside
`NDS_R2_DRAW_SUPPRESS_MASK`. **Both change what is drawn**, so each is a
*price* in the lab and a `BLOCKED(decision: ...)` with a synchronized
`artifacts/visibility` pair if it ever looks like landing. Rank on the
**clean-frame mean** (`FTR` is 0.99x hot/clean; an excursion ranking scores the
whole 331,472 at ~zero) and report hot/clean beside it.

**Do NOT un-pin ITCM symbols to make room.** Moving hot text changes the cost
being measured, and `linker/nds_hot_text.ld`'s curated list re-addresses itself
when members move (Task 94 note).

### The draw half splits 52/48 — MEASURED by suppression, cycle 96. Shading is already spent.

Gate arm, whole match, 1,600 samples, frames 441–2040, **86.7% coverage, DLDI
ON, exclusion OFF**, `-AllowRepeatedFrames`, `slips=0` on every arm. Ranked on
**clean-frame mean**, because `FTR` is 0.99x hot/clean and an excursion ranking
scores the entire 331,472 at ~zero. Zero instrument bytes: every arm is an
existing default-off lab flag, reverted after.

| arm | build | `FTR` clean mean | hot/clean | fighter triangles | Δ vs control |
|---|---|---:|---:|---:|---:|
| control | `build-c93-flat-bothcpu` | 385,814 | 0.99x | 635,840 / 603,432 | — |
| `NDS_R2_FIGHTER_SHADE_SKIP=1` | `build-c96-shade-bothcpu` | 382,337 | 0.97x | 635,840 / 603,432 | **−3,477** |
| `NDS_R2_FIGHTER_STATESPAN_SKIP=1` | `build-c96-statespan-bothcpu` | 226,434 | 0.98x | **0 / 0** | **−159,380** |
| `NDS_R2_DRAW_SUPPRESS_MASK=3` | `build-c94-ftrsplit-bothcpu` | 54,342 | — | 0 / 0 | −331,472 |

**The draw half decomposes additively, and it closes exactly:**

```
submission + state replay   385,814 - 226,434 = 159,380   48.1% of the draw
pre-submission draw         226,434 -  54,342 = 172,092   51.9% of the draw
                                       sum    = 331,472   == the draw half
```

Both parts non-negative, and the two independently-measured arms reproduce the
`FTR` total to the byte.

**`STATESPAN_SKIP` IS AN OVERLAY, NOT A SUB-OWNER — do not brief it as the
state-span price.** Its `gNdsFighterDLAllDrawP{0,1}HardwareTriangleCount` are
**0/0**: skipping the state spans also stops all geometry submission, so the
159,380 is "state-delta replay **plus everything downstream that no longer
happens**". The boundary between the two halves above is *where triangles stop
being emitted*, not a function boundary. This is the same handling
`mpProcessUpdateMain` gets.

**REFUTED — shading is not a candidate.** −3,477 is **inside the ±5,376
cross-build floor**, i.e. not resolvable from zero. The lever was already spent
by R2-03 E28, which removed the software light preparation
(`NDS_R2_FIGHTER_SOFT_LIGHT_KEEP` defaults 0, so the cut is in); `SHADE_SKIP`
now only removes the residue. Do not re-price it.

**The per-unit constant, which makes short probes valid here:** 1,239,272
fighter triangles over 2,041 frames = **607.2 triangles/frame**, so submission
costs **262 ticks per triangle** (gate arm). For calibration that is already
**2.4x cheaper** than the stage's generic emit (620/triangle, Task 103) and the
effect interpreter (626/command) — **the fighter submit path is not obviously
wasteful**, which argues against attacking it first.

**Where the cheapest mechanically-equivalent routes actually are.** The
pre-submission 172,092 is per-fighter-per-frame *policy* work — walk, reset,
validate, matrix prep, material prep — and it is precisely what charter R2-03
already names for deletion ("no `PrepareProductionRun` policy re-checks, no
traversal-state/stats dependency, no per-frame texture identity proof"). That
is deletion-shaped and needs **no** visible change. The submission 159,380 is
geometry, where the contract's allowed routes are pretransformed geometry,
precomputed matrices, quantized poses and precompiled GX streams — all of which
add text and must beat 1.85 cycles of `FTR` mean per byte.

**Note the framing this creates:** in the `STATESPAN` arm `FTR` clean mean is
**226,434, already under the charter's 250,000 line**. If fighter geometry
submission were free, `FTR` would meet its budget outright — so the 250K line is
reachable without touching the pre-submission half at all, and vice versa.

**Not run: the combined arm.** With `SHADE_SKIP` at or below the noise floor its
overlap with anything else is immaterial, so a third build would not have
changed a number. **A ceiling, not a candidate:** the `STATESPAN` arm's `WORK-H`
P95 is 1,475,328 against the control's 1,650,240 (−174,912) with over-gate
52.4% → 23.0% — that is what removing all fighter geometry buys, on a ROM that
draws no fighters.

### The pre-submission 172,092 — SEAMS NAMED, cycle 97. **No edit made; read the blocker before briefing an implementation.**

The 172,092 is per fighter per frame at 1.94 draws/frame ⇒ **~88,700 per fighter
per frame**. Its five seams, each with the question that decides whether it is a
deletion — *what input can change between frames?* Work whose inputs are fixed
at match load is re-proving a constant.

| seam | site | input that can change | verdict |
|---|---|---|---|
| **walk** | `ndsFighterCollectAllDObjsWithDL` | DObj tree topology | **match-load constant** for Mario/Fox — poses move, topology does not. Bake the collection order at load. Needs proof no status/motion alters the DObj set. |
| **reset** | `ndsFighterDLDrawResetTransientRendererStats` `:5041` | nothing — it clears | **deletion-shaped.** The Task 91 note at `:13474` already records that **70% of this function's memset traffic** is two sites here, clearing the per-list proof/counter prefix *once per part list per fighter per frame*. Those are diagnostic fields; much may already be dead at profile 0 / `NDS_TICK_HUD=0`. |
| **validate** | `ndsRendererAdapterValidateNativeOwnerCached` `:8679` | asset bases, selected count, root offsets | already *named* cached — **but see the blocker.** |
| **matrix prep** | `PrepareNativeOwnerHierarchy` / `…Matrices` | joint matrices, camera | **genuinely varies.** Not a deletion. The contract's routes are precomputed/quantized poses and reduced update rates — those are fidelity-adjacent and owner-gated. |
| **material prep** | `ndsRendererAdapterBuildNativeMaterialSnapshot` `:6895` | damage-flash modulate only | mostly **match-load constant**; 18.8% of scene memset traffic per the same note. Bake per model, patch the modulate per frame. |

**THE BLOCKER, and it is the same lesson the stage taught.**
`ndsRendererAdapterValidateNativeOwnerCached` **carries no engagement counter.**
The stage's equivalent did (`gNdsR2StagePrepareReuseCount` / `…BuildCount`),
and that is precisely how cycle 93 refuted the stage-prepare candidate at
**99.9% reuse for free, with no build**. Here there is no way to tell a hit from
a miss without adding one.

**So the first move is one counter pair, not an edit.** Spending a build
deleting work whose cache already hits would repeat the exact mistake cycle 93
avoided — and on this ROM a wrong guess costs a build, a whole-match run, and a
Boundary run. Add reuse/build counters to the fighter owner validate (and to
the material snapshot), read them on one whole-match gate-arm run, and only then
choose which seam to delete.

**Not done, deliberately:** no edit, no build, no run this row. The chain
(add counters → build → run → design the deletion → dual-route build → two runs
→ Boundary) did not fit the remaining budget, and this row touches the hot draw
path where an unverified commit is the worst outcome. A proven seam with no fix
beats a fix with no proof.

### The counters are in, and they refute two seams and name one (cycle 98)

**Two of the five pre-submission seams are dead, the walk's invariance is proven
for the first time, and the target is material prep at 99.95%.** No edit to the
draw path beyond the counters; no deletion landed this cycle — see the closing
paragraph for why, and what the next one inherits.

Gate arm, both-CPU, whole match, 1,600 samples, frames **442–2041** (the banked
arms landed on 441–2040; the one-frame offset is ordinary window jitter, same
`-StartFrame 441`, same stride, same match config), DLDI ON, exclusion OFF,
`-AllowRepeatedFrames`, `slips=0`. Build
`builds/build-c98-ftrpre-bothcpu`; artifacts
`artifacts/performance/2026-08-05_c98-ftrpre-bothcpu.json`, its `-rows.csv`, and
`...c98-ftrpre-excursion-bothcpu.json`. Coverage is inherited from the banked
arms' identity (same match config, same window) and was **not** re-measured with
`probe-match-window.ps1` this cycle.

| seam | counters | reading | verdict |
|---|---|---:|---|
| **validate** | `gNdsFtrPreValidateReuse` / `Build` / `Reject` | **3,961 / 2 / 0** | **REFUTED — the cache already hits 99.95%** |
| **walk** | `gNdsFtrPreWalkSame` / `Variant` / `First` | **3,961 / 0 / 2** | **CONFIRMED match-load constant — 0 variants** |
| **material** | `gNdsFtrPreMatSame` / `Variant` / `New` / `Evict` | **47,719 / 25 / 27 / 11,659** of 59,430 calls | **THE TARGET — 99.948% of comparable builds re-derive a byte-identical snapshot** |
| **reset** | `gNdsFtrPreResetTransient` / `Runtime` | **0 / 4,625** | **REFUTED — already dead at the shipped profile** |

**Engagement is an identity, not a plausibility.** Validate's three counters sum
to **3,963** and the walk's three sum to **3,963**, and
`gNdsFighterMarioFoxDLAllDrawCount` is **3,963** — so both censuses saw exactly
the draws that happened, no more and no fewer. The material counters sum to
59,430 == `gNdsFtrPreMatCalls`. Fighter triangles 636,480 / 604,044 against the
control's 635,840 / 603,432 (+0.1%) confirm the fighters drew normally under the
instrument.

**Positive control, and it is what makes the two refutations readable.**
`gNdsR2StagePrepareReuseCount` / `BuildCount` read **2,040 / 2 = 99.90%** in the
same run, reproducing cycle 93's stage figure. A counter mechanism that
reproduces a known result is what separates "this seam is already elided" from
"this counter never linked" — which matters most for the reset seam, whose whole
finding is a zero.

- **Validate is refuted for the reason cycle 93 predicted.** Two rebuilds in a
  whole match. Deleting the work behind this cache would have bought nothing,
  and without the counter it was indistinguishable from a seam that rebuilds
  every frame. This is the row's main justification: one run, no build spent on
  a guess.
- **The reset seam was already dead, and the source says why.** Both call sites
  of `ndsFighterDLDrawResetTransientRendererStats` sit in the `detailed_output`
  arm of an if/else (`:9417`/`:14271`), and the native owner production path is
  itself gated on `detailed_output == FALSE` (`:13862`) — so the bzero the
  cycle-97 table called "deletion-shaped" never executes on the gate arm.
  `Runtime` at 4,625 is the negative control. **The 70%-of-memset figure in the
  note at `:13474` is a Results-lab number and does not transfer to the gate
  arm.** Do not re-brief this seam.
- **The walk's inputs never moved once.** The hash covers `total_count`,
  `selected_count`, `selected_index_mask`, and every selected DObj pointer
  *together with the display-list pointer it will be drawn from* — 0 variants
  over 3,961 frame-to-frame comparisons with both CPUs live. That is the proof
  the cycle-97 table asked for ("needs proof no status/motion alters the DObj
  set"). Note what it licenses and what it does not: it says a baked collection
  order would be *correct*, not that the walk is expensive.
- **Material prep is the target.** 47,744 builds could be compared against the
  previous build for the same MObj; 47,719 were byte-identical. The 25 variants
  are real (frac texture animation advances `texture_id_curr/next`), so a design
  that simply freezes the snapshot is a fidelity change and is **not** what this
  points at. What it points at is that `ndsRendererAdapterPrepareNativeMaterials`
  (which walks each MObj chain twice) and `ndsRendererAdapterValidateNativeOwnerMaterials`
  (up to three `ndsRelocFindLoadedFileContaining` searches per material per
  fighter per frame — charter R2-03's "per-frame texture identity proof") are
  re-proving a constant ~999 times in 1,000.

**`Evict` 11,659 is table thrash, not variance — read it that way.** The census
is a 256-entry direct-mapped table keyed on the MObj pointer; a handful of
colliding key pairs ping-pong in one slot and every call for them counts as an
evict. Those 11,659 calls are **unclassified**, not evidence of change. The
classified sample is 47,744, which is what the 99.948% is taken over.
**Actionable for whoever reuses this instrument:** make it 2-way or key on
`(dobj, chain index)` instead, and Evict goes to ~0.

**Instrument cost, and it is not a baseline.** text **+992**, data 0, bss
**+2,112** = **+3,104**; `fake_heap_start` 0x02279424 → **0x0227a044**, the
address delta 0xC20 equalling the section delta to the byte. Proven headroom
111,584 → **108,480** (`check-boot-headroom.ps1` OK, 34.9x margin). The
`NDS_TICK_HUD=0` configuration was link-checked via
`smash64ds-battle-playable-proof-hwtri` and contains **0** `FtrPre` symbols, so
the shipped ROM pays nothing. `FTR` clean mean reads **397,454** (hot/clean
0.98x) against the control's 385,814 — **the instrument costs ~11,640 of the
very bucket it measures**, which is why the seam verdicts above are counter
readings and not tick figures. `WORK-H` P95 **1,671,104** against banked
1,624,064 is +47,040, far outside the ±5,376 floor. **1,624,064 still stands;
this arm is not a new baseline.**

**One unattributed reading.** `gNdsTaskmanArenaChosenSize` 1,351,680 with
`gNdsTaskmanArenaAllocFailCount` **6**. The G2 model predicts ~N/4,096 steps,
i.e. 1 step for 3,104 bytes, not 6. The control's own arena counters were not
read this cycle, so the *delta* is unmeasured and the 6 may largely predate this
build. Nothing failed — the ROM booted and ran the full match — but do not quote
the ~N/4,096 rule as confirmed until a control reading exists.

**Not done, deliberately: no deletion landed.** The seam the numbers chose is
material prep, and its cheapest correct form is genuinely open — a per-frame
memo has to read most of the MObj state the build reads (so it saves the emit
half, not the input half), while a match-load bake has to keep serving the 25
real variants and so still needs a check. `PROJECT_GOAL.md` prefers the bake and
the brief prefers deletion over caching; deciding between them needs a design
step, and landing either needs a dual-route build plus two whole-match runs. That
did not fit behind the run above, and this is the hot draw path, where an
unverified commit is the worst available outcome. **The next cycle inherits a
named seam with a number, an instrument already in the tree to verify against,
and two seams it must not re-brief.**

### G3 (original row, Boundary-derived) — the effect packet path

Build the GX packet per unique effect display list **at match load**, reserve
patch offsets for matrix and dynamic colour words, patch per frame, submit.
No re-parse, no per-list config rebuild, no per-command dispatch.

Design constraints (all standing law, see charter §3):
- §3.11 — fixed arena allocated at match load, sized by a unique-list census
  (1,360 list *instances*/match; count the unique templates first), explicit
  overflow policy, exercised in a soak. No gameplay-time heap allocation.
- §3.12 — packets are re-derived at scene entry; nothing keyed on pointers
  that survive a scene boundary.
- Byte-cost table + boot probe before the first measuring run (G2's headroom
  is the budget it spends from).
- Dream Land water frozen at frame 0; same geometry/textures/materials — the
  effect models are a closed owner-approved set. A change that alters a
  visible pixel needs the owner.

**Iteration protocol — one build, one run per decision.** Ship both routes in
ONE tickhud binary behind a gdb-settable runtime route (the
`NDS_R2_STAGE_ROUTE_PROBE` pattern): route 0 = interpreter, route 1 = packets.
Because the cost is a **per-list constant**, ticks/list from a few stops is a
valid iteration metric — flip the route mid-run and read both constants from
the same run, same frames, zero placement noise, zero extra builds. The
whole-match sampling run is reserved for the KEEP decision and re-baseline.
Success at iteration scale: packet-route ticks/list ≪ 83,632 on the gate arm
(≪ 102,730 on Boundary) — the submit-only residue should be a few thousand.

**There is no longer a gate-scale success criterion for this row.** The prior
one ("P95 moves by most of the ~315K recoverable in both arms") was written from
the unlabelled Boundary diagnosis and is refuted: on the gate arm, removing
*100%* of effect submits leaves WORK-H P95 at 1,536,768–1,578,333 against the
1,120,380 budget — a residual gap of 416,388–457,953. **G3 cannot close the gate
alone.** It remains a real Boundary-arm win and a partial gate-arm win; it is no
longer the lane's answer, and G2's ≥32 KB exit exists only to fund it.

### G4 — Re-baseline and pick the next lever from the residue

After G3 KEEP: bank new whole-match baselines (both arms — run them
concurrently on two runner slots once the parked calibration row passes).
**The gate decision reads on the both-CPU arm** (owner, 2026-08-05); bank its
load-frame-excluded P95 explicitly — the Boundary clean-frame figure
(~1,056,640, inside the budget) has no banked both-CPU sibling yet. If a
residual gap remains, promote from Parked in this order: the +52,928
regression bisect (largest known flat cost), `Tex` residue on non-effect
paths, then the charter §7 contingency ladder (rate reduction → fidelity →
owner-approved 30 Hz) — never widen the gate.

## Red Queue

The P1 acceptance-level rows, highest impact first. The gate lane above is
row 1's execution plan.

1. **Stable 30 FPS** — qualify the whole match at P95 ≤ 1.12M ARM9 ticks per
   presented frame on the **both-CPU stress config**, loading states excluded
   (owner, 2026-08-05; **gap 503,684** on the corrected 60-second match at
   86.7% coverage, cycle 80; lane G1–G4), on the accuracy melonDS
   fork. The shipped ROM stays the Boundary hwtri configuration. Hardware
   remains the final check for mechanisms the emulator cannot referee.
2. **Mario/Fox completeness** — replace battle-reachable weak status callbacks
   with source-backed behavior and prove both complete movesets naturally.
3. **Dream Land completeness** — close the remaining Whispy material/animation
   presentation debt without reintroducing gameplay-time texture conversion.
4. **Audio completeness** — implement or explicitly qualify every reachable
   voice, pitch schedule, composite cue, and overlapping match-audio path.
5. **Final acceptance** — the CPU-on one-minute match, complete-match capture,
   owner play/listen pass, reserve gate, Results transition, and teardown
   proof on the exact candidate ROM.

## Parked — open items with owners' notes, promote deliberately

- **SRC sub-owner instrument: LANDED, cycle 85.** Three cycles of "does not
  boot" were the boot cliff, exactly as cycle 82 concluded; G2 freed the room and
  the same design booted first try. **Cost +1,152 bytes** (text +80, bss +1,056)
  against 134,496 proven — 116x margin — links at `0x02273f24`, boot probe PASS
  (frames 60–67, 8 samples, `slips=0`), and the arena stayed at its full request
  (`gNdsTaskmanArenaChosenSize` 1,376,256, `AllocFailCount` 0), so the instrument
  did not re-starve what G2 just fed.
  **The shipped ROM pays none of it.** Every part — enum, globals, ring, names,
  both brackets, the publish and the two resets — sits behind `#if NDS_TICK_HUD`,
  and the published `smash64ds-battle-playable-hwtri` builds `NDS_TICK_HUD 0`.
  The only unconditional source change is inverting the early-return guard in
  `ndsR2AnimCachePreloadStep` so the bracket has a single exit, which is
  mechanically identical (`if (c >= N) return; load();` == `if (c < N) load();`).
  **Two ring buckets, not three.** `SHDT` = `ftMainProcSearchHitAll`
  (`reloc_backend_diagnostic_recorders.c:5663`) and `SWRM` =
  `ndsR2AnimCachePreloadStep` (`reloc_backend_assets.c:6424`, called from
  `battleship_scvsbattle.c:344`), both bracketing existing port wrappers so **no
  `decomp/` edit was needed**. The third sub-owner `SBAS` is **derived** as
  `SRC - SHDT - SWRM` by `analyze-tick-hud-excursion.ps1`: it costs no bytes, and
  `SBAS >= 0` on every frame is the *proof* the two ringed spans are nested
  inside SRC — the script throws if any frame goes negative. Appended after
  `WORK`, kept out of `named` (they are sub-spans of `SRC`, which `named` already
  counts), so `OTHR`/`WORK` and the WORK-H identity are byte-identical.
  `$bucketNames` moved in the same commit as the enum, and the sub-buckets are
  excluded from the sampler's `named` share too — verified on the boot probe,
  where `meanNamed` read 949,400 exactly and would have read 954,720 if they had
  been wrongly included.
- **R2-03 E35: mechanism CONFIRMED at whole-match scale, ownership REFUTED.**
  Its magnitudes were already retired for sitting on a 128-frame window. Cycle 85
  re-measured the mechanism on 1,600 frames and both halves of the verdict are
  now evidence. **Confirmed:** hit detection is genuinely bimodal and
  switches on with expensive frames — elevated `SHDT` (>5,500) occurs on 173 of
  1,600 frames and **82.7% of those are over gate against a 42.9% base rate**;
  hot p95 203,008 against clean p95 5,312, a 38x tail separation. **Refuted:**
  it does not own the excursion. **544 of 687 over-gate frames (79.2%) sit at the
  `SHDT` floor**, so hit detection can explain at most 20.8% of them, and it is
  12.9% of SRC's excursion against `SBAS`'s 87.1%. The honest reading is that
  E35 found a real minority mechanism and its window flattered it into the
  majority one.

- **Task 56 strips may have been closed on the boot cliff, not on strips
  (cycle 82, address evidence only — NOT re-tested).** `builds/build-t56-strips`
  links at **`0x02294f04`**, 1,792 bytes above the highest address proven to boot
  and 992 above the lowest proven to fail, i.e. deep in the failing region. Its
  recorded symptom — "cannot reach presented frame 12 in 900 s" — is the cliff
  signature, not a present-loop bug, and the control it was judged against
  (`build-t56-control`, `0x0228cea4`) is a 3-day-older, ~31 KB smaller tree, so
  it was never a matched control. Three attempts, two builds, three days were
  spent on this. **Do not re-open before G2**; after G2, re-link it and run
  `check-boot-headroom.ps1` before concluding anything about strips.
- **+52,928 ticks/frame regression** between `2494daf9ad` and `e49a98167c`,
  null control, real, NOT in the three reverted hunks. Untested suspects:
  `38bba475` BLENDPE prim/env bake + `key_generation` fence, `0a060c7b`
  alpha/blend recogniser, `e8c675d3`/`999fcdf8`. Re-open against the
  whole-match instrument only.
- **Concurrency calibration** (workflow, cheap): same tickhud ROM, solo run vs
  two concurrent runs on slots 2/3 — guest tick series should be identical
  (deterministic emulation; host load moves wall clock, not guest ticks). If
  clean, bless 2-concurrent measuring runs (Boundary + both-CPU
  simultaneously) and functional-verify overlap. Watch harness wall-clock
  liveness thresholds (STALLED/TOO SLOW) — they read observed frames/s.
- **`check-decomp-header-mirror.py` RED on HEAD** — `FTSTAT_OPENING1_START`,
  `nSYAudioBGMExplain`; pre-existing; a guard blind to its class of bug.
- **`sNdsRendererRuntimeTextureCacheEvictCount` liveness unproven** — read 0
  all run, never shown able to be non-zero. Do not cite evictions from it.
- **Per-build ELF resolution in harnesses** (`Makefile:60-90` names the fix):
  the root `.elf`/`.nds` pair is shared between build dirs; a published build
  intervening between a lab build and its measurement silently swaps the pair.
  Until fixed: lab build immediately before its measurement.
- **Particle `sqrtf` axis magnitudes** (`ndsParticleTransformForDraw`): move
  two `sqrtf` calls inside the existing `transform_id` guard; ~200K calls a
  match. P50/foreground lever, not the gate. Watch
  `gNdsTickHudForegroundTicks`.
- **Particle atlas admission is stale**: 24 live textures, sheet admits 14 of
  47; texture 1 is in `QUAD_MEASURED_LIVE` and lost its slot. Re-run
  `scripts/generate_nds_particle_banks.py` and re-derive; budget question is
  VRAM cache contention (PORTING.md: 16K/32K sheets failed via
  `PrepareRun` drops), not RAM.
- **GATE 6 price correction on record**: the source-effects flip was sold at
  +36,032 P95 on the bad window; real cost ~360,000 on every effect-active
  frame. The decision stands (make the submit path cheap, do not delete the
  models) — the number behind it did not.
- **RESOLVED cycle 80 — the Boundary verifier was RED for 35 commits, and the
  first attributed cause was wrong.** `EXPECTED_CENSUS_SHA256` went stale at
  **`fcf93d00`** (2026-08-04 17:05), *not* at `4a413079`. **RETRACTED:** the
  census does **not** hash `nds_renderer.c` source text — `parse_renderer_contract`
  extracts semantic facts (the key field tuple plus four required tokens), each
  failing closed with its own message, and returns hardcoded constants. That
  claim was inferred from a `read_text` call without reading what the parser
  does with it.
  Bisected: the pre-`fcf93d00` script against the pre-`fcf93d00` tree reproduces
  `829c895d…` exactly. A field-level manifest diff moves **six leaves, all in
  `renderer_key_contract`**, all corroborated by `nds_renderer.c`'s own defines —
  `current_cache_entries` 48→69 (`CACHE_COUNT 69u`), `cache_entry_bytes_profile`
  280/276→44/40, and three new fields `static_cache_entries` 24
  (`STATIC_COUNT 24u`), `dynamic_key_pool_bytes` 10,620 ((69−24)×236),
  `static_pointer_word_bytes` 288 (24×3×4). **Nothing in the texture corpus
  moved**, and every `decomp/` input still matched its own pinned sha256, so it
  was a reviewed consequence of a deliberate change, not corpus drift — which is
  why re-pinning was correct here and would *not* have been had a corpus field
  moved.
  The second failure ("Static texture generator reported no residency bytes",
  `verify-battle-mariofox-gcrunall-loop-harness.ps1:317`) was **one cause, not
  two**: that generator imports the census, died on the same digest, emitted no
  JSON, and the absent field read as 0. Its own `EXPECTED_INCLUDE_SHA256` then
  needed re-pinning too, and that delta is **pure provenance** — the include
  stamps the census digest in a header comment, so exactly one line changed
  while `EXPECTED_PAYLOAD_SHA256`, `EXPECTED_METADATA_SHA256`, residency 61,696
  and payload 61,210 all stayed put.
  **The durable lesson: re-pin in the same commit that changes what the pin
  covers.** A pin whose subject moves silently disables every gate downstream of
  it — here the entire Boundary profile aborted in pre-flight, so 35 checkpoints
  including the whole cycle-79 gate lane landed without a runtime check.
  Two secondary notes: `src/nds/generated/…static_textures.generated.inc` is an
  **untracked build product** whose on-disk copy was still the Aug-3 version, so
  the build's prerequisites for it do not include the census script; and its
  staleness was harmless only because the delta was a comment.
- **`check-one-minute-match-verifier.ps1` has drifted from its owner**
  (2026-08-03): 55 `Assert-Text` pins against exact source text, at least two
  red on refactors that changed nothing they guard. Regrade the pins against
  what each actually protects, or delete the ones already asserted by the
  owner's own gates.

## Standing measurement rules (the ones that gate evidence)

1. Whole-match `-RingDump` sampling is the only gate instrument; label every
   figure with its arm **and its coverage**; DLDI-on only. **Coverage is part
   of a baseline's identity, not a footnote** — a window is "whole match" only
   if its fraction was measured against the match clock.

   **FIXED, cycle 80.** `scene_harness.c` used to seed `time_limit = 7` under
   `NDS_R2_BOTH_CPU`, so the gate arm sampled frames 440–2040 of a 420-second
   match — **12.6% coverage, the opening minute** — while the identical window
   on Boundary covered 86.7% and ended at the buzzer. That one line superseded
   every both-CPU tick figure in the campaign. Both arms now run the 60-second
   match and measure identically (86.7%, clock 52 s → 0 s, logic:presented
   2.000); the soak's long match lives on `NDS_R2_SOAK_MATCH_MINUTES`.

   `scripts/probe-match-window.ps1` measures coverage and **reads the match
   timer out of the guest** (`gSCManagerTransferBattleState.time_limit`)
   instead of taking it from the command line — its `-TimeLimitMinutes`
   is now only a cross-check and disagreeing with the guest throws. Run it on
   any new arm before banking that arm's ticks.

   **The window ends 43 frames past the buzzer on both arms** (1998–2040, GAME
   SET, `SRC` < 50,000, gate-arm mean `WORK-H` 711,751). It is the same tail on
   both, so cross-arm comparison is sound; a single-arm figure should say
   whether it is full-window or gameplay-only.

   What the correction cost: the gap went 485,060 → **503,684**, i.e. the old
   early-match window was optimistic by 18,624. What it did **not** overturn:
   the SRC/MISC inversion, which held at 69.6% → 68.9% → 67.4% across a 12.6%,
   an 86.7% and a gameplay-only window. Note shares still drift *within* a
   window (both-CPU `MISC` 104,076–221,815 across 200-frame blocks), so a
   sub-window share is still not a match-level one.
2. Verify a counter is live in the shipped configuration BEFORE the measuring
   run; a proof-scoped counter reads 0, indistinguishable from clean.
3. Eliminate candidates with a liveness probe on an already-built ROM before
   spending a measuring run.
4. `ALL` is VBlank-quantized; judge on `WORK-H`.
5. Do not multiply a number back by what you divided it by and call the
   agreement a finding.
6. New tables/code: byte cost stated + boot probe before measuring (see G2).
7. Prefer one dual-route binary over separately-linked A/B ROMs wherever the
   change can be routed at runtime; this ROM's pacing is placement-sensitive
   and split builds have confused two comparisons. `sample-tick-hud-buckets.ps1
   -SetGlobals name=value` is the mechanism (cycle 79).
8. **A routed arm must prove the route took before its ticks are read.** A poke
   that silently fails still produces a complete, plausible percentile table,
   which reads exactly like a candidate that engaged and saved nothing —
   `-SetGlobals` did this on its first two runs (see `VERIFYING.md`). Pair
   every `-SetGlobals` with an `-ExtraGlobals` counter that cannot be zero if
   the route engaged.

## Acceptance Matrix

As last graded (cycle 76); a row changes state only when its gate runs.

| Acceptance condition | State | Current evidence / blocker |
|---|---|---|
| Mario human vs original level-3 Fox CPU, Dream Land, one-minute Time, items off | Pass configuration | Boundary registry exposes only canonical mode 163 |
| Original Wait -> countdown -> GO, timer, scoring, Time Up, Results | Focused gates pass | Final exact-ROM CPU-on owner run remains red |
| Mario and Fox complete source-equivalent gameplay behavior | Red | Battle-reachable weak callbacks remain |
| Dream Land collision, platforms, blast zones, wind, camera | Pass for current P1 stage | Dynamic presentation debt remains red separately |
| Recognizable Dream Land presentation and required animation | Red | Whispy material/animation debt; Task 62 candidate rejected |
| Complete overlapping BGM, FGM, voices, announcer, crowd | Red | Exact pitch/composite/voice coverage and listen gates remain |
| Stable 30 FPS, representative P95 <= 1.12M ticks | Red | Gap **503,684 on the both-CPU gate arm**, 60 s match at 86.7% coverage (356,292 is the Boundary figure and is not the gate); lane G1–G4 |
| Stable reserve, no corruption, clean teardown | Focused gates pass | Requalify after the final content/performance candidate |
| Reproducible public artifact | Red | Current local root ROM differs from the pinned public identity |

## Artifact Identity

Pinned public-build identity from `README.md`:

```text
smash64ds-battle-playable-hwtri.nds
11,428,864 bytes
SHA-256 4D795B4E83B335598B20A3B5953FDB1821797CC5E0A825FA96A0643ABBA4A090
```

Current shipping pair (cycle 75), re-verified on disk 2026-08-05:

```text
smash64ds-battle-playable-hwtri.nds   12,129,280 bytes
SHA-256 D16815BEA6A1BA2592B679CA84F747F0A9B9682FF4AE20B9D0A1E22657D47825
smash64ds.nds                         11,790,336 bytes
SHA-256 369FA9993823605A377C0FAC269711A61E7E4773E8066ECB8EAD2F445BD61EF3
tick-HUD sibling (builds/build-c75-tickhud-publish)   12,131,328 bytes
SHA-256 15FD0F8E1467878CC1D65C41ADC895F1102E51DAEE21634937958E1123CCE2CC
```

ROM hashes are not reproducible across rebuilds of identical source; compare
sizes and the build log, not hashes, when attributing a ROM to a tree.

## Lane Ownership

| Surface | Owner |
|---|---|
| Goal, fidelity, milestone, definition of done | `PROJECT_GOAL.md` |
| Dynamic queue, artifact identity, blockers | this file |
| Exact restart surface and next packet | `HANDOFF.md` |
| Verification workflow and measurement law | `VERIFYING.md` + Standing rules above |
| Stable architecture | `ARCHITECTURE.md` |
| Durable unresolved gaps | `KNOWN_ISSUES.md` |
| Measurements and rejected experiments | `PERF_LEDGER.md` |
| Chronological history | `PORTING.md` |

## Integration Rule

Keep only correctness-preserving, verifier-covered progress. Rendering may use
the fidelity budget in `PROJECT_GOAL.md`; gameplay must remain mechanically
equivalent to the original. Run the smallest relevant check, then one widest
relevant verifier for a kept checkpoint.
