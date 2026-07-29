# P1 Execution Board

Updated: 2026-07-27 22:45 Central

Boundary: `battle_playable_realtime`, mode `163`

This is the only dynamic P1 queue. `PROJECT_GOAL.md` owns the milestone and
fidelity contract. `HANDOFF.md` owns restart commands, `KNOWN_ISSUES.md` owns
durable gaps, `PERF_LEDGER.md` owns measurements and rejected experiments, and
`PORTING.md` is append-only history.

## Artifact Identity

Pinned public-build identity from `README.md`:

```text
smash64ds-battle-playable-hwtri.nds
11,428,864 bytes
SHA-256 4D795B4E83B335598B20A3B5953FDB1821797CC5E0A825FA96A0643ABBA4A090
```

Current local root artifact, rebuilt 2026-07-29 08:11 with R2-04 E4/E5
(`NDS_R2_ANIM_CACHE`) resident:

```text
smash64ds-battle-playable-hwtri.nds
11,507,712 bytes
SHA-256 748764951CCCCBA36FA76D27BFCAF02DEDEB46A49C0631FEADC132FB511E5F87
```

Boundary passed on this configuration and the worktree is clean at `9af1247`, so
this is a release candidate; the public-build pin in `README.md` still names the
older ROM and should be updated in whichever kept change publishes next.

## R2-03 E49 — DO NOT GRADUATE, and it proves the flash is structurally out of reach (2026-07-29)

Built E48's fix: epochs the generic path would draw from a raw vertex colour drop
`POLY_FORMAT_LIGHT0` and emit `GFX_COLOR` from the baked dense `rgba` at all four
emit sites. Flag `NDS_R2_UNLIT_VERTEX_EPOCH`, default 0 and staying there.

Top-screen diff against the reference, E48's frame pairs:

| frame | E47 arm | **E49 arm** | bounding box |
|---|---:|---:|---|
| 910 hitlag | 4,025 | **1,436** | x 147..223 y 142..197 |
| 911 hitlag | 4,104 | **1,539** | x 146..223 y 138..201 |
| 903 control | **0** | **780** | x 153..246 y 117..234 |
| 904 control | **0** | **838** | x 153..245 y 118..223 |

**Unlighting is directionally right**: the hitlag delta falls 64% and its bounding
box collapses from the whole screen to the fighter's flash region. **But the
control regressed from pixel-perfect to 780/838**, so the predicate also claims
ordinary epochs, where the native owner's lit appearance is the accepted one and
the generic path never runs to contradict it (E48: 0 calls on frame 904).

### Why the hitlag delta stops at 1,436 instead of going to zero

`src/nds/nds_native_fighter_owner.generated.inc` declares
`static const NDSNativeDenseVertex n[541]` — **the dense vertex colours are
compile-time constants.** The hurt flash is a *runtime* rewrite of the vertex
colours the display list feeds `state->vertex_colors[]`. No emit sourced from the
baked table can reproduce it, at any indentation. E49 therefore draws the fighter
unlit in its *un-flashed* colours: right shape, wrong values.

**This is the finding, and it retires a whole family of candidate fixes.** Every
approach that reads the generated vertex data is structurally incapable of
showing the flash. The remaining options are:

1. **A per-epoch constant colour.** The reference flash is a uniform white
   silhouette, so if it is genuinely uniform it needs one colour per epoch, not
   per-vertex data — a runtime override the emit can apply without touching the
   baked table. Cheapest, and the only one that keeps E32's tick win.
2. **Keep the fallback for flash frames only.** E32 then covers the shuffle but
   not the flash, and the `FTR` excursion survives on the frames where the flash
   is live. Costs most of E32's −35,648.
3. **Approximate the flash by another mechanism** (polygon alpha, a tint pass).
   Gates on the fidelity budget and the owner's eye rather than on a pixel match.

Option 1 is the one to price, and pricing it starts by measuring whether the
flash colour is actually uniform across the fighter's vertices on a hitlag frame
— which the E48 probe can answer by recording min/max of `vertex_color` instead
of a branch count. Measure before building; this defect has now cost seven
experiments, six of which were reasoned rather than measured.

## R2-03 E48 — E32's regression MEASURED after six wrong guesses (2026-07-29)

**The flash is a raw vertex colour, and the native owner lights it.** Measured,
not reasoned: `NDS_R2_FLASH_PROBE` counts which branch of the generic colour path
draws each vertex, per presented frame, latched at one hitlag frame and one
ordinary one.

| slot | frame 911 (hitlag) | frame 904 (ordinary) |
|---|---:|---:|
| 0 material-only | 0 | 0 |
| 1 no-vertex → `RGB15(31,31,31)` | 0 | 0 |
| **2 resolved** | **273** | 0 |
| 3 lit-shade recompute | 0 | 0 |
| 4 total calls | **273** | **0** |
| 5 last material colour | **0** | 0 |
| 7 last flags | **2** | 0 |

Flags `2` is `use_vertex_color = TRUE, use_material_color = FALSE`, and slot 3 is
zero, so all 273 arrived through the `vertex_color_valid != FALSE` entry. The
generic path therefore computes

```text
PackedResolvedColor(vertex_color, material = 0, use_material = FALSE)
  -> RGB15(vertex_color >> 27, >> 19, >> 11)
```

— the vertex colour **raw**. No material, no shade, no combination.
`ndsRendererHardwarePackedValidVertexColor` never calls `LitShadeColor` on that
route: **a valid vertex colour suppresses lighting.** The native owner has no
such rule. It decides `epoch_lit` from `geometry_mode & LIGHTING` alone and runs
the hardware lighting engine, which is where the dark maroon comes from.

The reference flash is **saturated white**, not the light grey this board and the
E32 report both recorded — the source writes white vertex colours and the generic
path emits them unchanged.

`gNdsR2FlashSnapB` being **all zero** is a second result worth keeping: on
ordinary frames this function is never called at all, because the fighter is on
the native owner and the stage is on the Task 36 replay. 273 against 0 is
independent confirmation that the fallback fires on hitlag frames and nowhere
else.

### Six hypotheses, six builds, six wrong

E36 `color_modulate`; E41 the fold arithmetic, then E16's hardware lighting; E42
`USE_VERTEX`; E47 the material derivation; and E48's own stated prediction, which
was slot 1 (white) and was also wrong. Every one asked *how the material combines
with the shade*. On these frames there is no material and no shade, so the
question was mis-framed from the start, and no amount of source reading was going
to correct it — only counting the branch did.

**Standing consequence:** E45's rule ("prefer one direct bracket over any amount
of algebra") was written for tick questions and never generalised. It applies to
any question about which path the code takes. Recorded in
`TASK_STANDING_RULES.md`.

### The fix, and its real scope

The native owner must reproduce the precedence: an epoch whose vertices carry
valid vertex colours and no material emits those colours **unlit**. Under
`NDS_R2_FIGHTER_HW_LIGHT` — `override`-forced to 1 on the hwtri targets — the
per-vertex software colour loop is *compiled out*, not skipped, and E29 removed
the `packed_color` field it wrote, so there is currently no path in the native
owner that emits a vertex colour at all.

Cheapest correct shape: mark such epochs, and in the emit write `GFX_COLOR` from
the baked `sNdsNativeFighterDenseVertices[].rgba` with the polygon attribute's
light mask cleared, instead of `GFX_NORMAL`. That is *less* work than lighting,
not more — but it touches the ITCM-resident emit, which is the hottest code in
the frame, so it is an implementation task rather than a one-line correction.

`NDS_R2_MATERIAL_DYNAMIC` (E47) is refuted and must graduate or be deleted with
this fix; it currently fixes nothing.

## R2-04 E4/E5 — GRADUATED, −132,352 WORK-H P95, Boundary green (2026-07-29)

E3 said the whole 301-ID animation space does not fit the arena and that the
answer was to preload the set the match actually uses. E4 measured that set and
E5 fixed how it is delivered.

### E4 — the working set is 41 assets, 91,104 bytes

`gNdsR204AnimSeen` dumped at frames 1801..1928: `Total=230, Distinct=41,
Repeat=189` — **82.2%** repeats, up from 64.6% at frame 928, so the set is
converged, not still growing. Decoding the bitmap gives 14 Mario and 27 Fox
animations totalling **91,104 bytes: 12.5%** of the 728,064 the full space would
need. That fits with room to spare.

The list is in `sNdsR204AnimWarmList[]` and is derived from observed play, so
`gNdsR204AnimForceLoadRepeat / Total` is its own regression check: if that ratio
falls, the list has drifted from what the match uses. An asset missing from it is
a performance outcome, never a correctness one — it simply takes the on-demand
path it takes today.

With the list resident: **`gNdsR2AnimCacheMisses` 29 → 2**, and both survivors
are pre-battle loads that happen before the warm walk is armed. No gameplay
frame loads an animation.

### E5 — a prepare-at-load burst is bounded by the BGM packet, not by generosity

E4 loaded all 41 in one call at `scVSBattleStartBattle`. Boundary refused the
build, and the failing run's own audio telemetry named the mechanism:

| field | control | E4 |
|---|---:|---:|
| `gNdsAudioBgmSeamMissCount` | 0 | **1** |
| `gNdsAudioBgmErrorStopCount` | 0 | **1** |
| `gNdsAudioBgmOverrunCount` | 0 | **1** |
| `gNdsAudioBgmPlaying` | 1 | **0** |
| `gNdsAudioBgmStopCalls` | 0 | 0 |

Playback stopped without anyone calling stop. The stream is double-buffered at
8,196 bytes per packet against 44,100 bytes per second, so the main thread owns a
hard **~186 ms** budget between buffer seams; 41 back-to-back NitroFS walks plus
84 KB of cartridge reads do not fit inside it, and missing one seam kills BGM for
the rest of the match. Cache off passes 4/4, so this is causal, not a flake.

E5 arms the walk at battle start and steps **one asset per
`scVSBattleFuncUpdate`**. The countdown is far longer than the 41 frames this
needs, and a stepped frame costs exactly what the on-demand path already costs
when a fighter changes action — which demonstrably does not miss a seam.

### Result, frames 802..929

| bucket | control | E1 | E4 burst | **E5 stepped** |
|---|---:|---:|---:|---:|
| **WORK-H P95** | 1,364,992 | 1,311,360 | 1,236,096 | **1,232,640** |
| delta vs control | — | −53,632 | −128,896 | **−132,352** |
| `gNdsR2AnimCacheMisses` | — | 29 | 2 | 2 |
| `gNdsR2AnimWarmLoaded` | — | — | 39 | 39 |

E5 reproduces E4's gain (the 3,456 difference is inside the 5,000–7,000
build-placement noise floor) and is **2.5× E1's**. Read `WORK-H`, not `WORK`:
E5's raw `WORK` P95 is 1,363,840 because one frame in the window took a 333,760
`HUD` excursion (`HUD` spread 325.94), and `WORK-H = WORK − HUD` is exactly the
series that removes the instrument.

Pacing cost of stepping is visible and confined to the ramp: VBI 3-intervals
118 → 186 over the whole 929-frame run, ~41 of which are the stepped loads during
the countdown. The burst arm paid the same work as one ~1-second stall instead.

**Boundary passed** with `NDS_R2_ANIM_CACHE=1` in both the published and
tick-HUD blocks, so the flag is graduated and default-on there. That also closes
E1's separate block: the lower-screen FPS-counter assert that refused the
cache-only arm 2/2 did not fire here. It is not explained, only no longer
reproducing — see `HANDOFF.md` for the open question and the shadow probe built
for it. The `Pupupu locked-30 presentation slipped` warning is pre-existing and
appears in the control and in the failing E4 run alike.

### Standing consequence

Recorded in `TASK_STANDING_RULES.md`: **prepare-at-load work on a live scene seam
is bounded by the BGM packet duration, not by loading-time generosity.** "Loading
time is cheap" is true of a loading *screen*; it is not true of a seam where the
music is already streaming. Anything longer than one packet has to be stepped.

### Known gap, not reachable in this milestone

The cache holds `syTaskmanMalloc` pointers and is never reset, so a second match
in one boot would hand out pointers into a torn-down heap. The milestone boots
directly into one match and has no rematch flow (`PROJECT_GOAL.md`, out of
scope), so this is not reachable today. Whoever adds match restart owns clearing
`sNdsR2AnimCacheCount` at the same seam that tears the arena down.

## R2-04 E1 — animation cache BUILT, −53,696 WORK P95, BLOCKED on Boundary (2026-07-29)

E0's plan built and behaving exactly as sized. `NDS_R2_ANIM_CACHE=1` keeps each
animation's **byte-swapped, pre-fixup** payload keyed by `asset_id` and re-runs
the fixups against the real destination, which preserves `lbRelocGetForceExternHeapFile`'s
"force" semantic — pristine data restored — while removing the NitroFS walk and
the cartridge read.

Engagement, frames 801..928, matching E0's window exactly:

| counter | value |
|---|---:|
| `gNdsR2AnimCacheHits` | **53** |
| `gNdsR2AnimCacheMisses` | 29 |
| `gNdsR2AnimCacheFills` | 29 |
| `gNdsR2AnimCacheBytes` | **66,016** |
| `gNdsR2AnimCacheRejects` | **0** |

53 hits against E0's 53 predicted repeats and 29 fills against 29 distinct: every
repeat served, nothing rejected, 66 KB resident.

| bucket | control | E1 | delta |
|---|---:|---:|---:|
| **WORK P95** | 1,365,952 | **1,312,256** | **−53,696** |
| WORK-H P95 | 1,364,992 | 1,311,360 | −53,632 |
| **fallback WORK-H median** | 1,284,928 | 1,229,632 | **−55,296** |
| clean WORK-H median | 1,013,376 | 1,011,712 | −1,664 |
| WORK P50 | 1,019,776 | 1,017,728 | −2,048 |
| VBI 2: / 3: | 793 / 128 | 801 / 121 | — |

**The gain is entirely on the excursion frames** (−55,296) with clean frames
essentially unmoved (−1,664). That is the shape R2-04's gate asks for, and P95 is
the metric `PROJECT_GOAL.md` gates on.

### BLOCKED: Boundary fails with the flag on

**`battle_playable lower-screen rolling FPS counter did not sample actual
presentation cadence.`** `FPS_HUD=290,13,15,17485504` — the harness recomputes
288 from the HUD's own `frames`/`ticks` inputs and the HUD reports 290.

Reproduced deliberately rather than assumed: **two runs with the flag on fail,
one clean control run with it off passes** (`Boundary verification profile
passed`). A third run, the control taken through the harness script directly
rather than `verify-all.ps1`, failed on an unrelated blank-capture
(`0/49152 dominant-green pixels`) and is a flake, not evidence — noted so the
next reader does not count it as a second control.

The likely mechanism is that the assert is an internal-consistency check between
the HUD's rolling value and an instantaneous recomputation, and this cut makes
the frame rate **non-stationary** — the first 29 loads are slow misses, then the
match speeds up — so a rolling average legitimately disagrees with a spot
recompute. That would make the assert an artifact rather than corruption. **It is
not graduated on that theory.** The flag stays default 0 until someone shows
which of the two is wrong; a verifier failure is a failure.

### E2 — causation is firm and BOTH explanations are refuted

**Causation, measured rather than assumed:** flag on **2 of 2 runs fail**; flag
off **3 of 3 runs pass** (`Boundary verification profile passed`). The one control
that failed differently — a blank capture, `0/49152 dominant-green pixels`, from
running the harness script directly instead of through `verify-all.ps1` — is a
flake and is **not** counted as a control.

**Refuted #1 — "non-stationary rate".** `nds_platform.c:2236-2239` writes all four
HUD fields adjacently from locals, and `fps_x10` is computed from exactly the
`elapsed_frames`/`elapsed_ticks` published beside it. Only two writers exist (that
group and the reset at :2145-2148, also a group). The harness reads all four in
one GDB `printf` at a breakpoint, so the read is atomic too. The assert therefore
holds across *any* cadence change, and a rolling-versus-spot mismatch cannot
happen. This theory was wrong.

**Refuted #2 — "the harness `BUS_CLOCK` constant is stale".** `NDS_R204_FPSHUD_SHADOW`
publishes a shadow of the same locals in the same breath. Sampled state:

| | primary | shadow |
|---|---:|---:|
| fps x10 | 265 | 265 |
| frame window | 14 | 14 |
| tick window | 17,721,728 | 17,721,728 |

`gNdsR204FpsHudShadowBusClock = 33,513,982`, identical to the harness constant,
and recomputing gives exactly 265. **The publish path is self-consistent and the
constant is right.**

So the observed `FPS_HUD=290,13,15,17485504` — where the recompute is 288 — is an
**intermittent** state that the probe did not catch, and it is not explained by a
wrong constant, a non-atomic publish, or a rate change.

### What to do next

1. **Read the shadow where the verifier reads the primary.** The probe above ran
   under `sample-tick-hud-buckets.ps1`, which stops at its own breakpoints and
   sampled a healthy sample; the anomaly belongs to the verifier's stop. Add the
   four shadow globals to the `FPS_HUD` printf at
   `verify-battle-mariofox-gcrunall-loop-harness.ps1:2100` and re-run with the
   flag on. If shadow and primary disagree *there*, something rewrites the
   primary between publish and that stop, and the writer is the bug.
2. **Independently, do E0's other half regardless: preload the working set at
   match start.** It is what R2-04 actually specifies, it takes the removable
   share from 64.6% to 100% rather than leaving 29 misses, and whatever this
   assert is reacting to, a match whose animation set never loads mid-gameplay
   cannot trigger it. **Read E3 below first — the obvious form of it does not
   fit.**

## R2-04 E3 — "preload everything" does not fit, measured (2026-07-29)

Before building the preload, the budget. The animation assets are individual
files under `nitro:/reloc/reloc_animations/`
(`nds_reloc_assets.c:130`/`:175` synthesise the paths), so their sizes are
readable off disk without running anything:

| set | files | bytes | avg |
|---|---:|---:|---:|
| `FTMarioAnim*` | 143 | 360,320 | 2,519 |
| `FTFoxAnim*` | 158 | 367,744 | 2,327 |
| **all** | **301** | **728,064 (711 KB)** | 2,419 |

**711 KB is not affordable.** `MEMARENA` reports a ~1.35 MB taskman arena and
`MEMRELOC` already accounts for 681,632 bytes of reloc data. Registering all 301
would also overrun `NDS_RELOC_LOADED_FILE_CAPACITY` (96) three times over. So the
literal reading of the phase bullet — *all* animation streams resident — is not
available on this hardware budget, and anyone starting from that sentence will
build something that cannot fit.

