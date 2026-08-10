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

**Superseded twice since.** Cycle 105's arena fix moved the gate arm to
**1,447,318** (gap 326,938) and cycle 108's AObj16 prebake takes a further
**~23,000**. The cycle-108 row carries why that figure is quoted as a range and
not as a single cross-build P95.

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

Noise floors — **recalibrated whole-match, cycle 100; the old ±5,376 was a
128-frame-era number and is far too tight**:

- **Same binary, same invocation: ZERO.** A whole-match run reproduces
  **bit-identically** — six runs over three binaries, rows-CSV SHA256 equal
  across every repeat pair. A figure that does not reproduce exactly means the
  invocation or the binary changed, not that the host was noisy.
- **Cross-build `WORK-H` P95: ≥14,080, and the sign is not reliable.** The same
  change measured on three A/B pairs moved P95 by −8,832, −2,368 and **+5,248**.
  Do not decide anything on a P95 delta below ~14,000, even between builds that
  link at the identical address.
- **Cross-build `WORK-H` P50: ~5,700**, and P50 kept its sign in all three
  pairs. **Rank on P50, mean and over-gate count; P95 is the gate's definition,
  not a usable A/B discriminator at these magnitudes.**
- Per-bucket placement ≥8,544 — buckets locate, `WORK-H` decides. 1.85 cycles of
  `FTR` mean per byte of added ARM text: a change that adds text must beat its
  own footprint.

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

### The baked draw plan is BUILT AND PROVEN EQUIVALENT, and blocked on a bss clobber (cycle 99)

**Nothing landed. Tree reverted to `b2575f12`; the patch is not in the tree.**
Three results the next cycle must not re-derive.

**1. The material-prep bake is REFUTED at the design step, from source.** The
cycle-97/98 reading that "damage-flash modulate is the only varying material
input" is wrong twice. `ndsRendererAdapterFighterColorModulate`
(`reloc_backend_renderer_dl.c:131`) is **not an input to
`BuildNativeMaterialSnapshot` at all** — it is a separate `color_modulate`
passed to the production-inputs builder. And
`decomp/BattleShip-main/decomp/src/sys/objanim.c:1390-1495`, imported live as
`src/import/battleship_sys_objanim.c:944-956`, writes **every** input the
snapshot reads: `scau scav trau trav scrollu scrollv lfrac primcolor envcolor
blendcolor light1color light2color`. Fighter texture parts additionally write
`texture_id_curr` (`reloc_backend_compat_shims.c:1392/1448`, mirroring
`decomp/.../ft/ftparam.c:1082-1183`). **There is no small enumerable
"varying words" set to patch unconditionally** — the varying set is the whole
snapshot, so an unconditional patch is the build. The 99.948% byte-identical
rate is a property of *this match's animation data*, not of the code, and
freezing on it is a fidelity change, i.e. the owner's call. Do not re-brief
"bake the material snapshot, patch what varies" without solving this first.

**2. What was built instead, and it works: the structural draw plan.** Bake the
collection, the resolved `NDSRelocLoadedFile`, root offsets, material counts,
matrix bindings and material DObjs once per (slot, owner-asset identity);
replay them; delete the per-frame walk, the whole eligibility pass, and
`ndsRendererAdapterValidateNativeOwnerCached`. Keyed on the validator's own
identity fields and additionally cleared from
`ndsRendererAdapterResetSceneCaches` (§3.12).

**Equivalence by construction, whole match, gate arm, frames 442–2041, DLDI ON,
`slips=0`, build `builds/build-c99-plan-bothcpu`:** a `#if NDS_TICK_HUD` mode
derived the plan live on every baked draw and memcmp'd it against the baked one
— **`gNdsFtrPlanVerifyMismatch` 0 over `gNdsFtrPlanVerifyRuns` 3,958**, covering
`material_dobj`, `matrix_dobj`, the resolved file and the root offsets, none of
which any cycle-98 counter covered. Engagement is an identity: hits 3,958 +
misses 5 (`gNdsFtrPreValidateReuse` 3 + `Build` 2) = **3,963 =
`gNdsFighterMarioFoxDLAllDrawCount`**, i.e. the validate ran **5 times in a
match instead of 3,963**. Fighter triangles 636,480 / 604,044 reproduce cycle 98
exactly. Cost: text **+1,008**, data 0, bss **+2,816**; headroom 108,480 →
**104,640** (`check-boot-headroom.ps1` OK); arena `ChosenSize` 1,351,680 →
1,347,584 with `AllocFailCount` 6 → **7**, exactly the one 0x1000 step the G2
model predicts for +3,840. `NDS_TICK_HUD=0` links and carries only
`gNdsFtrPlanRoute` (4 B) and `sNdsFighterDrawPlan` (1,864 B) — no instrument.

**3. RETRACTED, cycle 100 — there is no rogue store and the shipped ROM is not
corrupt.** This row read "something zeroes a fixed `.bss` address … that is a
live memory defect, it is older than this row". It is neither. The poke *lands*
and the guest never sees it: `-SetGlobals` writes main RAM through the GDB stub,
while `gNdsFtrPlanRoute` at `0x0226c560` begins a 32-byte ARM9 D-cache line
shared with `gNdsTickHudVBlankWaitTicks` (`0x0226c564`), which the tick HUD
writes every frame. That line is permanently resident and dirty, so the guest
keeps reading its stale cached 0 and each writeback stamps that 0 back over the
poke. See the cycle-100 section for the three-modality proof. `gNdsFtrPlanRoute`
is now selected at build time and the blocker is closed.

**4. SUPERSEDED, cycle 100 — "same-binary noise is ~9,800" is wrong.** The two
runs quoted here (**1,689,984** and **1,699,776**) are not a repeat pair: 1,586
of their 1,600 rows differ, largest single-frame delta −4,196,672, so they are
two different trajectory alignments rather than jitter on one. Measured
properly, a whole-match run **reproduces bit-identically** under a fixed binary
and invocation. The real floor is cross-build, it is much larger, and it is
calibrated in the cycle-100 section. Neither run is a baseline; **1,624,064
still stands.**

### The baked draw plan is PRICED and KEPT — a P50 lever, not gate progress (cycle 100)

**Default flipped to on** (`NDS_FTR_PLAN_ROUTE ?= 1`). The plan is now selected
at build time, not poked, for the reason in item 3 above.

**Equivalence first, and this time it survives in artifacts.** The cycle-99
claim of `VerifyMismatch` 0 existed only in console scrollback — all three c99
JSONs record `gNdsFtrPlanVerifyRuns` **0**, i.e. no surviving run ever engaged
the route. Re-established on both arms, whole match, 1,600 samples:

| arm | `VerifyRuns` | `VerifyMismatch` | `Hit`+`Build` | draws | fighter triangles |
|---|---:|---:|---:|---:|---|
| both-CPU | 3,960 | **0** | 3,960+3 | 3,963 | 636,480 / 604,044 |
| Boundary | 3,954 | **0** | 3,954+3 | 3,957 | 612,800 / 624,852 |

`Hit + Build == draw count` on both, and triangles are **byte-identical to the
control arm** on both. `gNdsFtrPreValidateReuse` falls **3,961 → 1**: the
owner-validate now runs 3 times a match instead of 3,963.

**The price — three A/B pairs, each pair two layout-identical builds** (both
arms pin the route flag to `.data`, so every pair links at the same
`fake_heap_start` with identical text/data/bss and the only difference is one
initialised word). Whole match, 1,600 samples, frames 442–2041, DLDI ON,
exclusion OFF, `slips=0`:

| pair | arm | `WORK-H` P50 | `WORK-H` P95 | over gate | `named` mean |
|---|---|---:|---:|---:|---:|
| c100b | **Boundary** | **−3,776** | −8,832 | −6 | −6,841 |
| c100 | both-CPU | −5,056 | −2,368 | −38 | −15,208 |
| c100b | both-CPU | **−9,472** | **+5,248** | −31 | −4,916 |

**P50 falls in every pair; P95 changes sign between pairs.** The P95 delta
spans 14,080 across three pairs of the same change, so **P95 is not resolvable
at this magnitude and no gate figure may be banked from this row.** This is the
same shape G1 measured and the board already predicted for `FTR`: it is a P50
lever, anti-correlated with the tail. `FTR` mean −9,415, `FTR` clean mean
−6,485 (c100 both-CPU pair).

Cost: shipped ROM pays `sNdsFighterDrawPlan` **1,864 B** `.bss` plus **8 B**
`.data` (the two route flags, which `.data` pinning keeps out of
`--gc-sections`), and **no instrument** — the `NDS_TICK_HUD=0` link
(`smash64ds-battle-playable-proof-hwtri`) carries exactly those three symbols
and none of the counters. Lab arms link at `0x0227af24`, **104,672 B** proven
headroom; the shipped-config link at `0x02272da4`, **137,824 B**. Arena
`ChosenSize` 1,347,584 / `AllocFailCount` 7 and heap
`gNdsTaskmanGeneralHeapFreeMin` 220,312 are **identical in both arms**, so the
plan adds no runtime memory pressure. Boundary verifier green; root ROMs
unchanged.

**Not done:** no new gate baseline (1,624,064 still stands), and the plan was
not extended past the fighter draw.

### The tail was CARTRIDGE I/O, and the animation arena was full — LANDED, cycle 105. **New gate baseline 1,447,318.**

`f082b3c8`. Whole match, both-CPU, 1,600 samples, frames 441/442–2041, DLDI ON,
exclusion OFF, `slips=0` both arms, `-AllowRepeatedFrames` (3/1600 control,
0/1600 candidate). Builds `builds/build-c105-anim-{ctl,cand}`; artifacts
`artifacts/performance/2026-08-09_c105-anim-{ctl,cand}{,-rows.csv}.json`.

| | control | candidate | delta |
|---|---:|---:|---:|
| `WORK-H` P50 | 1,102,720 | 1,112,576 | +9,856 |
| **`WORK-H` P95** | **1,639,299** | **1,447,318** | **−191,981** |
| `WORK-H` P99 | 2,043,946 | 1,671,012 | −372,934 |
| `SRC` P95 | 848,861 | 661,706 | −187,155 |
| `SINT` P95 | 477,286 | 380,515 | −96,771 |
| `SINT` frames > 400,000 | 113 | 67 | −46 |

**The diagnosis cost no build at all.** `-ExtraGlobals` on the existing tick-HUD
ROM at frames 478 and 2007: `gNdsRelocAssetPayloadReadCount` **+111** against
**113** frames whose `SINT` exceeded 400,000 (median 169,248). The arena read
`ReservedBytes 92,160 / UsedBytes 92,160 / Overflows 142 / Rejects 142` with
`OverflowLastSize` **912** — it was refusing a 912-byte animation with its cursor
at the ceiling, and a refused asset re-reads off the card on **every later use**.
`AUD` and `HUD`, whose work cannot vary with the match, read 4.9× and 2.5× on
those frames: the card stalls the machine, not just the caller.

**The 41-asset warm list was measured on the WRONG ARM.** It came off a Boundary
match; `NDS_R2_BOTH_CPU` puts a level-3 CPU on Mario too, and `gNdsR204AnimSeen`
dumped at presented frame 2038 holds **85** IDs of which the old list covered
**27**. Fixed by replacing the list with the measured 85 (union of both lists is
99 = 229,024 bytes and does not fit), sizing the arena to their 197,184 bytes
(`NDS_R2_ANIM_CACHE_ARENA_BYTES` 92,160 → **200,704**) and raising
`NDS_R2_ANIM_CACHE_ENTRIES` 64 → **128**, since 64 is below 85 and the entry
count is a hard refusal in both store paths.

Engagement exact: hits/misses/rejects **206/147/142 → 351/2/0**, overflows
142 → 0, `WarmLoaded` 39 → 83 (`WarmFailed` 0 both), `ArenaUsedBytes` 191,024 of
200,704. `gNdsR204AnimForceLoadTotal` **353** and `ForceLoadDistinct` **85** on
BOTH arms — identical, so the two runs played the same match and the delta is the
cache rather than a different trajectory. Pixel-identical at the
`time_remain=3000` lock over the whole 400×298 top screen at both captured tics,
max channel delta 0, against an **83.03%** same-build adjacent-tic floor on the
same crop. Boundary verifier passed; root ROMs unchanged.

**The cost is heap, and it is the last helping.** `gNdsTaskmanGeneralHeapFreeMin`
154,776 → **42,136** against the cache's own 32,768 `KEEP_FREE`, so the margin is
**9,368 bytes**. Boot headroom 34,816 proven; taskman arena 1,282,048 →
1,277,952 (one 0x1000 step, exactly the G2 model for +1,888 bytes). A soak
covering the full match, Sudden Death and Results read
`gNdsSyMallocOverflowCount` **0**, `ArenaReserveCount` 3 / `ReserveFailCount` 0,
`GenerationMismatches` 2 / `RangeFaults` 0. **Any future heap taker must free
something first** — and freeing `.bss` is the way, because that lowers
`fake_heap_start` and enlarges the heap the arena callocs from.

P50 rises with the +1,888 bytes, which is what puts 63 more frames over the line
(691 → 754) while P95 falls. The gate is P95.

### The residual tail is the FORCE-LOAD HIT PATH, not I/O — MEASURED, cycle 105

Per-frame probe, `-PerFrameGlobals`, candidate ROM, frames 1100–1129
(`artifacts/performance/2026-08-09_c105-spikeprobe{,-rows.csv}.json`). **Every
`SINT` spike is a frame with `gNdsR204AnimForceLoadTotal` +1 and every other
frame is +0 — 5 of 30, no exceptions in either direction**, with
`gNdsRelocAssetPayloadReadCount` and `…HeaderReadCount` both **+0** on all thirty.

| frame | ΔForceLoad | `SINT` | excess over the ~170,000 baseline |
|---:|---:|---:|---:|
| 1110 | 1 | 739,904 | **+570,000** |
| 1119 | 1 | 286,976 | +117,000 |
| 1120 | 1 | 335,104 | +165,000 |
| 1125 | 1 | 335,488 | +165,000 |
| 1126 | 1 | 295,936 | +126,000 |

So a **cache HIT costs 117,000–570,000 ticks** (~228,600 mean over these five).
That is not the memcpy. The Makefile already names the owner and already carries
its instrument: `NDS_R2_RELOC_FIXUP_TIMING` (`Makefile:307`, R2-06 E8) prices
`ndsRelocFinalizeLoadedFile`'s **five passes** separately, and E8 had already
traced 8 of 9 over-gate frames to the 16 frames that function runs on.

**Worth 121,331 at P95** — capping `SINT` at its own median takes the candidate
1,447,318 → 1,325,987 (over-gate 754 → 517), leaving a gap of 205,607. The
fixups are a pure function of the asset image and the cached image is
byte-identical every time, so the shapes to price are a precomputed fixup offset
list built once at warm time, or a (asset_id, destination) keyed cache of the
*post*-fixup image — the "position-dependent" objection in
`reloc_backend_assets.c` is about replaying into a DIFFERENT heap, and R2-04 E0
already recorded that the destination is caller-owned and reused.

### The force-load frame is PARTLY ATTRIBUTED (25.1%), and the rest needs a design not a micro-fix (cycle 106)

Two instrument builds, both flags the Makefile already carried for this exact
question. `builds/build-c106-loadsplit`
(`NDS_R2_LOADFRAME_TIMING=1 NDS_R2_RELOC_FIXUP_TIMING=1`), whole match, gate arm,
frames 443–2042; artifact `artifacts/performance/2026-08-09_c106-loadsplit.json`.
Denominator: total `SINT` above its own median across the 1,600 frames =
**63,115,584**.

| owner | ticks/match | % of the `SINT` excess |
|---|---:|---:|
| `ndsRelocFinalizeLoadedFile` **AObj16 pass** | 10,236,800 | **16.2%** |
| — of which Normalize / Swap / Successor | 4,168,960 / 3,250,752 / 1,704,320 | 6.6 / 5.2 / 2.7 |
| `gcAddDObjAnimJoint` (6,459 calls) | 3,422,272 | 5.4% |
| Fixup Internal + External + Attributes | 1,122,176 | 1.8% |
| `gcAddAnimJointAll` (55 calls) | 1,075,008 | 1.7% |
| **accounted** | **15,856,256** | **25.1%** |

E8's split reproduces: relocation 18.0% here against its 21.5%. **But the add
wrappers capture only 7.1%, so "the action change is ~78%" is true and
`gcAdd*AnimJoint*` is NOT where it lives** — 74.9% (47.3M ticks, ~134,000 per
force load) is still unattributed. `FixupSpritesTicks` 22,448,768 against
`FinalizeMaxTicks` 21,776,704 is ONE call at scene load, outside the window;
exclude it from any in-match figure.

**The whole-frame profile, gate arm, is banked** —
`artifacts/performance/2026-08-09_c106-profile/` (`census.{txt,json}`,
`arm9-profile.csv`), 400 frames from 441, per-frame regions, 1.21G cycles,
14.4M PCs, 1,205 of 3,545 FUNC symbols hit. Read section A (total cycles) as the
current cost ranking; **read section E's over-gate split with care on this ROM**
— the profiler build is slow enough that the 2-VBlank threshold marks 307 of 400
frames, so `armWaitForIrq` takes 58.9% of the "premium" and that is quantised
idle, not work. Instrument rows to discount in E: `ndsPlatformRenderDebugHud`
+40,955 plus the printf family (`_svfiprintf_r`, `_vfiprintf_r`,
`__ssvfiscanf_r`, `consolePrintChar`, `__utf8_mbtowc`) ≈ 72,700/frame together —
that is the tick HUD's own text and the shipped ROM pays none of it.

**Three levers this profile pointed at were ALREADY TRIED AND DOCUMENTED. Read
the owning file before building any of them again.**

- **`.text.hot` placement.** Section C ranks `ndsR2CubicValueFixed` (2,032 B) the
  #1 unplaced candidate for the 3,936 free bytes, exactly as it did for R2-03
  E66 — which admitted it and measured **`WORK-H` P95 +24,448**, P90 +8,000,
  over-gate 7 → 8. `linker/nds_hot_text.ld:179-201` carries that and the Task 94
  regression on the same list. **`.text.hot` is closed in both directions, and
  census sections C/D are a cost ranking, never a placement prediction.**
- **Hoisting the animation range check in `ndsRelocAssetIDForToken`.** R2-06 E11
  did it, with negative bytes added: the function fell 39,475 → 31,808 per load
  frame and **still lost** — `WORK-H` P95 +15,744, P99 +59,200, over-gate 9 → 11.
  `reloc_backend_assets.c:1840-1895` carries the full reasoning.
- **The `ndsAObjEvent32Normalized` linear scan.** Looks O(n²) — a 1,024-entry
  scan called from inside the command loop — but measured, the whole match makes
  **448** `NormalizeScript` calls (125 new + 323 reuse) over **973** commands, so
  it is ~1,375 ticks/frame and cannot be the spike.

**But that scan has a LATENT CORRECTNESS CLIFF worth its own row.**
`sNdsAObjEvent32NormalizedCount` reads **973 of `NDS_AOBJ_EVENT32_NORMALIZED_MAX`
1,024** at the end of a one-minute match, with `NormalizeFailCount` 0 — a
**51-entry margin**. Overflow takes the `ndsAObjEvent32Reject(12u, …)` path,
which returns FALSE, and the caller then **skips `ndsBaseGcAddDObjAnimJoint`
entirely** — i.e. the animation silently does not attach. A longer match, a
rematch, or a wider moveset would hit it. It is a table bound, not a heap bound,
so raising it costs 8 bytes an entry in `.bss`.

**What the next row must be, and the size it has to clear.** E11's own conclusion
is the standing rule now: *"a load-frame-only saving of ~8,000 ticks cannot be
banked through P95 here, because relinking moves the tail by more than the
saving. Either remove this work in one change large enough to clear ~16,000 of
tail movement, or move it off the gameplay frame entirely."* Cycle 105's arena
fix is what that looks like when it works — it moved the I/O off the frame rather
than making it faster. The equivalent for the remaining half is the **second**
repair E8 named: one **pre-finalized, resident** copy per warmed animation, so
the force load returns a pointer instead of memcpy + fixups + swap + normalize.
Design blockers to answer first, in order: the fixups write absolute pointers
derived from `loaded->data` (so a shared copy pins the destination), normalization
writes `command->u` **in place** (so the resident image is mutated — establish
whether anything writes the script after that), and the destination is
caller-owned through `lbRelocGetForceExternHeapFile(file_id, heap)`.
**RAM is the hard constraint: 85 × ~2,319 = 197,184 more bytes do not exist**
(heap free-min 42,136), so this has to REPLACE the per-load destination copy, not
sit beside it.

### The load frame is FULLY ATTRIBUTED, from the CSV that already existed (cycle 107)

`task37_census.py --split-by-symbol ndsRelocFinalizeLoadedFile` over the cycle-106
profile — **no build and no emulator run**, because `--split-by-symbol` partitions
the existing per-frame regions by whether a frame executed that symbol, and that
symbol executes on exactly the load frames. **74 marked frames vs 326 control,
premium 650,610 cycles/frame.** This supersedes the cycle-106 over-gate split,
whose threshold marked 307 of 400 frames and therefore mostly measured idle.

| symbol | +cyc/frame | %prem | |
|---|---:|---:|---|
| `armWaitForIrq` | 137,472 | 21.1 | quantised idle, not work |
| **`ndsRelocFinalizeLoadedFile`** | 65,335 | 10.0 | fixups |
| **`battleship_ftAnimParseDObjFigatree`** | 50,923 | 7.8 | real: new animation evaluated |
| **`ndsRelocAssetIDForToken.part.0`** | 43,875 | 6.7 | pure function, 110-branch chain |
| `ndsR2CubicValueFixed` | 22,112 | 3.4 | real |
| `memcpy` | 21,168 | 3.3 | payload copy |
| `gcPlayDObjAnimJoint` / `__aeabi_fadd` / `__aeabi_fmul` | 17,839 / 18,657 / 15,855 | 8.0 | real |
| `battleship_ftMainSetStatus` | 13,737 | 2.1 | real |
| `gcAddDObjAnimJoint` / `ndsFTParamsInvalidateFighterParts` / `armCopyMem32` | 9,757 / 9,513 / 9,263 | 4.4 | |
| alias churn (`Remove…StatusAliases`, `FindLoadedFileContaining`, `Remove…LoadedAliases`) | 5,395 / 4,688 / 3,528 | 2.0 | |

**Two blocks of almost equal size, and only one of them is recoverable.** The
reloc + copy family is **153,252/frame (23.6%)** and is pure port-side overhead
delivering bytes that are already in RAM; the animation re-evaluation is
**158,393/frame (24.3%)** and is real gameplay work — the fighter genuinely
changed animation and the new one is being evaluated. Do not brief the second as
waste.

### The token memo is REFUTED — the function is pure, the KEY is not stable (cycle 107)

`ndsRelocAssetIDForToken` looked like the ideal target: 6.7% of the load-frame
premium, **3,246,729 of its 3,246,729 cycles on load frames**, and a provably
pure function — every branch compares against an `ll*FileID` link-time address or
an integer literal, the two tail scans walk `static const * const` arrays of the
same addresses, and `ndsRelocIsMarioFoxAnimID` is range arithmetic. A memo needs
no invalidation at any scene boundary. **It still does not work.** Three builds,
each verified with a route-2 arm that consults the memo and then runs the chain
anyway and compares:

