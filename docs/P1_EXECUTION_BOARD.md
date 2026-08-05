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