The measured working set is **29 assets / 66,016 bytes**, which fits trivially.
That gap is the whole design question, and it has two honest answers:

1. **Generated warm list.** Emit the match's actual animation set as a table and
   preload exactly it at `scVSBattleStartBattle` (`src/import/battleship_scvsbattle.c:133`,
   beside `ndsRendererHardwarePrepareBattleStaticTextures` and
   `ndsIFCommonNativeOamPrepareClouds` — the existing prepare-at-load seam, and
   port-side so it is editable). `PROJECT_GOAL.md` explicitly endorses
   compile-time asset conversion and heavy loading-time preparation. A miss still
   falls back to the on-demand load, so an incomplete list is a performance
   outcome, never a correctness one. Risk: the list is derived from observed play
   and a gameplay change silently drops coverage, so it needs the E0 counters
   kept as its regression check (`repeat/total` should stay at 100%).
2. **Budget-bounded eager fill.** Keep the cache lazy but give it a byte budget
   near 128–192 KB and let it hold whatever the match touches. That is what E1
   already does, minus the match-start warm, and it is why E1 works at all.

**Do not start by preloading the 301.** The cache's failure paths degrade safely,
so trying it would not corrupt anything — it would just reject nearly everything
and read as a null, and the reason would be this table.

## R2-04 E0 — the phase is SIZED and its gate is reachable (2026-07-29)

R2-03's remaining lever is unattributed (E45/E46 left ~110 ticks per delta with
no owner), while **R2-04 is untouched and its gate is the one currently missed**:
"SRC-class P95 excursions gone from the histogram", plus "absorbs Task 75: all
animation streams for the match prepared at load; no first-use loading during
gameplay". E35 already showed 25 of 26 over-gate frames are SRC excursions. This
sizes the phase.

### The excursions are animation loads, and they are separable

Tick-HUD census with `NDS_TASK68_FALLBACK_CENSUS=1 NDS_TASK75_LOAD_CENSUS=1`,
128 samples at two windows:

| window | fallback frames | fallback WORK-H median | clean WORK-H median | clean P95 |
|---|---:|---:|---:|---:|
| 439..566 | 13 of 128 | 1,329,280 | 983,936 | 1,408,128 |
| 801..928 | 17 of 128 | 1,284,928 | 1,013,376 | **1,117,248** |

Fallback reasons in the second window: **`animLoad` 19, `animLock` 5, every other
reason 0.** `animLoad` is `lbRelocGetForceExternHeapFile` re-reading a fighter
animation off the cartridge inside the frame that needs the move — NitroFS open,
cartridge read, word byte-swap, then internal/external pointer fixups, per call.

**In the 801..928 window the clean P95 is 1,117,248, under the 1,120,000 gate.**
So the phase's gate is not a distant target: it is what the histogram already
reads once the animation loads are removed.

### Why the existing counter said the opportunity was zero, and why that was wrong

`animResident` reads **0** in both windows. That counter asks whether the
*destination heap* already holds the asset, and the destination is caller-owned
and reused, so it almost never does. It refutes a destination-side residency
check — not the opportunity. The right question is how often the **same
animation** is force-loaded more than once, which a 128-frame (~4 s) window
cannot answer because Mario returns to Wait/Walk/Jump across the whole minute.

New lab counters (bitmap over the 301 Mario+Fox animation IDs), cumulative to
frame 928:

| counter | value |
|---|---:|
| `gNdsR204AnimForceLoadTotal` | **82** |
| `gNdsR204AnimForceLoadDistinct` | **29** |
| `gNdsR204AnimForceLoadRepeat` | **53 (64.6%)** |

**Two facts fall out, and they decide the implementation.**

1. **64.6% of the cartridge reads are repeats.** An asset-keyed cache removes
   them; the destination-keyed check that read 0 never could.
2. **The match's animation working set is 29, not 301.** The whole ID space is
   301 (`MARIO_ANIM_WAIT` 0x1f3..`MARIO_ANIM_FIRE_FLOWER_AIR` 0x281 = 143, plus
   `FOX_ANIM_FIRST` 0x282..`FOX_ANIM_LAST` 0x31f = 158), and
   `NDS_RELOC_LOADED_FILE_CAPACITY` is **96** — so preloading *everything* is
   impossible and was the obvious wrong turn. Preloading the working set is not:
   29 fits with room to spare, and it takes the removable share from 64.6% to
   **100%**, which is literally R2-04's "prepared at load, no first-use loading
   during gameplay".

29 is measured to frame 928, roughly two thirds through the 3,600-tick match, so
budget for growth — but the headroom against 96 is large enough that this is a
sizing note, not a risk.

### Implementation note for whoever builds it

Cache the **byte-swapped, pre-fixup** payload. `ndsRelocApplyInternalPointerFixups`
writes absolute pointers derived from `loaded->data`, so a fixed-up image is
position-dependent and cannot be memcpy'd to a different heap. Copy the swapped
image in, then re-run the fixups — that removes the NitroFS walk and the
cartridge read, which is the part Task 71 profiled, and keeps pointer correctness
by construction.

## R2-03 E46 — the delta path into ITCM: GRADUATED, −12,416 WORK P50 (2026-07-29)

E45 left ~186 ticks per delta application unexplained after eliminating the tile
republish (~23, E44), the invalidation macro (one store) and the span entry (~33,
E45). The census ELF names the remaining candidate outright:

| symbol | address | where |
|---|---|---|
| `ndsRendererNativeApplyStateDelta` | `0x01ff9934` | ITCM |
| `ndsRendererNativeApplyStateSpan` | `0x02003a14` | main RAM |
| `ndsRendererSyncTextureTile` | `0x02003ae8` | main RAM |
| `ndsRendererRecordSetTile` | `0x0200d4e8` | main RAM |

The switch was already ITCM-resident; the loop that calls it and every helper it
dispatches to were not. So all 134.5 before-span applications a frame left
zero-wait ITCM for icache-served main RAM and came back. `.itcm` had **2,912
bytes free** and the whole path is ~1,088.

`NDS_R2_DELTA_PATH_ITCM=1`, placement and nothing else:

| arm | before-span | after-span | total |
|---|---:|---:|---:|
| lean baseline | 26,944.3 | 13,703.7 | 40,648.0 |
| **delta path in ITCM** | **24,494.8** | **13,025.2** | **37,520.0** |
| | −2,449.5 | −678.5 | **−3,128.0** |

Delta counts identical across arms (134.5 / 47.9 / 46.4 epochs) — the same work,
fetched faster. Per delta the two spans agree at **−18.2 and −14.2 ticks**, two
independent populations, which is the cross-check that separates a real effect
from layout luck on a change that is *itself* a relocation. `.itcm` goes 29,856 →
30,872, **1,896 bytes still free**.

### Frame level: the bracket understated it 4x

−3,128 is below the 5,000–7,000 whole-frame placement noise floor, so the bracket
alone could not license a frame-level claim. The tick-HUD A/B over 128 frames
(439..566, `NDS_TICK_HUD_DRAW=0`, both arms built from the same tree):

| bucket | control | E46 | delta |
|---|---:|---:|---:|
| **FTR P50** | 404,672 | 392,640 | **−12,032** |
| **WORK P50** | 1,010,240 | 997,824 | **−12,416** |
| WORK-H P50 | 1,006,848 | 996,480 | −10,368 |
| FTR P95 | 913,152 | 904,384 | −8,768 |
| STG P50 *(untouched)* | 173,312 | 174,080 | +768 |
| SRC P50 *(untouched)* | 327,360 | 326,144 | −1,216 |

VBlank histogram `2:472 3:87 4:4 5+:2` → `2:476 3:85 4:3 5+:2`.

**The gain is 4x the state-span bracket** because the bracket only ever saw the
two spans. `ndsRendererNativeApplyMaterial` (27.7/frame) calls
`RecordSetTile`/`RecordSetImage`/`RecordLoadTlut`/`RecordSetTileSize`, and the
texture prepare calls `SyncTextureTile` once per run (46.4/frame) — all of them
relocated too. Per E40's rule the untouched buckets bound the noise at ±1,200, and
FTR moved 10x that.

`ALL` P50 is unchanged at 1,119,808 in both arms. That is expected and is **not** a
refutation: `ALL` is VBlank-quantized at 560,190/VBlank, and reading it as flat is
what previously killed four good levers. `WAIT` rises by roughly what `WORK` sheds,
which is what a still-VBlank-bound frame looks like when real work is removed.

**GRADUATED.** `override NDS_R2_DELTA_PATH_ITCM := 1` in both the published and
tick-HUD Makefile blocks; **Boundary green**. No visual approval is owed — unlike
E16 and E32 this changes no arithmetic, only where the instructions live, so the
output is identical by construction.

**It does not explain the 186.** Instruction fetch is confirmed as a real
component and it is ~16 ticks of ~186, under 10%. Four mechanisms are now
measured and none is the driver: republish ~23, entry ~33, fetch ~16,
invalidation ~1. Roughly 110 ticks per application remain unattributed, and that
residue — not any of these — is what E26's fold has to be sized against. The next
instrument is R2-00a's stall attributor rather than another guess.

## R2-03 E44/E45 — E26 re-scoped, and two wrong answers on the way (2026-07-29)

With E43's corrected 26,944 in hand, the question was where inside the
before-span it goes. Two candidates were built and measured; both are refuted,
and the second refutes an arithmetic shortcut that would otherwise have
mis-scoped E26 by 3x.

### E44 — deferring the tile republish: BUILT, worse, reverted

`ndsRendererSyncTextureTile` republishes 19 fields from the active tile and runs
on every SETTILE/TEXTURE/SETTILESIZE. Statically that is **43 of the before-span's
140 applications, spread over only 17 spans** — so 26 republishes are overwritten
by a later one in the same span before anything reads them. Nothing inside a span
reads what it publishes (the `Record*` writers read `texture_tiles[]` and the
`texture_*` scalars, never `texture_render_tile_*`), and the texture prepare calls
Sync again at `nds_renderer.c:12176` before it reads, so deferring to one call per
span is exact.

It engaged exactly as predicted — **68.2 deferred, 39.4 flushed, 28.8 republishes
actually removed per frame** — and lost:

| arm | before-span | after-span | total |
|---|---:|---:|---:|
| lean baseline | 26,944.3 | 13,703.7 | 40,648.0 |
| + deferred Sync | 26,286.9 | 14,852.7 | 41,139.6 |

Before −657.4, after **+1,149.0**, net **+491.6 worse**. Reverted.

**The number worth keeping: `SyncTextureTile` costs ~23 ticks per call**
(657.4/28.8). The republish is **3.7%** of the before-span. It is not the cost,
and the two `stats` fields the deferral needed cost more in the after-span than
they saved in the before-span.

### E45 — the per-span entry is not the cost either, and the fit that said so was unsound

E44's result was read as evidence that the cost is fixed per span rather than per
delta, and a two-equation fit over the two spans
(`A*spans + B*deltas = ticks`) solved to **A = 392.3 fixed, B = 63.0 per delta**,
putting **68.5% of the before-span in per-span entry** and capping E26 at 8,478.
The only substantial thing on entry is `ndsRendererNativeSourceBoundary`'s
`ndsRendererHardwareEndBatch()`, so that was bracketed directly:

- **78.3 boundary calls/frame** (the static estimate was 74.3 — the model's span
  count was right)
- 80.4 ticks per call *as measured*, but the bracket inflated the two span totals
  by 7,426/frame across those calls, so ~47 of the 80.4 is the timer read itself
- **real cost ~33 ticks/call, ~2,585/frame — 14% of the 18,466 the fit claimed**

**The fit was unsound and this is the lesson.** Two equations in two unknowns has
zero degrees of freedom: it always solves, and it cannot be checked. It forced a
single per-delta coefficient onto two populations that genuinely differ — the
after-span's deltas cost more because `ApplyMaterial` runs between the spans and
dirties what they touch — and the only place that difference could go was the
"fixed" term. E43 had just earned a rule about measuring instead of assuming; this
is the same mistake wearing arithmetic.

**Corrected picture of the before-span (26,944/frame, 134.5 deltas, ~47 spans):**
entry ~1,900, **applications ~25,000, i.e. ~186 ticks per delta.** It is per-delta
after all.

### What this leaves E26

E26's target is back to roughly the whole before-span, **~25,000/frame**, because
the fold removes the applications themselves. What 186 ticks per delta *is* remains
open — it is far more than the switch and stores account for, and the leading
untested candidate is instruction-side: `ApplyStateDelta` lives in the fighter's
code section while the `Record*` helpers are generic renderer code, so every
application may cross into cold code. The repo emulator models icache, so this is
measurable. **Measure that before building the fold** — and note ITCM has only
1,024 bytes free, so if it is icache the repair is not simply "move it".

Two facts already banked for the fold itself: **LOAD_BLOCK appears only twice in
the entire before-span program**, so the `texture_loads[]` history ring — the
hardest thing to reproduce in a bulk install — touches at most 2 of 47 spans and
those two can keep the replay; and **COMBINE alone is 30.7% of applications**
(43 of 140), which is 4 stores and a counter, the cheapest possible fold target.

## R2-03 E43 — the replay bracket was pricing its own instrument (2026-07-29)

E38 sized E26's before-span at **33,708/frame** and that is the number the phase
has been planning against. It is 20% wrong, and the reason was already written
down in `docs/HANDOFF.md` as an unquantified caution: `ApplyStateDelta` opens
with a per-delta census block — E20's identical-operand arrays and E25c's effect
histogram — and `ndsRendererNativeApplyStateSpan`'s loop carries a second one,
E20's frame-stamp check. Both sit **inside** E38's bracket and run 134.5 times a
frame on the before-span alone.

`NDS_R2_SPAN_LEAN_TIMING=1` keeps E38's brackets and compiles both per-delta
blocks out. Same ROM target, same window (tick-HUD, frames 439..919):

| arm | before-span | after-span | total replay |
|---|---:|---:|---:|
| census in bracket (E38) | 33,707.6 | 16,243.3 | 49,951 |
| **census out of bracket** | **26,944.3** | **13,703.7** | **40,648** |
| instrument | 6,763.3 | 2,539.6 | 9,303 |

Delta counts are identical across arms (134.5 / 47.9 / 46.4 epochs), so the arms
are doing the same work. The instrument prices out at **50.3 ticks/delta on the
before-span and 53.0 on the after-span** — measured independently and agreeing to
5%, which is the cross-check that this is the instrument and not a code-placement
artefact.

**Consequences.** E26's before-span target is **26,944, not 33,708**. The replay
is **40,648, not 49,951**. The real per-delta cost is 200.3 (before) and 286.1
(after), not 250.6 and 339.1. E26 is still the largest single mechanism left in
R2-03 and still worth building — it is sized 20% smaller, not refuted.

### Standing rule this earned

A timing bracket must not enclose its own instrument. These two census blocks
predate the brackets and were correct when they were only counting; E38 wrapped a
timer around them without noticing. Any bracket around a per-item loop should be
built with the per-item census compiled out, and if both are wanted they are two
arms, not one build. Recorded in `docs/optimization/TASK_STANDING_RULES.md`.

## R2-03 E42 — E32's dark fighter is NOT `USE_VERTEX`: BUILT, REFUTED (2026-07-29)

E41 left the E32 regression traced to the native owner's material-colour path and
owed a diff of it against the generic path. That diff has a real finding in it:
`ndsRendererHardwarePackedVertexColor` has a branch the native owner does not.
With `USE_MATERIAL` set and `USE_VERTEX` **clear** the generic path applies no
lighting at all and returns the material colour *alone* — it replaces the shade.
`ndsRendererNativeShadeProductionActions` reads only `USE_MATERIAL` out of
`policy->vertex_flags` and always multiplies the material *into* a lit shade, in
both the E16 hardware fold and the software loop. A hurt-flash material on such a
run would be tinted dark instead of replacing the body colour, which is exactly
the reported symptom, and it explained E41's oddest result — the software arm
being *further* from the reference rather than corroborating it.

**It is unreachable.** `sNdsNativeFighterDirectPolicies[4]` in
`src/nds/nds_native_fighter_owner.generated.inc:487` — every one of the four
families sets `NDS_RENDERER_VERTEX_CONTEXT_USE_VERTEX`. The branch cannot execute
for any fighter epoch, so it cannot be the divergence. Built and measured anyway
rather than argued from the table, because the table is generated:

| frame | E32 candidate vs reference | E42 candidate vs reference |
|---|---:|---:|
| 480 (hitlag) | 1,826 | **1,786** |
| 481 (hitlag) | 1,536 | **1,496** |

The 40-pixel move is not the fighter. E42-candidate against E32-candidate differs
by **33 pixels in a 7x9 box at x 47..53, y 11..19** — the bottom-screen FPS
readout, which differs between any two runs. Reverted; the file is byte-identical
to HEAD. Do not re-derive this: **the flat-colour branch is dead code for the
fighter and adding it back is speculative.**

### What this leaves for whoever returns to E32

Four candidate mechanisms have now been eliminated by implementation, not by
argument: `color_modulate`'s affine lerp (E36), the fold's own arithmetic and
hardware-vs-software lighting (E41), and `USE_VERTEX` (E42). What survives is the
**lit-shade input itself**: the generic path takes an early exit on
`vertex_color_valid` and uses a baked `vertex_color` *without lighting it*, where
the native owner always re-lights `sNdsNativeFighterDenseVertices[].rgba`. That is
the only remaining structural difference between the two colour paths. Measure it
before building anything — dump both paths' inputs on frame 480 rather than
reasoning about the arithmetic, which is how E36, E41 and E42 were each spent.

### Harness defect this exposed

`capture-cut-g-exact-frames.ps1:257` asserts GO!-overlay state — recognized
calls, draw calls, SObj and OAM object counts, commit calls, `FrameIdle == 0` —
on **every** exact-frame capture. At frame 480 the match is 568 source ticks in,
the GO overlay is long gone, and the native OAM block is legitimately idle, so
the assert throws for both arms. The screenshots are written first (the file-
existence assert is *after* it at line 279), which is why E32 got its PNGs and
this defect went unrecorded. Every future mid-match exact-frame capture pays a
full emulator boot and then dies. **Fixed**: the always-true invariants (native
OAM owns the overlay, no fallback, no hot convert, no runtime upload, prepare
succeeded) are asserted every frame; the GO census is asserted only when
`gNdsIFCommonNativeOamFrameIdle == 0`, and when idle the opposite is asserted —
zero recognized/draw/commit/cloud-draw calls — so an overlay that goes idle while
still drawing is still caught. The pair check now also rejects a pair straddling
the idle transition. Verified both ways: frames 480/481, which threw before,
capture clean, and probe frames 200/201 (`time_passed = 8`) take the presenting
branch.