| config | hits | misses | evicted / declined | occupancy | hit rate | `VerifyFail` |
|---|---:|---:|---:|---:|---:|---:|
| 64 entries, evicting | 9,144 | 12,746 | 12,682 | — | 41.8% | **0** |
| 512 entries, evicting | 10,973 | 10,917 | 10,405 | — | 50.1% | **0** |
| 512 entries, **no-evict** | 4,257 | 17,633 | 17,121 | **512 of 512** | 19.4% | **0** |

`VerifyFail` 0 in all three over 24,374 verified hits, so purity is confirmed
dynamically and is not the problem. **Eight times the table moved the hit rate
41.8% → 50.1%, and no-eviction made it worse by filling every one of 512 slots
with keys that never repeat** — the population is large and mostly non-repeating,
which is the refuted `(dl-pointer, bind-ordinal)` `Tex` memo shape exactly.

**And R2-06 E11's own data already said why, which is the part worth keeping:
the 74.3% of calls that resolve inside the compare chain are the CHEAP ones, and
the cost is the 14.3% that miss and walk both 143 + 158-entry scans.** Those are
precisely the unstable-key calls a memo cannot hold. So the two facts compose:
the calls worth caching are the ones that cannot be cached.

Reverted; the patch is not in the tree. **Do not re-attempt a token-keyed cache
here.** What is still open is narrowing the *scan*, not caching its result — the
two arrays' contents are link-time constants, so an `[min,max]` bound computed
once at init would reject an out-of-range token in two compares instead of 301
iterations, and it would bite on exactly the 14.3% that are expensive. That is
unmeasured and it must clear E11's ~16,000 tail-movement bar to be bankable.

**Method note worth more than the result: read Evicts, not Hits.** A memo whose
misses are nearly all evictions is undersized or mis-keyed, and its hit rate says
nothing about which. Both numbers were in the instrument from the first build,
which is why this cost three builds instead of shipping a 4.6 KB regression.

### The AObj16 pass is PREBAKED at warm time — LANDED, cycle 108. ~−23,000 P95.

The first thing cycle 107's attribution said to move rather than speed up.
`ndsRelocNormalizeFighterAObj16File` (table-bytes discovery, u16 lane swap over
the script region, O(n²) successor scan, per-script normalize) ran on every
force load. It is **position-independent** — it never touches the pointer table
and every quantity it writes is an offset — so the transformed image survives
the per-destination copy and the pass can run **once per warmed animation**
instead of once per load.

The obstacle was the internal fixup list, which is *intrusive*: each raw slot
word is `(next_slot_index << 16) | target_word_index`, so applying the fixups
consumes the list, and the pass needs them applied. `ndsR2AnimPrebakeAObj16`
therefore records every `(offset, raw word)` while walking the list, applies,
transforms, then writes the raw words back. The restore is exact by
construction and independent of where the slots sit — deliberately stronger
than restoring a table region and assuming no slot lives past it. Every failure
path restores and declines with `aobj16_ready` 0, so a decline is a performance
outcome and never a correctness one.

**Engagement, whole match, both-CPU:** `PrebakeReady` 85 of 85 distinct,
`Skips` 351 of 351 cache hits, `SlotsMax` 21, all four decline counters 0,
`ExternalFixupFailCount` 0, `RelocPointerFixupFailCount` 0,
`NormalizeFailCount` 0. Trajectory unchanged: `ForceLoadTotal` 353, `Distinct`
85, `Hits` 351, `Misses` 2. `gNdsTaskmanGeneralHeapFreeMin` 42,136 — unchanged,
because the scratch is 64 slots against a measured max of 21.

**This row exists because the first two arms disagreed, and that is the lesson.**
Two separately-linked builds of the *same* change, differing only by 3,584 bytes
of scratch, read `WORK-H` P50 1,093,152 and 1,119,136 — **25,760 apart, 4.5× the
cross-build P50 floor** — while the second did strictly *more* work (351 skips
against 259) and used *less* RAM. Their P95 read −39,702 and −22,806 against the
c105 control. Relinking moved the body further than the change did, exactly as
R2-06 E11 says it will.

Settled on **one binary** with the runtime route `gNdsR2AObj16PrebakeRoute`
(`.data`, default 1, poked by `-SetGlobals` at the first frame-complete marker),
which is what standing rule 7 and the sampler's own header ask for:

| `build-c108-route`, one ROM | P50 | P95 | mean | over gate | SINT P95 | SRC P95 |
|---|---:|---:|---:|---:|---:|---:|
| route 0 | 1,110,400 | 1,431,696 | 1,136,266 | 741 | 360,195 | 653,632 |
| route 1 | 1,110,144 | **1,416,003** | 1,141,129 | 739 | 346,352 | 642,515 |
| delta | −256 | **−15,693** | +4,863 | −2 | −13,843 | −11,117 |

The route-0 arm is a *partial* control, and its counters say so: three entries
warm before the poke lands (warm stepping runs one asset per scene update) and
they are the hottest, so 111 of 351 hits already skip. The delta therefore
covers **68.4%** of the change → **~−23,000 whole**, which is what the
separately-linked arm read (−22,806). **The −39,702 was placement, not work.**
P50 −256 and mean +4,863 are the confirmation that the change touches only load
frames, as designed.

**Pixel-identical**, and with the floor beside it as `capture-cut-g-exact-frames`
requires: control `2026-08-09_c105-anim-cand-t3000-a.png` vs
`2026-08-09_c108-prebake-t3000-a.png` at the same tic differ on **0 of 119,200
pixels, max channel delta 0**, against a same-build adjacent-tic floor of
**83.03%** (98,968 pixels, identical in both builds). Boundary verification
profile passed. Promoted to both Makefile config blocks.

**Keep the route flag.** It is four bytes in `.data` and it makes the next A/B on
this seam free. Every future arm on a placement-sensitive seam should be one
binary with a route before it is two builds — this cycle spent four measuring
runs re-learning that.

**Worth ~7% of the 326,938 gap.** Banked and moved past; do not polish it.

### `ndsFTParamsInvalidateFighterParts` — the shaped target, and a dead pool (cycle 108)

The census's best-shaped candidate, traced to its mechanism.
`reloc_backend_compat_shims.c:1474` is a **recursive walk of the joint tree whose
entire job is invalidation**:

```c
parts = joint->user_data.p;                     /* 37.1 cyc/ex -- miss */
if (parts != NULL) { ...; parts->unk_dobjtrans_word = 0; }
for (child = joint->child; child != NULL; child = child->sib_next)
    ndsFTParamsInvalidateFighterParts(child, reset_mode);
```

It writes **one zero per part** and pays a pointer chase to reach each one:
79,874 executions of each of its two hot loads, at **37.1 and 30.8 cycles**, for
**4,946,580 of its 6,529,067 load excess**, at CPI **7.08**. The comment already
above it says the diagnosis was known — *"a recursive walk down the joint tree
whose every hot PC is a pointer-chasing load. The loads keep their cost wherever
this lives; what ITCM buys is the fetch of the loop around them"* — so the ITCM
pin it already carries was applied knowing it could not fix the loads. **Only a
layout change can.**

**And the layout change was already written, then left dead.**
`reloc_backend_compat_shims.c:494-496` declares

```c
static FTParts sNdsFTManagerPartsAllocPool[64];
static FTParts *sNdsFTManagerPartsAllocFree;
static sb32 sNdsFTManagerPartsAllocInit;
```

with an initialiser at `:499` that threads them into a free list — and **the
compiler reports all three as "defined but not used" on every build.** The pool
allocator is unreachable, so `FTParts` come from scattered heap allocations
instead, which is exactly why the walk misses on every node.

#### Both premises above are REFUTED (cycle 109) — the pool is NOT the fix

**1. The root-joint precondition fails, on paths that run every match.**
`nFTPartsJointTopN` is `0` (`ftdef.h:1071-1078`; `TransN` 1, `XRotN` 2, `YRotN`
3, `CommonStart` 4), so every `joints[4]` caller is passing a sub-joint:
`ftcommondamage.c:210` (damage), `ftfoxspecialhi.c:143` (Fox up-B),
`ftnessspecialhi.c:519`, `ftpikachuspecialhi.c:159`. `ftcommonguard1.c:256`
passes `joints[YRotN]` and `:358` `joints[XRotN]` — shielding, both fighters,
constantly. And `ftparam.c:2637-2638` invalidates two IK children by pointer
inside `func_ovl2_800EBD08`. A flat whole-fighter sweep would invalidate strictly
more than the original, so **the subtree structure is load-bearing** and cannot
be dropped.

**2. The two expensive loads are `DObj` fields, so a contiguous `FTParts` pool
cannot touch them.** Disassembling the ITCM copy and joining per-PC:

| pc | instruction | field | execs | cycles | cyc/ex |
|---|---|---|---:|---:|---:|
| `1fff274` | `ldr r4,[r0,#16]` | `joint->child` | 79,874 | 2,964,305 | **37.1** |
| `1fff266` | `ldr r3,[r0,#132]` | `joint->user_data.p` | 79,874 | 2,461,519 | **30.8** |
| `1fff282` | `ldr r4,[r4,#8]` | `child->sib_next` | 76,430 | 878,273 | 11.5 |
| `1fff272` | `str r2,[r3,#4]` | `parts->unk_dobjtrans_word` | 79,874 | 1,365,294 | 17.1 |
| `1fff28a` | `ldr r2,[r3,#0]` | `parts->transform_update_mode` | 41,135 | 1,056,909 | 25.7 |

**DObj traversal is 6,304,097 cycles; everything `FTParts` is 2,444,875.** The
pool addresses only the smaller half, and offsets 16 and 132 are 116 bytes apart
— two cache lines *per node*, in a struct whose layout is mirrored from decomp
and therefore not ours to repack (`check-decomp-header-mirror.py`).

**3. The whole function is too small to matter.** 13,718,726 cycles = **1.40% of
non-idle**. Deleting it entirely is worth ~15,500 ticks/frame at P50; the real
fix is worth ~6,560. Recursion accounts for 76,430 of the 79,874 entries, so
there are only **3,444 external calls walking ~23 joints each** (~11.2 calls per
frame, two fighters).

**The fix that would work, if it is ever worth a build:** flatten the joint tree
into a DFS-preorder `FTParts*` array once at build time, and give each joint its
subtree's `[start, count)` — a subtree is contiguous in preorder, so *any* joint
(not just a root) invalidates as a linear sweep, and precondition 1 stops
mattering. That removes both DObj loads and the `sib_next` chase: ≈5.8M, **0.59%
of non-idle ≈ 6,560 ticks/frame at P50**. Above the P50 floor (~5,700), below
the P95 floor (14,080), and **1/6 of the animation lane below**. Queue it behind
that; do not spend a build on it alone.

**The dead pool may now be deleted as ordinary cleanup** — the earlier "do not
delete, it is the intended fix" note is withdrawn. It addresses 2.4M of the 8.7M
and needs a flattening pass to be reachable at all.

### The fighter animation lane is 8.85% of non-idle — the largest lever found

Priced off the cycle-106 whole-match profile (no build, no emulator run), self
cycles from `census.json` and soft-float charged back with
`analyze-leaf-helper-attribution.py`:

| symbol | self | soft float | total | % non-idle |
|---|---:|---:|---:|---:|
| `battleship_ftAnimParseDObjFigatree` | 16,192,916 | 7,931,155 | **24,124,071** | 2.47% |
| `gcPlayDObjAnimJoint` | 16,595,669 | 6,706,718 | **23,302,387** | 2.38% |
| `ndsR2CubicValueFixed` | 19,420,815 | 2,542,351 | **21,963,166** | 2.24% |
| `gcPlayAnimAll` | 7,085,886 | 130,690 | 7,216,576 | 0.74% |
| `ftParamUpdateAnimKeys` | 5,291,274 | — | 5,291,274 | 0.54% |
| `ndsBaseGcPlayDObjAnimJoint` | 2,827,175 | 1,912,301 | 4,739,476 | 0.48% |
| **lane** | 67,413,735 | 19,223,215 | **86,636,950** | **8.85%** |

At `WORK-H` P50 1,107,008 that is **~98,000 ticks/frame**. The parser and the
joint evaluator are **the #1 and #2 soft-float callers in the entire build**,
ahead of collision, matrices and particles.

**Three findings decide what to build.**

**(a) The fixed cubic is not the problem — it is the most efficient thing in the
lane.** `ndsR2CubicValueFixed` runs at **CPI 1.74** against a build average of
2.85: compute-bound, already good. But it costs **320 cycles and 184
instructions per call** over 60,582 calls, of which the 10-register
`push`/`pop` pair alone is **1,707,080 cycles (8.8% of the function)**. It has no
hot site — the cost is flat, i.e. the *conversions and the call*, not the
Hermite. Making it "more fixed-point" is finished work.

**(b) `AObj` is 3.2x the D-cache, and the profile shows it missing on every
node.** The struct is 36 bytes (`objtypes.h:124-136`: `next`, `track` @4, `kind`
@5, six `f32`, `interpolate` @32). The hottest instruction in
`gcPlayDObjAnimJoint` is `ldrb r5,[r4,#5]` — `aobj->kind` — at **24.1 cyc/ex
over 143,916 executions = 3,465,773 cycles, 20.9% of the function**; with
`ldr r4,[r4,#0]` (`aobj->next`, 7.0 cyc/ex) the bare list walk is **4,470,121
cycles, 26.9%**. 143,916 visits over 42,210 calls = **3.41 AObj per joint**. At
~360 live nodes the working set is **12,960 bytes against a 4KB D-cache — 3.2x,
so it can never stay resident.** A 12-byte track brings it to 1.05x; **8 bytes
fits.** This is the strongest single argument in the lane and it is a layout
argument, not an arithmetic one.

**(c) The parser is the biggest item and the only one AOT deletes outright.**
`ftAnimParseDObjFigatree`'s top three loads are `ldr r4,[r0,#116]` at **33.1
cyc/ex**, `ldrb r3,[r2,#4]` at **29.6** (the bytecode stream, byte at a time) and
`ldr r3,[r3,#4]` at **25.6** — 3,125,707 cycles, 19.3% of the function. It is
re-interpreting a stream that never changes, which is the textbook
compute-once case in `PROJECT_GOAL.md`.

**Sizing.** Removing the parser interpretation (~60% of 24.1M), the AObj chase
(~3.5M), the conversion boundary in the evaluator (~2.2M soft float + ~7.8M
self) lands ≈**34M = 3.5% of non-idle ≈ 38,700 ticks/frame at P50**; carrying
fixed point through to the matrices reaches ≈60,000. Against the ~304,000 gap
that is 13–20% in one campaign — **6x the flattened-invalidate fix, and above
every noise floor.**

**Constraints any implementation must respect.**

- **It must not grow RAM.** Static headroom proven is **34,816 bytes** and
  +14KB of `.bss` once stopped the ROM booting. AOT tables must ship as **files
  through the existing anim cache arena** (`NDS_R2_ANIM_CACHE_ARENA_BYTES`
  200,704, `KEEP_FREE` 32,768) in the slot the figatrees already occupy — not as
  linked-in arrays. Same bytes, better content.
- **It must replace, not coexist.** 1.85 cycles of `FTR` mean per byte of added
  ARM text; a runtime selector between old and new evaluator pays for itself
  twice and wins nothing.
- **Derive the phase, do not accumulate it.** `t = length * length_invert`
  recomputes from scratch each frame, so its error is bounded per frame; a
  `phase += phase_step` accumulator **drifts over a long animation**, and
  animation drives hitbox positions and therefore knockback. Keep an integer
  frame counter and compute `phase = (frame * step) >> k`. This is the one part
  of the sketched design that is not equivalent-by-construction.
- **`length_invert` has readers outside the evaluator** —
  `reloc_backend_mp_collision.c:11918` writes it, and
  `battleship_sys_objanim.c` reads it at `:214`, `:292`, `:921`, `:942`
  (including two `length_invert <= length` runtime-float compares). Deleting the
  field is a wider change than deleting its use in the cubic.
- **Fighter path only.** `ndsBaseGcPlayMObjMatAnim` is a separate 4,463,648-cycle
  soft-float caller on the same `AObj` infrastructure; material, camera and
  stage animation must keep working unchanged.

**Do the free `SINT`/`SPHD`/`SHDT`/`SCPU` split first** — it costs no build and
those three are ~79.7K of over-gate discriminator, which can reorder this queue.

### Over-gate split: animation is the largest REAL discriminator (cycle 109)

Ran the free over-gate split off the existing census. **The raw ranking's #1 row
is the measuring instrument** — `ndsPlatformRenderDebugHud` 40,955 plus
`_svfiprintf_r`, `_vfiprintf_r`, `__ssvfiscanf_r`, `consolePrintChar`,
`__utf8_mbtowc` = **72,733 of 437,886, i.e. 16.6%**. `WORK-H` already subtracts
it, but anyone reading the census split directly will rank the tick HUD first.
Real discrimination is **365,153 cycles/region**:

| class | delta/region | % real |
|---|---:|---:|
| **animation lane** (incl. soft-float share) | **72,638** | **19.9%** |
| asset load | 51,789 | 14.2% |
| effects/renderer | 21,489 | 5.9% |
| `ndsFTParamsInvalidateFighterParts` | 6,563 | 1.8% |
| collision | 5,134 | 1.4% |

Animation is the largest class; asset load is second and partly taken already by
cycle 108's prebake, which postdates this profile. `SPHD`/`SHDT`/`SCPU` do not
appear as distinct symbol classes — their bucket deltas spread across collision
and soft float, neither competitive. **The split confirms the queue rather than
reordering it.** Independent agreement worth noting, not proof: the invalidate
walk reads 6,563 here against the 6,560/frame derived from its cycle share.

### Parser AOT slice: the format is compilable, but the win is not where it looked

Traced the whole `event16` stream. It is a u16 word stream —
`opcode:5, flags:10, toggle:1`, then an optional u16 duration, then one s16 per
set flag bit (`AObjAnimAdvance` is `p++`). Two things follow.

**The stream is already the compact fixed-point representation.**
`ftAnimGetTargetValue` just multiplies the s16 by a power-of-two frac
(`1/512` rotation, `1/4` translation, `1/4096` scale, and two non-power-of-two
`1/16384 - 3e-12` entries for `TraI`). Compiling to f32 records would make the
stream **2x bigger** — 20 bytes against 10 for a 3-track command — which on a
memory-bound ARM9 is a likely net loss. **The win is the conversion boundary and
the AObj layout, not a new file format.** Priced from the profile, the parser's
7,931,031 cycles of soft float are: `fcmpeq` 921,383 · `fsub` 1,753,743 ·
`i2f` 796,253 · `fadd` 1,454,187 · `fmul` 920,213 · `fcmpgt` 385,779 ·
`fcmple` 204,854 · `fdiv` **1,494,619 at 109.4 cycles a call**, the most
expensive helper in the build by 3x, on `1.0F / payload` where **payload is a
u16 frame count** — i.e. a reciprocal table hits every time.

**The parser's #2 hot load is the same AObj walk as the evaluator's #1.**
`ldrb r2,[r2,#4]` is `aobj->track` in the `track_aobjs[]` gather (29.6 cyc/ex),
mirroring `ldrb r4,[r4,#5]` = `aobj->kind` (24.1). **~6.2M of pointer chasing
across the two functions, one root cause.**

**Root cause found and first piece landed.** `gcSetupObjman` threads
`setup->aobjs[0..n-1]` into `sGCAnimHead` in ascending address order
(`objman.c:2462-2475`) — and **`aobjs_num` is zero in every scene**; `rg` over
`src/`, `sc/` and `vs/` finds no writer anywhere. So `sGCAnimHead` starts NULL
and every AObj in the game is an individual 36-byte `syTaskmanMalloc`
(`objman.c:640-645`) interleaved with everything else the scene allocates.
`battleship_sys_objman.c` now fills that seam with one contiguous block —
the same pooling the file already documents for GObj thread stacks, re-carved
per setup because the arena resets between scenes, declining safely to today's
behavior if the arena is tight. **No struct, format or arithmetic changes:
allocation locality only, so behavior is bit-identical.** Built green,
`check-boot-headroom` 33,216 proven.

**It cannot be measured alone, and neither can any other single piece.**
Estimated ~2,800 ticks/frame against a cross-build P95 floor of 14,080; and
**standing rule 7's runtime route does not apply here** — the seam runs at scene
setup, before the first frame-complete marker where `-SetGlobals` pokes, so
there is no one-binary A/B for it. Every piece of this lane is 1,500–7,800
individually. **Bundle them and measure once**, per the "clear ~16,000 in one
change" rule: AObj pool (~2,800) · comparisons through the proven
`nds_fcmp.h` (~1,500 parser + evaluator share) · `ftAnimGetTargetValue` by
integer bit assembly, exact for the six power-of-two tracks and needing
`target("arm")` because Thumb-1 has no `CLZ` (~1,240) · reciprocal table for
`1.0F/payload` (~1,430) · the evaluator's conversion boundary and inlining its
10-register `push`/`pop` (~2,000–7,800). Verify the AObj sites with
`analyze-dcache-stalls.py`, not with ticks — the mechanism check is immune to
the placement floor.

### SINT decomposed: it IS the animation lane (cycle 109)

`SRC_CPI_OPTIMIZATION.md` called this "the big one" — *which child of
`ftMainProcUpdateInterrupt` causes the +88,082?* Answered with
`scripts/analyze-subtree-attribution.py` (new, no build, no run: static call
graph from the disassembly, subtree per direct child, cycles and over-gate delta
from `census.json`). **Two children carry all of it:**

| direct child (exclusive subtree) | cycles | % non-idle | over-gate | syms |
|---|---:|---:|---:|---:|
| **`ftMainPlayAnim`** | **73,169,415** | **7.48%** | **+60,559** | 8 |
| `ftComputerProcessAll` | 49,550,399 | 5.06% | +24,386 | 71 |
| everything else | 2,432,810 | 0.25% | +2,131 | 29 |

**+84,945 of the +88,082.** And `ftMainPlayAnim`'s eight exclusive symbols are
exactly the animation lane — `ndsR2CubicValueFixed` 19.4M ·
`gcPlayDObjAnimJoint` 16.6M · `ftAnimParseDObjFigatree` 16.2M ·
`ndsFTParamsInvalidateFighterParts` 13.7M · `ftParamUpdateAnimKeys` 5.3M ·
`gcPlayMObjMatAnim` 1.1M · `ftMainPlayAnim` 0.5M ·
`ftParamsUpdateFighterPartsTransform` 0.4M.

**So the plan's steps 1, 2 and 3 collapse into one target.** Step 1 (the FTParts
pool) is refuted above *and* is only 6,563 of the child's 60,559. Step 2 says go
to step 3. `ftComputerProcessAll`, the other child, is **not AI logic** — it is
map collision: `mpCollisionGetFCCommonFloor` 7.5M,
`ndsStageMPSweepFloorLoopSweep` 6.4M, `ndsMPFindLineEndpoints` 5.4M,
`ndsStageMPAdjustFloorLoopWallSweep` 4.1M. A separate lever, correctly ranked
second.

**Do not quote a subtree number for `SPHD` or `SHDT`.** Their roots each have one
child that statically reaches 511 and 186 symbols, so the tool's own
"reachability is not execution" limit dominates and the percentages are
meaningless there. SINT's exclusive sets are small (8 and 71) and clean.

### Animation lane: what each remaining cut is actually worth

Priced per call site from the profile, with the corrected per-helper costs
(`fadd` 36.43, `fmul` 25.17, `i2f` 16.80, `fdiv` **109.38**, `fcmpeq` 10.59):

