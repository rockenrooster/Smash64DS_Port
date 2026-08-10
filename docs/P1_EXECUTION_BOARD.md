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

### REVERTED: the fused `f32 x f32 -> Q16` hangs the ROM, and the host bound missed it

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
`NDS_TASK51_STAGE_NATIVE` (42 baked world matrices via `MTX_MULT4x3`; needs the
differ at Tier 1 = 0), `NDS_DREAMLAND_DS_MESH` (needs the owner's visual A/B),
`NDS_R2_SHIELD_QUAD` (**the Makefile itself asks for this re-price**: the owner
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