**Still open, and it is not mine to re-pin.** The GO-presenting constants are
themselves stale — at frame 200 this build reports 3 OAM objects against an
expected 23, 608 prepare-palette bytes against 32, 3 cloud textures against 2,
57,344 cloud-texture bytes against 65,536, and 10 cloud draws against 2. So this
assert has not passed on *any* frame for some time, which also means
`verify-battle-playable-realtime-harness.ps1:21`'s `FastCaptureFirstFrame = 438`
has been dead. The constants postdate `4f4528f` (countdown GO and source-alpha
flare fidelity) and `6da286e` (crisp IFCommon alpha coverage). Re-pinning them by
observation would bless whatever the current state happens to be as correct,
which is a countdown-GO fidelity judgement and belongs to that owner, not to a
fighter-shade experiment. **Action for that owner: re-derive the GO census
constants, or delete them if the invariants above are the real contract.**

## R2-03 E40 — state tables to DTCM: BUILT, NULL, reverted (2026-07-29)

E39 established the replay's cost is memory, not logic: 2.9 genuinely distinct
writes per epoch at ~250 ticks each. E29 had won **26,816** by moving the fighter
geometry tables into DTCM, so the same lever was applied to the replay's two
tables — `sNdsNativeFighterStateDeltas` (840 B) and `sNdsNativeFighterStateSequence`
(196 B), together ~1 KB against 3,432 free. Bit-exact by construction: same data,
different address.

Built, gated and measured. `check-task20-dtcm-layout.ps1` passes with all four
fighter tables resident and `__irq_table` still 32-byte aligned at `0x02ff2600`.

| bucket | Δ P50 | Δ P95 |
|---|---:|---:|
| `FTR` (the target) | −4,544 | −384 |
| `WORK` | −3,776 | +1,280 |
| **`OTHR`** (cannot be affected) | **+5,568** | −64 |
| **`SRC`** (cannot be affected) | +128 | **−3,584** |

**NULL, and the pair proves it on its own.** `OTHR` and `SRC` cannot depend on
where the fighter's delta tables live, yet they moved ±3,500–5,568. That is the
build-placement noise, measured *inside the same comparison*, and −3,776 is
indistinguishable from it.

**Reverted rather than kept, and the reason is DTCM scarcity, not process.**
AGENTS.md says to keep every repeatable correctness-preserving gain, but
"repeatable" is exactly what one pair inside its own noise cannot establish — and
the 1,036 bytes are not free. E29 already recorded that
`sNdsNativeFighterPackedCorners` needs 3,756 and "does not fit safely"; spending
a quarter of the remaining headroom on an unmeasurable gain forecloses a better
tenant.

**Why it did not repeat E29's win:** E29 moved 8,656 bytes touched **1,878 times
a frame** in random order, which could not fit the 4 KB dcache. E40 moves 1,036
bytes touched **182 times** — and the 840-byte delta table already fitted
comfortably. Same lever, an order of magnitude less to win. **Size a placement
move by accesses per frame against cache capacity, not by "this worked before".**

### Standing rule this earned

**An A/B on a placement change must report an untouched bucket.** The 5,000–7,000
floor is a remembered constant; `OTHR`/`SRC` are a *measured* bound for the exact
pair in hand, and they cost nothing to read because the sampler already collects
them. Had only `FTR` been reported, −4,544 would have looked like a modest KEEP.

## R2-03 E39 — operand elision BUILT and REFUTED on engagement (2026-07-29)

Built the cheapest version of E26's idea and killed it with its own counter, for
about one build's cost. **Reverted; do not rebuild it.**

The elision is exact, by the argument E20 already wrote into
`ndsRendererNativeApplyStateDelta`: every case writes `stats` purely from
`w0`/`w1`, so identical operands to the previous application of the same effect
mean identical writes — and if the state did not change, the texture prepare
built from it is still valid, which kills the invalidation E25b identified as the
real cost. GEOMETRY excluded (the one read-modify-write case, and eliding it
would also skip `geometry_command_count`); validity cleared on every material
application and per owner execute.

**Engagement: 7,898 elided against 99,179 applied — 7.4%.** At E38's 251
ticks/delta that is ~3,700 ticks/frame, *below* the 5,000–7,000 build-placement
noise floor, so an A/B would have returned noise and a KEEP would have been
unearned. The counter answered it without one.

**The mechanism is structural, and it is worth carrying into E26.** The
before-span averages only **2.9 deltas per epoch** (134.5 over 46.4), mostly of
different effects, and `ApplyMaterial` resets any cross-epoch cache on 28 of
those 46.4 epochs. There is almost nothing for an operand cache to hit. So the
33,708 is **not** redundant work — it is ~3 genuinely distinct writes per epoch
paying ~250 ticks each in dispatch, call and invalidation overhead. E26 must
therefore replace the *dispatch*, not deduplicate the *writes*: one install per
epoch instead of three calls.

### Hazard found the hard way, and it is not specific to this cut

The first build cached across owners and put **every frame in the 5+ VBlank
bucket**. `ndsRendererNativeApplyStateDelta` is **shared** — the stage owner and
the hierarchy modes reach it through their own spans — and the counter proved it
immediately: **850 applications a frame against the fighter's 182.4**. Anything
memoising in that function must be armed around the fighter production spans
specifically, or it silently elides the stage's state writes against operands
cached from the fighter. Confining it restored the histogram to 2:445 3:111 4:8
5+:2, against the control's 2:442 3:115 4:6 5+:3.

**Read that as a general rule: before memoising in a shared renderer helper,
count its calls from the owner you think you are optimising.** A 4.7x
discrepancy between "the deltas I sized" and "the deltas that arrive" is the
whole bug, and one engagement counter exposes it.

## R2-03 E38 — E26 scoped: fold the BEFORE-span, and only that (2026-07-29)

E26 is R2-03's own bullet in the switch plan ("per-epoch generated submit
consuming only baked facts — no `PrepareProductionRun` policy re-checks, no
traversal-state/stats dependency"). E34-b settled that the material must stay
live. What was never measured is how the replay splits **across** that material,
which is what decides how much of E26 is tractable: the before-span is pure
prologue, while folding the after-span means re-applying static writes *over*
live material writes and getting the ordering exactly right.

Timed separately (`NDS_TASK91_DRAW_PHASE_CENSUS`, frames 439..919, 46.4 epochs a
frame):

| span | ticks/frame | deltas/frame | ticks/delta |
|---|---:|---:|---:|
| **before** | **33,707.6** | 134.5 | 250.6 |
| after | 16,243.3 | 47.9 | 339.1 |
| **total replay** | **49,951** | 182.4 | |

> **Superseded in part by E43.** Every tick figure in this table is inflated by
> the per-delta census sitting inside the bracket: the before-span is **26,944**,
> the after-span **13,704**, the replay **40,648**. The *split* below — 67.5% /
> 73.7% — survives, because both arms move together. Use 26,944 as E26's target.

**The before-span is 67.5% of the cost and 73.7% of the deltas — and it is the
half with no ordering problem.** So E26 reduces to: bake the resolved
post-before-span state per epoch, install it, leave `ApplyMaterial` and the
after-span exactly as they are. Target **26,944/frame** (E43-corrected) for a
change with no material interaction to reason about.

Install every field *except* `prim_color`/`env_color` and their companions: E34-b
showed those are the only state that varies at runtime, so leaving whatever the
live path put there is both correct and what keeps the material live.

**Also note the replay is now 49,951, not the 65,026 E25b sized it at.** E12, E28
and E29 have shipped since. Size E26 against 33,708, not against a share of
65,026 — and re-measure before claiming a share of any older total.

After-span deltas cost 339 ticks each against the before-span's 251, which is
consistent with the material application between them dirtying what the
after-span then re-touches.

## R2-03 E32 — DO NOT GRADUATE: visual regression found, mechanism named (2026-07-29)

**The visual gate was answered by measurement, and the answer is no.** Flag stays
default 0. Details in `ClaudeOpus5_R203_E32_ShuffleFold_20260729.md`.

Frame-locked captures of the same presented frames from both arms
(`capture-melonds.ps1 -ExactFirstFrame N -ExactSecondFrame N+1 -FoxCpuMode 1
-SoftwareRenderer`, each arm built to its own `NDS_OUTPUT_ROOT`):

| frame | differing pixels | share |
|---|---:|---:|
| 480 / 481 (hitlag) | 1,826 / 1,536 | 0.661% / 0.556% |
| 510 / 511 (control) | 188 / 188 | 0.068% |

The control makes the comparison sound: on non-hitlag frames the arms are
pixel-identical apart from the bottom-screen `FPS`/`UP` readout, which must
differ because the arms run at different speeds. The `CUTG_EXACT` rows agree byte
for byte at every frame, state hash included — E32 is render-side only.

**On hitlag frames the struck fighter renders dark maroon instead of light grey**
(`artifacts/visibility/e32-compare-480.png`, `e32-compare-481.png`). That is the
hurt flash, not the shake: per E34, `prim_color`/`env_color` are the only
per-epoch state that varies at runtime, and Task 39's hurt flash is what varies
them. The reference arm falls back to the generic path during animlock (E31) and
gets its flash handling; the candidate stays on the native owner and applies the
material differently.

**E32 is really two changes and only one was measured.** It was framed as folding
the shuffle into the world matrix; its actual effect is *not falling back during
animlock*, and the fallback was also hiding a material seam. Fix the native
owner's hurt-flash colour to match the generic path, then re-run these four
captures — the tick win (`FTR` P95 913,920 → 412,992) is real and worth returning
for.

**E36, the first hypothesis, is REFUTED — do not retry it.** The damage flash
looked like the mechanism: `ndsRendererHardwareModulatePackedColor` is an affine
lerp `L(x)=x*k+c`, the engine computes `ambient + diffuse*dot`, and E16 writes
`L()` into both registers, so `L(A)+L(D)*d = L(A+D*d) + c*d`. Built the exact
correction (ambient full lerp, diffuse scale only) and captured: **1,826 -> 1,827
pixels.** `color_modulate`'s alpha is zero on these frames, and the *material*
path is multiplicative anyway (`ndsRendererHardwareScaleMaterialChannel5`), which
distributes over `ambient + diffuse*dot` exactly — so `prim_color` is carried
correctly. The change was **reverted rather than shipped unproven**; the
arithmetic still predicts a real defect wherever alpha is non-zero, but nothing
in 439..566 exercises it.

**E41 ISOLATED THE CAUSE: the native owner's material-colour path.** Three arms
of the same presented frame 481 (`artifacts/visibility/e41-three-way-481.png`),
built by flipping the `HW_LIGHT` override for one diagnostic build and restoring
it immediately:

| arm | `HW_LIGHT` | fold | Fox renders | px vs reference |
|---|---|---|---|---:|
| reference | 1 | off (generic) | **light grey** | — |
| candidate | 1 | on | **dark maroon** | 1,536 |
| E41 | **0** | on | **dark maroon** | 3,027 |