| cut | cycles | ticks/frame | status |
|---|---:|---:|---|
| `aobj->length += anim_speed` — 111,168 `fadd` | **4,049,850** | ~4,600 | **needs the representation change** |
| parser `fsub`+`fadd` clock arithmetic | 3,207,930 | ~3,640 | same |
| parser `fdiv` on `1.0F/payload` | 1,494,619 | ~1,700 | reciprocal table, payload is a u16 |
| cubic `length * length_invert` `fmul` | 1,524,849 | ~1,730 | fuse into the fixed conversion |
| parser `ftAnimGetTargetValue` i2f+fmul | 1,534,213 | ~1,740 | CLZ bit assembly, needs `target("arm")` |
| AObj list scatter (both functions) | ~6,200,000 | ~2,800 | **DONE** — pool, cycle 109 |
| evaluator `fcmpeq` ×180,454 | 1,911,008 | ~2,170 | **DONE** — `nds_fcmp.h` |
| cubic `(f32)q` i2f | 1,017,778 | ~1,160 | **DONE** — cycle 109 |

**No single remaining cut clears the 14,080 cross-build P95 floor**, which is why
they ship as one bundle and get measured once. The largest, at 4.6K, is one
line: `aobj->length += dobj->anim_speed`, executed for every active node every
frame.

**That line is why the representation change is unavoidable.** `length` is
consumed only as `length * length_invert` (cubic), `length * rate_base`
(linear), and `length_invert <= length` (step) — every consumer immediately
converts it or multiplies it. As Q16 it would make all three cheaper *and* turn
the `+=` into an integer add. But `AObj` is shared with material, camera and
stage animation, its layout is decomp-mirrored
(`check-decomp-header-mirror.py`), and `length` has consumers in
`gcParseDObjAnimJoint`, `gcGetDObjTempAnimTimeMax`,
`gcGetAObjTrackAnimTimeMax` and `gcCheckGetDObjNoAxisTrack`. **This confirms
`FIXEDPOINT_ANIMATION.md`'s own sequencing: the compact per-fighter track array
that replaces `AObj` is the change, and it cannot be a quick edit.**

**Landed this cycle in the kernel:** `ndsR2S32ToF32Bits` replaces the last
`__aeabi_i2f` in `ndsR2CubicValueFixed` with CLZ bit assembly. Bit-exact to
`__aeabi_i2f` including round-to-nearest-even above 2^24, **proven over all
4,294,967,296 inputs** by `scripts/check_s32tof32_exact.py` (no NaN exclusion to
make — every s32 maps to a finite float). `check_r2_cubic_error_bound.py` is
unchanged and green at 0.002842 rotation / 0.006702 translation, which is what a
bit-exact change must do. Disassembly confirms the call is gone and one ARM
`clz` replaced it; the two `__clzsi2` references in the ELF are inside libgcc's
`__clzdi2` and are present in the pre-change build too. Boot headroom 32,992.

### `NDS_TASK51_STAGE_NATIVE` is REFUTED — it costs 88 frames of 30 FPS

`FTR_STG_OPTIMIZATION.md` lists Task 51 as a candidate "gated on visual
equivalence". It never gets that far: **the STG budget does not clear, and the
bucket Task 51 exists to shrink gets bigger.** Measured whole-match, both-CPU,
DLDI on, 1600 samples, frames 439→2038, distinct ROMs (`70DB81A6…` vs
`25F8B43E…`) from one tree (`2674756a8c`) differing by exactly one `#define`.

| | OFF | ON | delta |
|---|---:|---:|---:|
| **`WORK-H` P50** | 1,108,096 | 1,121,472 | **+13,376** |
| `WORK-H` mean | 1,165,492 | 1,177,915 | +12,423 |
| `WORK-H` P95 | 1,580,416 | 1,581,824 | +1,408 |
| **`STG` P50** | 195,776 | 198,144 | **+2,368** |
| `STG` mean | 200,294 | 202,379 | +2,085 |

**The VBlank histogram settles it**, and it is immune to bucket placement
because it is wall-clock quantization:

| interval | OFF | ON |
|---|---:|---:|
| **2 VBlanks (30 FPS)** | **1,119** | **1,031** |
| **3 VBlanks (20 FPS)** | **836** | **921** |
| 5+ | 21 | 26 |

**88 frames moved from 30 FPS to 20 FPS.** `ALL` P50 flips 1,118,592 →
1,678,016 — the median frame crossing from two VBlanks to three, exactly the
quantisation `all-is-a-quantized-gate` describes, here reporting a real event
rather than an artifact.

**Why it loses, and the lesson that generalises.** Task 51 replaces a per-frame
CPU compose of projection × view × model with 42 baked `MTX_MULT4x3` emissions.
But `FTR_STG_OPTIMIZATION.md` records in its own STG section that the **stage
prepare cache already runs at 99.9% reuse** — so the CPU compose it replaces was
already amortised to nearly nothing, while the 42 matrix commands are paid to
the geometry engine *every frame, unconditionally*. It trades a cached no-op for
real recurring GX work. **Before replacing CPU work with hardware commands,
check the cache hit rate on the work being replaced** — a 99.9%-reused compose
is not a cost, and beating it requires the replacement to be free, not merely
cheaper per invocation.

Leave `NDS_TASK51_STAGE_NATIVE ?= 0`. No visual qualification is needed; it
fails on performance first.

**Correction (cycle 109): `NDS_DREAMLAND_DS_MESH` is NOT untested.** This section
said it was, and that was wrong. It is **Task 62, REVERTED at the owner's visual
gate on 2026-07-25** — `docs/optimization/archive/Task62_AB_Results.md` has the
verdict and `artifacts/visibility/task62_v7.png` the evidence: the mesh drew as
opaque white alpha-card rectangles, because the compiler discarded the UV,
colour/alpha, material-epoch and depth metadata, and the host silhouette oracle
ignored the same semantics. Its `−29.6%` stage-work figure survives **as
rejected-experiment evidence only**. `check-published-roms.ps1:36` and
`check-harness-registry.ps1:95` both enforce `=0`, so a published ROM carrying it
fails the verifier. Do not schedule an A/B for it; a future attempt has to keep
that metadata, which is a new compiler, not a flag flip.

### Vertex memo LANDED, engagement perfect, ticks below the floor

`gNdsMPVertexF32Hits=155,515`, `Fills=11`, `Overflow=0`. **Dream Land's floor
sweep touches exactly ELEVEN distinct vertices, and the same eleven `(f32)`
conversions were being redone 155,515 times.** A 99.993% hit rate, no overflow
against the 128 cap. The premise is confirmed as strongly as a counter can
confirm anything.

The ticks are not:

| | bundle | +vertex memo | delta |
|---|---:|---:|---:|
| `WORK-H` P50 | 1,113,536 | 1,113,792 | +256 |
| `WORK-H` mean | 1,169,961 | 1,165,628 | −4,333 |
| `WORK-H` P95 | 1,565,760 | 1,568,960 | +3,200 |
| 2 VBlanks (30 FPS) | 1,093 | 1,083 | −10 |

Every number is inside its floor. **And the sizing on the board above was
optimistic — correct it before reusing.** The lane's `__aeabi_i2f` total is
2,516,993 cycles, but this change only removed
`ndsStageMPSweepFloorLoopSweep`'s **73,530 calls = 1,235,304 cycles ≈ 1,400
ticks/frame**. `mpCollisionGetFCCommonFloor`'s own 56,567 calls (950,326
cycles) are in a different function and are **still there** — the same memo
applied there is the obvious next increment, and it is also sub-floor alone.

Kept: it removes real work, is bit-identical, is proven to engage, and Boundary
passes. Not claimed as a tick win.

### THE SAMPLER IS BIT-DETERMINISTIC — the "noise floor" is placement, not noise

Ran the **identical ROM twice** (`c109-batch.json` vs `c109-batch-rerun.json`,
three minutes apart). The only differing field in either file is `capturedUtc`.
**`buckets` compares equal**: every bucket, every percentile, `named=1,145,659`
and the VBlank histogram all reproduce exactly.

| WORK-H | run 1 | run 2 | variance |
|---|---:|---:|---:|
| mean | 1,164,005 | 1,164,005 | **0** |
| P50 | 1,111,808 | 1,111,808 | **0** |
| P95 | 1,548,288 | 1,548,288 | **0** |

Per-bucket variance is 0 for `SRC`, `GCRA`, `SINT`, `SPHD`, `SCPU`, `SHDT`,
`FTR`, `STG`, `MISC`, `OTHR`. Not "small" — zero.

**Three consequences, and they overturn standing practice.**

1. **Never repeat a sampler run.** It cannot disagree with itself. The board's
   "run a third A when the A/B is noisy" rule is meaningless for this instrument
   and every confirmation run ever spent on it bought nothing. That is ~25
   minutes recoverable per would-be repeat.
2. **The 14,080 cross-build figure is NOT a noise floor. It is deterministic
   placement sensitivity.** This matters enormously: noise can be averaged down,
   placement cannot. No number of runs will ever separate a 3,000-tick code win
   from a 6,000-tick re-addressing shift. **Only measuring ONE binary two ways
   can** — standing rule 7's `.data` route is not a convenience, it is the sole
   available method.
3. **It reconciles the Boundary flake.** Guest execution is deterministic; the
   *host-side* 30-second gdb marker budget was what varied. Both observations
   were true and they are not in tension.

**Applied to this cycle's seven-cut batch** (control = AObj pool + cubic `i2f`
only): `WORK-H` P95 **−32,128**, P50 +3,712, 2-VBlank frames −19. The P95 figure
is **real and repeatable for that binary** — it is not noise. It is also **not
attributable**: `FTR` moved **−7,808** and `SCPU` **+9,024**, and neither can be
produced by animation or collision cuts, so placement moved at least as much as
the code did. **Banked as "this binary is 32,128 better at P95", NOT as "these
cuts are worth 32,128."** The distinction is the whole lesson.

### THE STRUCTURAL FINDING: this campaign cannot measure itself cut-by-cut

Four exact, verified, work-removing cuts landed this cycle. **Not one is
measurable on its own**, and that is not a property of the cuts:

| cut | work removed | ticks/frame | measured |
|---|---:|---:|---|
| AObj pool | ~6,200,000 shared | ~2,800 | not measurable |
| cubic `i2f` | 1,017,778 | ~1,160 | not measurable |
| loop-invariant hoist | 1,955,955 | ~2,220 | not measurable |
| fused multiply | 1,524,849 | ~1,730 | not measurable |
| vertex memo | 1,235,304 | ~1,400 | not measurable |

Floors: `WORK-H` P95 **14,080** cross-build, P50 **~5,700**, per-bucket
**8,544**, and `.text.hot` re-addressing alone moves P50 **~6,144**. **Every
individual cut in the remaining priced tables is 1,100–4,600 ticks/frame.** The
gap is ~290,000. So the campaign needs on the order of **seventy such cuts**,
and no single one of them can ever be shown to work.

**Two consequences, both actionable.**

1. **Stop A/B-ing individual cuts.** It burns ~25 minutes per arm to return
   noise with a confident-looking sign, which is how Task 51's *real* regression
   and the fused multiply's *false* one both got their apparent evidence.
   Accumulate exact, mechanism-verified cuts and measure in batches large enough
   to clear 14,080 — roughly **ten cuts at a time**.
2. **Mechanism verification is the per-cut gate, not ticks.** What made every
   cut above trustworthy was *not* a tick delta: it was disassembly (`bl`
   removed, literal hoisted out of the loop), exhaustive proof
   (`check_s32tof32_exact.py` over 2^32 inputs), engagement counters
   (155,515 hits / 11 fills) and Boundary. That combination is cheap, fast, and
   does not lie. Require it per cut; require ticks per batch.

### NEXT TARGET, fully specified: the port-side collision lane (5.46% of non-idle)

`SINT`'s second child, `ftComputerProcessAll` (+24,386 over-gate), is **not AI
logic** — it is map collision, and unlike the animation lane **every hot symbol
is ours**, with no decomp-include problem blocking an edit.

| symbol | self | soft float | sf calls |
|---|---:|---:|---:|
| `mpCollisionGetFCCommonFloor` | 7,533,938 | 3,389,278 | 144,408 |
| `ndsStageMPSweepFloorLoopSweep` | 6,447,004 | 1,596,312 | 98,126 |
| `ndsMPFindLineEndpoints` | 5,366,814 | 434,355 | 25,856 |
| `ndsStageMPAdjustFloorLoopWallSweep` | 4,078,548 | 3,301,075 | 98,857 |
| `ndsStageCollisionLoopGeometryReady` | 4,075,073 | — | — |
| `ndsMPFCSegmentCrossesKernel` | 3,460,265 | 4,958,252 | 224,280 |
| +4 more | 8,683,678 | 91,305 | 5,431 |
| **total** | **39,645,320** | **13,770,577** | |

**53,415,897 cycles = 5.46% of non-idle, ~60,400 ticks/frame at P50.**

**The diagnosis, per helper per function** (`__aeabi_i2f` 16.80 cyc/call,
`fsub` 36.43, `fdiv` 109.38):

- **`__aeabi_i2f`: 149,821 calls, 2,516,993 cycles.** 73,530 of them in
  `ndsStageMPSweepFloorLoopSweep` alone — **77% of that function's soft float**.
  They exist because `ndsMPVertexX/Y` return `s32` decoded from **s16** stage
  data (`reloc_backend_mp_collision.c:288-296`) while
  `ndsMPFCSegmentCrossesKernel` takes `float`. **Dream Land's collision geometry
  is static** — the stage does not move — so every one of these conversions
  recomputes a constant.