Software shading did not restore it, it moved *further* away — so E16's hardware
lighting is refuted. Fox is in the same position in all three, so the fold's
arithmetic is refuted too. What remains: control frames 510/511 are
pixel-identical between arms, so the native owner draws Fox correctly on ordinary
frames; the only thing hitlag adds is the hurt flash writing `input->materials[]`
(E34's 108 runtime changes), and E36 already excluded `color_modulate`. So the
flash arrives through `stats->prim_color` and the native owner tints with it
where the generic path brightens.

**The fix is to diff `ndsRendererR2MaterialChannel` →
`ndsRendererHardwareScaleMaterialChannel5` (native, multiplicative into
diffuse/ambient) against `ndsRendererHardwarePackedVertexColor` (generic,
combines material with the vertex colour and its validity mask). This is a
rendering-correctness fix owed regardless of E32** — E32 only decides whether
those frames are ever drawn by the native owner.

**Superseded below:** the source reading that pointed at E16. The fold's
arithmetic looks right: `dFTDisplayMainShufflePositions` holds ±50/±100 in source
world units and E32 copies them through the port's documented 4096 conversion
unchanged, and `ftdisplaymain.c:1205` applies them with
`G_MTX_PUSH | G_MTX_MUL | G_MTX_MODELVIEW` between view and model — a world-axis
translation, which is what adding into the world matrix's translation row
reproduces. One part is still open: the source has **two** application sites
(`ftdisplaymain.c:1205` for the body, `lbcommon.c:1629` for an *attach* DObj), so
E32's per-binding loop may over-apply.

**The leading explanation is now E16, not E32.** Hitlag frames are the only
frames where the native owner and the generic path draw the same fighter, so
E16's hardware lighting has never been compared against the software shade on
identical input — E32 is merely what made that comparison happen. E16 was
graduated on frames that could not expose it.

**Obstacle, found the hard way:** `NDS_R2_FIGHTER_HW_LIGHT` is `override`-forced
to 1 for the hwtri targets (`Makefile:543`, `:657`), so a command-line
`NDS_R2_FIGHTER_HW_LIGHT=0` is **silently ignored** — the build succeeds and the
config header still reads 1. Always check the built `nds_build_config.h` rather
than trusting the command line.

**Harness note:** exact-frame capture is *not* gated to the Cut G window by frame
number, only by its assertion set — the captures land, then the GO-text
assertions throw. `capture-melonds.ps1` also passed its own `-FoxCpuMode -1`
"unset" sentinel into a callee validating `0..1`, so the default invocation died
after a full emulator boot; **fixed in this commit** by normalising the sentinel
up front.

## R2-03 E35 — the gate is owned by the SIMULATION, not the renderer (2026-07-29)

**Highest-value row on the board, and it is no longer a fighter row.** Full
report: `docs/optimization/ClaudeOpus5_R203_E35_SrcExcursion_20260729.md`.

`WORK-H` P50 is **1,011,200**, inside the 1,120,000 gate. Only P95 misses, so the
question is what an *expensive* frame runs. Three results, from a 128-frame ring
dump (frames 439..566) carrying the Task 75 per-frame load counter, plus two
`NDS_TICK_HUD_DRAW=0` per-PC census windows:

**1. Loading was oversized by half.** Load-free P95 1,419,264 against 1,468,800
over all frames, so eliminating on-demand loading is worth **~49,536**, not the
~103,488 the board has carried since Task 75 E0. Still real, still not the gate.

**2. E32 does not land the gate.** Applying its measured `FTR` cap frame by frame
across all 128: 34/128 over gate -> **26/128**, P95 1,468,800 -> **1,377,408**.
Reading only the worst fourteen frames suggested it might land the gate outright;
it does not. **Rank the whole distribution, never the visible top of it** — a P95
is a position in a sorted list, decided by the frames just below the ones that
catch the eye.

**3. 25 of the 26 remaining over-gate frames are `SRC` excursions**, 13 of them
load-free and arriving in consecutive runs (452–453, 475–477, 517–521, 542–543).
`SRC` is `scVSBattleFuncUpdate` x2 — the SSB64 simulation.

Profiling 517–521 against a matched control at 508–512, per frame, excluding
`armWaitForIrq` (a consequence: three VBlanks instead of two):

| block | ticks/frame |
|---|---:|
| **softfloat** | **283,072** |
| collision (`gmCollision*`, `ndsStageMP*Sweep*`) — four functions enter from zero | 75,088 |
| a third owner drawing — all zero in control | 66,498 |
| overlay 2 (`func_ovl2_800ED490`, `func_ovl2_800EDBA4`) — zero in control | 24,773 |

`MISC` confirms the draw half independently: 47,424 -> 125,184–157,888.

**The float is attributed, and it is NOT the float Task 92 closed.** Task 92 E0
closed soft-float as a conversion target on a ~90-second average whose largest
caller was `gcPlayDObjAnimJoint` at 54.2%. In this excursion **every caller it
classified is flat** — `gcPlayDObjAnimJoint` +2,217/frame, the renderer float
callers +20 to +131 — while a population that is *exactly zero in the control*
appears with 62,830/frame of caller self time: `func_ovl2_800ED490` (a `Mtx44f`
multiply, 27 mul + 21 add per call), `func_ovl2_800EDBA4` (walks a joint DObj to
its root and back rebuilding world matrices), `gmCollisionSetInvertMatrix`,
`gmCollisionTransformMatrixAll`, `gmCollisionTestRectangle`,
`gmCollisionGetWorldPosition`. All of `decomp/.../gm/gmcollision.c`.

**The excursion is hit detection with live hitboxes.** That also explains its
other two signatures: the consecutive-frame runs are an attack's active frames,
and `MISC` tripling is the hit effect drawing as its own owner. Task 92's verdict
closed the class it measured; this caller set was not in it, so it is not
evidence against acting here.

**The exactness-preserving cut was priced and REFUTED before it was built.** The
obvious first move was an E5/E12-shape redundancy memo — `func_ovl2_800EDBA4`
carries two memo flags cleared once a frame by `parts->unk_dobjtrans_word = 0`
(`ftparam.c:2185`). Exact call counts, free from the profiler CSV because a
function's entry PC retires once per call, say there is nothing to memo:
**`func_ovl2_800ED490` runs 27.2 times a frame**, `GetWorldPosition` 32,
`TransformMatrixAll` 22.6, the rest 16 each. The within-frame memo already works.

**The cost is arithmetic and the unit price is the finding:** 230,850 cycles over
6,053 `fadd` calls is **38 cycles per soft-float add** (`__mulsf3` 27). The
excursion adds ~6,410 float ops/frame for ~217,734 cycles. Ordinary frames
already run ~5,952 ops (~182,000); excursion frames ~12,362 (~400,000).

**OWNER DECISION, sized and ready.** The only lever is float→fixed on the
collision path. `PROJECT_GOAL.md` permits it — "Mechanical equivalence is
required. Bit-exact or numerically identical execution is not" — and its
sacrifice order ranks gameplay fidelity *above* stable 30 FPS. But `gmcollision.c`
decides hit detection and is verifier-gated by the Task 9 state hash, and
re-bounding a bit-exact gate is not a call to take unsupervised. Combined with
E32 this is roughly the whole remaining gap: the 26 projected over-gate frames
span 1,152,192–1,614,080, and removing ~280,000 puts all but four under 1,120,000.

**Caveat now on record:** `_ntrcardRomReadSector` measured +95,357 on the
excursion with the HUD drawn and −95,356 (entirely in the control) with it
compiled out — same frames, same deterministic match. Cartridge reads complete
against wall time, so **never attribute cartridge activity to a frame across two
differently-timed builds.** The load *counter* is frame-stable because a finalize
is a software event; the sector read is not.

## R2-03 E34 — E26's premise MEASURED: the epoch state is 99.5% static (2026-07-29)

**E26 is viable, and this is the number it was missing.** Its §2a correction
raised the live material as a per-epoch problem; the harder version is that
`ndsRendererNativeApplyMaterial` writes the same `stats` fields, so contamination
can propagate *forward* into later epochs through any field their before-span
does not itself rewrite. If that happened often, the baked table would be a
fiction. So it was measured before either design was built.

`NDS_R2_FIGHTER_EPOCH_STATE_PROOF=1` hashes the state each epoch hands to its
runs — the fields `PrepareProductionRun` and the shade read: `geometry_mode`,
`othermode_h/l`, combine w0/w1, env/prim colour, texture flags/tile/on/scale,
the two light colours, the light direction, and the **active** 20-word
`NDSRendererTileState` — keyed by epoch index, counting frames whose value
differs from the one already stored.

| | frames 439..919 |
|---|---:|
| samples | 22,566 (47.0/frame) |
| **changes** | **108 — 0.48%** |

**The state is a function of the epoch index 99.5% of the time.** That is the
same shape as E5's run facts (1.9% churn) and it says the fold is a table plus a
small repair, not a table plus a fiction.

**And the residue has an owner.** 108 changes over 480 frames is ~0.2/frame,
which tracks the hitlag population E31/E32 mapped — Task 39's hurt flash writes
`input->materials[]` live, and hitlag covers roughly 10 frames in 128. So the
0.48% is very likely the flash colour, which is exactly the field E26 §2a already
says must stay a runtime write ("bake the table and write colour per frame" —
E1a's shape). Confirm that attribution by re-running the proof with `prim_color`
and `env_color` dropped from the hash: if changes go to zero, the fold is clean
and colour is the only runtime input.

**Method note:** `gNdsR2EpochStateUnstableEpochs` reads 0 in this window because
the census reports *deltas* and that counter saturates during the pre-439 warm-up.
Its absolute value needs a single-stop read, not a windowed difference — do not
read that 0 as "no epoch is unstable", which contradicts the 108.

### E34-b — the attribution CONFIRMED, and the limit of what it licenses

`NDS_R2_FIGHTER_EPOCH_STATE_PROOF=2` is the same hash with `prim_color` and
`env_color` removed. Same window, same ROM shape:

| | frames 439..919 |
|---|---:|
| samples | 22,296 (46.4/frame) |
| **changes** | **0** |

Level 1 is the positive control: the identical hash caught 108 changes, so it is
demonstrably time-sensitive, and dropping exactly two fields took it to zero.
**Apart from the two colours, the per-epoch state is exactly a function of the
epoch index.** §2a's "two snapshots plus an after-span field mask" is therefore
unnecessary — one snapshot per epoch plus two colour writes reproduces it.

**But do not read this as a construction guarantee, and do not bake the material
out.** `ndsRendererAdapterBuildNativeMaterial` rebuilds every material from the
live `MObj` each frame (`src/port/reloc_backend_renderer_dl.c:7830`), and its
texture-derived fields — palette image, TLUT, block image, tile sizes, texture
state — are keyed off `mobj->texture_id_curr`/`texture_id_next`, which
`ndsRendererAdapterSaveNativeMaterialTextureIds` exists specifically to
save/restore. A texture animation moves them. This window contained none, which
is why only colour varied; a window that contains one would show more, and a
table baked from this measurement would render the wrong texture.

**So E26's safe shape is: fold the two static spans, keep `ApplyMaterial` live
and unchanged.** The measurement licenses removing the *replay*, not the
material. Anything that also removes the material application must first prove,
from the source rather than from a window, that no fighter material's texture
identity moves during a match.

## R2-03 E33 — the run prepare still has no hot spot, re-confirmed (2026-07-29)

Per-run split on the current build (`NDS_R2_FIGHTER_RUN_PROOF=2`, frames
439..919), 62.8 runs a frame:

| run phase | ticks/frame | share |
|---|---:|---:|
| Tail | 14,224 | 29% |
| TexPrep | 12,506 | 26% |
| UV | 11,705 | 24% |
| Validate | 8,757 | 18% |
| TexReuse | 1,216 | 3% |
| sum | 48,408 | |

`TexPrepCount` 47.0/frame against `TexReuseCount` 16.7 — the same 46.4-of-62.8
full prepares E25b found, unchanged by E28/E29/E32 as expected (none of them
touched the invalidation).

**Nothing here is a cut.** Four phases between 18% and 29% is exactly E25's
"PrepareProductionRun has no hot spot", now re-confirmed on a build three cuts
newer. **Do not go hunting for one.** The cost is structural — it is the
per-invalidation re-prepare — and E26's replacement is the answer, not a
micro-optimisation of Tail or UV. E5 already refuted the UV loop specifically.

**Two instrument defects found doing this, both of which waste a build:**

- **At `NDS_R2_FIGHTER_RUN_PROOF=1` every one of these tick counters reads
  exactly 0.** They need level 2. A run at level 1 reports a complete, plausible
  all-zero table rather than failing.
- **`gNdsR2RunCallCount` reads 0 at level 2 as well** — it belongs to the E5
  falsifier, not to these brackets, and is not wired on the canonical production
  path. Its own comment warns about precisely this ("hooked that table and
  honestly reported zero calls"); it is still true. Do not use it as the liveness
  denominator for the tick split — use `TexPrepCount + TexReuseCount`.

Do **not** compare the 48,408 above against the census build's
`gNdsR2SubmitPrepTicks` 39,043: different bracket boundaries **and** different
binaries, which the standing rules forbid comparing.

## R2-03 partition at HEAD, and where the phase stands (2026-07-29)

Census build, all three cuts on, frames 439..919:

| phase | ticks/frame | pre-E28 |
|---|---:|---:|
| Walk | 3,224 | — |
| Validate | 10,148 | — |
| Preflight | 3,224 | 3,272 |
| **Root** | **44,502** | 44,785 |
| **State replay** | **61,441** | 72,798 |
| Shade | 22,579 | 57,715 |
| **Submit** | **94,395** | 105,630 |
| — of which Prep | 39,043 | 41,928 |
| execute sum | 226,141 | 284,200 |

**Caveat, in E30's family: the State bracket is instrument-inflated.**
`ndsRendererNativeApplyStateDelta` carries a `NDS_TASK91_DRAW_PHASE_CENSUS` block
that runs *per delta* — three array reads, two compares and two array writes
against 194.4 deltas a frame — and it sits **inside** `gNdsR2ExecStateTicks`.
Call it 6,000–10,000 of the 61,441. The other brackets wrap whole calls and are
not exposed this way. Before sizing a state-replay cut, re-measure with the
per-delta census block compiled out, or the cut will be sized against a number
that includes the probe.

**Ranked remaining, against R2-03's provisional 250,000 budget with FTR P50 at
408,512:**

1. **State replay 61,441 coupled to Prep 39,043 = ~100,500.** E25b showed these
   are one mechanism: 194.4 deltas invalidate `texture_prepare_valid`, forcing
   46.4 of 62.8 runs into the full prepare despite a 99.5% texture-memo hit rate.
   E26's spec (`..._E26_Spec_GeneratedEpochState_20260728.md`, **including its
   §2a correction** — two snapshots plus an after-span write mask, material stays
   a runtime step) is the design. This is the switch plan's own R2-03 bullet.
2. **Submit emit ~55,352** = 29.5 ticks/corner over 1,878 corners, after E29 put
   both vertex tables in DTCM. High for four FIFO writes; unexamined.
3. **Root 44,502** over 32 roots. `ndsRendererNativeApplyRootLightPreamble` is
   not it — that is a handful of `stats` writes under `optimize("Os")`. The
   remainder is `BindProductionRoot` + E17's split matrix load + `glStoreMatrix`,
   and E22/E23 already priced the matrix load and refuted the projection hoist.

**DTCM is now full at its safe margin.** Data tops out at `0x02ff2298` against
the `0x02ff3000` ceiling — 3,432 bytes. `sNdsNativeFighterPackedCorners` (3,756)
does **not** fit. It could be made to fit by raising the ceiling toward the
measured boot-stack low-water at `0x02ff3340`, but that leaves under 500 bytes of
margin and the table is read sequentially, where the data cache already does
well. Not worth the stability risk — treat the DTCM lever as harvested.

## R2-03 E32 — the hitlag shuffle folded; FTR's tail is GONE (2026-07-29)

**KEEP candidate, flag default 0, awaiting the owner's visual approval.**
`NDS_R2_FIGHTER_SHUFFLE_FOLD`.

The renderer was disabling the entire native fighter owner whenever
`fp->shuffle_tics != 0` — i.e. giving up its fast path on **every hit**. A split
counter attributed all 5 fallbacks in frames 460..500 to `shuffle_tics` and
**zero** to `is_use_animlocks`.

It never needed to. `ftdisplaymain.c:1205` is one `G_MTX_PUSH` +
`syMatrixTra(x, y, 0)` + `gSPPopMatrix` around the whole fighter draw, and
`lbcommon.c:1627` writes the identical effect as `f[3][0] += x; f[3][1] += y;` on
the part's **world** matrix before the camera. `PrepareNativeOwnerMatrices`
already builds exactly that matrix, so the offset goes in at the same point in
the same space — **mechanically equivalent by construction, not an
approximation.** `shuffle_tics` leaves the eligibility disjunction;
`is_use_animlocks` stays (measured firing zero times).

| | FTR P50 | FTR P95 | FTR max | frames > 600k | WORK P95 |
|---|---:|---:|---:|---:|---:|
| E30 | 404,672 | 913,920 | 918,976 | **11** | 1,467,840 |
| **E32** | 408,512 | **412,992** | **414,656** | **0** | **1,381,120** |

The bimodal distribution collapsed to flat. Frames over the 1,120,000 gate
**35/128 -> 27/128**; VBlank `2:472 3:87 4:4 5+:2` -> **`2:489 3:72 4:4 5+:1`**.
Engagement read from the same run: `gNdsR2ShuffleFoldedFrames = 20`, two fighters
across ten burst frames. Ordinary frames pay `FTR` +3,456 median (the per-binding
adds), at the noise floor and bought back many times over.

**NOT verified: the visual gate.** A zero offset would flatten `FTR` identically
by simply not shuffling, so "the burst disappeared" is *not* evidence the effect
survived. Only a screenshot or play test confirms the fighter still shakes, by
the right amount, and that electric hits shake horizontally. Build the same ROM
with `NDS_R2_FIGHTER_SHUFFLE_FOLD=0` for the comparison arm — that arm is the
generic path and is correct by construction. Flag stays default-0 until approved.

**Boundary on the enabled arm: PASSED**, with the flag defaulted to 1 for the
whole run and the default reverted after. A first attempt was discarded because I
reverted the default *while that run was still building* — `make` re-reads the
Makefile per invocation and the profile runs several, so its (passing) result was
not trustworthy. **Never edit a build flag while a verifier is running; the tree
a verifier reads has to be still.**

The committed state is flag **default 0**: every hunk is inside
`#if NDS_R2_FIGHTER_SHUFFLE_FOLD` and the eligibility condition falls through to
its original `#else`, so the shipping configuration is unchanged by construction.

**Gate now: WORK P95 1,381,120 = 1.23x, gap 261,120** (from 1.37x at the R2-08
readiness table). Remaining tail is `SRC` asset loading (~103,488, Task 75 E0)
plus `OTHR`/`MISC`.

Write-up: `docs/optimization/ClaudeOpus5_R203_E32_ShuffleFold_20260729.md`.

## R2-03 E30 — the median is inside the gate; the tail is three other things (2026-07-29)

**The single most important row on this board.** E28+E29 took 58,304/frame out
of the fighter. `WORK` P50 fell 1,071,488 -> **1,010,240, inside the 1,120,000
gate**. `WORK` P95 went 1,496,064 -> 1,467,840 — **essentially not at all.**

**The steady-state fighter cost and the P95 gate are now different problems.
More median cuts will not close the gate.** Do not queue another median cut
without a reason that survives this row.

Decomposing the 8 worst `WORK` frames against the median frame — the actual
frames, not independently-sorted columns — gives three causes that do not
co-occur (3 frames HUD-only, 3 FTR-only, 2 both):

| bucket | excess over median frame | share |
|---|---:|---:|
| **FTR** | 2,538,432 | **41.9%** |
| **HUD** | 1,868,608 | **30.9%** |
| **SRC** | 1,298,624 | **21.4%** |

**HUD was the instrument, and it is now switchable off.** `HUD` is 960 at the
median and **345,024** on 9 of 128 frames, periodic at 13.25 presented frames =
0.494 s = `NDS_BATTLE_FPS_HUD_SAMPLE_TICKS` (`BUS_CLOCK / 2`). It is the tick
HUD's own block: eleven 128-entry ring sorts and thirteen `vsnprintf`/`iprintf`
console lines. **None of it exists in the published ROM**, and the GDB sampler
reads `sBattleTickHudRing` directly and never reads `sBattleTickHudP50/P95`.
`NDS_TICK_HUD_DRAW=0` removes it: `WORK` P95 **1,548,032 -> 1,467,840**, VBlank
`2:446 3:109 4:9` -> **`2:472 3:87 4:4`**, frames over gate 39 -> 35.

**Every measurement this campaign took on the tick-HUD ROM carried ~345,024
ticks of instrument on ~7% of frames — exactly the frames the P95 gate is
decided on.** Pass `NDS_TICK_HUD_DRAW=0` for measurement; default stays 1 for
device reads and screenshots. (`sBattleTickHudRing` is now `volatile`: with
nothing in the ROM reading it, `--gc-sections` deleted the array and the sampler
failed. A measurement buffer whose only consumer is a debugger must say so.)

**Highest-value unowned row: the FTR bursts — and they are a NATIVE-OWNER
FALLBACK.** `FTR` is bimodal, 401,856 median or ~900,000 with nothing between, on
frames **478–482 and 544–548**: two contiguous five-frame events 62 frames apart.
Both probes are done and they agree:

- **Geometry does not spike.** P0 triangles/frame: 468–473 **256.0**, 473–478
  **384.0**, 478–483 (burst) **320.0**. The window *before* the burst draws more
  than the burst does.
- **The native execute gets cheaper.** Phase census over the burst: **38.2 epoch
  calls instead of 58.8**, 49.0 submits instead of 80.4, and **178,800 ticks
  instead of 286,988** — while whole-frame `FTR` doubles. Both bursts are
  counter-identical (191 epochs, 245 submits over 5 frames): one event, twice.

So the work **left the native execute** and reappeared outside every bracket the
phase census owns. `FTR` also brackets the DObj walk, the revalidation and the
owner prep (E2/E3 113,199/frame, E4 MatrixPrep 91,338/frame), so the excess is in
one of those or in a fallback to the generic interpreter. **Optimising the native
fighter execute cannot touch these frames either way.**

**ANSWERED — it is the hitlag shuffle turning the native owner off.**
`NDS_TASK68_FALLBACK_CENSUS=1` over frames 460..500 (40 frames, containing the
burst): `FallbackCount = 5`, reason **[2] `AnimLock` = 5**, every other reason 0,
denominators `Calls`/`Eligible` 82/82. **One fallback per burst frame.** The site
is `reloc_backend_renderer_dl.c:12224`:

```c
if (native_owner_enabled && (production_mode || hierarchy_mode) &&
    ((fp->is_use_animlocks != FALSE) || (fp->shuffle_tics != 0u)))
    native_owner_enabled = FALSE;      /* whole fighter -> generic path */
```

`shuffle_tics` is SSB64's hitlag shuffle (`fttypes.h:1146` "Model shift timer",
set from `ftParamGetHitLag` in `ftparam.c:236`). Two hits ~2 s apart with ~5
presented frames of hitlag each is exactly the signature.

**The source makes the fix easy.** `ftdisplaymain.c:1205` is one `G_MTX_PUSH` +
`syMatrixTra(x, y, 0)` around the *whole* fighter draw and one `gSPPopMatrix` —
a constant whole-model translation from
`dFTDisplayMainShufflePositions[is_shuffle_electric][shuffle_frame_index]`. It
touches no geometry, material or animation. The native owner already loads a
per-root matrix (E17's `ndsRendererLoadHardwareSplitMatrices`), so **folding the
offset into that load reproduces the source exactly at ~zero per-frame cost**
instead of dropping the whole fighter to the interpreter on every hit.
Mechanically equivalent by construction, not by approximation.

Expected: ~500,000 excess ticks removed from ~10 frames per 128 — 41.9% of the
tail excess. **Check first** which half of the disjunction fires: `AnimLock` is
shared by `is_use_animlocks` and `shuffle_tics`, so split the counter or read
`fp->shuffle_tics` on a burst frame before assuming shuffle is the whole story.

**Harness fixed in the same change.** `census-fighter-draw-phases.ps1` collapsed
its window twice in one session and printed a complete, plausible table both
times — the second produced a "no fallback occurred" reading from the wrong
frames, which was briefly written down as a refutation. GDB `if` at top level
resumes exactly once (Task 96's rule), so a missed stop lands the script's own
`continue` somewhere later unnoticed. It now throws unless the A stop is exactly
`StartFrame` and the window is `WindowFrames` (+1 tolerated on B). **A
measurement that quietly answers a different question is worse than one that
fails.**

`SRC` (21.4%) is Task 75 E0's known load population, sized at ~103,488, unchanged
by Runtime 2.

**Gate now: P50 passes; P95 1,467,840 = 1.31x, gap 347,840** (was 1.37x).

Write-up: `docs/optimization/ClaudeOpus5_R203_E30_TailDecomposition_20260729.md`.

## R2-03 E29 — the fighter's hot tables move to DTCM, −26,816 (2026-07-28)

**KEEP.** The emit reads `sNdsNativeFighterPreparedDense` (8,656 bytes) and
`sNdsNativeFighterDenseNormals` (2,164) once per corner, 1,878 corners a frame,
in packed-corner order — randomly. Both sat in main RAM behind the ARM9's **4 KB
data cache**: a 2.7x overcommit, so essentially every corner missed. **DTCM —
16 KB of single-cycle uncached CPU-local memory — held 184 bytes.**

Paired 128-frame A/B against E28:

| bucket | better | worse | median delta |
|---|---:|---:|---:|
| **FTR** | **128/128** | **0** | **−26,816** |
| STG | 108 | 20 | −1,280 |
| WORK | 120 | 8 | −28,096 |

P0/P1 triangle counts identical (136,640 / 146,880). VBlank `2:438 3:117` ->
`2:446 3:109`.

**`STG` improved even though the stage never touches these tables.** Data-cache
pressure is a whole-frame shared resource, so moving a table out of main RAM pays
subsystems that never referenced it. Worth remembering when a cut's benefit shows
up outside its own bucket.

**Why the space was free, and why that needed measuring.** `__sp_usr` sits at the
top of DTCM and the region length spans the space the stack grows down into, so
the linker cannot catch a collision — a good reason the space had gone unused.
Measured: at the frame marker `sp = 0x02296530`, main RAM. Game code runs on a
Calico thread stack; only the *boot* stack enters DTCM, reaching `0x02ff3340`,
2,880 bytes down. 12,948 contiguous bytes were untouched at frame 900.

**Guard rails, because this fails silently.** Linker
`ASSERT( __dtcm_bss_end <= 0x02ff3000 )` encodes the measurement; a new
`.dtcm.fighter` section placed first and followed by `. = ALIGN(32)` keeps
Calico's `__irq_table` on its boundary regardless of the data-driven table sizes
(the Task 20 gate caught exactly that); and the Task 20 allow-list now carries
both owners with the DMA/IPC/ARM7 audit recorded. `forbiddenDmaRefs=0`.

**The struct shrink is bundled, not claimed.** Dropping `shaded_rgba` and
`packed_color` (dead under `HW_LIGHT` — every epoch is lit, so the loop writing
them never runs) takes the struct 16 -> 12 bytes. The `_Static_assert` demanding
16 was **right on its own terms**: two per 32-byte line, no straddling. Measured
alone in main RAM the shrink was a median −5,376 with a **mean of −1,122** — at
the noise floor, the straddle penalty eating the win. In DTCM there are no cache
lines, so 12 is strictly better and buys 2,164 bytes of margin under the boot
stack. That is the only reason it ships.

**Two process failures.** `make` does **not** regenerate the generated includes —
`build.ps1` does. The first E29 build changed the struct to four fields against a
four-day-old include holding six positional initializers; GCC warned, assigned
`gx_z = 0`, and produced a complete A/B on a ROM with every Z coordinate zeroed.
The generator now emits designated initializers so that mismatch cannot recur
silently. And the build-output `Select-String` filter dropped the warning —
**filter build output for new warnings, not for a fixed list of expected ones.**

**Next, and now cheap:** 7,144 bytes of DTCM remain, 4,264 under the boot stack's
low-water mark. `sNdsNativeFighterPackedCorners` (3,756) fits, though it is
streamed rather than randomly indexed and should benefit less.

Write-up: `docs/optimization/ClaudeOpus5_R203_E29_FighterTablesInDTCM_20260728.md`.

## R2-03 E28 — E16's dead producers, −31,488 (2026-07-28)

**KEEP.** E16 skipped the per-dense-vertex shading loop with a runtime flag but
left the work that computes that loop's *inputs* running on every lit epoch:
`ndsRendererHardwarePrepareLitDirection` (nine 32x32->64 multiplies, three
64-bit squares, an `sqrtf`, three float divides) and
`ndsRendererHardwareGetLightShadeLut`. Neither result has any other consumer in
a shipping configuration.

Paired 128-frame A/B, one tree, control = `NDS_R2_FIGHTER_SOFT_LIGHT_KEEP=1`:

| bucket | better | worse | median delta |
|---|---:|---:|---:|
| **FTR** | **128/128** | **0** | **−31,488** |
| WORK | 113 | 15 | −31,680 |

WORK frames over the 1,120,000 gate: **52/128 -> 40/128**. VBlank histogram
control 2:409 3:148 4:7 5+:2 max:18, candidate 2:438 3:117 4:9 5+:2 max:18 —
29 frames moved from a 3-VBlank interval to a 2-VBlank interval.
`gNdsFighterDLAllDrawP0/P1HardwareTriangleCount` identical in both arms
(136,640 / 146,880 over 480 frames): geometry is bit-identical, as the mechanism
requires — the removed values had no reader.

**The lesson, and it is general.** A flag that skips a *consumer* does not skip
its *producers*, and a single tick bracket around both cannot tell you which one
you removed. E24 read this same function and concluded "the action walk isn't
the cost" — correct, and it missed this because the dead work is in the preamble
*above* the walk, inside the condition that decides whether the epoch is lit.
**Price a skipped loop's inputs separately from the loop.**

**Second lesson, methodological.** The sorted-percentile table read
`WORK P95 +73,664` and every other number negative. That was an artifact: each
column's P95 is a different frame, and the P95 frame is an excursion frame whose
placement moves between arms (`WAIT` P95 fell by almost exactly the same amount,
which is the tell). Both arms run the same deterministic ROM from the same start
frame, so **frame N is the same game state in both — pair by frame number, not
by sorted percentile.** The pairing is free and it is what turned an ambiguous
result into 128/128.

**E27 is REFUTED and its probe is removed.** `gNdsR2MaterialOnlyInvalidations`
measured 2.0/frame against 28.0 material applications: 26 of 28 material
invalidations of the texture prepare hit a prepare the before-span deltas had
already dirtied. A split validity would reach ~1,800 ticks, below the noise
floor. It stays a necessary *component* of E26, not a cut of its own. The probe
also read `state.texture_prepare_valid` inside `ndsRendererNativeApplyMaterial`,
which the M3 stage falsifier correctly rejects as an unclassified read — the
standing "remove temporary probes" rule would have caught it before Boundary did.

Graduated R2-03 total is now E17 17,600 + E16 35,072 + E28 31,488 = **84,160**
of the 250,833 gap (34%).

Write-up: `docs/optimization/ClaudeOpus5_R203_E28_DeadSoftLight_20260728.md`.

## Bug #10 — closed and folded in (2026-07-28)

`06992f10812` "Fix Mario pelvis texture clamp", cherry-picked from `2cbc6189d15`
on `codex/fix-mario-bottom-rendering` onto the R2 branch so authorship is
preserved. Epoch 0 loads a 32x24 CI4 source into a 32x32 DS texture; its N64 T
axis is CLAMP with mask 5, so coordinates 24..31 resolve to row 23, while the DS
sampler wrapped through the eight zero-padded transparent rows — the aperture
was *inside* textured pelvis triangles, not at a geometry or culling seam, which
is why five earlier causes were eliminated. One line in
`ndsRendererHardwareTextureMaskedClampNeedsWrap` disables wrap when the logical
clamp edge is at or before the mask period.

It arrives with its own gates rather than needing new ones: a host fixture for
the exact 32x24 case, a structural pin in `check-gbi-decode-fixtures.ps1` so the
line cannot be silently reverted, the `pause_under20` camera oracle, and the
controller-playback DTCM move that oracle needs in order to write pads over GDB.
The DTCM layout checker was not relaxed to accommodate it — every Calico
boundary assertion survives, parameterised by the new 32 bytes, with added
all-or-none and per-symbol address/size/alignment pins.

Folding it in did surface a real harness defect, fixed in the same cycle.
Boundary failed twice on the locked-30 pacing gate reading
`logic/present = 422/212` with a phase histogram summing to 211. The ROM was
right: taskman's own counter and the fighter route both read 424 updates for 212
presents, an exact 2:1. Two terms compared counters incremented at *different*
instructions of one iteration, so they were asserting where the debugger stopped.
Both are now a four-state stop-phase model that rejects five of the eight sign
combinations — strictly stronger than the equality it replaced — with taskman's
independent counter disambiguating the one aliasing pair. E8 did not create the
window; it changed where in the frame the stop lands. Full derivation in
`docs/optimization/TASK_STANDING_RULES.md`.

The opt-in Task 25R trace carried the third instance of the same defect and is
now fixed too (`6221406`). Its rows all come from one fixed marker, so the skew
is constant and the contract is *stronger* than the four-state model: take the
skew from the first row, require it reachable, require every later row to agree
— a dropped or doubled update then disagrees with its neighbours instead of
hiding inside a tolerance. The final reconcile runs the BPLAY_PACE snapshot
through `Test-BattlePlayablePacingStopPhase` rather than a logic-only bound.
Eight synthetic cases cover it with no ROM or emulator; the registry pins the
new contract and bans the old equality.

## R2-03 gate MISSED 2.00x, and the 56% nobody had measured (E13/E14, 2026-07-28)

The owner observation below is now measured, and it turned over the phase.

**The gate.** Fighter draw, both fighters, bracketed on the tick-HUD ROM over 479
frames: **501,624 ticks/frame against §7's 250,000 for the pair.** Over by a
factor of 2.00. Mario alone measures 237,219 per draw call; either fighter on his
own very nearly exhausts the budget written for both. That budget was set in
R2-00b without a measured per-fighter cost.

R2-03 has shipped -47,486 (E9+E10, E12) against a 250,833 gap — 19% of it.

**Where the draw actually goes** (per frame, both fighters):

| phase | ticks | share |
|---|---:|---:|
| Walk / Validate / Reset | 20,595 | 4.1% |
| OwnerPrep (matrices + materials) | 143,684 | 28.6% |
| Build production inputs | 37,292 | 7.4% |
| **`...ExecuteNativeFighterOwnerProduction`** | **279,617** | **55.7%** |
| tail | 20,436 | 4.1% |

Both shipped cuts landed in the 28.6%. The 55.7% had never been bracketed, so it
was never a candidate — E3's split stopped at the point the owner inputs are
built, and everything past it went into one unnamed remainder.

**The 3D hardware is idle.** `GXSTAT` sampled either side of 946 fighter
submissions: FIFO entries 0 entering, 0 leaving, max 0, geometry engine busy on
0 of 946. Positive control passes (OR of raw words `0x06009F00`, bit 26 =
FIFO-empty set, so the register is live and the zeros are real).

**The ARM9 is the whole cost.** Cutting fighter polygons is the *wrong* lever: it
spends visual fidelity to work around a CPU failing to feed hardware that has
headroom. `PROJECT_GOAL.md` permits the trade; this says we have not earned it.

**E15 corrects what comes next.** E14 read "447 ticks per hardware triangle" off
this bracket and recommended a captured command stream on R2-02 E2's precedent.
That statistic divided an inclusive bracket by the wrong denominator — most of
the bracket is not per-triangle work. **The emit is ~99 ticks/triangle and 20% of
the execute**, so a DMA'd stream caps out near 62,693 against a 250,833 gap. The
recommendation is withdrawn; see the E15 section below for the real ranking.

Full write-ups: `docs/optimization/ClaudeOpus5_R203_E13_FighterPriceAndGate_20260728.md`,
`..._R203_E14_SubmitSplitAndGxIdle_20260728.md`.

## R2-03 E15 — the fighter is a per-epoch machine (2026-07-28)

The execute partitions completely. Per frame, both fighters (instrumented build;
brackets cost ~31,165/frame, so absolutes are inflated ~10-20% and the ranking is
the finding):

| phase | ticks/frame | share |
|---|---:|---:|
| Preflight | 3,247 | 1.0% |
| Per-root: bind, composed matrix, `glStoreMatrix`, light preamble | 40,785 | 13.1% |
| Per-epoch: two state spans + material | 52,065 | 16.8% |
| **Per-epoch: shade actions** | **86,207** | **27.7%** |
| Run prepare | 42,520 | 13.7% |
| Raw emit | 56,873 | 18.3% |
| Cross emit | 5,820 | 1.9% |
| residual | 18,487 | 5.9% |

**48.5 epochs and 66.2 runs per frame, averaging 12.7 triangles per epoch.** Each
epoch pays ~2,850 ticks of state and shade *before a triangle is emitted*, against
~1,255 of prepare-and-emit. **~70% of the execute is per-epoch and per-root setup;
20% is geometry.**

Ranked leverage:

1. **Shade actions, 86,207.** E1 refuted memoising it *across frames* and that
   stands — but E1 never asked whether the shade recomputes **per epoch** what is
   constant **per root**, which a cross-frame memo cannot see. 48.5 epochs against
   ~28 roots is the shape that makes it worth asking.
2. **Epoch state spans, 52,065.** R2-02 F found adjacent-run redundancy in the
   stage's spans; the fighter's have never been checked.
3. **Per-root 40,785** over ~28 roots — contains the GX matrix load and
   `glStoreMatrix`. Whether every root needs its own palette store is unasked.
4. Run prepare 42,520 — already cut by E12, diminishing.
5. Emit 62,693 — ordinary, and the least promising per unit of risk.

Write-up: `docs/optimization/ClaudeOpus5_R203_E15_ExecuteSplit_20260728.md`.

## R2-03 E17 — split matrix load BUILT, −17,600, awaiting owner approval (2026-07-28)

**First implementation of the E16 sequence, and it stands on its own.**
`NDS_R2_FIGHTER_HW_MTX`, default 0.

The fighter composed modelview x projection on the CPU (one 4x4 20.12 multiply
per root) and loaded the product. Now both are loaded separately and the
geometry engine — idle on 946 of 946 submissions per E14 — performs the
multiply. The compose is skipped outright; E16b proved it has no other consumer
under mode 9.

| bucket | A: composed | B: split | delta |
|---|---:|---:|---:|
| **WORK P50** | 1,118,144 | 1,099,584 | **−18,560** |
| **WORK P95** | 1,585,408 | 1,528,064 | **−57,344** |
| **FTR P50** | 507,456 | 489,856 | **−17,600** |
| STG P50 (control) | 175,552 | 175,296 | −256 |
| **VBlank 2 / 3** | 320 / 233 | **381 / 167** | **+61 frames at 30 FPS** |

The size matches the mechanism: ~28 roots x a 4x4x4 20.12 multiply ≈ 18,000
predicted against 17,600 measured. `STG` moving 256 is the placement floor and is
the control on whether `FTR` is real.

**NOT GRADUATED — needs the owner's eye.** Vertex positions now round in
hardware rather than on the CPU, a sub-pixel difference, and `AGENTS.md` gates
rendering-side changes on visual approval rather than exactness. **Boundary
passes in BOTH configurations, flag 0 and flag 1** — the second run was nearly
skipped, and verifying only the default would have meant approving a change on a
green run of the arm it replaces. The candidate capture
(`artifacts/visibility/ClaudeOpus5_R203_E17_SplitMatrix_candidate_20260728.png`)
shows both fighters and the stage correct with no distortion.

**On approval:** add `override NDS_R2_FIGHTER_HW_MTX := 1` to the published
`smash64ds-battle-playable-hwtri` block and the tick-HUD block. E16's hardware
lighting then builds on top of the vector matrix this establishes.

**Correction to E16a/E16b's stated reason.** Both claimed the fighter loads
through "matrix mode 1, position only, which never updates the vector matrix".
Wrong: libnds names mode 1 `GL_POSITION` and mode 2 `GL_MODELVIEW`, and the code
already used mode 2, so a vector matrix was always being written. Found when the
invented name `GL_MODELVIEW_VECTOR` failed to compile. The prerequisite survives
for a narrower reason — what landed in the vector matrix was the composed MVP,
and normals must not be rotated by a projection. Same fix, wrong cause.

Write-up: `docs/optimization/ClaudeOpus5_R203_E17_SplitMatrixLoad_20260728.md`.

## R2-03 E25 — PrepareProductionRun has no hot spot (2026-07-28)

Measurement, reusing E11's existing `NDS_R2_FIGHTER_RUN_PROOF=2` instrument.
480 frames, 62.8 runs/frame, ranking only:

| phase | ticks/frame | share |
|---|---:|---:|
| tail (field writes + batch begin) | 13,753 | 27.7% |
| texture prepare | 13,076 | 26.3% |
| UV | 12,206 | 24.6% |
| entry validate | 8,761 | 17.6% |
| texture reuse | 1,283 | 2.6% |

**Four roughly equal quarters**, so no partial optimization reaches the 42,281.
Each re-derives a fact E5 measured at **1.9% churn**: ~63 runs a frame each
rebuild a description of themselves that changed for one run in fifty.

That is exactly the switch plan's R2-03 bullet — *replace* PrepareProductionRun
with a per-epoch submit consuming baked facts, rather than optimize inside it.
E12 already proved the trade on the texture quarter alone (−32,724).

**The state replay and the prepare are one mechanism.** `TexPrepCount` is
46.4/frame against 62.8 runs — the full prepare runs on 74% of them — while
E12's texture memo hits 99.5% (`R2_TEXMEMO=1899,9,9,0,0`). The reconciliation is
that `ndsRendererNativeApplyStateDelta` calls
`NDS_RENDERER_INVALIDATE_TEXTURE_PREPARE` on every OTHERMODE / COMBINE / TEXTURE
/ GEOMETRY / IMAGE / TILE delta, and E20 counted **194.4 delta applications a
frame**. The replay's job is to move state; moving state is what makes the
prepare expensive.

State replay 65,026 + prepare 42,281 = **107,307/frame, over half of what R2-03
still owes, and they are the same problem.** This reframes the last four
results: E20/E21 priced the delta *write* (cheap, ~280 ticks) when the cost is
the invalidation it triggers; E23 and E24 each removed one side of a coupled
pair and measured null. **Any cut that optimises one side while the other keeps
re-dirtying the state will read as null** — which is exactly the pattern
observed.

**E25c rules out the cheap fix.** Splitting the 194.4 applications by effect:
COMBINE 41.7, TEXTURE 35.7, LIGHT_COLOR 27.4, TILE 23.9, OTHERMODE 13.4, IMAGE
10.6, PRIM 6.7, GEOMETRY 2.0. Invalidating total **127.3/frame**, of which
**70.2 move the 20-word tile state** against 57.1 cheap scalars. A generation
counter would bump more often than there are runs (62.8), and hashing the tile
per run is a worse ratio than E8's losing memo. **There is no cheap validity key
for a value legitimately rewritten more often than it is read — the memo variant
is refuted, not deprioritised.**

**NEXT (unowned):** this is why the plan specifies a generated per-epoch submit
with *no traversal-state dependency* rather than a faster prepare or a cheaper
replay. The generator knows each epoch's final state at build time, so neither
the replay nor the re-derivation needs to exist at runtime. Breaking the
coupling cannot be done from either end alone, so the memo-only variant
(extending `sNdsR2RunTextureMemo` with `poly_fmt`, `scale_s/t`, `origin_s/t`,
`offset`, `vertex_flags`, `textured`) should be treated as a fallback, not the
plan: with 194.4 invalidations a frame still landing, its hit rate would be the
observed 26%, not E5's 98.1%.

**ITCM is now full**: E16 left 1,024 bytes free (31,744/32,768), and the census
and run-proof instruments together overflow it by 172. Measure with one at a
time; anything new on that chain needs `noinline` outside
`.itcm.native_fighter`.

Write-up: `docs/optimization/ClaudeOpus5_R203_E25_PrepareRunPartition_20260728.md`.

## R2-03 E24 — the shade's action walk is not the shade's cost (2026-07-28)

**NULL, reverted.** After E16 the shade's per-action loop is pure bookkeeping:
`stats->vertex_count` (a max) and `gNdsRendererProfileSourceVertexLoadCount` (a
sum), both pure functions of the static action table. E24 baked them per epoch
at load and replaced the walk with two lookups — the switch plan's "consume
baked facts" applied to its smallest piece.

**+2,048 FTR P50**, inside the placement floor; triangle counts unchanged, so it
worked and simply saved nothing. The census bracket for the shade reads 58,105,
but that is `ApplyMaterial` plus instrument overhead, **not** the walk. E18's
ranking-only caveat holds for the third time.

Redirects R2-03's remaining work to where the census actually points:
`SubmitPrepTicks` **42,281/frame** — `ndsRendererNativePrepareProductionRun`,
which is precisely the plan's R2-03 bullet ("no PrepareProductionRun policy
re-checks"). E5 already measured those facts at 1.9% churn and E12 memoised the
texture half for −32,724; the rest is UV and policy.

**Process failure recorded**: reverting E24 with `git checkout --` destroyed a
second agent's uncommitted bug-#10 probes in the same file. Recovered from the
21:23 snapshot, which archives the working tree. Rule added to
`TASK_STANDING_RULES.md`; eleven commits of hunk-filtering were undone by one
destructive command.

## R2-03 E16 — hardware lighting GRADUATED, −35,072 (2026-07-28)

The fighter's per-vertex software shade moves onto the DS geometry engine. Four
parts: a load-time `GFX_NORMAL` table, one `GFX_DIFFUSE_AMBIENT` per epoch
carrying the source light colours folded with the material and the damage flash,
`GFX_NORMAL` instead of `GFX_COLOR` in the emit, and `POLY_FORMAT_LIGHT0`.
Light 0 is white and the *source's* light colours are the material's diffuse and
ambient, so the engine evaluates the source equation rather than an approximation.

| bucket | A (E17 only) | B (E17+E16) | delta |
|---|---:|---:|---:|
| **FTR P50** | 489,536 | 454,464 | **−35,072** |
| WORK P50 | 1,099,328 | 1,063,360 | −35,968 |
| VBlank histogram | 2:381 3:171 4:11 5+:3 | **2:418 3:139 4:6 5+:3** | |

**Geometry bit-identical**: P0 181,440 / P1 173,502 in both arms. Engagement:
`gNdsR2LightVectorWrites = 1,114` (2/execute). Boundary green with the flag on;
ITCM 31,744/32,768. `NDS_R2_FIGHTER_HW_LIGHT` defaults 0 and requires E17.

**E16a**, which decided the design: the light direction changes **0 times in
22,296 epochs**, so `GFX_LIGHT_VECTOR` — which stores the vector transformed by
the vector matrix *at write time* — is written once per execute under an identity
matrix. Colours move on 32% of epochs and material on 72%, and both fold into the
single per-epoch `GFX_DIFFUSE_AMBIENT`.

**Three bugs, all worth their rules.** (1) libnds's `NORMAL_PACK` does not mask
its z argument, so a negative z sign-extends into bits 30-31 — the *light number*
in `GFX_LIGHT_VECTOR`. Every light vector went to light 3, which `POLY_FORMAT`
never enables, so light 0 kept a zero vector and the fighters rendered
ambient-only. (2) Only two of the **four** production emit paths were converted;
`PrimitiveGroups` and `CrossRun` kept writing a colour the shade no longer
updates. (3) Returning early from the shade skipped the action walk that
maintains `gNdsRendererProfileSourceVertexLoadCount`, which Boundary caught as
the complete-stage owner entering the generic transform path — geometry was
already identical, so only the harness saw it.

**Harness gap recorded**: no frame-locked cross-build capture exists.
`capture-melonds.ps1 -ExactFirstFrame` is gated to the Cut G GO-text window, and
live captures drift because the faster arm reaches a later match clock at the
same wall delay. Every rendering-side change from here needs one.

E17+E16 together are −52,672, 21% of R2-03's 250,833 gap. **The phase does not
close on them**, and what remains is not another cut of this kind: E22 showed the
per-root matrix loads are all distinct, E21 showed the state replay is not
redundant, and E18 capped the shade at 53,760 of which E16 takes 35,072. The rest
needs a structural change to what the fighter draw *does*.

Write-up: `docs/optimization/ClaudeOpus5_R203_E16_HardwareLighting_20260728.md`.

## R2-03 E22/E23 — the last unpriced item resolves into E16 (2026-07-28)

**E22 measurement stands. E23 implementation REVERTED — sub-floor.** The per-root
matrix bracket was the phase's last unpriced item. It is now priced, and it is
not where the money is.

480 presented frames, `NDS_R2_FIGHTER_HW_MTX=1`, per frame:

| counter | per frame | share |
|---|---:|---:|
| matrix loads performed | 30.0 | |
| elided by the existing generation check | **0.0** | 0% |
| **identical projection** | **29.0** | **96.7%** |
| identical modelview | 0.0 (6 in 480 frames) | 0.04% |

The modelview is genuinely per-root; the projection is per-frame camera data
re-pushed 29 extra times. E23 skipped it, proved engagement from the same run
(`Skipped=16,750 / Loaded=1,114`, 93.8%), and measured **−3,008 FTR P50 /
−4,800 WORK P50** — under the 5,000–7,000 placement floor, and matching a
first-principles estimate of ~2,900. Reverted rather than keep a hot-path
`memcmp` and 64 bytes of BSS for a delta the instrument cannot resolve from zero.

**The durable lesson: a redundancy's share is not its cost.** The repeated work
is GX FIFO traffic, and E14 already proved this path never backpressures. FIFO
writes are stores; stores are cheap. A 96.7% redundancy rate on cheap work is
worth less than a 7% rate on expensive work. Price one instance before pricing a
cut from a share.

Also: E22's first pass compared projection and modelview *jointly* and reported
**zero** redundancy. The 96.7% only appeared when the halves were scored
separately — E21's rule one level down, so a call that writes several things
needs one counter per thing.

**Two things settled for free.** E17's candidate and control emit
`P0HardwareTriangleCount = 136,640` over the same 480-frame window, identical to
the digit — E17 changes no geometry at all, a stronger check than it shipped
with. (E20/E21's "320/frame" was that quantity over a different window; over
439..919 the rate is 284.7. The control must come from the same window.)

**Queue after this: nothing is unpriced.** The per-root matrix load is ~6,000–
8,000 in total; the balance of that bracket is
`ndsRendererNativeApplyRootLightPreamble`, which is E16's territory. **E16 is the
only cut left in the phase**, and E22 folded the last open item into it.

Write-up: `docs/optimization/ClaudeOpus5_R203_E22_E23_ProjectionReload_20260728.md`.

## R2-03 E21 — the state-delta guard is REFUTED (2026-07-28)

**E20's cut does not exist. Do not build it.** The section below stands as the
measurement it was; this is what its own falsifier returned.

Every case in `ndsRendererNativeApplyStateDelta` writes `stats` purely from
`delta->w0`/`w1`, so identical operands to the previous application of that
effect means identical writes. Per-effect operand tracking, validity cleared on
every material application (conservative direction):

| counter | per frame | share |
|---|---:|---:|
| delta applications | 194.4 | |
| within-frame index repeats (E20) | 124.8 | 64.2% |
| **identical-operand applications** | **14.0** | **7.2% of applications, 11.2% of repeats** |
| of those, GEOMETRY | 0 | |
| material invalidations | 29.7 | |

**Only 14 of 194.4 applications a frame re-write what is already there** —
~3,920 ticks, below the 5,000–7,000 placement floor. The other ~110 "repeats"
are the same knob set to *different* values, which is necessary work. A guard
would pay a compare on all 194.4 to skip 14: **E8's shape**, which cost +16,301
and was deleted.

E19's structural check passes (P0 triangles 320/frame, control rate).

**The durable lesson: count identity of the write, not identity of the target.**
The two differ by 9x here, and the first produced a 35,000-tick opportunity that
does not exist. Third time this cycle a plausible headline survived until one
more counter was added — after E13's inert probe and E19's collapsed geometry —
each costing one build to catch.

**Queue after this:** E16 is again the only large cut identified in the phase
(35,000–50,000), E17 awaits visual approval, and the per-root matrix work
(~40,000, inflated bracket) is the only unpriced item left worth measuring. E17
already establishing E16's vector matrix now matters more, not less.
*(E22 has since priced that item and folded it into E16.)*

Write-up: `docs/optimization/ClaudeOpus5_R203_E21_StateGuardRefuted_20260728.md`.

## R2-03 E20 — the state replay repeats itself 1.8x a frame (2026-07-28)

**Superseded by E21 — the 64.2% is real but is not redundancy. Kept for the
reasoning trail.**

E19 refuted deletion as a pricing method, so this asks R2-02 F's question
instead: not what the phase costs, but how much of it is **redundant**.

479 frames, both fighters, per frame:

| counter | per frame |
|---|---:|
| state-span calls | 80.2 |
| **delta applications** | **194.4** |
| **repeats within the frame** | **124.8 (64.2%)** |
| distinct applications | 69.6 |
| span cost | 54,510 |

70 deltas exist and 69.6 distinct applications happen a frame: **every delta is
applied once for real and ~1.8 more times redundantly**, worth **~35,000
ticks/frame** at 280 ticks an application.

E19's structural check applied — P0 triangles 320/frame, its control rate — so
the arm measures what it claims.

~~**Worth 25,000–30,000 realised.** Best return-to-risk on the board.~~
**Withdrawn by E21: the realised figure is ~3,920, below the placement floor.**

**Falsifier before building, one build:** "applied twice in a frame" is not
"the second was a no-op" — something between them may have changed that state. So
the guard must be **value-based, not frame-based**, and the question is how many
of the 124.8 repeats write a value equal to the current one. If most write a
different value, this collapses the way E19's method did.

Write-up: `docs/optimization/ClaudeOpus5_R203_E20_StateSpanRedundancy_20260728.md`.

## R2-03 E19 — the state spans cannot be priced by skipping them (2026-07-28)

**Method refuted, no number produced.** `NDS_R2_FIGHTER_STATESPAN_SKIP`, default
0, must not be used to cost this phase.

E18's pricing method was pointed at the next ranked item and reported **−251,520
FTR P50** — five times the bracket, and fiction. The spans establish the texture,
polygon-format and geometry-mode state the emit requires; without them every run
is rejected before submitting. Hardware triangles went **320/306 per fighter to
8/0**, so the delta is ~618 triangles a frame ceasing to exist.

**E18 was re-checked against the same failure and holds:** its arm reads 320/306,
identical to control, so the shade skip removed the colour computation and
nothing else. **53,760 stands.**

**What the spans still need.** The only figure remains E15's bracket, ~52,000,
which carries that build's 10-20% inflation and is ranking-only. The right method
is **R2-02 F's, not E18's**: measure how much of the replay is *redundant* — how
many adjacent epochs re-apply state already current — which is the same question
R2-02 F asked of the stage's spans, needs a counter rather than a deletion, and
prices the achievable cut rather than the whole phase.

Write-up: `docs/optimization/ClaudeOpus5_R203_E19_StateSpanMethodRefuted_20260728.md`.

## R2-03 E18 — E16's ceiling is 53,760, not 90,295 (2026-07-28)

**Correction to the number this board carried as the phase's largest
opportunity.** `NDS_R2_FIGHTER_SHADE_SKIP`, lab only, default 0.

E16 priced hardware lighting at "most of 90,295" off E15's shade bracket. That
bracket came from a build carrying the whole E15/E16 census — whose own write-up
says its absolutes are inflated 10-20% and only its ranking is safe — and it
enclosed the per-epoch preamble as well as the per-vertex loop the cut replaces.

Measured directly by skipping that loop, both arms at `HW_MTX=1`:

| bucket | shade on | shade skipped | delta |
|---|---:|---:|---:|
| **WORK P50** | 1,099,584 | 1,044,800 | **−54,784** |
| **FTR P50** | 489,856 | 436,096 | **−53,760** |
| STG P50 (control) | 175,296 | 173,824 | −1,472 |
| **VBlank 2 / 3** | 381 / 167 | **431 / 123** | **+50 frames at 30 FPS** |

Engagement is unarguable: both fighters render as **black silhouettes** against
an untouched stage (`artifacts/visibility/ClaudeOpus5_R203_E18_ShadeSkip_silhouettes_20260728.png`).

**Ceiling, not expected value.** Hardware lighting still writes GX light and
material state per epoch or root, so the honest range for E16 is **35,000–50,000**.

**Does E16 still justify itself — yes, but it is no longer obvious.** 53,760 is
21% of R2-03's 250,833 gap, and with E17's 17,600 the two are ~28% of it. Nothing
identified in the phase is larger; the next ranked items are epoch state spans
(~52,000) and per-root matrix work (~40,000). But it is a four-part change with a
light-space risk against a 35,000–50,000 return, not the ~90,000 that was on this
board. **Sequencing: E17 graduates on its own first, and the epoch state spans
should be priced the same way before E16 is built** — they may be cheaper per
tick won.

Write-up: `docs/optimization/ClaudeOpus5_R203_E18_ShadeCeiling_20260728.md`.

## R2-03 E16 — the shade pass IS the DS's hardware lighting (2026-07-28)

Premise proven without exception, and it is the largest cut identified in R2-03.

`ndsRendererHardwareLitShadeColorPrepared` computes, per vertex,
`ambient + diffuse * dot(normal, light_dir) / 127` — with `light_color_1` as
diffuse, `light_color_2` as ambient, and the `rgba` field of the dense vertex
holding the **normal** (F3DEX packs normals there for lit vertices). That is,
term for term, the Nintendo DS geometry engine's hardware lighting equation.

Measured over 479 frames, both fighters:

| counter | per frame |
|---|---:|
| **lit epochs** | **48.5** |
| **unlit epochs** | **0** |
| epochs on the LUT path | 48.5 (100%) |
| epochs applying a material | 27.7 (57%) |
| **vertices lit** | **513.1** |
| vertices copied from a shared source | 21.5 |

**Not one fighter epoch in a match is unlit**, at ~169 ticks per shaded vertex.

**Why E1's refutation is explained rather than worked around.** E1 found the
shade output changes on 1,796 of 1,835 frames. It does: the light direction is
transformed into each root's local space by that root's modelview, the fighter
animates, so every dot product changes every frame. It is unmemoisable for a
structural reason — and that is exactly the problem DS hardware solves, by
setting the light vector once in view space and applying the current matrix per
vertex in silicon.

**`GFX_LIGHT_VECTOR`, `GFX_LIGHT_COLOR`, `glLight` and `POLY_FORMAT_LIGHT` appear
nowhere in `src/nds` or `src/port`.** The renderer has never used DS hardware
lighting, while E14 measured the geometry engine idle on 946 of 946 fighter
submissions.

Design: pack normals into `GFX_NORMAL` words at load time; set light and material
per root (~28/frame) instead of per vertex (~534/frame), folding `color_modulate`
into the material; emit the precomputed normal word instead of the computed
colour word — **one FIFO word either way, traffic unchanged**. Expected: most of
90,295 ticks/frame.

**Prerequisite the design does not survive without.** The fighter loads an
identity projection plus the **CPU-composed MVP** as the modelview, through
`ndsRendererHardwareSetMatrixMode(GL_MODELVIEW)` — mode 1, position only, which
**never updates the vector matrix**. Normals are transformed by the vector
matrix, so a naive `GFX_NORMAL` would light against whatever was left there, and
loading the composed MVP into it instead is equally wrong because normals must
not be rotated by the projection.

Fix: load projection into `GL_PROJECTION` and modelview into
`GL_MODELVIEW_VECTOR` (mode 2). The plumbing exists —
`NDSRendererNativeFighterRoot` already carries both `composed_matrix` and
`modelview_matrix`, and only the projection needs adding. The row-3 unit scaling
commutes with the right-multiply by the projection, so split loading reproduces
the current transform exactly, modulo hardware-versus-CPU rounding. The light
vector is then written once per frame in view space while the vector matrix is
identity.

**SETTLED — the compose is deletable, so the matrix change ships first.** Traced
every `state->matrix` / `matrix_valid` reference in `nds_renderer.c`. Under
canonical mode 9 the composed matrix has exactly two consumers: the hardware load
at 23773 (the call being replaced) and a `matrix_valid == 0` **flag test** at
17594 in `ndsRendererNativePrepareProductionRun`. Every other reference —
`ndsRendererNativeLoadVertexBlock`'s CPU vertex transform via
`ndsRendererNativeApplyVertexActions` (sole call site 18838),
`ndsRendererComposeMatrix` via `ndsRendererNativeBindOwnerRootState` (18789),
both inside the non-production `ndsRendererExecuteNativeFighterRootHardware`;
plus hierarchy mode 7 at 19174/19473 and the generic DL interpreter — is
unreachable from `ndsRendererExecuteNativeFighterOwnerProduction`.

So `ndsRendererAdapterComposeNativeRootMatrix` can be deleted and a 4x4 multiply
per root leaves the 120,407 MatrixPrep bracket **independently of the lighting**.
The matrix change is therefore its own graduation, and the correct ordering is
matrix first, lighting on top — cheaper and far less risky than one four-part
change. The replacement must keep `state->matrix_valid` TRUE for the 17594 test
and carry a generation key equivalent to `state->matrix_generation` so the
existing hardware-matrix de-duplication still elides redundant loads.

**NOT IMPLEMENTED.** It touches the matrix mode, the load-time table format, the
emit's per-vertex word, and per-root light/material state. Being a rendering-side
change it gates on a screenshot pair plus **the owner's visual approval**: the DS
light model is not bit-identical to the N64's and colours will shift slightly.
`PROJECT_GOAL.md` lists "simplified lighting" among the allowed compromises, but
the call is the owner's.

Write-up: `docs/optimization/ClaudeOpus5_R203_E16_ShadeIsHardwareLighting_20260728.md`.

**Next: implement E16 behind a flag, capture the A/B screenshot pair, and put it
in front of the owner.**

### Open, not chased: GXSTAT bit 15 is set

Matrix stack overflow/underflow error latched at least once during a normal
match. It is a sticky flag and may date from init or teardown rather than
gameplay, and nothing observable is wrong. Recorded because it is an error bit
that is on.

## One fighter is worth ~400,000 ticks (owner observation, 2026-07-28)

**Superseded by the section above — measured at 271,424 WORK P50, not ~400,000.
Kept for the reasoning trail.** The inference below was sound but read the
quantization boundary as the whole cost; the actual transition needed less than
the boundary implied because `WAIT` absorbed part of it.


The owner noticed that knocking Fox off-screen, so he stops rendering, takes the
build to **~29 FPS** from ~20. That is not a small effect and it is arithmetically
informative.

The frame is VBlank-quantized at 560,190 ticks. Wall is **1,531,768** = 2.73
VBlanks, which rounds up to 3 → 20 FPS. Landing on 2 VBlanks needs wall
**≤ 1,120,380**, a saving of **~411,000**. Removing one fighter produced exactly
that transition, so **one fighter costs on the order of 400,000 ticks/frame** —
render, pose, matrices and everything downstream.

Two consequences.

**`PROJECT_GOAL.md` §7's budget table looks mis-proportioned.** It allots 250,000
to *combined* fighter rendering and 100,000 to fighter pose. Two fighters at
~400,000 each is ~800,000 of a 1,120,000 frame. Either the budget or the
implementation is wrong by a factor of two, and the budget was never validated
against a measured per-fighter cost.

**It is also a free instrument.** Suppressing one fighter's draw is a controlled
A/B that the tick-HUD reads directly, and it partitions the per-fighter cost into
render versus pose versus matrix without any new probe. Queued as the next
measurement after the R2-03 gate, reporting the 2/3/4/5+ VBlank histogram and max
interval per `AGENTS.md` — never min FPS.

Recorded as an owner observation, not a measurement: the FPS figure is a HUD
reading, and the ~411,000 is inferred from the quantization boundary rather than
bracketed.

**Outcome (E13).** Built as `NDS_R2_DRAW_SUPPRESS_MASK` and run. The observation
reproduces exactly — the median frame moves from three VBlank intervals to two,
and the 2-interval share goes 217/566 to 458/566 (histogram `2:458 3:102 4:5
5+:1`, max 17). The cost is **271,424 WORK P50**, not ~400,000.

The frame is also **CPU-bound, and this pair is what proves it**: `WAIT` went
*up* when Fox stopped drawing, 246,720 to 271,232. A rasterizer-bound frame that
loses a quarter of its pixel load waits less; a CPU-bound frame that loses
271,424 ticks of ARM9 work finishes earlier and waits longer.

## R2-03 E11/E12 — the fighter had no key for the cache that already existed

`ClaudeOpus5_R203_E11_PrepareRunSplit_20260728.md`,
`ClaudeOpus5_R203_E12_RunTextureMemo_20260728.md`.

**`PrepareProductionRun` 82,042 → 49,318 ticks/frame; the texture prepare inside
it 45,952 → 12,362.** Graduated to the published block.

E5 proved this function is a pure function of `run_index` and then declined to
build the memo because "~119 UV writes/frame can't explain 21,504 ticks". The
arithmetic was right and the premise was wrong: **a census row is self time.**
E5's bracket read ~21,500, the frame census row read 22,205, and four brackets
inside the function read **82,042** — the difference being the texture resolver
it calls out to, which the census charges separately to
`ResolveOrBindTexture` (18,803) and `SyncTextureTile` (12,004).

The cut is not a new mechanism. The resolver already opens with a site cache
keyed on `state->source_command_site`; the native fighter path does not
interpret display lists, so it has no site and has **never once hit that cache**.
The memo is the same cache re-keyed on `run_index`. R2-05 gets it for free.

| counter | value |
|---|---|
| memo hits | 1,074 (8.4/frame — every textured call) |
| fills | **9 in total, not per frame** |
| stale entries | 0 |
| mismatches, level 2 | **0 of 1,083** |

Nine distinct textured runs, resolved once each for the whole match. Predicted
35,000–45,000 in E11 before building; delivered **−32,724**, recorded as
measured rather than rounded into the band.

Three rules added to `TASK_STANDING_RULES.md`: check whether an instrument
measured the symbol or the work before rejecting a candidate as too small; ask
of every shared cache a native path inherits what its key is and whether this
caller has one; and a default-off `#if` does not hide a probe from a
source-level checker.

## R2-02 F — generic emit split, and the stage target moved

`ClaudeOpus5_R202_F_GenericEmitSplit_20260728.md`, `ea6b1fc`. The per-segment
counters existed and had never been read. **Segments 1/2/3/6 — Whispy's eyes and
mouth, both flower beds — cost 43,998 ticks/frame for 21 triangles**, against
segment 4's 22,843 for 76. At 2,095 ticks per triangle they, not segment 4, are
the largest remaining stage lever; the "segment 4 is the largest" line below is
superseded. R2-02's plan already named them ("small specialized update+draw
path") and that path was never built.

Three cuts refuted on the way, each with a number:

| candidate | measurement | verdict |
|---|---|---|
| merge adjacent runs | 1.0 of 21 repeats the previous state; 18 rebind a texture | dead, ~1,200 |
| revive Task 51 | 0.0 triangles take the path, 1,634 ticks/frame failing, emit +4,754 | structurally dead |
| guard the texture bind | 21,978/frame over 54 runs, both guards already present | near the floor |

Task 51's 2026-07-23 kill named its own revisit condition — find a scene where
bindings 20–29/33–38 submit GX — and that condition is now met. It still fails,
for a *different* reason: `Task51EnsureWorld` rejects on `task36_segment_active`,
and only a rigid binding opens that bracket, which an actor segment does not
have. Pinned so the next reader does not repeat the three builds.

The texture-bind floor caps the actor rewrite near **30,000**, not 44,000.

## R2-03 E5 — the premise is proven, the obvious cut is not worth building

Three counters over one canonical match settle whether R2-03's baked-facts
submit is possible (`ClaudeOpus5_R203_E5_RunFactMemo_20260728.md`,
`de34e051181`, `12968f83dd2`, `fad10d4cf91`):

| question | measurement | answer |
|---|---|---|
| do a run's facts ever change? | 0 misses / 112,300 calls | no |
| does the function ever reject? | entry 112,367 == success 112,367 | no |
| does a UV write ever change anything? | 0 changes / 208,874 writes | no |

`ndsRendererNativePrepareProductionRun` is a pure function of `run_index` in the
canonical configuration. The switch plan's "consuming only baked facts, no
policy re-checks, no per-frame texture identity proof" is achievable, and the
table can be generated rather than discovered.

**But the obvious implementation banks nothing.** The UV loop is only ~119
writes/frame — about 13 of the 67 runs are textured, touching 106 of 541 dense
vertices — which is low thousands of ticks against the bucket's 21,504. The cost
is spread across per-call validation and `texture_prepare_*` bookkeeping, and
`texture_prepare_valid` is already a cache with 44 invalidation sites. Building
a memo for the arithmetic was dropped on the measurement rather than attempted.

Next on this phase is an internal cost split of the function, not an
implementation. Two side effects any memo must preserve are recorded in §4d of
the writeup: the GX bind at `:17313` and the harness-visible texture-prepare
counters.

**The bigger fighter lever is MatrixPrep at 91,338/frame** — four times this
bucket, moving every frame, and where the bulk of the ~460K gap to the 1.12M
gate has to come from.

## Runtime 2 (2026-07-27)

The owner approved `Smash64DS_Runtime2_SwitchPlan.md` and it is now the live
renderer direction; `optimization/archive/NATIVE_RENDERER_PLAN.md` is history.
R2 phases are rows here, measured under `TASK_STANDING_RULES.md`.

| phase | state | evidence |
|---|---|---|
| R2-00a stall attributor | **done, gate met** | `optimization/ClaudeOpus5_R200a_StallAttributor_20260727.md` |
| R2-00b re-baseline + budgets | **done** | `optimization/ClaudeOpus5_R200b_BaselineAndBudgets_20260727.md` |
| R2-01 battle-path skeleton | **done, gate met** | `NDS_R2_PATH`, `src/nds/r2/`; Boundary green |
| R2-02 Dream Land direct runtime | **stage budget MET — 177,088 vs 180,000; E1a/E2/E7/E8 shipping** | `optimization/ClaudeOpus5_R202_E8_PreflightElision_20260728.md` |
| R2-03 fighter direct draw | **unowned — not started** | |
| (R1 harvest) hardware sqrt | done, KEEP | `optimization/ClaudeOpus5_R203_E1_HardwareSqrt_20260728.md` (filename mislabels it R2-03) |

**R2-02's stage budget is MET.** `STG` P50 is **177,088** against the 180,000
provisional budget — 2,912 under — after E1a, E2, E7 and E8. E3 is retracted and
E4 refuted its whole approach; neither contributed. The two soft-float files
named `R2-03` are Runtime 1 harvest, not that phase; they are corrected in place
per the never-rename rule.

```text
STG P50   351,488  baseline
          256,704  after E1a  (-94,784)  prepare-run elision      -- clean
          224,320  after E2   (-30,912)  GXFIFO DMA rigid replay  -- clean
          212,480  after E7   (-11,840)  view-projection hoist    -- bit-exact
          177,088  after E8   (-35,392)  preflight elision        -- bit-exact
          180,000  budget
          -------
           -2,912  UNDER

         (173,120  E3 and E4-C both  (-51,200)  BOTH REVERTED: that number is
                                                the price of not drawing the
                                                flowers, not of drawing them
                                                faster)
```

**E8 is the first arm that followed §7 rather than optimising around it.** For
the five segments the Task 36 replay does not serve, the owner preflight cleared
a 1,292-byte `NDSRendererStats`, initialised a traversal state, and replayed 21
run-level and 16 binding-level state spans to produce a `preflight_stats` and a
traversal state that **nothing reads** once E1a's prepared run table is valid:
`CapturePreparedSegment` early-returns for an ineligible segment, and
`sNdsNativeStageOwnerExecution.traversal` is referenced nowhere outside the
function. The one member that escapes the loop, `sync_command_count`, is now
memoised beside `epoch_mask`. Task 104 had written that sentence down already,
one level lower and for three segments; it was true of the other five and of the
whole loop body. Engagement reads exactly **5 elisions per frame**, and the Task
36 replay stays READY at its full 3,916 words.

Pacing: **2-VBlank frames 13 → 198 of 565**, `WAIT` P50 −202,368, `WORK` P95
−77,504. The DS top screen is **pixel-identical** to the pre-E8 arm — 0 of
121,600 pixels — at presented frame 500 and at the `time_remain` 1800
simulation-clock lock, against a control arm proven reproducible run-to-run.

**All three kept cuts now ship.** `NDS_R2_STAGE_DIRECT`, `NDS_R2_STAGE_DMA` and
`NDS_R2_STAGE_VIEWPROJ` are default-on in the published
`smash64ds-battle-playable-hwtri` block and in the `tickhud`/`proof` block —
`STG` P50 **351,488 → 212,480, −40%**, and the frame moves off the 3 VBlanks the
previous shipping ROM sat at. None of the three spends the fidelity budget, so
none of them needed the owner's visual-oracle call: that clause governs
approximations, and these are exactness-preserving. E7 is bit-identical to its
control on all 42 composed matrices at six frames spanning the camera's range of
motion. The tick-HUD block sets the three *without* `override`, deliberately —
they are the live A/B surface for the rest of the phase — and the graduated
default tick-HUD build hashes `DFBE1ED0E2BB97DB`, byte-identical to the explicit
lab build, so measurement and shipping are the same binary.

**E7 also corrected a wrong rationale that had already been written down twice.**
The cut was designed as an associativity hoist that would spend the Task 49
Tier-2 pixel budget. Dumping `binding_composed` out of both ROMs showed no delta
at all: `ndsRendererAdapterBuildCameraMatrices` already returns
`projection = MtxMul(lookat, persp)` with `modelview_valid` FALSE for the battle
camera, so the compose was `world × (lookat × persp)` — **one multiply per
binding, never two** — and the −11,840 is the per-binding camera-cache lookup and
three 64-byte `MTXCOPY` memcpys that stopped happening. Both E6 and E7 were
designed against arithmetic and both resolved to memory traffic. **Do not size
the next stage matrix lever by counting multiplies.**

**The mechanism, established by E4** —
`optimization/ClaudeOpus5_R202_E4_ActorSegmentsRefuted_20260728.md`. A **rigid**
binding's captured stream is `PUSH` + `MULT4x4` of a constant world under the
camera the segment bracket loads live every frame, so it replays. A **dynamic**
binding's stream is a `MATRIX_LOAD4x4` per triangle of projection × view × model,
so replaying it pins that geometry to the camera the capture frame happened to
have — which is exactly why the flowers sat in a fixed screen band under every
camera. Hence the invariant, now written into both masks:

> `NDS_TASK36_REPLAY_SEGMENT_MASK` must name exactly the segments whose every
> binding is in `NDS_RENDERER_TASK36_RIGID_BINDING_MASK`.

E3 broke it by widening one mask. E4 arm C restored it by widening both — and
lost the flower beds anyway, for an unrelated reason: **the rigid emit path is
single-binding by construction.** `ndsRendererNativeStageEmitNoZTriangle` drops
a triangle whose corners are not all bound to the run's own binding, and the two
flower beds are the only cross-matrix geometry on Dream Land — 10 of their 15
triangles. That is the `cross_matrix_triangles=10` that
`M3_NATIVE_STAGE_CHECK_OK` prints on every Boundary run, and it had been on
screen the whole time.

It is also why the flowers are expensive: a cross-matrix triangle falls to the
generic tail, which loads a composed matrix **once per vertex**. 15 flower
triangles cost 35 matrix loads a frame; Whispy's 12 single-binding triangles cost
12.

Two hypotheses died cheaply on the host and should have died before E3 landed:
every actor triangle carries coordinate shift 0 (so Task 51's missing shift
compensation is irrelevant), and `NDS_TASK51_STAGE_NATIVE` defaults to 0 and is
compiled out of every ROM measured (so E3's premise — "Task 51 already baked
those world matrices" — was false).

**Nothing shipped.** `NDS_R2_STAGE_ACTORS` is deleted. The published ROMs are at
defaults and Boundary-green at **62.750%**, `stage_body` green 44.848% / detail
52.242%. One real defect was found and kept: replay asserted
`task36_local_pushed = TRUE` for every run, so each admitted actor segment bought
an unmatched `glPopMatrix(1)`. Capture now records the run's actual `PUSH`/`POP`
balance.

**The stage partition, re-measured on the graduated program 2026-07-28**
(`census-stage-run-phases.ps1`, frames 439–499, `build-r2-02-census-e7`, total
242,574). This is what E8 was aimed from, and what the *next* stage arm must be
aimed from — the majority is no longer preflight:

```text
prepare owner                 111,849   46.1%
  prepare matrices             42,557          (54,901 before E7)
  renderer prepare owner        49,840
    apply state span             20,370   21 calls @   970   <- E8 elides
    init stats + traversal       13,565    5 calls @ 2,713   <- E8 elides
    unattributed                 13,721          (16 binding-level state spans)
    prepare run                     995   21 calls @    47   (E1a: was 98,828)
  validate task36 world          8,588
  prepare materials              5,623
display commit                130,219   53.7%
  generic emit                   67,126   21 runs @ 3,196, 103 tris @ 652
  replay                         29,124   33 runs @   883
  loop overhead                  13,120   54 iterations
  per-segment scaffolding        13,852    8 commits @ 1,732
```

**The next stage lever is `generic emit`, 67,126 ticks/frame** — the 21 runs and
103 triangles the Task 36 replay does not serve, at 3,196 per run against the
replay's 883 and 652 per triangle against ~294. E4 established it cannot be
reached by widening the replay masks. layer1 (segment 4) is 76 of those 103
triangles across only 6 of the 21 runs, so the cost is per-run dominated and the
15 actor-segment runs are the expensive half.

The older defaults-build partition below (total 401,506) is retained only as the
pre-E1a reference; do not aim new work from it.

```text
prepare owner (preflight)     238,609   59.4%
  renderer prepare owner        165,045
    prepare run                  98,828   21 calls @ 4,706  <- E1a takes this
      head policy/memset/tex       69,379
      dense vertex loop            22,339   143 dense @ 156
    apply state span             30,117   21 calls @ 1,434  <- NOT elided by E1a
    init stats + traversal       16,793    5 calls @ 3,359  <- 1,292-byte clear
    task36 reuse check              693
    validate topology               610
    unattributed                 18,004
  prepare matrices               54,242   16 dynamic bindings @ ~3,390
  validate task36 world           8,577
  prepare materials               5,675
  config / frame setup            2,523
display commit (actual submit) 162,399   40.4%
finish owner                       498
```

**Read this against §7's actual instruction, which has not been followed.**
R2-02 says the static majority becomes *"a fully direct owned path: no generic
preflight, no stats temporaries, no per-frame texture resolution; the runtime
shape is `DreamLand_Run17()`, not discover/validate/rebuild/resolve/prepare/
submit"*. E1a, E2, E3, E4 and E5 all optimised the discover/validate/prepare
pipeline instead of replacing it. Segment 0 already has the prescribed shape —
`ndsRendererNativeStagePrepareGeneratedSegment0`, gated by
`NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE`. Segments 1–7 do not. **Extending
that generated program to the remaining segments is the phase's own design and
is the credible lever §8 asks for before any budget is relaxed.**

Ranked by size, and all of it is preflight the direct path deletes rather than
optimises:

1. `prepare matrices` **54,242** — `ndsRendererAdapterPrepareInitialMatrices`
   walks each dynamic binding's DObj parent chain every frame. The flowers'
   worlds are provably constant (E4 arm C: the runtime rigid-constancy check
   accepted them), so this is recomputing a known-constant world and then
   composing the camera onto it. Splitting camera from world is what Task 36
   already does for the 26 rigid bindings via `MULT4x4` under a once-loaded
   camera. Largest single item and the most clearly structural.
2. `apply state span` **30,117** — 21 calls E1a's `r2_reuse` memo does not
   cover, because it sits outside that guard. Careful: it mutates the running
   `state` that later runs consume, so it cannot be skipped per-run without
   also proving the successor's incoming state.
3. `init stats + traversal` **16,793** — a 1,292-byte blanket clear plus
   traversal init, 5× a frame, for the five segments Task 104's elision does not
   reach. §3.4 names this shape explicitly; extend Task 104's pattern.
4. `unattributed` **18,004** inside the owner span — uncensused, and bigger than
   item 3. Bracket it before assuming it is small.

Items 1–3 total 101,152 against a 44,320 requirement, so the budget is reachable
without relaxing it — the work is structural, not another memo.

**Also on this row — de-cross the flowers in the generator.** For each of the 15
foreign corners emit a duplicate dense vertex pre-transformed into the run's
binding space (`v' = W_run⁻¹ · W_foreign · v`, a compile-time transform because
both worlds are constant). That is +15 dense vertices, no new runs or triangles,
and it makes every flower triangle single-binding. Only then widen
`NDS_RENDERER_TASK36_RIGID_BINDING_MASK` and `NDS_TASK36_REPLAY_SEGMENT_MASK`
together; gate the transform on the Task 49 Tier-2 differ (the inverse-multiply
is where fixed-point error enters); verify with a frame-locked crop of segments 3
and 6 plus a triangle count and `task36_runtime_rigid_mask` read from the run
that produced the buckets. Whispy (20–24) is out of scope — materially animated,
and at 12 single-binding triangles it was never the expensive half.

**Closed 2026-07-28: the R2-02 flags are graduated and the published ROMs carry
them.** This row previously asked the owner to make that call. It was the wrong
ask — the owner is the visual oracle for changes that *spend the fidelity
budget*, and all three of these are exactness-preserving, so the decision was
the verifier's and not a matter of taste.

**What else is left in the stage.** layer1 (segment 4) is 22,738 ticks/frame for
76 triangles and is still generic: its six runs submit through the raw composed
matrix (binding 29, submit classes 0 and 6), which is the camera and genuinely
moves. Moving it onto the segment-bracket path is generator work worth ~19,000 —
now the *largest* remaining stage lever, and still not enough for the gate alone.

**R2-03 is owned; E0–E3 are done and the target is named.** The frame is
re-baselined on the post-R2-02 program: **REAL WORK 1,264,844 against the
1,120,000 budget, gap 144,844** (it was 407,000 at Task 65).

**The sizing is finished and the target is one span: fighter matrix
preparation, 91,338 ticks/frame — 63% of the gap.**
`ndsFighterMarioFoxDLAllDrawForSlot` costs 497,231 ticks/frame inclusive (the
census's 37,206 is self time); the split is walk 3,138, reset 6,675,
revalidation 9,916, owner prep 113,855 (**matrix 91,338** + material 21,504),
submit and tail 361,936. Two independent builds agree to 0.6%.

Three named mechanisms cover 93% of the gap: fighter matrix prep 91,338,
fighter material prep 21,504, stage layer1 22,738. The matrix half is not a
memo — the pose moves every frame — it is §7's generated per-epoch submit
consuming baked facts, and soft-float's 177,503 is largely the same ticks
counted another way (the source's joint transforms are float; the render side
converts them to 20.12 every frame). The material half *may* be a memo and
deserves the E3 falsifier first, at one build for ~21,500.
`optimization/ClaudeOpus5_R203_E4_MatrixPrepIsTheTarget_20260728.md`.

Two candidates were closed on the way, both with evidence rather than opinion:
the shade loop is **not** a memo (inputs and outputs both changed on 1,796 of
1,835 frames), and walk + revalidation is 4% of the function, not the 37% a
self-time-versus-inclusive mix-up made it look like.

**R2-03 E1 took `sqrtf` from 15,760 to 9,720 ticks/frame, −6,040**, bit-exact
against IEEE over 8.7M checked inputs, Boundary green. The 8-frame A/B read
**flat on every bucket** — the saving sits inside the 5,000–7,000 placement
floor, and the symbol census is what resolved it. That constructive half is now
in `TASK_STANDING_RULES.md`: when the predicted saving is near the floor, gate
on the census, which times the function directly and has no placement term.

Only 38% though, not the 17× the hardware's 13-cycle latency suggests: libnds's
`sqrt64` is write / **poll-busy** / write / **poll-busy** / read, and the I/O
polling costs about what the software root did. **On this hardware a coprocessor
is only worth it if the result can be collected without spinning on it.**

**R2-02 E1a took `STG` P50 −94,784, down on 128/128 frames**, and 4-VBlank
frames fell 50 → 12 out of 566. Boundary green; required-region detail 62.792%
vs 62.778%. `NDS_R2_STAGE_DIRECT`, default 0, owner visual approval outstanding.
`STG` is now 256,704 against the frozen 180K budget — gap 76,704, was 171,488.
E2 is `ndsRendererAdapterPrepareNativeStageMatrices` (55,077 bracketed), which
is **not** frame-invariant: the camera moves, so a reuse key will not work
there and E0's sizing method does not transfer.

### CLOSED — the gate metric is sound (R2-00c, 2026-07-27)

**R2-00a's phantom-work finding is refuted, and the row it opened is closed.**
It compared halt measured in a profile ROM against `WAIT` measured in a
different tick-HUD ROM; placement differs between builds, so a frame index does
not name the same workload in both. One ROM carrying both instruments
(`NDS_TASK37_PROFILE_PER_FRAME_REGION=1`, new) settles it over 128 frames of one
run: `ALL` agrees to **0.04%**, `WAIT` to a constant **−851 ticks/frame**, and
the 27 excursion frames (median −860) are no different from the other 100
(median −847). `WORK-H` P95 is not inflated by a mis-scoped bracket; the 1.12M
gap is real. Evidence:
`optimization/ClaudeOpus5_R200c_WaitBracketAudit_20260727.md`.

**What replaced it is a real optimization row.** The excursion is genuine
execution — `armWaitForIrq` falls 323,450 ticks/frame and **+286,619** of work
takes its place, on 21% of frames — and the same per-frame regions attribute it:
softfloat ~49,600, **the tick HUD measuring itself ~44,300**, cart read +
relocation + bulk copy ~36,000, geometry submission ~14,500, collision ~5,700,
animation ~2,700, then a diffuse tail over ~59,000 PCs. Four unrelated causes on
the same frames, which is why five previous tasks found no single mechanism.

Two consequences worth acting on:

- **The frames are not load-free.** `_ntrcardRecvByCpu` + `ntrcardRomRead` are
  12,639 ticks/frame higher there. Task 75's preload targets something real, but
  its ~103,488 estimate must be re-derived against the measured ~36,000.
- **`WORK-H` cannot remove all of the instrument.** `ndsPlatformTickHudSample()`
  runs after the buckets are latched, so the percentile sort (19,605
  ticks/frame) lands in the *next* frame's `ALL`. ~2% of the P95, not 33%, but
  it is the metric charging the ROM for being measured.

R2-00a's other findings stand: no GX, DMA or cart *stall*; ledger closed;
bit-identical reproduction of the prior census.

### The frame, re-ranked on attribution that holds (R2-00c §7)

`task65_subsystem_census.py` named functions with `addr2line -f`, which resolves
through DWARF — and DWARF still describes functions the linker
garbage-collected. It charged 24,240 ticks/frame to `ndsRendererTask29GXRecord`,
which is not in the binary. The census now bisects the ELF symbol table and
overrides addr2line; that **renames 18,987 of 59,366 PCs, 32%**. Aggregates
survive (REAL WORK 1,446,638 vs R2-00b's 1,446,348, 0.02%); the per-symbol table
did not, and that is what targets are picked from.

| group | ticks/frame | % of work | cyc/insn |
|---|---|---|---|
| soft-float | **177,857** | **12.3%** | 1.19 |
| matrix | **156,627** | **10.8%** | 2.35 |
| gx-submit | 144,852 | 10.0% | 2.72 |
| texture-resolve | 108,681 | 7.5% | 4.91 |
| `mem*` | 98,207 | 6.8% | 2.60 |

**Soft-float is the largest block and it is not stalled** — 1.19 cyc/insn, and
`__aeabi_fadd` is already hand-written ITCM assembly. Nothing to win by making
it faster; the only lever is calling it less, i.e. float→fixed at the call sites
in imported gameplay and animation. **Matrix construction is 156,627, not the
55,077 R2-02 E2 was sized at** — the bracket saw one call, the census sees seven
symbols across stage and fighter. Re-scope E2 against that.

The attributor is installed repo-local at
`emulators/melonds-attributor/melonDS.exe` (`D81FC0BF…`) rather than replacing
`emulators/melonds/melonDS.exe`, so measurements taken with `DE80E46B…` stay
comparable. `check-melonds-policy.ps1` passes with it present.

**R2-00b replaced the stale Task 65 baseline.** REAL WORK is **1,446,348**
ticks/frame, not 1,527,277; the gap to the 1.12M gate is **326,348**, not
407,277. Stall is 62.1% of work (memory 555,943, non-memory 342,494), so the
architectural premise is unchanged — memory stall alone still exceeds the whole
gap.

It also corrected an attribution defect Task 65 shipped: `task65_subsystem_census.py`
filed `src/port/reloc_backend_renderer_dl.c` under `PORT/reloc`, charging
**147,777 ticks/frame of renderer adapter work to a bucket named after
loading.** Corrected, **the renderer is 723,554 ticks/frame — 50.0% of the
frame's work** — and all gameplay is 190,649 (13.2%). Any plan built on Task
65's §2 table under-counted the renderer by that amount.

Note for every future phase gate: the census attributes by where code lives and
the tick-HUD buckets attribute by bracket. They are not interchangeable, and the
shared kernels (616,701 ticks/frame) are what differ between them. State which
view a gate quotes.

## Red Queue

1. **Stable 30 FPS:** qualify representative active gameplay at
   P95 <= 1.12M ARM9 ticks per presented frame on the accuracy-focused custom
   melonDS fork. Hardware remains the final check for mechanisms the emulator
   cannot referee.
2. **Mario/Fox completeness:** replace battle-reachable weak status callbacks
   with source-backed behavior and prove both complete movesets naturally.
3. **Dream Land completeness:** close the remaining Whispy material/animation
   presentation debt without reintroducing gameplay-time texture conversion.
4. **Audio completeness:** implement or explicitly qualify every reachable
   voice, pitch schedule, composite cue, and overlapping match-audio path.
5. **Final acceptance:** run the CPU-on one-minute match, complete-match capture,
   owner play/listen pass, reserve gate, Results transition, and teardown proof
   on the exact candidate ROM.

**Performance lane (2026-07-28):** `WORK-H` P95 **1,579,584** after R2-02 E8,
against the 1,120,000 gate — gap **459,584**. (It was 1,647,424 after Task 104;
E7 and E8 took the rest.) `WORK` P50 is 1,163,328 and P95 1,592,320. VBlank
intervals 2:198 3:349 4:14 5+:4 of 565, max 18 — the median frame is still three
intervals, but 35% now present in two where 2% did before Runtime 2. Two search spaces are closed by measurement — exactness-preserving
(Tasks 78–96) and visual approximation in its payload form (Tasks 98–99). The
raster axis was opened in `optimization/RASTER_AXIS_CAMPAIGN.md` and **Task 100
closed it at the first test** — a quarter of the frame's pixels stopped being
drawn and `STG` moved −320 against a ≥40,000 criterion, for the architectural
reason that the DS rasterizer consumes already-swapped polygon RAM and cannot
stall the CPU. Pixels join words and triangles; do not propose another fill,
coverage, AA or overdraw lever.

**Task 103 ran and moved the lane.** Partitioning `STG` in place found that
Tasks 51–55, 99 and 100 all worked the run loop, which is only 35% of the
bucket; **61% (238,254 ticks/frame) is outside the segment commit entirely, in
the owner prepare path, and has never been profiled.** It also found the 21
generic runs the Task 36 replay does not serve cost 63,903 ticks for 103
triangles, and that GX words cost 9.51 ticks each — retiring Task 55 E2's "words
are free" as a below-noise null.

E3/E4 then closed the attribution exactly — all four writers of
`gNdsTickHudStageTicks` tapped with zero added instrument, partition closing to
192 ticks (0.05%) against the build's own `STG`.

**Task 104 took the first cut out of it — KEEP, default on, Boundary green.**
On each of the three Task 36 replay-hit segments the owner cleared a 1,292-byte
`NDSRendererStats` and then overwrote all 1,292 bytes with a copy, to transport
**four live bytes** (`sync_command_count`, the only member read after the segment
loop). Eliding both accesses: `STG` P50 **−22,016**, `WORK-H` P50 **−26,240**,
P95 **−28,352**, VBlank 4-interval **39 → 28**, `FTR` flat. `WORK-H` P95 is now
**1,647,424**. Detail in `optimization/ClaudeOpus5_Task104_FourLiveBytes_20260727.md`.

That result also explains Task 103 E7's 28% realisation and produced a standing
rule: **size a memory lever by bytes that stop being touched, not instructions
that stop executing** — removing one of two accesses to the same cache lines
relocates the misses rather than eliminating them.

**Task 105 then closed the rest of that axis at E0, for one census run and no
builds.** `memset`'s residue is ~16,018 ticks split five ways (Task 84 E1.3
priced `InitStats` at 72% of the family's time), and a re-attributed `memcpy`
census found ~294 matrix copies/frame across five sites worth 3,300–10,600
nominal each — every one discounting to 1,000–3,000 under Task 104's own rule,
below the floor. Two rows in that census are inlined-range artifacts and are
marked as such. **The memory-traffic axis is harvested;** the residue is
structural, in `NDSRendererMatrix20p12` being 4×4/64 B for affine transforms the
DS loads as 4×3/48 B, and is not worth a Runtime 1 refactor.

**Task 106/107 E0 then sized the last untested large lever and re-aimed the
lane.** A 30 Hz simulation (`NDS_TASK106_UPDATES_PER_PRESENT=1`, default 2,
nothing shipped) is worth `WORK-H` P50 −158,592, taking the median to
**1,119,616 — 384 ticks under the gate**. But `WORK-H` P95 falls only −119,744,
because the `SRC` excursion above its own median is **+518,016 on the control
and +522,720 on the candidate — unchanged**. Halving the update rate halves
median `SRC` and leaves its tail intact: the excursion is asset loading driven
by animation events, and fewer update ticks do not reduce how many distinct
animations a match loads.

**The gate is a tail statistic, and Task 75 E0 has now measured what owns it.**
A load counter at `ndsRelocFinalizeLoadedFile`, ringed per frame, discharges
Task 71 §5's obligation — and answers it **no**. All 5 load frames in the window
are `SRC` excursions, so a load is *sufficient*; but **2 of the 7 excursions
carry no cartridge activity at all** (frames 453 and 454, at 2.0× and 1.9× the
`SRC` median), so a load is *not necessary*. The counter cross-validates against
the independent native-owner counter exactly (7 loads, `animLoad:7`).

Sized against the distribution rather than one frame: `WORK-H` P95 is 1,656,896
over all frames and **1,553,408 over load-free frames only**, so eliminating
on-demand loading is worth **~103,488** — 19% of the 536,896 gap, against Task 71
§5's extrapolated ~170,000. And the resulting P95 would be frame 454, a load-free
excursion, so the preload buys 103,488 and hands the gate to an unidentified
cause.

**Highest-value unowned row: profile a load-free `SRC` excursion.** Frame 453 —
single-frame spike, `SRC` 636,096, zero loads, no fallback, `FTR`/`STG` at
median. Task 71's per-PC census windowed on the frame is the instrument; its own
window (469–470) contained a load, so this population has never been profiled.
Whether the residual shares a cause with the loads (relocation, figatree parse)
decides whether one fix serves both and whether the preload's ceiling is higher
than 103,488. Row 51's preload bridge is real but must not start as a subsystem
against 19% of the gap.

Stage levers, still unowned, now second in priority:

1. **The `PrepareRun` head — 67,119 ticks/frame over 21 calls**, the largest
   block inside `ndsRendererPrepareNativeStageOwner` (now ~138,600 after Task
   104) that nothing has attacked. Long span, so the sizing is trustworthy.
   Task 81's closed stage memo does **not** cover it: that was a texture-identity
   memo at the bind seam, and Task 81 measured zero stage texture binds in
   battle. **Highest-value unowned row on the board.**
2. **`ndsRendererAdapterPrepareNativeStageMatrices` — 55,077 ticks/frame at one
   call per frame.** Never profiled; same in-place span method.
3. **Bring the 21 generic stage runs under the Task 36 replay** — 63,607
   ticks/frame for 103 triangles, less the replay's own ~1,785/run. Note this
   cannot be done by widening `NDS_TASK36_REPLAY_SEGMENT_MASK`, which would
   freeze dynamic stage geometry; mode 2 replays complete rigid segments only.

(1) and (2) are per-frame preparation over a topology Task 44 has already proven
unchanged, which is the shape an incremental update attacks. With one call per
frame there is no per-run transfer problem of the kind that killed Task 79 E1.

Task 62's reduced DS-native static mesh remains a **REVERT**. A source-exact
follow-up now preserves material/UV/color/alpha and matches the flag-0 top
screen pixel-for-pixel, but submits the same 525 static vertices. The reduced
candidates have no run/material provenance, so the corrected Task 60/61 gates
recommend none. Keep `NDS_DREAMLAND_DS_MESH=0`; details and the earlier
CPU/GX reduction remain rejected-experiment evidence in
`optimization/archive/Task62_AB_Results.md`.

## Lane Ownership

| Surface | Owner |
|---|---|
| Goal, fidelity, milestone, definition of done | `PROJECT_GOAL.md` |
| Dynamic queue, artifact identity, blockers | this file |
| Exact restart surface and next packet | `HANDOFF.md` |
| Stable architecture | `ARCHITECTURE.md` |
| Verification workflow | `VERIFYING.md` |
| Durable unresolved gaps | `KNOWN_ISSUES.md` |
| Measurements and rejected experiments | `PERF_LEDGER.md` |
| Chronological history | `PORTING.md` |

The current dirty Task 62 follow-up/runtime files are user-owned. Preserve them;
do not infer qualification or overwrite them during documentation cleanup.

## Acceptance Matrix

| Acceptance condition | State | Current evidence / blocker |
|---|---|---|
| Mario human vs original level-3 Fox CPU, Dream Land, one-minute Time, items off | Pass configuration | Boundary registry exposes only canonical mode 163 |
| Original Wait -> countdown -> GO, timer, scoring, Time Up, Results | Focused gates pass | Final exact-ROM CPU-on owner run remains red |
| Mario and Fox complete source-equivalent gameplay behavior | Red | Battle-reachable weak callbacks remain |
| Dream Land collision, platforms, blast zones, wind, camera | Pass for current P1 stage | Dynamic presentation debt remains red separately |
| Recognizable Dream Land presentation and required animation | Red | Whispy material/animation debt; Task 62 candidate rejected |
| Complete overlapping BGM, FGM, voices, announcer, crowd | Red | Exact pitch/composite/voice coverage and listen gates remain |
| Stable 30 FPS, representative P95 <= 1.12M ticks | Red | No current qualifying full-match result |
| Stable reserve, no corruption, clean teardown | Focused gates pass | Requalify after the final content/performance candidate |
| Reproducible public artifact | Red | Current local root ROM differs from the pinned public identity |

## Integration Rule

Keep only correctness-preserving, verifier-covered progress. Rendering may use
the fidelity budget in `PROJECT_GOAL.md`; gameplay must remain mechanically
equivalent to the original. Run the smallest relevant check, then one widest
relevant verifier for a kept checkpoint.