- **`__aeabi_fsub`: 68,164 in the kernel + 47,991 in the wall sweep =
  4,231,527 cycles.** The kernel's first act is `sx = v2_x - v1_x; sy = v2_y -
  v1_y` — **per-line constants**, recomputed on every one of 224,280 calls,
  along with the four `min_x/max_x/min_y/max_y` comparisons right after them.
- **`__aeabi_fdiv`: 11,684 calls, 1,278,000 cycles** at 109 cycles each.

**The fix is precomputation, and it is small.** E51 already established that
**Dream Land has 7 collision lines total**
(`reloc_backend_mp_collision.c:350-354`). A per-line record of
`v1x, v1y, v2x, v2y, sx, sy, min_x, max_x, min_y, max_y` as `f32` is **7 x 40 =
280 bytes**, built once when the stage geometry is bound, and it deletes the
i2f conversions, the sx/sy subtractions and the min/max comparisons from the
per-query path.

**This is NOT the thing E51 refuted.** E51 killed a `line_id -> (group, kind)`
lookup table on the grounds that the `yakumono_count` loop has a trip count of
one, i.e. there was no O(n) to remove. This removes *conversions and repeated
arithmetic on static data*, not loop iterations. Different mechanism, and the
7-line count E51 measured is what makes this cheap rather than what makes it
pointless.

**Risk note for whoever takes it:** this is gameplay collision — the subsystem
behind "fighters floating under the stage" in `BUGS.md`, and the kernel's own
header warns that proximity alone must never report a hit. The cache must be
invalidated on stage bind, and Boundary is mandatory, not optional. It is the
highest-value remaining target and it deserves a fresh session, not the tail of
one.

### The soft-float-free kernel is NOT a measured win — placement ate it

Both cuts are in, exact and Boundary-verified, and together they delete
**3,480,804 cycles** of proven work (hoist 1,955,955 + fused multiply
1,524,849) -- about **3,950 ticks/frame**. The arm does not measure faster.
Whole-match, both-CPU, 1600 samples, against tonight's control:

| | control | bundle | delta |
|---|---:|---:|---:|
| `WORK-H` P50 | 1,108,096 | 1,113,536 | **+5,440** |
| `WORK-H` mean | 1,165,492 | 1,169,961 | +4,469 |
| `WORK-H` P95 | 1,580,416 | 1,565,760 | −14,656 |
| 2 VBlanks (30 FPS) | 1,119 | 1,093 | **−26** |
| 3 VBlanks (20 FPS) | 836 | 875 | +39 |

**The confound is identified, not guessed: `FTR` P50 moved +4,736.** Animation
changes cannot affect fighter draw, so that number is pure placement. Text grew
**220 bytes**, worth only ~407 cycles by the 1.85-cycles-per-byte rule, so this
is not footprint — it is `.text.hot` re-addressing, which
`linker/nds_hot_text.ld:179-201` already measures at **6,144 `WORK-H` P50 on 122
of 128 frames** when one member of the curated 8 KiB list is perturbed. The work
removed (~3,950) is smaller than the perturbation removing it causes (~6,000).

**Kept anyway, and deliberately.** The board's standing rule is that milestone
targets are directional and every repeatable correctness-preserving gain is kept
and accumulated. These delete real work, are exact, and are verified. What is
*not* claimed is a win: **do not cite this bundle as a tick improvement.**

**RETRACTED follow-up: do NOT re-curate `.text.hot`.** The first version of this
entry proposed exactly that. It is a documented dead end, recorded in the very
file the edit would have touched. `linker/nds_hot_text.ld:174-200` closes the
list **in both directions**: Task 94 moved `gcPlayDObjAnimJoint` *out* (top-ranked
candidate, zero eviction) and regressed `WORK-H` P50 **6,144** on 122 of 128
frames, with `STG` rising 3,712 despite never calling it; R2-03 E66 admitted
`ndsR2CubicValueFixed` *in* -- ranked #1 unplaced at 1,815,752 recoverable stall
cycles -- and regressed `WORK-H` P95 **+24,448**. **Two independent estimators
got the sign wrong.** HANDOFF carries the same closure. Re-deriving it costs a
build.

**The correct answer is standing rule 7, which these cuts can actually use.**
Placement noise is a property of comparing two binaries, not something to
optimise away. The AObj pool could not take a runtime route because it runs at
scene setup, before the poke lands -- but the hoist and the fused multiply are
**per-frame code paths**, so both arms fit in ONE binary behind a `.data` global
driven by `sample-tick-hud-buckets.ps1 -SetGlobals`. That removes the ~6,000
placement term entirely and is the only way this lane's ~3,950-tick cuts can be
judged on their merit. Note the rule's own caveat: the poke lands after ~3 warm
steps, so the OFF arm is partial and must be scaled.

**Until that route exists, stop measuring animation cuts by building two arms.**
Every cut in the priced table is smaller than the placement term, so a two-arm
A/B on any of them returns noise with a confident-looking sign.

### Whole-match sampler invocation, exactly (cycle 109 — cost three runs)

The HANDOFF line "`-Samples` to 4096" is the parameter's *ceiling*, not the
window to use, and reading it as the window wasted a 30-minute run.

- **`-Samples 1600`.** That is the match. It is where every banked figure's
  denominator comes from (`754/1600`, `707/1600`). **4096 runs past the end**
  and dies at `TimeoutSeconds` having reached ring stop 15 of 43.
- **`-AllowRepeatedFrames` is required on the gate arm.** Without it the run
  completes and is then *thrown away*: about 4 presented-frame numbers per 1600
  repeat. They are not double-reads — the harness itself annotates each one
  `payload DIFFERS (real second iteration)`, so the samples are distinct
  iterations that reported the same frame counter.
- **`-NoBuild`** or the sampler rebuilds with no `MakeFlags` and silently wipes
  the arm's configuration.
- **`-JsonOut`**, not `-OutName`.

**`capture-melonds.ps1 -ExactFirstFrame` does not work on this ROM.** It demands
`-ExactSecondFrame` as well *and* `-SoftwareRenderer`, and then delegates to
`capture-cut-g-exact-frames.ps1`, which fails with *"Exact frame 439 lost
native-OAM GO recognition or drawing state"* — the exact-frame path is gated on
Cut G's GO-text state, not on arbitrary battle frames. For an ordinary visual
A/B use the delay-based capture, or qualify through the Boundary verifier, which
produces `artifacts/visibility/latest.png` plus its regional analysis.

### RESOLVED: Boundary's 30s marker budget, three wrong verdicts, and the fix

Three Boundary failures in one night all read *"GDB marker capture timed out
after 30 seconds"*, each with a **different** stall PC
(`__syscall_lock_acquire`, then `memcpy`), and the last had already printed most
of the marker dump before expiring. That is a capture finishing late, not a ROM
hanging.

**Root cause.** The Boundary path (no `-OneMinuteMatchProof`) borrowed
`$RendererBenchmarkTimeoutSeconds` for the marker capture — default **30** —
while the one-minute proof path gets 300. Thirty seconds has to cover the gdb
attach, four breakpoints and the whole dump, and that dump has grown to dozens
of `printf` lines as counters were added across the campaign. Nobody moved the
budget with it. Fixed in
`verify-battle-mariofox-gcrunall-loop-harness.ps1:1346` — floored at **120s**
via `Max()`, so an explicit larger value still wins and the renderer benchmark's
own budget is untouched. A timeout is a ceiling, not a sleep; a passing run pays
nothing.

**It cost a wrong answer, not just time.** The fused multiply was bisected to a
"hang" and reverted on *red with it, green without it* — then the identical
reverted tree failed too. **`NDS_TASK10_GIT_SHORT` is compiled in, so every
commit changes the ROM image**: the passes were at `189cd20680`, the failing
re-test at `5b6cb20aa3`, same source, different binaries, on a ROM whose pacing
is placement-sensitive. Standing rule 7 was applied to the sampler and not to
the verifier. **"Re-run the same tree" is not "re-run the same binary" in this
repo.**

**Both changes are now in and verified** on the repaired harness, each with
zero marker timeouts:

- the loop-invariant hoist in `gcPlayDObjAnimJoint` (~1,955,955 cycles: the
  1,069,318-cycle `AOBJ_ANIM_END` literal reload plus the 886,637-cycle
  `parent_gobj->flags` chain, both re-run once per node for a value that never
  changes);
- `ndsR2F32MulToFixed`, the fused `f32 x f32 -> Q16` (1,524,849 cycles).

**`ndsR2CubicValueFixed` now contains no `bl __aeabi_*` at all** — ten `umull`,
one `clz`. The kernel is free of the soft-float library.

**Standing rule, added:** judge nothing on a verifier whose flake rate on one
*unchanged binary* has never been measured. If Boundary fails, re-run before
bisecting.

### Whole-match sampler invocation, exactly (cycle 109 — cost three runs)

The HANDOFF line "`-Samples` to 4096" is the parameter's *ceiling*, not the
window to use, and reading it as the window wasted a 30-minute run.

- **`-Samples 1600`.** That is the match. It is where every banked figure's
  denominator comes from (`754/1600`, `707/1600`). **4096 runs past the end**
  and dies at `TimeoutSeconds` having reached ring stop 15 of 43.
- **`-AllowRepeatedFrames` is required on the gate arm.** Without it the run
  completes and is then *thrown away*: about 4 presented-frame numbers per 1600
  repeat. They are not double-reads — the harness itself annotates each one
  `payload DIFFERS (real second iteration)`, so the samples are distinct
  iterations that reported the same frame counter.
- **`-NoBuild`** or the sampler rebuilds with no `MakeFlags` and silently wipes
  the arm's configuration.
- **`-JsonOut`**, not `-OutName`.

**`capture-melonds.ps1 -ExactFirstFrame` does not work on this ROM.** It demands
`-ExactSecondFrame` as well *and* `-SoftwareRenderer`, and then delegates to
`capture-cut-g-exact-frames.ps1`, which fails with *"Exact frame 439 lost
native-OAM GO recognition or drawing state"* — the exact-frame path is gated on
Cut G's GO-text state, not on arbitrary battle frames. For an ordinary visual
A/B use the delay-based capture, or qualify through the Boundary verifier, which
produces `artifacts/visibility/latest.png` plus its regional analysis.

### RETRACTED: the fused-multiply "hang" bisect was not controlled

The section below concludes the fused multiply hangs the ROM, on the strength of
*red with it, green without it*. **That inference is withdrawn.** Later the same
night the **identical reverted tree failed too** — same `GDB marker capture
timed out after 30 seconds`, stalled in `memcpy()` instead of
`__syscall_lock_acquire`. A tree that passed twice and then failed cannot be the
control in a bisect.

**Why the "same tree" was not the same binary.** `NDS_TASK10_GIT_SHORT` is
compiled in (`nds_build_config.h`), so **every commit changes the ROM image**.
The passing runs were at `189cd20680`, the failing re-test at `5b6cb20aa3`; the
sources matched but the binaries did not, and this ROM's pacing is
placement-sensitive — which is the whole reason standing rule 7 exists. Four
Boundary runs spanning three commits are four different binaries.

**The failure signature says timeout, not hang.** The budget is a fixed 30 s,
the stall PC differs every time (`__syscall_lock_acquire`, then `memcpy`), and
the last failure had already printed the full marker dump before expiring. That
is a run finishing late, not a lock-up. No leaked `melonDS`/`gdb` processes and
1% CPU at the time, so idle-machine load does not explain it either.

**Consequences.** The fused multiply (1,524,849 cycles) is **unproven, not
disproven** — it may be perfectly correct. The loop-invariant hoist below is
equally unverified. **Neither is in the tree**; HEAD is the last state with a
recorded pass, and the hoist is parked in the session scratchpad as
`objanim.hoist.c`.

**Before either is retried, Boundary needs to be trustworthy again.** Establish
how often it passes on one *unchanged binary* (not one unchanged tree), and
raise the 30 s marker budget or find what made it marginal. Judging a
performance change on a harness with an unmeasured flake rate is how tonight
produced a confident, wrong verdict.

### The fused `f32 x f32 -> Q16` attempt (verdict retracted above)

Attempted the last cut that does not need the representation change — replacing
`ndsR2F32ToFixed(length * length_invert, BF)` with one integer multiply of the
two significands (`ndsR2F32MulToFixed`), worth **1,524,849 cycles**. It built,
it removed the last soft-float call from the kernel (disassembly showed **zero
`bl __aeabi_*`, 10 `umull`, 1 `clz`**), boot headroom held at 32,800, and
`check_r2_cubic_error_bound.py` passed **green and numerically unchanged**
(0.002842 / 0.006702).

**And the ROM hangs.** Boundary failed twice with a GDB marker-capture timeout,
stalled in `__syscall_lock_acquire` with the frame-complete marker never
reached. Reverting only that change and re-running gave **`Boundary
verification profile passed`** on the same tree. Two red with, one green
without: the fused multiply is the cause. Backed out uncommitted; HEAD is the
verified-green state.

**The lesson is about the checker, not the arithmetic.** The error bound samples
a *gameplay-plausible domain* of `(length, length_invert, values, rates)` and
compares against the decomp float; it never executes the parser, so an input the
parser really produces — `length_invert` is **overloaded**, holding `1.0F/payload`
for the cubic arms but a raw `payload` for `SetValAfter` and `1.0F` from
`gcAddAObjForDObj` — can sit outside every sampled domain and still reach the
kernel. **A host bound that passes is not a substitute for Boundary on a change
that alters saturation or range behavior.** The next attempt needs the real
input distribution instrumented out of a run first, not more host sampling.

Do not re-attempt this cut without that. It is 1,524,849 cycles — worth having,
not worth a second unexplained hang.

### The DMA spin is GEOMETRY SUBMISSION, not a free win (cycle 108)

Followed up the census's largest site and it is **not** the clean win it looked
like. All three sites share one shape (`nds_renderer.c:13647`, `:22936`, and
inside `ndsRendererTask36ReplayRun`):

```c
DC_FlushRange(packet->words, word_count * sizeof(u32));
while ((DMA_CR(0) & DMA_BUSY) != 0u) { }   /* leading wait */
DMA_SRC(0) = (u32)packet->words;
DMA_DEST(0) = (u32)&GFX_FIFO;
DMA_CR(0) = DMA_FIFO | word_count;
while ((DMA_CR(0) & DMA_BUSY) != 0u) { }   /* trailing wait -- the 507 cyc/ex */
```

The destination is **`&GFX_FIFO`**, so this is geometry going to the 3D engine.
**Deleting the trailing wait does not recover 6,654,860 cycles**, for two
independent reasons:

1. **NDS DMA steals the bus.** The ARM9 does not run freely during the transfer;
   it stalls on any bus access. Overlap only exists for work running entirely
   out of ITCM/DTCM, so the recoverable share is bounded by how much
   cache-resident work follows the submit, not by the whole wait.
2. **`DMA_FIFO` interleaves with CPU FIFO writes.** Removing the trailing wait
   is only safe if nothing touches `GFX_FIFO` or a GX command register before
   the next submit's leading wait. This file writes `GFX_TEX_FORMAT` and
   `GFX_PAL_FORMAT` directly, so it needs a deferred-sync guard plus an audit of
   every GX write site — not a two-line deletion. A mistake here corrupts the
   command stream, which is the failure mode the verifier is weakest at
   catching.

**And the transfer itself is already efficient.** ~504 cycles for ~13,200
executions works out near DMA's ~2-cycles-per-word floor at roughly 250 words a
transfer, ~33 transfers a frame. That is the **cost of the geometry volume**,
not per-transfer overhead, so batching buys nothing either. Reducing it means
submitting less geometry, which is a `PROJECT_GOAL` fidelity decision.

**What is still open here, in order of cheapness:** count words per transfer
from `gNdsRebirthHaloPackedWordCount / SubmitCount` (a counter that already
exists) to confirm the 250-word estimate; then, if a deferred-sync guard is
wanted, do the GX-write audit first and put the change behind a runtime route so
it can be A/B'd on one binary.

### The D-cache census — 17.83% of non-idle is data-load excess (cycle 108)

`scripts/analyze-dcache-stalls.py`, run on the cycle-106 profile. No build, no
emulator run. Ranks every memory access by its measured `average_cycles` and
joins it to the disassembly, so the output names the *instruction* — base
register and offset — not just the function.

| class | cycles | executions | cyc/ex |
|---|---:|---:|---:|
| **load (data)** | 252,353,053 | 35,715,768 | **7.07** |
| load (literal pool) | 57,122,935 | 8,760,593 | 6.52 |
| load (stack) | 66,067,255 | 12,628,225 | 5.23 |
| store | 116,346,526 | 31,178,647 | 3.73 |
| other | 676,279,870 | 249,102,962 | 2.71 |

A cached ARM9 `ldr` retires in ~1–3. Excess over a 3-cycle baseline, data loads
only, is **174,495,113 cycles = 17.83% of non-idle work**.

**The single largest site is not a cache miss at all.**
`ndsRendererTask36ReplayRun`, `ldr r3, [r1, #184]` at **507.2 cycles per
execution**, 13,200 executions, **6,654,860 excess — 58% of that function.** Six
instructions earlier `r1` is loaded with `#67108864` (**0x04000000**, the I/O
base), so this reads **0x040000B8 = DMA0CNT**, and `cmp r3,#0 / blt` back to
itself is a **spin on the DMA busy bit**. The CPU is idle-waiting on hardware.
There is a second such loop immediately above it. This is real recoverable time
and it is nothing to do with layout: overlap the transfer with CPU work, or
resize it.

**Some extreme sites are the write buffer being charged to the next load.**
`ndsRendererAdapterBuildNativeMaterialSnapshot`'s `ldrh r6, [r5, #56]` reads
138.8 cyc/ex and sits **immediately after `bl memset`** — the memset's stores
drain, and the stall lands on the following load. So "memset is 2.09%" and "this
load is expensive" are largely the same cost, counted once, at the load. **Do
not add them.**

**Genuine structure misses, which is where a layout change pays:**

| cyc/ex | execs | excess | site |
|---:|---:|---:|---|
| 37.1 | 79,874 | 2,724,683 | `ndsFTParamsInvalidateFighterParts` `ldr r4,[r0,#16]` |
| 30.8 | 79,874 | 2,221,897 | `ndsFTParamsInvalidateFighterParts` `ldr r3,[r0,r3]` |
| 45.4 | 37,834 | 1,602,763 | `ndsBaseGcRunAll` `ldr r3,[r0,#20]` |
| 24.1 | 143,916 | 3,034,025 | `gcPlayDObjAnimJoint` `ldrb r5,[r4,#5]` (`aobj->kind`) |
| 33.1 | 41,047 | 1,237,303 | `battleship_ftAnimParseDObjFigatree` `ldr r4,[r0,#116]` |

Top functions by data-load excess: `ndsFighterMarioFoxDLAllDrawForSlot`
9,071,206; `ndsRendererExecuteNativeFighterOwnerProduction` 8,824,490;
`ndsRendererTask36ReplayRun` 7,648,881; `ndsFTParamsInvalidateFighterParts`
6,529,067; `ndsBaseGcRunAll` 4,583,792; `gcPlayDObjAnimJoint` 3,990,670.

**`ndsFTParamsInvalidateFighterParts` is the best-shaped candidate on this
board.** CPI **7.08**, 6.53M of load excess concentrated in **two instructions**
on the same base register, and it sits inside the simulation — where `SRC`
(+171,383) decides the gate — rather than in fighter draw, which `FTR` (+13,768)
proves does not. Two loads at 37.1 and 30.8 cyc/ex off `r0` is a structure
walked once per part per frame that never stays resident.

### THE MACHINE IS MEMORY-BOUND — 65% of non-idle cycles are stall (cycle 108)

**Read this before proposing any instruction-count optimization.** The profile
reports instructions as well as cycles, and nobody had divided them.

**Whole profile: 1,211,130,791 cycles / 342,792,094 instructions = CPI 3.53.**
Non-idle: 978,488,987 / 342,785,681 = **CPI 2.85**, so **635,703,306 cycles —
65.0% of all non-idle work — are stall**, not issue. ARM9's ideal is ~1.0.

**This retires the fixed-point conversion campaign as briefed, and it is the
reason every arithmetic experiment this cycle measured sub-floor:**

| symbol | cycles | instructions | CPI |
|---|---:|---:|---:|
| `__aeabi_fadd` | 33,839,425 | 28,429,032 | **1.19** |
| `__aeabi_fmul` | 21,887,296 | 19,160,867 | **1.14** |
| `ndsR2CubicValueFixed` | 19,420,815 | 11,143,118 | 1.74 |
| `battleship_ftMainProcUpdateInterrupt` | 5,656,799 | 490,727 | **11.53** |
| `ftMainProcPhysicsMap` | 3,643,426 | 414,209 | **8.80** |
| `ndsFTParamsInvalidateFighterParts` | 13,718,726 | — | 7.08 |
| `ndsRendererTask36ReplayRun` | 11,463,652 | — | 7.18 |

**The soft-float helpers are the most efficient code in the build.** They are
libgcc ARM assembly in ITCM and issue at CPI ~1.15 — near ideal. The 8.9% they
cost is honest instruction count, and deleting it is a real 8.9%, but it is
being taken out of the *efficient* 35% of the machine. Meanwhile
`ftMainProcUpdateInterrupt` and `ftMainProcPhysicsMap` — which are exactly the
`SINT` (+88,082) and `SPHD` (+28,941) over-gate discriminators — run at **11.53
and 8.80 CPI and are almost pure stall.** At CPI 2 the interrupt proc would cost
981,454 instead of 5,656,799.

**Named the mechanism in the animation evaluator, per instruction.** The hottest
instruction in `gcPlayDObjAnimJoint` is `ldrb r5, [r4, #5]` — `aobj->kind` — at
**24.1 cycles per execution**, 143,916 executions, **3,465,773 cycles, 23% of the
function**. That is one D-cache miss per AObj node per frame: ~360 live AObj
nodes against a **4 KB** ARM9 data cache means the list cannot stay resident, so
every node is a miss every frame. `aobj->next` (offset 0, same line) then costs
only 7.0. Converting this function's arithmetic to fixed point would not touch
the 24.1.

**Rank by STALL, not by cycles — it is a different top ten:**
`ndsFighterMarioFoxDLAllDrawForSlot` 21,261,766 stall at CPI 5.60,
`ndsRendererExecuteNativeFighterOwnerProduction` 20,320,720 at 4.27,
`ndsRendererNativeEmitProductionRawUntexturedRun` 16,411,238 at 2.48, `memset`
14,979,168 at 3.73, `ndsRendererCommitNativeStageSegment` 14,425,717 at 2.61,
`ndsFTParamsInvalidateFighterParts` 11,779,708 at 7.08.

**What this means for the lane order.** Data layout, working-set size and
placement are the lever; instruction count is not. Cycle 105's arena fix
(−191,981, the largest win of the campaign) was a memory-system fix, and that is
not a coincidence. The next measurement should be a **D-cache working-set
census** of the simulation's hot structures — AObj/DObj/MObj lists and
`FTParams` — not another conversion. `scripts/analyze-leaf-helper-attribution.py`
and the per-PC `average_cycles` column already do this for free: a load above
~10 cycles/execution is a miss, and they can be ranked exactly the way the stall
table above was built.

### How big a win has to be — the sensitivity curve (cycle 108)

Computed free from the head configuration's rows. **This is the number to size
any future proposal against**, and it ends the practice of judging a lever by
whether its P95 delta clears a floor.

| uniform body-wide saving | over gate | % over | new P50 | new P95 |
|---:|---:|---:|---:|---:|
| 0 | 707 | 44.2% | 1,107,008 | 1,411,283 |
| 25,000 | 599 | 37.4% | 1,082,008 | 1,386,283 |
| **50,000** | **469** | **29.3%** | 1,057,008 | 1,361,283 |
| **100,000** | **295** | **18.4%** | 1,007,008 | 1,311,283 |
| 200,000 | 149 | 9.3% | 907,008 | 1,211,283 |
| 291,000 | 80 | 5.0% | 816,008 | 1,120,283 |

**The distribution is packed against the line.** The median clears the gate by
only **13,372**, **238 frames sit within 50,000 of it from above**, and 412
within 100,000. So a body-wide saving is worth far more than its P95 delta
suggests: 50,000 moves **238 frames** from 20 FPS to 30 FPS while moving P95 by
exactly 50,000, which at P95 alone would read as a modest win. The VBlank
histogram says the same thing in presentation terms — **2:1062, 3:895** — the
match is already 30 FPS on two thirds of frames and the job is the other 895.

**Only one lane on the map is the right size.** Soft float is ~**98,500 ticks
per frame** (8.9% of non-idle), i.e. converting it lands on the 100,000 row:
707 → 295 over gate, 412 frames crossing. Nothing else measured this cycle is
within an order of magnitude — the compare sub-lane is 0.5%, `memset`/`memcpy`
is 3.92% but concentrated in fighter draw which `FTR` proves is not where the
gate is decided, and every remaining local edit is worth 500–5,000 ticks against
a 291,000 gap.

**Two mechanisms are refuted, so the arithmetic must actually not happen:**
recompiling is out (`Makefile:3165-3179`, `battleship_gmcollision.o -marm` read
−2,304, inside the floor, because `-marm` only buys the call sites and cannot
change libgcc's own mode), and `-ffinite-math-only` does not lower a single
compare. Every float call from Thumb is a `blx` — a real interworking switch
each way, which is why `fadd` measures 36.4 cycles against a ~15–20 cycle ARM
body — but that cost is unreachable without removing the call.

**Anything at or beyond the 100,000 row is a `PROJECT_GOAL` sacrifice-order
decision and needs the owner.** Reduced animation update rates and independent
update rates are explicitly listed as allowed (visual fidelity is sacrifice #2),
and the contract says to test them only after cheaper equivalents are exhausted
— which cycles 105–108 have now largely done at this seam, each refutation
recorded above.

### What is actually left: the over-gate frames, decomposed (cycle 108)

Taken on `build-c111-fcmp`, the current head configuration, whole match both-CPU.
**This corrects a working assumption that has been steering the campaign.**

**44.2% of frames are over gate — 707 of 1600 — not the ~5% that "P95 is the
load-frame boundary" implies.** `WORK-H` P50 is **1,107,008** against a gate of
1,120,380, so the median frame clears it by only 13,372 and nearly half the match
does not. The excess summed over every over-gate frame is **91,928,908 ticks**,
and the 80 frames above P95 average 1,559,629, i.e. **439,249 over gate each**.

Splitting every bucket by over-gate vs under-gate names the discriminators
exactly, and they are not where the last three experiments looked:

| bucket | under-gate | over-gate | delta |
|---|---:|---:|---:|
| **`SRC`** | 303,813 | 475,196 | **+171,383** |
| ↳ `GCRA` | 298,699 | 470,129 | **+171,430** |
| ↳ ↳ `SINT` | 140,411 | 228,493 | **+88,082** |
| ↳ ↳ `SPHD` | 67,785 | 96,726 | +28,941 |
| ↳ ↳ `SHDT` | 4,235 | 31,424 | +27,190 |
| ↳ ↳ `SCPU` | 37,605 | 61,135 | +23,531 |
| `MISC` | 112,351 | 129,263 | +16,911 |
| `FTR` | 390,903 | 404,671 | +13,768 |
| `AUD` | 3,425 | 16,302 | +12,877 |
| `STG` | 200,013 | 201,512 | +1,499 |

**`GCRA` is `gcRunAll`** — `battleship_sys_objman.c:80` calls it "the SOLE
gateway to the whole simulation", so it is fighters, stage, camera, effects,
items, weapons and interface together, and it is ~33% of every frame. Its
over-gate excess is **almost entirely its named children**: 88,082 + 28,941 +
27,190 + 23,531 = 167,744 of 171,430, leaving the `SOBJ` residual at ~3,700.
There is no unattributed mass hiding in the simulation.

**Consequences, in order:**

1. **`FTR` is confirmed dead as a gate lever** — it separates the two
   populations by only **13,768** against `SRC`'s 171,383. Anything that only
   moves fighter draw (the `memset`/`memcpy` concentration in
   `ndsFighterMarioFoxDLAllDrawForSlot`, 30% of all memset calls) buys P50, which
   is already inside the gate, and buys almost nothing where the gate is decided.
2. **`SINT` is still the single largest discriminator at +88,082**, after cycle
   105's arena fix and cycle 108's prebake. The loader's *copy* is closed, but
   whatever else `SINT` brackets is not.
3. `SPHD`, `SHDT` and `SCPU` are the next three and together are **79,662** —
   comparable to `SINT` and never yet split. `SCPU` is CPU-player AI and is
   structurally doubled on the gate arm.
4. **Stop pricing levers against P95 alone.** With 44.2% over gate, a change that
   lowers the *body* moves 707 frames across the line; the same change judged
   only at P95 looks like noise. That is why the compare conversion read
   sub-floor while being real.

**Do not spend another cycle on `memset`/`memcpy`.** Priced by the same method
and refuted: the concentrated caller is fighter draw (see 1 above), and
`ndsMPCollisionEnsureLineGroups`'s 10,050 calls are **two 16-byte** zeroings
(`movs r2, #16` at `0205c7b8`/`0205c7c2`) worth ~0.04% of non-idle, not the
267-cycle global average. The 267 average belongs to large copies elsewhere.

### The COMPARE lane is priced and it is too small — 0.5% fully converted (cycle 108)

First slice attempted off the soft-float map, and the useful result is its size.
`include/nds/nds_fcmp.h` replaces the soft-float comparison helpers with integer
tests on the bit pattern. **Exact, not approximate**, and proven that way:
`scripts/check_fcmp_exact.py` sweeps **all 4,294,967,296 bit patterns** against
12 predicates and every one matches IEEE-754, including both signed zeros and
all 16,777,214 denormals. NaN is the one documented exclusion (67,108,856
disagreements, reported rather than hidden). Not sampled, and not argued from a
comment — the failure mode is a single pattern class that a random sample never
draws: `-0.0f == +0.0f` is TRUE in IEEE while the patterns differ, so a naive
`bits(x) == bits(0.0f)` is wrong for exactly one input in 2^32, and
`if (payload != 0.0F) { 1.0f / payload; }` would then divide by zero.

**`-ffinite-math-only` does not remove these calls.** Checked compile-only in
seconds before writing anything: GCC emits the same `bl __aeabi_fcmp*` with and
without it, in both ARM and Thumb. There is no build-flag shortcut, so the calls
have to go at the call sites.

Applied to `gcPlayDObjAnimJoint`, the single largest caller in the profile
(227,040 calls, 2,582,802 cycles). Mechanism confirmed in the object file
without the emulator: **81 → 76 static `__aeabi_fcmp*` sites**, ROM `.text`
+92 bytes.

| vs the route-on arm | P50 | P95 | mean |
|---|---:|---:|---:|
| `build-c111-fcmp` | 1,107,008 | 1,411,264 | 1,129,509 |
| delta | **−3,136** | **−4,739** | **−11,620** |

All three negative and all **below the cross-build floors** (P50 5,700, P95
14,080) — exactly the ~2,600 predicted from the attribution. Kept under the
board's accumulate rule because it is bit-exact and provably less work, but
**it is not independently bankable and must not be cited as a win.**

**The number that matters is the lane's ceiling.** The five comparison helpers
are 12,909,690 cycles (1.32% of non-idle) in total, and the port-editable share
— everything not inside `decomp/` — is only **~5.0M, about 0.5%, ~7,300 ticks at
P95 even if every site were converted**. `ndsBaseGcPlayMObjMatAnim` and
`ndsBaseGcPlayDObjAnimJoint` look port-side in the profile but are the *decomp*
originals renamed by `battleship_sys_objanim.c`'s `#define` block, so their
compares are not editable. `include/nds/nds_mp_floor_crossing.h` is compiled on
the host too, so a DS header cannot simply be added to it, and its constants are
negative (`-epsilon`) which the `_C` forms do not accept.

**Do not spend another cycle on comparisons.** The arithmetic is where the lane
actually is: `fadd`+`fsub` 33.8M (3.46%), `fmul` 21.9M (2.24%), `fdiv` 10.1M
(1.04%) — **6.74% against the compares' 1.32%** — and that needs a genuine
fixed-point conversion of a whole subsystem, not a predicate swap. The header
and the exhaustive checker stay because they make every future site free.

### The soft-float bill is MAPPED — 8.9% of non-idle work, and it is spread (cycle 108)

First full attribution of the ARM9 soft-float helpers to the functions that
**call** them. Free: no build and no emulator run, off the cycle-106 profile that
already existed. Tool is `scripts/analyze-leaf-helper-attribution.py`; output is
`artifacts/performance/2026-08-09_c106-profile/softfloat-attribution.json`.

**`__aeabi_fadd` is the largest non-idle symbol in the entire profile** —
33,839,425 cycles, ahead of every renderer function — and "optimize
`__aeabi_fadd`" is not a task anyone can do. Self time is the wrong view for a
leaf helper (the standing `self-time-is-not-a-subsystem-budget` rule), and a
*static* call-site ranking is worse: `ndsOpeningRoomRenderDLPreview` has 277
static soft-float calls and **zero** cycles in a battle profile. The profile is
per-PC, so the instruction count at a `bl <helper>` **is** that site's exact
dynamic call count — the same trick as `entry-pc-gives-exact-call-counts`,
applied to call sites instead of entries.

**Two traps, both of which gave wrong numbers by hand first.** `__aeabi_fsub` is
a two-instruction thunk that falls through into `__aeabi_fadd`, so its self time
is ~1 cycle per call and all its real cost is charged to fadd. Charging fsub's
370,065 calls at fadd's rate *and* into fadd's divisor moves fadd from a
nonsensical **60.6 to 36.4 cycles/call**, and the campaign total from 11.24% to
**8.9%**. A helper reached by a tail `b` is still a call site. The script handles
both; do not redo this by hand.

| subsystem | soft-float cycles | % non-idle | fns |
|---|---:|---:|---:|
| **animation evaluation** | 25,112,349 | **2.57%** | 10 |
| collision / stage MP | 17,471,376 | 1.79% | 17 |
| matrices / transform | 12,998,043 | 1.33% | 12 |
| other | 12,861,181 | 1.31% | 106 |
| gameplay (other decomp) | 11,176,042 | 1.14% | 90 |
| renderer, particles, CPU AI | 7,912,649 | 0.81% | 24 |

Per-call cost, measured: `fdiv` **109.4**, `fadd` **36.4**, `l2f` 28.6, `fmul`
25.2, `i2f` 16.8, the compares 10–15. The helpers themselves are libgcc's
hand-written ARM assembly and **already resident in ITCM**, so there is no
placement or implementation win here — only call volume.

**Top callers:** `battleship_ftAnimParseDObjFigatree` 7,931,155 (0.81%, self
16,192,916), `gcPlayDObjAnimJoint` 6,706,718 (0.69%, self 16,595,669),
`ndsMPFCSegmentCrossesKernel` 4,958,252, `ndsBaseGcPlayMObjMatAnim` 4,463,648,
`syMatrixLookAtReflectF` 3,807,640. **The two animation functions together are
5.34% of non-idle work counting their self time — roughly 75,600 ticks at the
current P95.**

**Read this as a base-cost lane, not a tail lane.** A load frame is base +
premium, so anything that lowers the base lowers P50 *and* P95 together; this is
the first lane in cycles that does both. `ndsR2CubicValueFixed` is **already
converted** and its one remaining `fmul` is documented as unavoidable at
`battleship_sys_objanim.c:211` — do not re-open it.

**Two constraints on any conversion, both already paid for.** L7's fixed-point
collision won +534 and lost 6,481 **to its own text**, and the standing rate is
**1.85 cycles of `FTR` mean per byte of added ARM text** — so a conversion must
replace float code with *equal or less* text, not sit beside it. And on
`-mthumb` there is no SMULL, so a `(s64)a*b` becomes a library call; the cubic
kernel carries a `target("arm")` attribute for exactly this and a new kernel
needs one too.

**Adjacent, same method, not soft float:** `memset` 20,471,352 (2.09%) and
`memcpy` 17,862,751 (1.83%) at ~260 cycles a call.
`ndsFighterMarioFoxDLAllDrawForSlot` alone drives 25,617 memsets and 17,833
memcpys — 30% of all memset calls — on top of 25,885,593 self cycles.
`ndsMPCollisionEnsureLineGroups` re-zeroes 10,050 times.

### The force-load seam is CLOSED — zero-copy is structurally impossible, cycle 108

The obvious next move after the prebake was to stop copying altogether: the
arena already holds every warmed animation, so finalize each image once and hand
back the pointer. **It cannot be done at this seam, and the reason is one line of
the caller.** `decomp/BattleShip-main/decomp/src/ft/ftmain.c:4623-4624`:

```c
lbRelocGetForceExternHeapFile(motion_desc->anim_file_id, (void*) fp->figatree_heap);
fp->figatree = fp->figatree_heap;
```

**The return value is discarded.** The fighter always animates from its own
`figatree_heap`, so the data must physically be there and the destination copy
is mandatory. The port already knew this and nobody noticed: the generic arm of
`lbRelocGetForceExternHeapFile` (`reloc_backend_assets.c:7396-7407`) copies the
result back into `heap` and returns `heap` whenever the pointer differs. That
guard is the same fact, written down years earlier.

Built and measured anyway, because the counters name the failure precisely.
Handing back the arena pointer does not read as a performance regression — it
reads as a **different match**: `ForceLoadTotal` 353 → **3,210**, `Distinct`
85 → **94**, `CacheHits` 351 → 3,146, `WORK-H` P95 **2,275,200**. Fighters
animating from a stale slot thrash their state machines. Reverted; no flag, no
dead code, nothing in the tree.

**Three facts from the attempt are permanent and cost real runs — do not
re-derive them:**

1. **Nothing writes into a finalized animation file during its residency.**
   Checksummed each file across its whole residency in the caller's slot:
   **351 checked, 351 stable, 0 mutated**, over exactly **2 slots** (one per
   fighter). This retires the older caveat above the R2-04 E0 counters — "the
   renderer does mutate loaded fighter data" is true of fighter data at large
   and false of an AObj16 animation script. Any future sharing design at a seam
   that *does* let the destination move is licensed by this.
2. **All 301 Mario+Fox animation assets have no external references** —
   `reloc_extern_offset` 0xffff and `extern_file_ids_num` 0, scanned statically
   off the built NitroFS tree with no build and no emulator run. So
   `ndsRelocApplyExternalPointerFixups` takes its early-out for every one of
   them, and there is no cross-file pointer here that could go stale.
3. **The internal fixup list is at most 21 entries** (`PrebakeSlotsMax` 21 over
   the whole match). Twenty-one pointer writes is not a cost, so baking the
   internal fixups per destination — the obvious fallback once zero-copy died —
   is worth approximately nothing and must not be briefed as a lever.

**Taken together this CLOSES `ndsRelocForceLoadFighterAObj16File` as a cost
centre.** After cycle 108's prebake, a cache hit is a mandatory ~2.3 KB memcpy
plus ~21 pointer writes plus bookkeeping. The remaining load-frame premium is
what cycle 107 already attributed and named correctly: **animation
re-evaluation, 158,393/frame (24.3%), which is real gameplay work, not port
overhead.** That is where the next attempt on the `SINT` tail belongs — as
specialization or a lower update rate, not as another caching layer.

### The dormant-flag seam is EXHAUSTED — audited, cycle 105, no build

Prompted by the "audit the 0 flags" rule. **The Makefile's `?= 0` defaults are
not the shipped configuration** and reading them as such is how this audit almost
spent two builds. Compared `builds/build-c105-anim-cand/nds_build_config.h`
against every `^[A-Z0-9_]+ \?= 0` in the Makefile: **41 flags are overridden**,
and every large measured lever is already ON — `NDS_R2_CUBIC_FIXED` (60,509
ticks/frame), `NDS_R2_STAGE_DIRECT`, `NDS_R2_STAGE_VIEWPROJ` (54,901),
`NDS_R2_STAGE_PREFLIGHT`, `NDS_R2_FIGHTER_HW_MTX` (−17,600),
`NDS_R2_FIGHTER_HW_LIGHT` (ceiling 53,760), `NDS_R2_FIGHTER_RUN_MEMO` (~45,900),
`NDS_R2_FIGHTER_MTX_DIRECT`, `NDS_R2_FIGHTER_SHUFFLE_FOLD`, the Task 16 float
set. A `build-c106-cubic` arm was built before this was checked and is a **null
build** — same `nds_build_config.h`, same `fake_heap_start 0x0228c004`; it was
caught before its measuring run, not after.

Of the 71 still off, all but a handful are censuses, probes, falsifiers or
lab-only suppressors. The real remainder, each already carrying its own gate:
`NDS_TASK51_STAGE_NATIVE` (**now refuted on performance**, cycle 109: P50
+13,376, `STG` +2,368, 88 frames lost from 30 FPS), `NDS_DREAMLAND_DS_MESH`
(**not "needs the owner's visual A/B" — it HAD one and failed it**, Task 62,
reverted 2026-07-25, and two checkers enforce `=0`), `NDS_R2_SHIELD_QUAD` (**the Makefile itself asks for this re-price**: the owner
bought the model route at "36k p95" and that figure came off a 128-frame window,
which the whole-match rule says is unusable). **Do not re-audit the flag list.**

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

### FTR LANDED −22,689: three abstractions leave the hot fighter path (cycle 110)

**Four arms, one pre-slice baseline built and measured for the purpose.** The
baseline reads `FTR` mean **385,508**, which is the owner's stated ~385–390K to
the ticket — so the reference is right and the deltas below are real progress,
not a re-anchoring.

| arm | `FTR` mean | Δ | `WORK-H` mean | Δ |
|---|---:|---:|---:|---:|
| pre-slice baseline (`4fc9d79d14~`) | 385,508 | — | 1,062,929 | — |
| + slice 1, emit capture hook | 374,332 | −11,176 | 1,052,509 | −10,420 |
| + slice 2, flat baked compose | 366,597 | −18,911 | 1,044,687 | −18,242 |
| **+ slice 3, `m4x4` intermediates** | **362,819** | **−22,689** | **1,040,085** | **−22,844** |

`FTR` P50 396,032 → 379,328 and P95 399,040 → 382,464 across arms 1–3.
**Every control drifts under ±950 and non-monotonically** across all four arms
(`STG` −538, `SRC` +919, `SINT` −167, `SCPU` +572, `MISC` −264) while `FTR` falls
monotonically by 22,689 — so this is mechanism, not relinking.
`scripts/compare-tick-hud-arms.py` prints this table and states, from
`romSha256`, whether a given pair carries a placement term at all.

Slice 2's arm is exact — **identical `romSha256` in both arms**
(`13B8DF73…`), the `.data` route poked and read back at end of run, 3,951 calls
and **0 rejects**, and every unrelated bucket flat within ±40 (`STG` −20, `SRC`
−25, `SINT` +6, `SCPU` −8). `FTR` and `WORK-H` agree to 87 ticks, so the whole
delta lands in the bucket that owns the change. Slice 1's arm is cross-build, but
−11,176 with the disassembly showing the instructions gone is not a placement
artifact, and the combined −18,911 clears the 14,080 placement term outright.

**Slice 1 — the Task 36 stage-capture hook, deleted from all five fighter emit
loops.** `ndsRendererHardwareWriteVertex16Words`/`…TexCoordWord`/`…ColorWord`
carry a record hook compiled in whenever `NDS_TASK36_HW_COMPOSE == 2`, which the
shipped ROM sets. On the fighter path it can **never fire**: the capture window
is opened by `ndsRendererTask36ReplayCaptureBeginRun` and closed by `…EndRun`,
both inside `ndsRendererCommitNativeStageSegment` bracketing one **stage** run,
and BeginRun faults outright on a non-stage index. The effect-packet capture is
armed the same way around an effect display list. So a stage-capture abstraction
and a diagnostic capture were being tested **once per fighter corner**.

The proof is the disassembly, not the clock: the untextured loop goes **19 → 11
instructions a corner** and the textured **25 → 14**, every stack spill gone
(the maybe-call was forcing the vertex words to memory and then reloading the
loop-end pointer). Per-PC profile data prices exactly the deleted instructions at
**10,892 + 3,231 = 14,123 ticks/frame** on the tick-HUD ROM against a measured
11,176 — 1.26x, which for a cycles estimate is agreement.

**Report the shipped number separately: ~7,300, not 11,176.** The effect-packet
half is `NDS_TICK_HUD`-only, so the instrument carries a hook the published ROM
does not (`sNdsEffectPacketArmed` is absent from
`smash64ds-battle-playable-hwtri.elf`; `sNdsRendererTask36CaptureActive` is
present at the address the hot loop loads). The Task-36-only share is 7,598 +
~1,600 of the 14,123, so scale the measurement by 0.651.

**Slice 2 — the flat baked world compose, and the recorded design was wrong.**
`PrepareNativeOwnerMatrices` asked `BuildDObjWorldMatrix` for each of the 14/18
bindings independently, and because that entry point knows nothing about the
order it is called in it paid for the ignorance every time: a linear-probed hash
lookup on the binding, a walk all the way to the root, one hash probe per
ancestor hunting a prefix somebody already built, and a store per composed step —
`BuildDObjWorldMatrix` self 13,947 + `FindDObjWorldMatrix` 4,385 = **18,332
ticks/frame of machinery** around ~50 local builds a frame whose arithmetic none
of this touches.

The order is not unknown, it is baked. One forward pass over `BindingParents`
composes every world starting from its parent binding's finished matrix, straight
into `sNdsRendererAdapterNativeOwnerModelviews` so the worlds need no second
home — **no new RAM, no duplicate representation**.

**The correction, caught statically before the build: `BindingParents` is the
nearest *bound* ancestor, not the DObj parent.** This board's own recorded
pseudocode said `world[i] = local_i × world[BindingParents[i]]`, and
`generate_nds_native_owners.py:1169-1177` walks `parents[]` past every UNBOUND
joint to build that table. Mario binding 1 is joint 5 whose real parent is the
unbound joint 4; the recorded form would have silently dropped joint 4's local
matrix. The live chain is still walked — just to the parent binding instead of to
the root, one to three joints instead of full depth. Prefix publishing is also
kept, because the generic display-list path shares that hash for effects parented
under fighter joints: **what is deleted is the probing, not the publishing**, and
that is why the win is 7,735 rather than the full 18,332.

**Graduated, not left behind.** With 0 rejects over 3,951 calls the route and its
two counters are deleted and the compose is unconditional, fail-closed to the
per-binding path on any disagreement between the baked table and the live tree.

### The ~331K fighter draw is reconciled: 314,555 ticks/frame in named symbols

`scripts/analyze-fighter-draw-reconciliation.py` (no build, no run — it reads the
c106 census plus the soft-float caller attribution). Idle removed first,
`%non-idle × 1,128,000`, soft float re-attributed to callers, and census-only
instrumentation excluded rather than counted:

| group | ticks/frame | what it is |
|---|---:|---|
| **matrix preparation** | **96,207** | local build 18,245 · split load 15,248 · mul_affine 14,902 · world 13,947 · TraRotRpy 11,916 · mul 9,842 · pair load 7,167 · find 4,385 |
| **production driver** | **54,043** | `ExecuteNativeFighterOwnerProduction` 30,582 · `PrepareProductionRun` 21,205 |
| **emit / GX submission** | **48,115** | untextured 31,684 · textured 11,587 · cross 4,772 |
| **adapter driver** | **44,680** | `DLAllDrawForSlot` 29,841 · `ftDisplayMainDrawDefault` 10,269 |
| **material / shading state** | **35,568** | snapshot 11,656 · shade 8,512 · state delta 8,277 · material 5,603 |
| **fighter parts / params** | **18,711** | `ndsFTParamsInvalidateFighterParts` 15,815 |
| **display contract / plan** | **17,231** | spread over 17 symbols, none above 3,754 |
| **total** | **314,555** | 95% of the owner's ~331K |
| *(excluded: instrumentation)* | *28,049* | debug HUD 15,181 · `ndsFtrPreMaterialCensus` 9,061 · `cpuGetTiming` 3,807 |

**This answers the "is it exhausted" question with a shape, not a verdict.** The
two largest groups are not leaf arithmetic. `ExecuteNativeFighterOwnerProduction`
spreads 26.5M cycles over **709 distinct PCs** with no site above 5.1%, and
`PrepareProductionRun` over 384 with none above 3.7% — that is the signature of
whole-body architecture cost, exactly what the plan claims and what a per-helper
refutation cannot see.

**Three targets are now named with numbers, in order:**

1. **`ndsRendererNativeBindProductionRoot` copies two 64-byte matrices per root**
   (`nds_renderer.c:23897,23905`), 12,422 times a match at **416 cycles a call**
   = **5,958 ticks/frame**, and its own comment says the value is read only by
   the split loader and a flag test. The stage owner already holds
   `sNdsNativeStageOwnerExecution.projection` **as a pointer** — the fighter is
   the one that copies. Half of this is data that has to be read either way, so
   price it at ~3,000 before committing to it.
2. **`ndsRendererLoadHardwareSplitMatrices` is 1,064 cycles a call** over 12,431
   calls, flat across 185 PCs, and **absent from the census's non-mem stall
   ranking** — so it is memory stall, not placement. Slice 3 (below) removes the
   `m4x4` intermediates; the `scaled_modelview` copy that remains can go too by
   writing `MTX_LOAD_4x4`'s 16 words as 12 unscaled plus 4 rescaled.
3. **`ndsFighterMarioFoxDLAllDrawForSlot` is 9,844 bytes with cyc/insn 5.60**,
   the 3rd largest non-mem stall in the build (4,655,284), and **51% of its
   instructions never execute** — 7,108 bytes sit in cold runs of ≥64 bytes.

**But do not cold-split it on that number — most of it is the instrument.** The
largest cold run is 1,848 bytes of `ndsFighterDrawPlanVerify`, which is inside
`#if NDS_TICK_HUD` behind `gNdsFtrPlanVerify != 0`, so it **does not exist in the
shipped ROM** — `smash64ds-battle-playable-hwtri.elf` has the same function at
9,680 bytes, 164 short of the profiled build's 9,844.
`RestoreNativeOwnerMaterialTextureIds` is already outlined there as
`.part.0.constprop.0` (156 B). What is left to win by marking arms `cold` is
several hundred bytes, not 7,108. **This is the addr2line trap in a new place:
the names were right and the shipped relevance was not.** Check the shipped ELF
before costing any placement work off a profiled-build census.

### The next architecture is the per-run descriptor, and it is 89,611 ticks/frame

With matrix preparation now the smaller half of what it was, the largest
untouched block is **production driver 54,043 + material/shading state 35,568 =
89,611 ticks/frame**, spent over ~30–37 runs a frame — roughly **2,400 ticks per
run of setup**. `ndsRendererNativePrepareProductionRun` alone is 18.4M cycles
over **384 distinct PCs with no site above 3.7%**, which is what per-run policy
re-derivation looks like: texture params, poly format, UV scale and vertex flags
resolved from live state every frame for a run whose descriptor is immutable.

That is exactly the plan's "AOT compact GX-facing run descriptors", and unlike
the emit stream it needs **no new RAM**: the run's immutable fields already exist
in `sNdsNativeFighter*` tables, and what the runtime recomputes is the *binding*
of those fields to the current texture/material state — which changes only when
the material does. The shape is a per-run descriptor validated once per material
epoch instead of once per run per frame, with the epoch counter as the key.

**Sizing note for whoever takes it:** the emit half is close to its floor.
Untextured emit is now 11 instructions and 3 GX FIFO words a corner over 537,780
corners a match (51.1 cycles a corner before slice 1). Going below that needs a
pre-packed command stream DMA'd to `GFX_FIFO` — the stage path already runs at
DMA's ~2 cycles/word floor against the fighter's ~17 — but at 3.5 words a corner
that stream is ~19–26 KB of main RAM, against `gSYTaskmanGeneralHeap`'s ~9,368 B
of slack over the anim cache's `KEEP_FREE`. **RAM is the blocker there, not the
mechanism**, so it needs something freed first.

**Slice 3 — MEASURED −3,778.** `NDSRendererMatrix20p12` is `s32 m[4][4]` and
libnds' `m4x4` is `int m[16]`, both row-major and both 64 bytes:
`ndsRendererCopyMtx20p12ToM4x4` was writing element i to element i. Both per-root
loaders now hand `glLoadMatrix4x4` the matrix directly through a
`_Static_assert`-guarded accessor, deleting three of the four 64-byte
intermediates and 128 bytes of stack traffic a call. On its own it is under the
placement term, which is exactly why it shipped **with** the other two rather
than as its own arm — the cumulative −22,689 is what clears the floor.
The fourth intermediate is `scaled_modelview`, still needed because the bottom
row is rescaled; writing `MTX_LOAD_4x4`'s 16 words as 12 unscaled plus 4 rescaled
removes it, but that bypasses the `glLoadMatrix4x4` wrapper the Task 29/34/49
census records through, so it needs the wrapper preserved by hand.

**Verified, not just measured.** `verify-all.ps1 -Profile Boundary` passes with
the flat compose unconditional, and `artifacts/visibility/latest.png` shows both
fighters fully articulated on Dream Land — a wrong world matrix is exactly what
destroys that, so the screenshot is the check that matters here.

### Cycle 110 slices 4–7: FTR −37,640, ALL −67,718, and one refuted arm

Continuing the same lane. Every arm 1600 samples, `NDS_R2_BOTH_CPU` off, DLDI on.

| arm | FTR mean | ALL mean | WORK mean | what changed |
|---|---:|---:|---:|---|
| c110 baseline | 385,508 | 1,285,825 | 1,075,918 | pre-slice |
| slice 3 | 362,819 | 1,252,041 | 1,075,918 | slices 1–3 (committed) |
| slice 5 | 349,955 | 1,238,289 | 1,063,906 | +4, +5 |
| slice 6 | 340,691 | 1,225,916 | 1,048,038 | **counter gate — REFUTED** |
| slice 6b | 348,069 | 1,237,683 | 1,061,493 | counters restored |
| slice 7 | **347,868** | **1,218,107** | 1,037,278 | DTCM summary + flat parts |

**Slice 5 — MEASURED −11,683**, against a 5,958 prediction.
`ndsRendererNativeBindProductionRoot` copied the caller's projection and
modelview into the traversal state: two 64-byte struct copies, 12,422
executions, 416 cycles a call. With `NDS_R2_SHADE_SKIP_SOFT_LIGHT` the
production path has no reader of either field, so both were dead stores. The
split loader now takes the caller's matrices directly, which removed the copy it
was making of the copy — that second-order copy is the extra 5,725.

**Slice 4 — kept, honestly inconclusive at −1,181** against a −6,500 prediction,
under the placement floor. It was **mis-sized off the wrong revision**: the c106
profile ELF was built at `1b467da` and I resolved its line numbers against HEAD,
~85 lines adrift, so the biggest row landed on a blank line and the field I
targeted was free. `scripts/analyze-symbol-line-profile.py` now reads
`NDS_TASK10_GIT_SHORT` out of the build's own `nds_build_config.h` and quotes
every line from that commit, so the mistake cannot recur.

**Slice 6 — the counter gate is REFUTED by the gate itself.** Compiling out the
`sNdsRendererRuntimeFrameSummary` per-call counters (matrix load, batch
begin/reuse/end, texture prepare/reuse) was worth **FTR −7,378 and STG −2,776**,
an order of magnitude past the ~1,300 the per-line profile showed — the profile
only sees the two symbols the lines live in, and *every* hardware batch on every
path pays them. It is not available: `verify-all.ps1 -Profile Boundary` also
runs `verify-battle-mariofox-gcrunall-loop-harness.ps1`, which asserts exact
batch and texture-prepare accounting off those globals, and it failed with
*"Canonical realtime HW build drifted from exact source-weapon-aware batch and
texture-prepare accounting"*. **`-Profile Boundary -List` prints one row and the
run executes about six checks** — grepping `scripts/` is not how you find out
who depends on a global.

**Slice 7 — the same 10,154 recovered without touching the evidence.**
`sNdsRendererRuntimeFrameSummary` is 108 bytes; it now lives in `.dtcm.bss` at
`0x02ff21e0`. DTCM is single-cycle and outside the 4 KB D-cache, so the counters
keep counting and stop paying main-memory latency *and* stop evicting fighter
data. Nothing DMAs it (DMA cannot read DTCM).

**Slice 7 also flattened the parts-invalidation walk — and it is NOT an FTR
lever.** `ndsFTParamsInvalidateFighterParts` is 15,815 census ticks/frame over
159,748 joint visits: 86 cycles a joint for two word writes, because
`user_data.p`, `transform_update_mode`, `unk_dobjtrans_word`, `child` and
`sib_next` are five separate cache lines. The subtree is now preordered once
into a flat `FTParts*` array keyed on `(root, gNdsTaskmanHeapGeneration)` —
sound because that generation is bumped at the two taskman-heap rewind
primitives, the only way a live fighter tree can be rebuilt. **But the tick HUD
charges that walk to `SRC`/`SINT`, not `FTR`.** Slice 7's arm is FTR −201 and
WORK −24,215; the win is real and it is in the gameplay buckets.

> **Read this before sizing the next FTR slice.** The census→bucket mapping in
> the reconciliation below is *not* the tick-HUD bucket mapping. "fighter parts
> / params 18,711" is outside `FTR`. Only the emit, production-driver, matrix,
> material and display-contract groups are inside it. A census row is not an
> FTR row until the arm proves it.

Two instrument facts banked from this cycle. First, **`-RingDump` and per-frame
stops agree to the tick**: `c110-slice6b` and `c110-slice6b-ring` are the same
`romSha256` and report FTR 348,069 both ways, STG within 2. Every arm in this
lane is comparable regardless of mode. Second, **a faster ROM breaks per-frame
sampling**: slice 7 tripped the repeated-presented-frame guard 315 times in 1600
because the 60 Hz loop now fits two iterations inside one presented frame more
often. Ring dumps saw 5, all payload-DIFFERS (never a stale read). Use
`-RingDump -AllowRepeatedFrames` from here.

### Slice 8: the fighter material block is a constant, and a census already knew

**FTR 347,868 → 333,322, −14,546.** `WORK` −14,066, `ALL` −6,509. Boundary
passes. Cumulative for cycle 110: **FTR 385,508 → 333,322, −52,186**;
`ALL` −74,227.

`ndsRendererAdapterBuildNativeMaterialSnapshot` reconstructs a 100-byte N64
display-list material command block out of a pointer-chased `MObj` — ~761 cycles
a call, about twelve times a frame, 13,176 ticks/frame, of which **2,124 is the
single `mobj->sub.flags` load missing cache at 139 cycles an execution**. It is
a pure function of `mobj->sub` plus `texture_id_curr/next`, `lfrac` and
`palette_id`.

**The measurement that decided it needed no build and no new code.** Cycle 98
left an invariance census in the tree (`ndsFtrPreMaterialCensus`, hashes the
snapshot and compares against the previous one for the same MObj) and nobody had
read it. Off the shipped tick-HUD ROM with `-ExtraGlobals`:

```
gNdsFtrPreMatCalls=20,100  Same=20,069  Variant=0  New=31  Evict=0
```

**Zero variants.** Thirty-one distinct materials, each built once and then
rebuilt identically twenty thousand times. That is not a cache question, it is a
constant being recomputed — so the skip is a 12-byte `(MObj, heap generation,
FNV-1a of the complete input set)` key per materials-array slot. Complete
coverage rather than the fields I believed could animate: the builder reads
nothing else, so equal inputs is equal output by construction, with only a 2^-32
collision to argue about. The heap generation is in the key because MObj
pointers are taskman-arena addresses that a scene rewind reuses. Skipping the
build's write-back of `texture_id_curr/next` is safe because the stored hash is
taken *after* it — a match means the write would store what is already there.

**Two things this slice got wrong first, both worth keeping.**

*The engagement counter was not optional.* The first arm measured FTR −13,587
while `gNdsFtrPreMatCalls` did not move, which is consistent with both "the skip
fires and the census counts a different call site" and "the skip never fires and
this is placement noise". Only a purpose-built `gNdsR2MatKeySkip`/`KeyBuild`
pair settled it: **28,786 skips against 30,606 builds**, and the second arm then
reproduced the win with `WORK` moving too (−14,066, where the first arm's `WORK`
was −6,470 with buckets shuffling). A change whose engagement you cannot read is
not measured, it is guessed at.

*Narrowing the hash is REFUTED.* Six of `MObjSub`'s thirty words are read by
nothing in the builder (`unk48`, `unk4C`, `unk68..unk74`), so hashing them looked
like the reason only 48.5% of calls skip. Restricting the hash to the read set
returned **bit-identical counters — 28,786 and 30,606 again** — for +1,155. The
rebuilds are `keys[count].mobj != mobj`: the materials array is indexed by
(selected-root slot, chain position) and **which DObj lands in slot *i* rotates
between frames**, so about half the lookups find the right block under the wrong
index. Reverted to the complete hash, which measures the same and needs no field
audit to stay correct.

### Slice 9: the other half was free, and the counter said which half it was

**FTR 340,916 → 329,034, −11,882**, `WORK` −10,958. Boundary passes. Cumulative
for cycle 110: **FTR 385,508 → 329,034, −56,474**.

Slice 8 left 51.5% of material lookups rebuilding and I had two candidate
explanations. Rather than guess again — the narrow-hash guess had just cost a
build — I split the miss counter by reason and ran it. The answer was not close:

```
gNdsR2MatKeySkip=28,786  gNdsR2MatKeyBuild=30,606
gNdsR2MatKeyMissIdentity=30,606  gNdsR2MatKeyMissInputs=0
```

**Every rebuild was an identity miss. Not one was an input change.** The block
was always correct and always filed under the wrong row, because the materials
array was indexed by the selected-root index `i`, which rotates between frames,
while the material `DObj` is stable for the fighter's life. So hash the DObj to
a row and keep it there (linear probing; distinct DObjs get distinct rows; two
roots that genuinely share a material DObj share its row and its block, which is
right; cleared on a taskman-heap rewind because these are arena pointers). 36
bytes, and `root->materials` moves out of the once-primed invariants into the
per-frame build, since which row a root points at is now a per-frame fact.

**After:** `Skip=59,362  Build=30  MissIdentity=30  MissInputs=0`. The fighter
material command block is now constructed **thirty times in a sixty-second
match** — once per distinct material — against 30,606 before and 59,392 with no
key at all. That is the whole of `ndsRendererAdapterBuildNativeMaterialSnapshot`
deleted from the frame, and it is deleted rather than cached: the block that
survives is the one the builder would have produced, proven by a key over its
complete input set.

The lesson is the counter, not the fix. Two consecutive slices were sized by
reasoning about *why* a skip missed; the first reasoning was wrong and cost a
build, the second was right only because a five-line counter answered it in one
run. **Split the miss counter by reason whenever a skip rate is below what the
invariance census predicts.**

### Slice 10: the display-contract event had two halves with opposite lifetimes

**FTR 329,034 → 323,871, −5,163**, `WORK` −7,400. Boundary passes. Cumulative
for cycle 110: **FTR 385,508 → 323,871, −61,637**; `ALL` −76,630.

`NDSFighterDisplayContractEvent` carried its four DObj/Gfx pointers — read by
three tight per-root loops in the same pass that walks the collection — next to
its render preamble, read once per root by
`ndsRendererAdapterBuildNativeProductionInputs` a pass later, after the matrix
and material work has evicted it. 56 bytes together, so the halves landed on
different lines and 32 events spanned 1,792 of a 4 KB D-cache.

c106 priced the eviction exactly: **2,966** ticks/frame on
`root->preamble.geometry_mode = event->geometry_mode` and **1,759** on
`if (event->light_valid)` — about 110 cycles an event of pure miss for what
reads like six field copies.

The preamble now lives in its own array **in the consumer's own
`NDSRendererNativeFighterPreamble` layout, written by the producer** in
`ndsFighterDisplayContractSelectDL` — including the flags word both build sites
used to derive, identical arithmetic one pass earlier, into a line that is hot
because the event was just written. The event is 16 bytes; 512 of pointers plus
768 of preamble instead of 1,792 interleaved. Each of the readers becomes a
24-byte struct copy or one field out of a dense array. **Five readers, not the
four static analysis found** — the fifth seeds `persistent_stats` from
`light_valid`, which is now the preamble's `LIGHT_VALID` bit. The build caught
it; a grep for `event->` had not.

### Slice 11: FLAT. The 3,429-tick row was not 3,429 ticks of removable work

**FTR 323,871 → 324,013, +142. Nothing was bought.** Boundary passes; kept only
because it is strictly less code (one function and one chain traversal gone).

Two changes, and the counters refuted both premises before the ticks did.

`ndsRendererAdapterSaveNativeMaterialTextureIds` was a second walk of the same
MObj chain the prepare walk already traverses, reading the same two fields, and
the c110 profile charged it **3,429** ticks/frame. It is now folded into
`ndsRendererAdapterPrepareNativeMaterials` as two `s32 *` out-parameters, with
`*out_count` set on every `return FALSE` so a partial walk still rolls back
exactly what it mutated. **The recovered cost was ~0**, and the arithmetic says
why: `gNdsR2MatKeySkip + gNdsR2MatKeyBuild` is 59,392 over 1,600 frames, i.e.
**~37 chain nodes a frame**. Folding removed the call and the pointer chase, not
the four loads and stores — perhaps 15 cycles × 37 ≈ 550, under the floor.
**A profiler row for a leaf that is one line of a loop prices the loop iteration,
not the work you can delete by inlining it.** Price the delta, not the row.

Second: the material-row hash moved from `(ptr >> 4) & 31` to multiplicative
`(ptr * 2654435761u) >> 27`, because slice 8 measured that shift hash at a 48%
miss rate (28,786 skip / 30,606 build) and it read like row collisions. **The
slice-10 baseline already ran 59,362 skip / 30 build on the shift hash** — the
counters are bit-identical across the two ROMs, so the collisions slices 9 and
10 removed were never the hash's. Kept anyway at equal cost: the 48% miss proved
the shift hash is fragile to whatever the allocator does next, and the multiply
is one instruction either way.

### Slice 12: the I-cache, not the arithmetic — 73.6% of the driver never runs

**FTR 324,013 → 318,266, −5,747**, `ALL` −5,797, `WORK` −5,061, `WORK-H` P95
−11,584. Boundary passes. **No logic changed at all** — this moves code.

`ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` is the largest non-idle symbol
in the ROM (112.5M cycles, 2.91%) at **4.21 cycles per instruction** — a
function waiting on memory. Its 10,708 bytes retired only **1,414 distinct
PCs**. Diffing the executed PC set against `objdump` says **7,880 bytes (73.6%)
never execute**, in 69 runs. The ARM946E-S I-cache is **8 KB**.

Two inlined blocks held most of it: `ndsRendererAdapterPrimeProductionInputs`
(~1,640 B, runs **once per match**) and the per-binding world build inside
`ndsRendererAdapterPrepareNativeOwnerMatrices` (~3,150 B with
`ndsRendererAdapterBuildDObjWorldMatrix` inlined into it, and it declined **0
times in 49,422 binding visits**). Both are now `noinline, cold, Os`.
`flat_worlds` is decided once before the loop, so hoisting the test out is a
pure transformation — and it is what lets the fallback leave the function.

`SRC` −3,011 and `GCRA` −2,906 came along, which is the tell that the lever was
the shared I-cache and not anything local to the draw.

**The tooling for this is now a two-command recipe** — `task37_census.py
--pc-detail SYMBOL` for the executed PC set, then the cold-run diff against
`objdump`, then `addr2line` on each run's first address. Run it on any hot
symbol over ~4 KB before optimizing its arithmetic.

### Slice 13: the DObj world cache had ZERO readers

**FTR 318,266 → 317,247, −1,019.** Boundary passes; fighter pixel counts
identical. Small, but it deletes work that was provably dead.

The c112 census settles what grep could not:
`ndsRendererAdapterFindDObjWorldMatrix` **0 cycles** and
`ndsRendererAdapterBuildDObjWorldMatrix` **0 cycles** over a whole match, while
`ndsRendererAdapterStoreDObjWorldMatrix.part.0` burns **4,744,740**. Every one
of those stores fed a cache with no reader, and streamed ~4 KB a frame of write
traffic through a **4 KB** D-cache. The store is gone from the flat compose; the
cache and its fail-closed filler are untouched.

With no reader, the composed matrices no longer have to be world-space, so the
flat compose now seeds from the **camera** instead of the identity. That deletes
the `world * camera` that ran once per binding (~31 a frame of the 55.5
`ndsRendererMtxMulAffine20p12` calls a frame, 695 cycles each) and costs
nothing: the first multiply of a root chain used to be against the identity.
The hitlag shuffle folds the same way — `world * T * camera` reassociates to
`world * (T * camera)`, one 4x4 a frame instead of a row-3 add per binding.
Fixed-point reassociation is not bit-exact; these matrices reach GX and nothing
else.

**Why only −1,019 when ~12,000 of multiplies left?** The seed setup added **116
bytes** to a function already over the I-cache. Slice 12's lesson, charged
again at the till.

### Slice 14: REFUTED — outlining code that RUNS is a different lever

`noinline` on `ndsRendererAdapterPrepareNativeOwnerMatrices` and
`ndsRendererAdapterBuildNativeProductionInputs`, on the theory that shrinking
the driver toward 8 KB is good regardless of what moves. The driver did shrink
**10,528 → 9,612 bytes** and **FTR rose 2,192**. Reverted; the revert rebuilt
the slice-13 ROM **bit-identically** (`sha B7F2493F0C13`, every bucket to the
tick), which is also the cleanest proof yet that this sampler is deterministic.

**Slice 12 won by removing bytes with ZERO executions. Slice 14 lost by removing
bytes with many.** Size is not the metric; executed-vs-resident is. Both
functions now carry a comment saying so, because the next reader will otherwise
try it again.

### Slice 15: the driver fits the I-cache — 10,528 → 7,516 bytes

**FTR 317,247 → 313,421, −3,826.** `WORK` −11,561, **`WORK-H` P95 −21,248**,
`ALL` −7,308. Boundary passes. Again: no logic changed.

Same recipe as slice 12, run again on the post-slice-12 build, with the cold-run
attribution improved to sample **four points inside each run** instead of only
its first address — one run had been credited entirely to `ndsFtrPreWalkCensus`
when three quarters of it was `ndsFighterDLAllDrawAccumulateStats`.

Three more never-executed bodies, all inlined into the driver:

| body | why it never runs |
|---|---|
| `ndsFighterDLAllDrawAccumulateStats` | needs `detailed_output`, never set |
| `ndsRendererAdapterPrepareNativeOwnerHierarchy` + `…GetHierarchyCameraMatrices` | only `FAST_RUN_NATIVE_FIGHTERS`; live mode is `…OWNER_PRODUCTION` |
| `ndsRendererAdapterValidateNativeOwnerCached` | a plan hit skips it, and the plan hits every frame |

All three are `noinline, cold, Os` — still live, still correct, just no longer
renting I-cache lines from the code that runs.

**The driver is 7,516 bytes: under the ARM946E-S 8 KB I-cache for the first
time.** Cumulative for cycle 110: **FTR 385,508 → 313,421, −72,087**.

### Slice 16: REFUTED — cold BYTES are not a never-entered BODY

`ndsRendererAdapterBuildDObjXObjMatrix` is **72% never-executed** (1,758 of
2,440 bytes), so its four alternate matrix kinds —
`GetDObjVectorTracks`, `BuildFighterPartsMtx`, `BuildBillboardMtx`,
`BuildRecalcLocalMtx` — went `noinline, cold, Os`. The function shrank
**2,440 → 964 bytes** and **FTR rose 14,963**, `WORK` +14,901. Reverted.

`ndsRendererAdapterBuildFighterTraRotRpyDirect20p12`, checked in the same pass,
is **97.9% hot** — nothing to outline there at all.

**The discriminator is entry count, not cold-byte count.** Slices 12 and 15 won
because their bodies are never *entered*: a flag that is never set
(`detailed_output`), a mode that is never selected (`FAST_RUN_NATIVE_FIGHTERS`),
a branch that never fires (`flat_worlds == FALSE`), a once-per-match primer.
Slice 16's helpers ARE entered — every joint calls `GetDObjVectorTracks`, which
then returns early. Outlining that turns a predicted fall-through into a call
plus a guaranteed I-cache miss, ~69 times a frame.

**Before applying the cold recipe, ask whether the ENTRY is cold, not whether
the body is.** A cold run that starts mid-function is an early return, not dead
code.

### Slice 17: REFUTED — and it CORRECTS slice 13's stated mechanism

Two things, one build.

**The material key narrowed correctly and measured worse.** `MObjSub` is 120
bytes and the builder reads most of it, so the cycle-109 "narrow to the read
set" refutation stands — the read set IS the struct. But only **nine** words
*animate*, and they are contiguous: `MObj+0x58..0x6F` (the five colour tracks
`gcPlayMObjMatAnim` writes plus the prim level pair) and `MObj+0x80..0x8B`
(`texture_id_curr/next`, `lfrac`, `palette_id`). Nine words, two cache lines,
against thirty-four words and five. Shipped with a fail-closed half: the whole
34-word hash re-checked every fourth frame, `gNdsR2MatKeyMissStatic` counting
disagreements. **FTR +3,342** — a volatile frame-counter load and a branch per
entry, plus a 16-byte key. Reverted to the narrow hash alone (slice 18), and
that scaffold's result is now a comment: **~14,848 full checks, 0
disagreements.**

**And the entry PC refuted slice 13's story.** `ndsRendererMtxMulAffine20p12`
executed its prologue **88,758** times in c112 and **88,825** in c115 — slice 13
deleted no multiplies at all. The per-binding `world * camera` it "removed" was
never running, because `NDS_R2_FIGHTER_HW_MTX` hands the camera to the hardware
and `camera_modelview_valid` is FALSE on this path. (The visual gate agrees: had
the seed really changed from identity to camera, the image would not have been
pixel-stable.) Slice 13's −1,019 is the dead world-cache stores, full stop; its
seed rework is behaviour-preserving scaffolding that slice 18 then made pay.

**`docs/optimization/` memory says "Entry PC gives exact call counts" and I did
not use it before spending the build.** A symbol total divided by a guessed
per-call cost is not a call count.

### Slice 18: don't fold the base in until a joint contributes — −10,804

**FTR 313,421 → 302,617, −10,804.** `WORK` −11,014, `WORK-H` −10,909,
`ALL` −4,206. Boundary passes, fighter pixels stable.

All **55.5** `ndsRendererMtxMulAffine20p12` calls a frame come from
`ndsRendererAdapterComposeOwnerWorldsFlat`, at **687 cycles** each. The loop used
to seed `out` from its base — the parent binding's world, or the identity for a
root — and then multiply every joint into it. So one call per binding was
*copy the base in, then multiply the base straight back out*.

Now the base is not folded in until the first joint that actually contributes:
that joint multiplies against the base directly instead of against a copy of it,
and when the base is the identity the multiply disappears entirely. A binding
whose joints all decline still gets `out = base`, as before.

Carries slice 17's narrowed material key too, without its scaffold.

### Slices 19–21: three refutations that bound where cycle 110 stops

**Slice 19/19b — the cold recipe is SPENT.** `BuildNativeHierarchyInputs` (598 B)
and `ndsFighterDrawPlanVerify` are both never-entered, exactly the pattern that
won slices 12 and 15, and marking them cold measured **FTR +4,959** on its own
(19b). The driver was already **7,516 bytes, under the 8 KB I-cache**; once it
fits, further shrinking buys nothing and the outlining still costs. **Slices 12
and 15 were not "shrink the function" wins, they were "get it under the cache"
wins**, and that is a threshold, not a gradient.

**Slice 20 — build the first joint straight into the output. KEPT.**
`FTR +289` (flat) but **`WORK-H` P95 −10,688, `WORK` P95 −12,288, `SRC` P95
−4,160**. One fewer 64-byte temporary and one fewer multiply per binding; the
mean is at the noise floor and the tail is not.

**Slice 21 — the E23 projection skip stays refuted.** Only the modelview half of
the fighter split load is per-root; E22 measured 29 of 30 loads re-pushing a
byte-identical projection, and E23's −3,008 was discarded as under the placement
floor. Re-testing it looked justified by the standing rule that repeatable gains
are kept and accumulated — but a content-keyed skip measures **FTR +4,566**. A
64-byte `memcmp` costs more than the eighteen FIFO writes it saves; E23's revert
was right and its −3,008 was probably placement. **The rule "keep every gain" does
not license re-running a refuted experiment without new evidence.**

**Cycle 110 total: FTR 385,508 → 302,906, −82,602 (21.4%).**

### Slice 22 (ARCHITECTURAL): the prepared dense UVs are immutable state

**FTR 302,906 → 301,162, −1,744.** Boundary passes. Engagement: **Skip 30,189,
Build 15** — the loop runs fifteen times in a match instead of 30,204.

First step of Requirement 3 proper: not a faster interpreter, a piece of
per-frame runtime work replaced by immutable data. In the Requirement 5 format:

| | |
|---|---|
| **runtime work deleted** | the per-run UV preparation loop, 30,204 executions/match → 15 |
| **immutable replacement** | `sNdsNativeFighterPreparedDense` itself — it already existed in DTCM and already held the answer |
| **remaining dynamic state** | the resolved texture's scale/origin/offset; the loop re-runs when they move |
| **RAM/text** | 67 × 24 B stamps + 67 valid bytes ≈ 1.7 KB bss; compare is five words against a hot traversal state |
| **working set** | −154 DTCM writes and −3 baked-table loads per textured run, every frame |
| **verification** | Boundary; `segment0_prepared_dense_checksum` **byte-identical 0xf1c6fadc** across slices 13/15/20/22 |
| **removable next** | move the fill to load time and `RunFirstUnique`/`UniqueCount`/`UniqueDense` leave the hot path's cache footprint |

**The proof came from instrumentation that was already in the tree.** Building
`NDS_R2_FIGHTER_RUN_PROOF=2` and reading four counters over a whole match:
**246,736 UV writes producing 106 distinct dense vertices, `gNdsR2UvChangeCount`
= 0, `gNdsR2UvOutOfRange` = 0.** Not one write in a match ever changed a value,
and the proof array covered every id. 154 writes a frame to re-derive a
constant. **Read the counters a previous cycle left before designing anything.**

The stamp carries `gNdsTaskmanHeapGeneration`. A P1 restart rewinds the taskman
heap and could put a different dense table behind the same run index with the
same texture metrics; without the fence the stamp would skip on metrics that no
longer describe the vertices.

### The run preparer, split — whole-pipeline attribution for Requirement 6

`NDS_R2_FIGHTER_RUN_PROOF=2`, whole match, **83.1 run preparations a frame**
(instrumented totals; the timing calls roughly double the function, so read the
*shares*, not the absolutes):

| phase | tk/frame | share | verdict |
|---|---:|---:|---|
| Validate | 9,119 | 15.0% | never rejects (`rej=0`); a stamp would trade compares for compares |
| TexPrep | 15,626 | 25.7% | memo covers it — Hit 17,976 / Miss 9 / **Stale 0** |
| TexReuse | 1,584 | 2.6% | the memo path, 38 cyc/call against 376 |
| **Uv** | **16,162** | **26.6%** | **DELETED, slice 22** |
| Tail | 18,224 | 30.0% | publishes `texture_prepare_*` the emit reads in the same call |

**Next architectural slice, named and blocked:** collapse the state-delta spans
at bake time (`Task36ReplayRun` 17,796 + `ApplyStateDelta` 9,010 tk/fr, ~500
applications a frame over a static 70-entry table and 196-entry sequence). The
redundancy census that would size it (`NDS_R2_DELTA_CENSUS`, already written,
`gNdsR2SpanDeltaRepeats`) **does not build: `region 'itcm' overflowed by 64
bytes`.** Evict a resident for the diagnostic arm before designing the bake —
the c115 census lists 28 ITCM residents that never execute, 2,354 B idle.

### Slice 23: two redundant passes deleted, and why the mean did not move

**FTR 301,162 -> 302,217, +1,055.** Two real deletions, both unconditional, both
measured *up*. Kept anyway -- less work and less code -- but the number is the
finding, not the win.

- **The root's render preamble is held by reference, not copied.** The 24-byte
  `NDSRendererNativeFighterPreamble` was copied out of the immutable contract
  table into every selected root every frame: 39.5 `ldmia`/`stmia` pairs a frame
  at **144 cycles each**, which the c115 per-PC census prices at 3,250 tk/fr on
  one source line.
- **The MObj counting pre-pass in `PrepareNativeMaterials` is gone.** It chased
  `mobj->next` over every material chain a second time (1,215 tk/fr on that one
  line) purely to reject an over-capacity chain before writing. The write walk
  already carries the same bound and already reports `*out_count` for rollback.

**The lesson, and it is general: a per-PC census attributes a cache miss to the
instruction that TAKES it, not to the work that could be deleted.** Both cuts
removed a *first reader*, not a *reader*. The preamble's 24 bytes are still
consumed by `ApplyProductionPreamble`, so the two line fills simply moved to it;
all the copy actually saved was the write-allocate and write-back of the
destination. The counting walk was paying the misses the write walk then rode
warm. Deleting a redundant pass over data that is still consumed saves the
**instructions**, not the **fills** -- and at this scale the instructions are
under the placement floor. Price a deletion by asking what stops being *touched*,
not by summing the cycles the profile parks on it.

### `build.ps1` was BROKEN on this branch and nothing noticed

`generate_nds_native_owners.py` -- which `build.ps1` runs and `make` does not --
died on `M3_STAGE_FALSIFIER: named source closure is absent:
ndsRendererAdapterPrepareNativeOwnerHierarchy`. **A clean checkout could not
generate the fighter IR**, and four of six generated `.inc` files are gitignored,
so a clean checkout could not build.

Root cause: `named_c_closure`'s regex demanded `^ident...\bname(`, so a
definition whose name starts its own line could never match -- and the cycle-110
cold-outlining slices gave **six** functions in `reloc_backend_renderer_dl.c`
exactly that shape:

```c
static sb32 __attribute__((noinline, cold, optimize("Os")))
ndsRendererAdapterPrepareNativeOwnerHierarchy(
```

Fixed at the root in `scripts/stages/generate_nds_native_stage.py`: the leading
return type is now optional. Admitting a bare `name(` at line start is safe
because the existing loop already rejects any match whose next `;` precedes its
next `{`, which is every call site. **A generator that only `build.ps1` runs is
invisible to every measurement cycle -- run it after touching a signature in a
manifest-named closure.**

The same run then caught slice 23 honestly: the manifest classifies
`input.preamble.flags`, and the by-reference root spells it
`input->preamble->flags`, which the arrow scanner attributes to `input.preamble`
and then loses. The preflight now hoists `const NDSRendererNativeFighterPreamble
*preamble = input->preamble;` so the field stays visible as `preamble.flags`.
**That falsifier is worth its keep** -- it found a consumed field going dark in
the same change that made it happen.

### The fighter emit is bound by VERTICES, not by words and not by data layout

`--pc-detail` on all three emitters, whole match, no build. Exact loop-iteration
counts, so these are corner counts, not estimates:

| emitter | corners/frame | entries/frame | cyc/corner | tk/fr |
|---|---:|---:|---:|---:|
| raw untextured | **1,711** | 53.2 | 40.5 | **34,606** |
| raw textured | **437** | 13.8 | 50.7 | **11,085** |
| cross-matrix | **127** | 15.7 | 84.2 | **5,350** |
| **total** | **2,275** | 82.7 | | **51,041** |

The untextured loop is eleven ARM instructions in ITCM reading DTCM, and it still
runs at **3.55 cycles per instruction**. The per-PC rows say exactly where:

```
str r3,[ip,#1164]    8.00   GFX_VERTEX16 word 2
bne <loop top>       7.94   stalled behind that store draining
add r1,r0,r3,lsl#2   6.00   stalled behind the GFX_NORMAL store
ldr lr,[r5,r3,lsl#2] 5.16   DenseNormals[dense_id]
str lr,[ip,#1164]    3.00   GFX_VERTEX16 word 1
str lr,[ip,#1156]    2.93   GFX_NORMAL
```

**~28 of the 40.5 cycles are the GX writes**, and the textured loop pays the same
~28 for FOUR words rather than three -- so the stall is **per vertex**, not per
word. Three consequences, all now settled:

- **A baked/DMA'd GX stream is refuted as a lever here.** Packed FIFO format
  costs *more* words for the same vertices, and words are not what stalls.
  HANDOFF's "the emit half is near its floor" was right; its reason (eleven
  instructions a corner) was not the reason.
- **A smaller vertex format (`VTX_10`) is refuted for the same reason** -- fewer
  words per vertex, identical vertex count.
- **Fewer vertices is the only lever, and that is exactly what strips are.**

### Task 56 mode 2 drew 35.6% of the fighter BACKFACING -- a generator bug, fixed

Found statically, no build, before spending a measurement.
`scripts/fighters/check_fighter_primitive_streams.py` expands every generated
group back into oriented triangles under the DS strip rule (triangle *k* is
`(v_k, v_k+1, v_k+2)` for even *k* and `(v_k+1, v_k, v_k+2)` for odd *k*) and
compares them with the run's source triangles:

```
mode 1: 1,714 vertex submissions in 513 groups, 626 triangles ... OK
mode 2: 1,012 vertex submissions in 162 groups, 626 triangles
  REVERSED WINDING: 223 triangles (35.6%) -- these are culled away on hardware
```

`_stripify_run`'s mode-2 heuristic tried three initial active edges:
`(t0[1],t0[2])`, `(t0[0],t0[2])`, `(t0[0],t0[1])`. The middle one is not a
directed edge of `t0` -- it emits `[t0[1], t0[0], t0[2]]`, the mirror of the
source triangle -- and **every triangle in a strip inherits its first triangle's
winding**, so a strip that started there came out entirely backfacing. The
longest-strip search picked it whenever it won on length. Corrected to
`(t0[2],t0[0])`; mode 2 is now **1,014 vertices in 163 groups, every source
triangle drawn exactly once with the source winding**.

**That is what made Task 56 unusable, and it is not what it was killed for.** The
2026-07-24 KILL row reads "does not move ALL" over a **128-frame window at frame
600** -- the window class the whole-match instrument later invalidated -- against
a control built three days earlier and ~31 KB smaller, and its "ROM hangs the
present loop" symptom has separate address evidence pointing at the boot cliff.
Nobody checked whether the geometry it drew was the fighter's geometry.

The runtime half was wrong too. `EmitProductionPrimitiveGroups` was
`noinline, cold, optimize("Os")` in `.main` while its raw siblings sat in ITCM at
eleven instructions a corner, and it branched on `textured` once per **vertex**.
A 46% vertex cut cannot survive being paid for at twice the per-vertex rate. It
now has the same placement, the same inner-loop shape, and the type test hoisted
to the group.

**One invariant the original missed:** batches are REUSED across runs without
re-issuing `glBegin`, so every other emitter -- cross-matrix, raw, the next run
sharing the batch -- assumes the primitive type is still `GL_TRIANGLE`. The strip
emitter now restores it before returning: one FIFO word against 53 run emissions
a frame, and no invariant left for a future caller to violate.

**`NDS_R2_STRIP_ROUTE=1` is the one-binary A/B**, same instrument as
`gNdsR2AnimCutRoute`: both emitters compiled, `gNdsR2FighterStripRoute` in
`.data`, `aligned(32)`, default 1. Task 56 was killed against a control that was
a different binary; this one is not. At `NDS_R2_STRIP_ROUTE=0` the test folds to
a constant and the unselected emitter is dead-coded, so the shipped ROM pays
nothing for the instrument.

### Slice 24 (ARCHITECTURAL): Task 56 strips GRADUATE -- FTR P50 -11,584

**One binary, `build-c116-t56route`, `romSha256` identical across both arms,
same melonDS `DE80E46BDCF1FD98`, 1600 samples an arm, DLDI on.
`gNdsR2FighterStripRoute` read back **0** and **1** at end of run, so the poke
landed and was never stamped.**

| bucket | A: raw corners | B: strips | delta |
|---|---:|---:|---:|
| **FTR P50** | 313,856 | **302,272** | **-11,584** |
| **FTR P95** | 316,672 | **305,408** | **-11,264** |
| **WORK-H P50** | 952,512 | **941,312** | **-11,200** |
| WORK P50 | 958,016 | 946,560 | -11,456 |
| STG P50 | 189,184 | 189,184 | 0 |
| ALL P50 | 1,118,336 | 1,118,336 | 0 |
| OTHR P50 | 201,024 | 211,648 | +10,624 |
| WAIT P50 | 181,760 | 192,000 | +10,240 |

Read the last three rows together: **`ALL` P50 is identical to the tick and the
saved work reappears as `WAIT`.** That is `ALL` being VBlank-quantised wall time,
exactly the trap that killed this lever in 2026-07 -- and exactly why the
standing rule judges on `WORK-H`, not on `ALL`. `STG` unchanged to the tick is
the control that the route touched only the fighter path.

Predicted ~20,000 from 2,148 raw corners a frame becoming ~1,160 at 40.5
cycles; measured **-11,584**, about 57% of that. The likely remainder is that
part of the ~28-cycle GX stall is **per polygon**, not per vertex, and strips
cut vertices while leaving all 626 triangles. Worth knowing before the next
geometry lever is priced off the vertex count alone.

Arm B carries one artefact frame -- `FTR` max 4,496,896 against arm A's 322,112,
with `SRC`, `GCRA` and `SINT` all maxing at ~4.5M in the same sample. They cannot
all be true at once (their sum exceeds that frame's `ALL`), so it is a perturbed
ring sample, not a hitch; it inflates arm B's *mean* by ~2,600 and is why the
means move less than the medians. Judge this one on P50/P95.

`NDS_TASK56_FIGHTER_PRIMITIVES` now defaults to **2**, so `make p1` and the
tick-HUD ROM both ship strips. `NDS_R2_STRIP_ROUTE` stays 0 by default: the
route is an instrument, and at 0 the unselected emitter is dead-coded away.

### The `.data` route WORKS — first attributable animation measurement (cycle 109)

Built the standing-rule-7 route the determinism finding demanded.
`gNdsR2AnimCutRoute` (`src/import/battleship_sys_objanim.c`, `.data`,
`aligned(32)` so no neighbouring counter's write-back can stamp the poke): bit 0
the loop-invariant hoist, bit 1 the fused `length * length_invert`. Default 3 is
shipped behaviour, so an unpoked ROM is unaffected. ROM
`builds/build-c109-route2`, `NDS_R2_BOTH_CPU=1`, DLDI on, 1600 samples/arm.

**Provenance — every control holds.** `romSha256` **identical across both arms**
(`5CE68200B1831473…`), same melonDS hash, same sample count. The route read `3`
and `0` respectively in `-ExtraGlobals` at *end of run*, so the poke landed and
was never stamped back. `gNdsR2CubicEvals` was **292,857 in both arms** — the
route changes how a value is computed, never how often, so identical eval counts
are a semantic-equivalence control, not a coincidence.

| bucket | cuts on | pre-cut | delta |
|---|---:|---:|---:|
| **WORK-H mean** | 1,128,367 | 1,132,109 | **−3,742** |
| WORK-H P50 | 1,101,824 | 1,102,528 | −704 |
| WORK-H P95 | 1,550,400 | 1,572,352 | −21,952 |
| **SRC mean** | 387,316 | 391,204 | **−3,888** |
| SRC P50 | 368,768 | 369,728 | −960 |
| OTHR P50 | 203,712 | 197,888 | **+5,824** |

**The two cuts are worth ≈3,700–3,900 ticks/frame of mean `WORK-H`.** The
attribution is what makes this the result: `SRC` is the bucket the changed code
lives in, and its mean moved −3,888 against `WORK-H`'s −3,742. Those agree.
Rough corroboration from the cycles model — the hoist's 1,955,955 predicted
cycles plus ~8 cycles saved on each of 292,857 cubic evals — lands inside 1.4x of
the measured figure, which for a cycles estimate is agreement, not precision.

**Read the mean, not the percentiles, and here is why.** P50 −704 is real but
near the instrument's 64-tick quantum. **P95 −21,952 is NOT attributable to these
cuts** — P95 sits at 1.55M, far over the gate, and the whole-match tail is effect
DObj submits, not animation; two arithmetic cuts in the joint loop cannot move it
by 22K. `OTHR` P50 moved **the wrong way** (+5,824) and nothing in these cuts can
touch `OTHR`. Both are the honest residue: **a one-binary route removes the
*placement* term because addresses are identical, but not the *cache-occupancy*
term, because the two arms still execute different instruction bytes.** The mean
is the statistic to read — it uses all 1,599 rows rather than one order
statistic.

**What this settles.** The cut is **3.8x smaller than the 14,080 cross-build
placement term that was hiding it.** That vindicates the route and retroactively
explains why the 7-cut batch's −32,128 P95 was uninterpretable. Any future cut in
the 1,000–5,000 tick class must be measured this way; there is no other method.

### The FTR pre-submission half is enumerated: four of five seams already elided

Walked `FTR_STG_OPTIMIZATION.md`'s FTR ask seam by seam. Every one now has a
number, and the plan's "delete working-set traversal and policy work" is
**already done** — which is why its own note says the baked plan "only saved
roughly 6-9K".

| seam | status | evidence |
|---|---|---|
| walk | **baked plan landed** | `native_owner_plan_hit` takes `ndsFighterDrawPlanApply`; 0 hash variants over 3,961 comparisons |
| validate | **99.95% cached** | 3,961 reuse / 2 build (cycle 98) |
| reset | **dead at shipped profile** | both call sites are in the `detailed_output` arm |
| resolve + its lookups | **already elided** | it is the `else` of the plan-hit branch, keyed on the 99.95% validator |
| matrix prep | **the only live seam** | see below |

**The resolve check is worth keeping as a method note.** I was about to memoise
`ndsFighterDrawPlanResolve` on the walk hash. Two things stopped it: the hash does
**not** cover four inputs the resolve reads (`expected_asset_id`, the display
contract's `event->dl`/`material_dobj`/`matrix_dobj`, the MObj chain length, and
the loaded file's own fields), so that key would have been unsound; and the memo
measurement independently proved the resolve is not hot — 30,385
`ndsRelocFindLoadedFileContaining` calls for the WHOLE match across all 30
callers, when resolve alone at ~10 selected DObjs per fighter per frame would
exceed 40,000. **A memo whose key is narrower than its function's inputs bakes a
stale plan silently**; that is the Task 36 lesson in a new place.

### The one live FTR seam is named: `ndsRendererAdapterBuildDObjLocalMatrix`

**62 `bl __aeabi_*` sites in one per-joint-per-frame function** — 36 `fmul`, 10
`fadd`, 7 `fcmpeq`, **6 `fdiv`**, 3 `fsub` — against 1,362 instructions. That is
more soft-float sites than the figatree parser carried, and this is the plan's
"compact matrix preparation" box.

**But its interior is already fixed point, so do not brief this as a rewrite.**
It declares `s32 translate[4]`, `s32 scale_x/scale_y`, reads rotations through
`ndsFloatBits` (a bit read, not a conversion), and its own comment says it works
in fixed point off the sin/cos table. The soft float is concentrated at its
**f32 boundary**, specifically the MVP-recalc scale path
(`sNdsRendererAdapterMvpRecalcScaleX * dobj->scale.vec.f.x` and `.y`, plus
`cobj->projection.persp.scale`) — and that path is gated on
`has_mvp_recalc_rpy_0x47`.

**So the next step is ONE COUNTER, not an edit,** and this is the third time this
cycle that rule has paid: **62 static call sites is not 62 executions.** Count how
often `has_mvp_recalc_rpy_0x47` is true and how many of the 6 `fdiv` are on it
before touching anything. Tonight the same shortcut produced a 6x error on the
material-lookup seam and, earlier in the campaign, killed the animation lever at
1.64x its target. If the recalc path is cold, this function is not a lever
either and the FTR half is fully closed; if it is hot, the fix is the same exact
converter technique the parser slice just proved, not a representation change.

**This is also where the two plans converge.** `FTR_STG_OPTIMIZATION.md` line 49
says the fixed-point animation representation feeds directly into this box, and
`FIXEDPOINT_ANIMATION.md` deliberately kept one float boundary at DObj in stage 1
and routes "fixed pose -> fixed local matrix" to stage 2 **gated on profiling
justifying it**. The 62-site count is the beginning of that justification, not the
end of it — the counter is.

### The matrix group is 20x machinery, not redundancy — the replacement is a one-pass baked compose

**The deduction that sets the design.** `ndsRendererAdapterBuildDObjWorldMatrixUncached`
walks each joint to the root and rebuilds **every ancestor's** local matrix, which
is O(n*d). That looks like the lever, and it is not: the local builder runs
**101,569 times a match = ~50 a frame**, and there are ~50 bound joints a frame
(25 x 2 fighters). **Each joint's local matrix is therefore built exactly once** —
the linear-probed world cache already collapses the ancestor rebuild. Do not
re-attack the walk.

So the 84,051 ticks/frame is not redundant work. At ~50 joints it is **~1,680
ticks per joint**, against a 4x3 fixed-point compose whose arithmetic is ~36
multiplies, on the order of **80 cycles**. That is a **20x machinery-to-math
ratio**, and it is the quantified form of the plan's thesis: the expensive part is
the architecture around the arithmetic, not the arithmetic.

**Where the 1,680 goes** (six symbols, per frame): `BuildDObjLocalMatrix` 18,290 ·
`LoadHardwareSplitMatrices` 15,218 · `MtxMulAffine20p12` 14,939 ·
`BuildDObjWorldMatrix` 13,962 · `BuildFighterTraRotRpyDirect20p12` 11,868 ·
`MtxMul20p12` 9,773. Per joint that is a cache probe, a chain walk, a `DObj` field
gather, two out-of-line matrix calls and a copy — to produce 12 words.

**The replacement, and it needs no new generator.** The IR already carries
`sNdsNativeMarioBindingParents[14]` / `sNdsNativeFoxBindingParents[18]`,
`BindingJoints`, and `JointSchedule`. Because `BindingParents` is a parent INDEX
into the same binding array, a single pass in baked topological order can compose
every world matrix into one contiguous `NDSRendererMatrix20p12[18]`:

    for i in 0..count-1:                      # baked topological order
        local  = pose_to_local_20p12(i)       # from the fixed pose
        parent = BindingParents[i]
        world[i] = (parent == ROOT) ? local : mul_affine(local, world[parent])

That deletes, per joint: the cache probe, the chain walk, the ancestor loop, and
one level of call indirection — keeping exactly one `mul_affine` and one local
build, which is the irreducible arithmetic. It is **not** a patch table and **not**
per-frame patching, so it does not repeat the +124K FIFO-template mistake; it
deletes traversal and policy, which is what the plan asks for.

**Why this is where the two plans meet.** `pose_to_local_20p12` is the seam
`FIXEDPOINT_ANIMATION.md` stage 2 feeds: today the pose arrives as `DObj` f32
rotate/translate/scale and the local build converts it, so an AOT fixed track
writing a Q12 pose removes the conversion *inside* the same loop this design
introduces. Land the compose first (pure algorithm, same inputs, verifiable by
comparing matrices against the existing path), then swap the pose source.

**Verification available before any measurement:** the new pass must produce
matrices bit-identical to `BuildDObjWorldMatrix` for every bound joint. That is a
direct A/B inside one build — compute both, compare 12 words, count mismatches —
and it is fail-closed: any mismatch falls back to the existing path. The
`gNdsR2AnimCutRoute` pattern already in the tree is the shape for it.

### Both architectures, quantified: 307K against a ~290K gap

With the conversion fixed (`%tot x 1.2378 -> x frame budget`, idle removed first),
the two re-opened architectures size up as follows. This is the accounting that
was missing every time I called a lane closed.

| lane | ticks/frame | composition |
|---|---:|---|
| **FTR fighter draw** | **230,930** | matrix 84,051 · prepare 61,432 · emit 43,282 · replay 30,577 · material 11,588 |
| **animation** | **75,953** | cubic 22,339 · play 19,128 · parse 18,709 · invalidate 15,777 |
| **combined** | **~307,000** | vs a ~290,000 gap from `WORK-H` P95 to the gate |

**Independent agreement, worth more than either number alone:** the animation
figure derived from census percentages is **75,953**, and the board's over-gate
split derived **72,638 cycles/region** for the animation class by a completely
different route. Within 5%. That cross-check is what makes the corrected
conversion trustworthy rather than merely arithmetic.

**So the two plans are right and my refutations were sizing errors.** Together
these lanes are the whole gap. Neither is a micro-optimisation lane; both are
architecture, exactly as `FTR_STG_OPTIMIZATION.md` and `FIXEDPOINT_ANIMATION.md`
say.

**One more retraction rides on this.** `ndsFTParamsInvalidateFighterParts` was
retired at "~6,560 ticks/frame"; corrected it is **15,777**. The *mechanism*
refutation stands — the dead `FTParts` pool cannot reach loads that are `DObj`
fields — but the size was under-read like everything else on this lane, and at
15,777 a preorder-flattened subtree sweep is worth revisiting on its own.

**The AOT half of FTR already exists and is not the problem.**
`src/nds/nds_native_fighter_owner.generated.inc` is **408 KB** of build-time IR:
`sNdsNativeFighterDenseVertices[541]`, `PackedCorners[1878]`,
`RunFirstCorner[67]`, `sNdsNativeMarioJointSchedule[25]`,
`sNdsNativeMarioBindingParents/Joints[14]`, and `sNdsNativeMarioFifoWords[4034]`,
emitted by `scripts/fighters/generate_nds_native_owners.py` (3,187 lines) through
`build.ps1`'s `generate-native-fighters`. **So "fighter asset at build time" is
done.** The 230,930 is what the runtime still does *on top of* that IR — which
means the replacement target is the runtime owner path, not a new generator.

**Where the 84,051 matrix group actually goes** (six symbols, ~50 builder calls a
frame): `BuildDObjLocalMatrix` 18,290 · `LoadHardwareSplitMatrices` 15,218 ·
`MtxMulAffine20p12` 14,939 · `BuildDObjWorldMatrix` 13,962 ·
`BuildFighterTraRotRpyDirect20p12` 11,868 · `MtxMul20p12` 9,773. That is a
per-joint local-build then compose-to-world then load-to-hardware chain walked off
the live `DObj` tree every frame — while `BindingParents` and `JointSchedule` are
already baked in the IR above. **This is the single largest addressable group in
the milestone and it is where the two plans meet:** a fixed-point pose feeding a
baked parent-chain compose deletes both the float boundary and the tree walk.

### RETRACTION: FTR is NOT closed. I mixed two instruments and under-read it 2.36x

The owner re-opened FTR as an architectural replacement and told me to stop trying
to prove it closed by measuring individual helpers. That was right, and the reason
is a defect in my own arithmetic, not a difference of judgement.

**The error.** I converted census cycles to ticks/frame by dividing by the
tick-HUD's 2,038 presented frames. Those are **different instruments on different
builds**. The census's own section E prints control frames at **2,240,292
cycles/frame** and over-gate frames at 3,266,336 — roughly 2x the shipped
`WORK-H` of ~1,128,000 — because the profiled build carries profiling overhead.
Census absolute cycles per frame are therefore NOT the shipped frame cost, and
dividing them by a tick-HUD frame count is meaningless.

**The sound bridge is percentage.** `armWaitForIrq` is **19.21%** of the census
total, so non-idle is **978,488,987** of 1,211,130,791 and
`%non-idle = %tot x 1.2378`. Applied to a 1,128,000-tick frame:

| symbol | %tot | %non-idle | **ticks/frame** | group |
|---|---:|---:|---:|---|
| `ndsRendererNativeEmitProductionRawUntexturedRun` | 2.27 | 2.81 | **31,693** | emit |
| `ndsRendererExecuteNativeFighterOwnerProduction` | 2.19 | 2.71 | **30,577** | replay |
| `ndsFighterMarioFoxDLAllDrawForSlot.constprop.0` | 2.14 | 2.65 | **29,878** | prepare |
| `ndsRendererNativePrepareProductionRun` | 1.52 | 1.88 | **21,222** | prepare |
| `ndsRendererAdapterBuildDObjLocalMatrix` | 1.31 | 1.62 | **18,290** | matrix |
| `ndsRendererLoadHardwareSplitMatrices` | 1.09 | 1.35 | 15,218 | matrix |
| `ndsRendererMtxMulAffine20p12` | 1.07 | 1.32 | 14,939 | matrix |
| `ndsRendererAdapterBuildDObjWorldMatrix` | 1.00 | 1.24 | 13,962 | matrix |
| `ndsRendererAdapterBuildFighterTraRotRpyDirect20p12` | 0.85 | 1.05 | 11,868 | matrix |
| `ndsRendererNativeEmitProductionRawTexturedRun` | 0.83 | 1.03 | 11,588 | emit |
| `ndsRendererAdapterBuildNativeMaterialSnapshot` | 0.83 | 1.03 | 11,588 | material |
| `ftDisplayMainDrawDefault` | 0.74 | 0.92 | 10,332 | prepare |
| `ndsRendererMtxMul20p12` | 0.70 | 0.87 | 9,773 | matrix |

**Total 230,930 ticks/frame = 59% of the ~390K `FTR` bucket**, before any share of
the leaf helpers (`__aeabi_fadd` 2.79%, `fmul` 1.81%, `memset` 1.69%, `memcpy`
1.47%, `fdiv` 0.84%) is attributed to fighter draw. The remaining ~100K to reach
331K is those leaves plus the long tail — the bucket and the symbols now
reconcile, which they did not when I claimed closure.

By group: **matrix 84,051** (six symbols) · prepare 61,432 · emit 43,282 ·
replay 30,577 · material 11,588.

**What this retracts, specifically.** Every "not a lever" verdict I wrote on this
lane was computed the wrong way and is **2.36x low**:

| I said | actually |
|---|---:|
| local matrix 7,766/frame inclusive | **18,290** |
| material snapshot ~4,960/frame | **11,588** |
| "combined addressable seam ~12,700" | **230,930** |

So `matrix` alone is **84,051 ticks/frame**, six times the placement term, and the
individual symbols are 10K-32K each rather than the sub-noise figures I reported.
The plan's "tens of thousands to >100K" projection is supported by the census; my
refutation of it was an artifact.

**The method rule this establishes, because it cost a whole cycle's conclusions:**
**never divide a census cycle count by a tick-HUD frame count.** Convert through
`%tot -> %non-idle -> x frame budget`, and take the idle share out first
(`armWaitForIrq` is a fifth of the profile). The census's own per-frame numbers
describe the PROFILED build, which runs at roughly half the shipped build's speed.
This sits alongside `whole-match-instrument-only` as an instrument-boundary rule.

**Consequence for the work:** matrix + prepare is **145,483 ticks/frame** of
preparation, which is what an owner-path replacement targets, and `replay`
(`ndsRendererExecuteNativeFighterOwnerProduction`, 30,577) plus `emit` (43,282)
are the machinery behind it. FTR is re-opened as an architectural task with a
quantified target, not a closed lane.

### The free step is taken: the last live FTR seam is 7,766 ticks/frame, not a lever

Took the per-PC/census attribution (no build, no run) on
`ndsRendererAdapterBuildDObjLocalMatrix`, joined against the 101,569 calls the arm
counter measured. That closes the FTR pre-submission half with a number.

| quantity | value |
|---|---:|
| inclusive cycles, whole match | **15,826,891 (1.31% of non-idle)** |
| calls (runtime counter) | 101,569 |
| **cycles per call** | **155.8** |
| self cycles | 3,067,306 (30.2/call) |
| **inclusive per presented frame** | **7,766** |
| **self per presented frame** | **1,505** |
| cyc/insn | 2.65 (non-idle is 2.85 — NOT a stall outlier) |
| text | 2,980 bytes |

**Verdict: not a lever.** Deleting this function's entire self time buys ~1,505
ticks/frame, under the placement term; even its whole inclusive cost is 7,766
against a ~290,000 gap. The 62 static soft-float sites that made it look like the
parser's twin resolve to **155.8 cycles a call**, and its fallback arm — which is
where the 36 `fmul` looked like they lived — executes **zero** times.

**So the FTR half of `FTR_STG_OPTIMIZATION.md` is closed, seam by seam, each with a
number:** walk baked, validate 99.95% cached, reset dead, resolve elided, material
lookups 30,385/match, local matrix 7,766/frame inclusive. That is consistent with
`FTR` separating the over/under-gate populations by only **+13,768** and with the
plan's own closing lines, which scope FTR/STG as permanent headroom rather than
the P95 lever. The plan's architecture (build-time draw program → immutable
topology → fixed pose → patch dynamic → direct GX) is not refuted; what is
refuted is that any *remaining individual seam* in it pays measurably. It is a
multi-cycle rewrite whose payoff is the sum, and the plan warns the last attempt
at that shape regressed **+124K**.

**METHOD CAUTION, and I nearly shipped it wrong.** The census's third numeric
column is **bytes**, not call count. Reading it as calls made this symbol look
like 2,980 invocations against the counter's 101,569 — a fake 34x discrepancy that
would have "invalidated" the census for sizing. Read `census.txt`'s own header
(`pack nonmem-stall cycles bytes cyc/insn stall/byte symbol`) before quoting any
column from it. The two instruments agree perfectly once the column is right, and
their agreement is what makes 155.8 cycles/call trustworthy: an inclusive cycle
total from the profiler divided by a call count from a counter in the executed arm.

### The counter answered it: the local-matrix fallback is DEAD, 0 of 101,569

Counted the arms of `ndsRendererAdapterBuildDObjLocalMatrix` before editing it,
and the answer refutes the hypothesis in the row above.

| arm | count | verdict |
|---|---:|---|
| xobj matrices (`valid != FALSE`) | **101,528** | the live path, ~50 calls/frame |
| **fallback** (`BuildDObjFallbackMtx` + `MtxFromN64`) | **0** | **never executes on the gate arm** |
| mvp-recalc identity | 41 | negligible |

**My guess was wrong, and cheaply.** I predicted the 36 `fmul` were
`ndsRendererAdapterMtxFromN64` converting a float `Mtx` to 20.12 — the classic
fixed-pose boundary. That arm runs **zero** times in a 60-second both-CPU match,
so whatever float it contributes to the symbol's 62 `bl __aeabi_*` is dead code
the linker kept. **62 static sites overstates the executed cost by an unknown
margin, and that is exactly what a static count cannot tell you.** Fourth time
this cycle that counting beat inferring; the cost was one build instead of a
rewrite of a matrix path that feeds every fighter's world transform.

**What executes is the xobj path** — `ndsRendererAdapterBuildDObjXObjMatrix` and
`ndsRendererAdapterMulInto`, both inlined into the one symbol, so they cannot be
attributed by symbol. `BuildDObjFallbackMtx` is the only callee that survived
separately, at 26 instructions and **0** soft-float calls.

**The next step is FREE and needs no build or run.** Per-PC attribution restricted
to this symbol off the existing profile CSV (`--split-by-symbol` plus a per-PC
join) will say which of the 62 sites execute and at what cyc/ex, the same way the
33.1-cyc/ex load in the parser was found. Do that before writing anything: if the
executed float is a handful of runtime-float multiplies in the xobj compose, the
exact-converter technique the parser slice proved applies directly; if it is
dominated by dead-but-linked `MtxFromN64`, the FTR half is closed and the
remaining pre-submission cost is the material snapshot's own derivation.

**Calls per frame is the number to size any fix against:** 101,569 / 2,038 ≈
**49.8 builder calls per presented frame**, consistent with ~25 joints x 2
fighters. A cut of one soft-float call per call is worth ~50 helper invocations a
frame — about 1,250 cycles at fmul's 25.17 — so a fix here needs to delete
several per call to clear the placement term.

### FTR pre-submission: the asset lookup is NOT the seam — my premise was 6x wrong

`FTR_STG_OPTIMIZATION.md` asks for traversal and policy work to be deleted from
the ~172K pre-submission half. Cycle 98 had already refuted validate and reset
and named **material prep** (99.948% byte-identical re-derivation). Its stated
mechanism was `ndsRendererAdapterValidateNativeOwnerMaterials` doing **three
`ndsRelocFindLoadedFileContaining` searches per material per fighter per frame**,
and that function's memo is **one entry deep** (`last_loaded_file_index`) while
the three pointers it is asked for — `palette_image`, `block_image`,
`current_image` — are three interleaved streams. That predicts near-total memo
thrash. Widened it to four-way move-to-front and counted.

**The counters refuted the premise.** Whole match, both-CPU, gate arm:

| counter | value | reading |
|---|---:|---|
| total calls | **30,385** | **~15 per frame**, not the ~178,000/match the material call count implied |
| `Way0` | 25,434 | **83.7% — the one-entry memo was already catching most of it** |
| ways 1–3 | **4,385** | 14.4%: the scans this change actually deletes |
| reached the linear scan | 1,014 | 3.3% |

**Why the estimate was 6x off:** `ValidateNativeOwnerMaterials` sits *behind* the
owner validate cache, which cycle 98 measured at **3,961 reuse / 2 build**. Its
three searches per material are therefore already elided ~999 times in 1,000, so
they never became per-frame volume. Reading a per-call cost off an inner
function's call count without checking the cache in front of it is what produced
a 6x error — the same shape as the self-time lesson.

**Verdict: KEEP, banked, not a lever.** 4,385 deleted scans ≈ **500
ticks/frame** — real and repeatable but far under this instrument's resolution.
It is free in footprint (**binary byte-identical**, 160 bytes of headroom, the
function stayed out-of-line at 276 bytes rather than inlining into all 30
callers), and the standing rule is to keep every repeatable
correctness-preserving gain and accumulate it. The counters stay so the seam
never has to be guessed at again.

**Equivalence was verified, not assumed,** and the non-obvious half is the
boundary: `ndsRelocPointerRangeInLoadedFile` accepts `addr == base + data_size`
(it tests `>`, not `>=`), so two adjacent allocations would both appear to hold a
pointer on the seam — except the inner `ndsRelocRangeInLoadedFile` rejects
`size > data_size - offset`, and there the remainder is 0 while every caller
passes size >= 1. At most one file can match, so "first match in scan order" and
"the matching way" name the same file.

**Do not read this run's WORK-H against the parser ROM's.** It is a different
binary (mean 1,121,957 vs 1,133,754, VBI-2 1136 vs 1086), so placement dominates
and none of that difference is attributable to this change.

**What is left of the plan's FTR half is architectural.** With validate, reset and
now the lookup underneath material prep all refuted, the remaining
pre-submission cost is `PrepareNativeOwnerHierarchy`/`…Matrices` — which the
cycle-97 table already classed as *genuinely varying*, not deletable — plus the
material snapshot's own derivation, whose 25 real variants are frac texture
animation. That is the native fighter draw program the plan describes, and the
plan's own warning applies: the previous attempt at the FIFO-template mechanism
regressed **+124K**. It is a multi-cycle project with no one-flag step, and `FTR`
separates the over/under-gate populations by only **+13,768**, so it is headroom
work rather than gate work — which the plan itself says in its closing lines.

### Parser slice LANDED and measured: WORK-H mean −6,805, 20 frames to 30 FPS

`ftAnimParseDObjFigatree` is replaced port-side (`src/import/battleship_ftanim.c`,
selected by `reloc_backend_compat_shims.c`). **45 `bl __aeabi_*` in one function
became 21**, and 4 of those 21 are out-of-range fallback arms that measured zero
executions. Gone entirely: all 12 `i2f`, all 6 `fmul`, all 5 `fcmpeq`, both
ordered compares. `battleship_ftAnimParseDObjFigatree` and
`battleship_ftAnimGetTargetValue` are **absent from the linked ELF** — the linker
drops them, so this replaces rather than coexists and the ROM grew **1,024 bytes,
exactly the reciprocal table**. Boot headroom re-checked: 29,952 proven, green.

**Measured on ONE binary (route bit 2), identical `romSha256` both arms:**

| | parser ON | parser OFF | delta |
|---|---:|---:|---:|
| **WORK-H mean** | 1,133,754 | 1,140,559 | **−6,805** |
| **WORK-H P50** | 1,099,008 | 1,104,704 | **−5,696** |
| SRC mean | 387,487 | 391,970 | −4,483 |
| WORK-H P95 | 1,568,896 | 1,568,896 | 0 |

**Pacing, from the VBlank histogram — the honest statement:** 2-VBI 1066 → 1086,
3-VBI 896 → 868. **20 frames moved from 20 FPS to 30 FPS**, and the tail got
slightly worse (4-VBI 53 → 61, 5+ 22 → 23). Net +1 frame total, which reconciles.

**Do not quote `ALL` P50 from this run.** It reads **−559,040**, which is one
VBlank period (560,190) almost exactly: the median sampled row straddles the
quantisation boundary, landing at 2 VBI in one arm and 3 in the other. `ALL` is
VBlank-quantised and this is the trap it is known for. The histogram above is
what the pacing claim rests on.

**Correctness checked four ways, not asserted.** `check_ftanim_target_exact.py`
proves the s16 conversion **bit-identical over all 65,536 × 8 inputs** (six of
the eight fracs are powers of two and an s16 always fits f32's mantissa, so it is
an exponent subtraction with no rounding step). `check_ftanim_transcribe.py`
inverse-substitutes the port body and compares token streams — **1964 tokens,
identical** — which is the check that mattered, because `AObjAnimAdvance` is
`p++`, used more than once per expression and advancing **conditionally** on a
toggle bit, so a miscount desynchronises the stream and moves hitboxes. It is
**mutation-tested 3 for 3**, including a dropped advance. The reciprocal table is
compile-time-folded and verified bit-identical to a runtime `1.0f/n` against a
`volatile` divide. And at runtime `gNdsR2CubicEvals` is **292,857 with either
parser**, in both arms of both A/Bs: the same commands set the same kinds the same
number of times.

**A checker that only says GREEN is a rubber stamp.** The transcribe checker's
first draft whitelisted a bare `break ;` token pair to excuse a dead `#else` arm
— which would have deleted **all fourteen of the parser's real breaks** and
passed anything. It evaluates the preprocessor instead. Mutation-test any
equivalence checker before believing its GREEN.

**Engagement:** 210,948 parser calls, 56,148 reciprocal hits, **0 misses** —
every payload in a 60-second both-CPU match is under 256, so the 256-entry table
is empirically right and no fallback divide ever ran. The off arm reads 156 calls
rather than 0 because `-SetGlobals` pokes at the first frame-complete marker, so
0.07% of the match runs the default route before the poke lands. Expect that
floor on every route arm; it is not leakage.

**One divide is deliberately KEPT** at `ftanim.c:205`,
`(value_target - value_base) / payload`. `x/n` and `x*(1/n)` round differently and
that feeds a `rate_base` the cubic amplifies by `length`; Linear is 1.7% of nodes,
so the trade would have been fidelity for nothing measurable.

### Parser slice: the mechanism was a PORT REWRITE, not a patch extension

Sized the parser's remaining soft float from the cycle-106 profile: `fdiv`
**1,494,619** at 109.4 cycles a call (the most expensive helper in the build by
3x) on `1.0F / payload` at exactly **two sites**, `ftanim.c:170` and `:244`;
`fsub` 1,753,743; `fadd` 1,454,187; `fcmpeq` 921,383; `fmul` 920,213; `i2f`
796,253; `fcmpgt` 385,779; `fcmple` 204,854. **≈7.9M cycles ≈ 9,000
ticks/frame** — the densest remaining block in the animation lane.

Two facts make the `fdiv` free of numerical risk: `relocdata_types.h` documents
the payload as **a u16 following the command word**, so a reciprocal table is
indexed by a small integer and hits every time; and this build passes **no
`-ffast-math`**, so a compile-time `1.0f/n` initializer is correctly rounded and
therefore **bit-identical** to the runtime divide. The six `payload` assignments'
u16→f32 conversions are exact through the already-proven `ndsR2S32ToF32Bits`
(a u16 has ≤16 significant bits, so no rounding occurs at all) — reuse, not a
second converter.

**But the mechanism I reached for is closed.** `src/import/battleship_ftanim.c`
`#define`s the parser's name, which renames its **definition and its call sites
together**, so no macro can redirect only the calls. The obvious next seam —
extending `scripts/decomp-patches/battleship/src_ft_ftanim.patch`, which already
carries DS-guarded inserts to this exact file — **runs against a standing owner
decision**: 2026-08-06, the eight patches **migrate port-side over time**, and
that table lists this very patch. Adding to it moves the wrong way.

**The slice was a port-side replacement of `ftAnimParseDObjFigatree`**, selected
by `src/port/reloc_backend_compat_shims.c:1545`, which already defined that symbol
and merely forwarded. That satisfies "replace, don't coexist" and *retires* a
decomp-patch dependency instead of deepening one. **Written, measured and landed —
see the section above.**

**Correction to a recorded blocker:** the note that "the parser has no textual
override hook" was wrong. The shim at `:1545` is the hook; what the macro blocks
is redirecting the *helper* `ftAnimGetTargetValue`, not the parser itself.

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
5. **Hit-effect presentation** — owner filed against the N64 reference,
   2026-08-05/06: the A-attack spark is oversized (two multipliers, light ramps
   with damage to 56px, heavy is flat), and the fire *burn* on the victim is
   absent. Detail, seams and the owner's verbatim wording are in `docs/BUGS.md`;
   do not restate them here. The spark ceiling is explicitly a port choice with
   the owner as oracle (`386fb8e2`), so it closes on their eye, not a
   measurement. **`docs/BUGS.md` item 3 — the owner's billboard observation —
   is a design input for any future effect work and bears directly on why the
   G3 packet path was refuted; read it before opening that lane again.**
6. **Final acceptance** — the CPU-on one-minute match, complete-match capture,
   owner play/listen pass, reserve gate, Results transition, and teardown
   proof on the exact candidate ROM.

## The eight decomp patches migrate port-side over time (owner, 2026-08-06)

The owner tightened `AGENTS.md` mid-session to **"Treat `decomp/` as read-only
reference source. Our Source of Truth. Never edit it."**, deleting the
sanctioned-exception paragraph that used to permit tracked patches. Asked
whether the eight existing patches should be grandfathered, migrated, or kept as
an acknowledged exception, the owner chose **migrated port-side over time**.

Until that finishes the rule text and the tree disagree: `fetch-battleship-reference.ps1`
still applies all eight on every fetch and they are load-bearing, so **do not
delete one without its port-side replacement landing in the same change.**

**Migrate opportunistically** — when you are already working in that file's area,
not as a sweep. Nothing here is on the gate lane.

| patch | added | shape | difficulty |
|---|---:|---|---|
| `src_sys_objman` | 124 | 19 `while (TRUE);` panics -> recorded NULL allocation failures, mid-function at 19 allocator sites | **hard** — needs port-side allocators; a wrapper cannot reach a mid-function panic |
| `src_mv_..._mvopeningroom` | 653 | "NDS entry slice": port code inside the decomp file, original body `#if !defined`'d out | **medium** — already port code, but interleaved, so it is a whole-file fork not a lift |
| `src_sys_objanim` | 105 | animation-script parsers: event bound + recorded fault on unknown opcode | medium; **pinned by `generate_nds_native_stage.py` TEXT_INPUTS — re-pin in the same change** |
| `src_ft_ftanim` | 36 | guarded inserts, 1 line replaced | case-by-case |
| `src_sys_taskman` | 32 | guarded inserts, 1 line replaced | case-by-case |
| `src_mn_..._mnstartup` | 18 | guarded inserts | case-by-case |
| `src_sc_scmanager` | 6 | framebuffer end address + two whole functions disabled | mid-function constant; needs reimplementation |
| `src_sys_objhelper` | 6 | guarded inserts | case-by-case |

Across all eight, only **4 source lines are actually replaced**; the rest are
additions under `#if defined(SSB64_TARGET_NDS)`.

**The one worked example is `2b693142`** (damage-spark scale). It migrated
cleanly *only* because the value being changed was reachable from the source
maker's return value (`pc->xf`), so the port wrapper could adjust it on the way
out. Look for that shape first; most of the rows above do not have it.

**Worth separating when prioritising:** most of the added lines convert an
infinite-loop panic into a recorded failure. Those do not change what SSB64
does — they change what happens when a pool the N64 never exhausts is exhausted
on DS. That is closer to "physically cannot work on DS" than to a behavioural
divergence, and it is the weakest case for urgency.

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

   **The readback is now IN THE ARTIFACT (cycle 100).** The harness always
   printed `SETGLOBAL=<name>,<value>` straight after each poke, but nothing
   parsed it, so the proof lived on the console — the one place this project
   has repeatedly agreed not to read. `sample-tick-hud-buckets.ps1` now parses
   it into a `setGlobals` block (`requested` / `readback` / `stuck`) and
   **throws** rather than emitting a percentile table when a poke did not take.
9. **A poke can land and still not be seen — check the cache line before
   trusting `-SetGlobals`.** The stub writes main RAM; the ARM9 keeps its own
   copy. If the target shares its 32-byte line with anything the guest writes,
   that line is dirty, the guest keeps reading its stale value, and every
   writeback stamps it back over the poke — the readback still says the write
   succeeded. Measured cycle 100 on `gNdsFtrPlanRoute` (`0x0226c560`, sharing a
   line with the per-frame `gNdsTickHudVBlankWaitTicks`): poked 7, read back 7,
   **0** route hits over 1,216 draws, 0 at end of run — while a sibling four
   bytes lower in the **previous** line survived the identical batch, and a
   second variable 12 bytes *higher* in the **same** line was erased with it. A
   4-byte store cannot do that; a 32-byte line writeback does exactly it, and
   the disassembly's only three references to the address are loads.
   **Consequence: a flag that must be routed at runtime needs its own clean
   cache line, or it belongs at build time.** The fighter draw plan took the
   build-time route (`NDS_FTR_PLAN_ROUTE`).

## Acceptance Matrix

As last graded (cycle 76); a row changes state only when its gate runs.

| Acceptance condition | State | Current evidence / blocker |
|---|---|---|
| Mario human vs original level-3 Fox CPU, Dream Land, one-minute Time, items off | Pass configuration | Boundary registry exposes only canonical mode 163 |
| Original Wait -> countdown -> GO, timer, scoring, Time Up, Results | Focused gates pass | Final exact-ROM CPU-on owner run remains red |
| Mario and Fox complete source-equivalent gameplay behavior | Red | Battle-reachable weak callbacks remain |
| Dream Land collision, platforms, blast zones, wind, camera | Pass for current P1 stage | Dynamic presentation debt remains red separately |
| Recognizable Dream Land presentation and required animation | Red | Whispy Route 7 owner-approved and promoted 2026-08-08; remaining stage presentation not regraded |
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

Current shipping pair after Fox blaster native promotion, verified 2026-08-09:

```text
smash64ds-battle-playable-hwtri.nds   12,211,200 bytes
SHA-256 C49F2C528F9EA13BA9F05985248C1BA2CCD5681EAA7A2B0C5023F5557F2D7EA4
smash64ds.nds                         11,915,264 bytes
SHA-256 54C07FAC80C50418949908701F7C2BDBF27512C5F96AC09086FABBB0DF6AC68A
tick-HUD sibling (builds/build-tick-hud-buckets)   12,218,368 bytes
SHA-256 B7800E4921E1F2BCC89EB7A4BBECDA279F44111D226BEA32D05EF7FA319C1A4F
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
