# Bug investigation notes (P2 repair queue)

Working notes behind `docs/BUGS.md`, which stays the lean owner-facing queue
(`docs/BUG_FIXING_PROCESS.md`). One entry per queue row that has evidence
worth keeping; append, do not rewrite history.

## CURRENT FIGURES -- read this before quoting any number below

This file is append-only, so a superseded figure stays where it was written and a
reader who greps finds the **oldest** occurrence first. Four recorded
measurements went stale on 2026-09-10 and at least one agent quoted a superseded
one straight out of this file. The current values are here; everything below is
history, correct as of its own date and not necessarily now.

| Quantity | Current | Superseded | Where corrected |
|---|---|---|---|
| Worst four-fighter pack set | **399,416 B raw / 371,444 B VRAM-bound** | 506,636 | Kirby hat deferral + foreign-model object liveness |
| Pack gate shortfall | **>=80,176 B on current shell direct bound**; exact ceiling still unknown | 331,032 | VRAM-bound lower endpoint vs relaxed 291,268 B ceiling |
| Largest remaining pack lever | **zero** (low-only is dead) | 112,388 | "The largest remaining pack lever is worth zero" |
| Four-fighter startup deficit | **CLOSED locally** -- frame 64 reached, four pose slots bound | 2,348 B at `ndsFtPoseOpen`; ~20 B at the player tag | "Four-fighter startup and frame-45 latch are closed locally" |
| Particle atlas | **32,768 / 32,768, full**; 5 excluded need 5,120 B | 31,872 / 32,768, 12 unadmitted | "Nine figures that disagreed" |
| ITCM free | **392 B**, measured from a linked ELF | 864 B, then "zero free" | "ITCM has 392 bytes free" |
| Character-select worst frame | **4,409,600 ticks** (slice, uncommitted) | 11,701,888 / 10,712,832 | "8 KiB / 4-node slice verification" |
| Item draw owners | **26 of 45** | 21, 25 | Capsule, `5bf2e7088f7` |
| Frame-45 latch deficit | **CLOSED locally** -- 53,128 B free-min, cap stays -1 through frame 64 | 13,436 B | "Four-fighter startup and frame-45 latch are closed locally" |

Two figures that are not contradictions and need their qualifier every time:
Kirby's bytes differ by what is being measured (image census, post-deferral base
plus reserve, profile-A W before and after), and `WORK-H P95` has three published
values from three configurations.


## The four-fighter pack gate is RED, and the verdict is a floor (2026-09-09 night)

The pack estimator (`scripts/fighters/estimate_fighter_pack.py`) had never run.
It refused on an unhandled guard expression, then on a missing element layout —
both fail-closed by design, both real gaps. Repaired and run:

    closable fighters today: 12 -> 793 one-through-four-kind sets
    worst set (any size)      Captain+Link+Pikachu+Kirby   W_A_worst = 506,636 B
    worst under vram bound    Donkey+Captain+Link+Kirby    W         = 494,610 B
    worst with raw motions    Captain+Pikachu+Ness+Kirby   W_B_worst = 2,129,593 B
    verdict: RED

Full output: `builds/resume-20260905/agents-0906/scratch/estimator_hwtri_full.txt`.

**The verdict does not depend on the two unknown terms**, which is the part worth
recording. `F_NEW_BASE = 208372` (`estimate_fighter_pack.py:2468`, "verified
constants from the review section 6.1; do not re-derive"), `FLOOR_BYTES = 32768`,
so `W_CEILING = 175,604`. `D_other` and `D_binder` are *deductions* the tool
explicitly refuses to zero (`:4117`). At the optimistic reading — both zero — the
allowance is 175,604 against a worst set of 506,636. **It fails by 331,032 B at
the most generous possible value, and any real deduction makes it worse.** This is
a floor, not a direction.

### Do not conflate this with the frame-45 crash

The four-fighter match NULL-dereferences at frame 45 with 12,164 B free against
the 25,600 B GObj latch — a deficit of **13,436 B**, which S2 + 4a clear with about
5,000 B to spare. That is a different and much smaller constraint. An earlier note
of mine called S2 + 4a "sufficient with margin"; that was true of the latch and
false of the pack gate, and the two were quoted as one number.

### The levers, and the gap they leave

    7.1 bank membership   256 banks / 57,060 B unresolved
    7.2 weapon natives    334 objects / 56,152 B with no native owner
    7.3 u32 by readers    1,942 objects / 135,388 B unresolved
                          (1,924 of them WITH identified readers)

Roughly 248 KB identified against a shortfall of at least 331 KB. **Necessary and,
on current numbers, not sufficient.** `PROJECT_GOAL.md` allows heavy load-time
preparation, precomputation, compile-time conversion and aggressive baking, and
treats loading time as cheap — that is where the remainder has to come from.
Reducing content is not the sacrifice space.

## Collision parity passes on all nine stages (2026-09-09 night)

`P2-4-stage-production.md:201-210` recorded that no collision parity sweep existed
and no verifier covered a flagged stage — so collision was *unknown* on eight of
nine, which is weaker than known-bad.

It needed no ROM. The port loads each stage's collision integers verbatim from the
same source data and reads them through endian-aware accessors, so a host
comparison against the staged O2R payloads covers every static segment, flag,
bound and map object at zero tolerance. `scripts/stages/check_collision_parity.py`:

    COLLISION_PARITY: PASS (9 VS stages)
    missing_segments: none      differing_segments: none
    WALL_TIME: 0.188s

Not wired into `verify-all.ps1` — a front gate that arrives red blocks the
Boundary guard, and wiring is a separate decision now that it is green.

## CORRECTION: the partition check was NOT dead (2026-09-09, later)

The entry below claims the arena partition-end check can never fire, and that the
arena-partition theory of the character-select cost was therefore unfalsifiable by
its own instrument. **Both claims are wrong, and they were committed in
`619baccd301`.**

The committed code at HEAD, before any of tonight's agent edits, is

    region_words = (use_low_detail != 0u) ?
        (NDS_FIGHTER_PACKET_ARENA_WORDS / 4u) :
        (NDS_FIGHTER_PACKET_ARENA_WORDS / 2u);
    region_base = battle_slot * region_words;
    if (region_base + region_words > NDS_FIGHTER_PACKET_ARENA_WORDS)

`region_words` is **detail-conditional**. At high detail it is `ARENA/2`, so slot 2
gives `region_base = ARENA` and `region_base + region_words = 1.5 x ARENA`, which
exceeds the arena. **The check fires for slots 2 and 3 at high detail** — exactly
the mechanism the arena-partition theory described.

The "provably never true" derivation used `ARENA_WORDS / SLOTS`, which is the
*replacement* an agent was writing at the moment the file was read. A file under
active edit was read and reported as the baseline. That is the second instance
tonight of the same error — the first was a run-9 corner list read mid-edit — and
the countermeasure is the same: **read `git show HEAD:<path>` when establishing
what the code does, not the working tree, while agents are running.**

What stands from the entry below: the probe did preflight on
`gNdsFighterPacketArenaDeclines`, that symbol is absent from three of four ELFs,
and the cadence figure was blocked by it. What does not stand: the claim that the
check is dead, and the dismissal of the arena-partition mechanism. Both defects
were real. The animation-cache reload loop was the more serious and is the one
measured and fixed; the partition was a live second mechanism.

**Open risk from the fix now in the tree:** the replacement makes the partition a
flat `ARENA_WORDS / SLOTS`, which **halves what a high-detail packet gets** — from
half the arena to a quarter. A packet that fit in `ARENA/2` and does not fit in
`ARENA/4` would now fail at the recorder's capacity check instead of the partition
check. That is a behaviour change and needs measuring before it lands.

## A probe witness that is provably dead code (2026-09-09 night) — SEE CORRECTION ABOVE

`scripts/menus/probe-p2-shell.ps1` preflights on `gNdsFighterPacketArenaDeclines`
and refuses when it is absent — which it is, from three of four current ELFs. It
is declared `__attribute__((used))` at `nds_renderer_preamble.c:3419` with a
comment saying it is retained for the shipping-cadence CSS witness. Its only
writer (`nds_renderer_native_common.c:8918`) sits behind

    region_words = ARENA_WORDS / SLOTS;  region_base = slot * region_words;
    if (region_base + region_words > ARENA_WORDS)

For any `slot < SLOTS` that is `(slot+1) * region_words`, which cannot exceed
`SLOTS * region_words`. **The condition can never be true**, the branch is
eliminated, nothing references the array, and `--gc-sections` drops it despite
`used` — the retention rule this build follows is *referenced*, not *attributed*.

Two consequences. The CSS shipping-cadence figure cannot be captured until the
probe's dependency is resolved. And the counter was the sole evidence for the
theory that character select's cost came from slots 2 and 3 "hitting the bounds
check every draw" — **that check cannot fire**, so the theory was unfalsifiable by
its own instrument. The real defect was the animation-cache sizing, which is
landed and measured.

## The condition-mode diagnostic path SIGILLs (2026-09-09 night)

Recorded before the evidence is lost: the agent that found it wrote no scratch
file, so its stdout log is the only record and a relaunch overwrites that.

While landing the alpha rendermode gate, a **condition-mode barrel-witness
experiment was abandoned because that diagnostic action path SIGILLed** — an
illegal instruction — while GDB called the controller-playback helper, *before*
the barrel condition was ever evaluated. The ordinary Jungle fixed-present probe
is clean, and that clean run is what the gate's evidence rests on.

Two reasons this matters beyond the experiment it killed:

  - An illegal instruction in a diagnostic path is a defect in its own right, and
    this project's rules treat crashes and unexplained state as failures rather
    than noise.
  - It sits on the **controller-playback helper**, which is the same machinery a
    deterministic attract-demo replay would need if the demos turn out to be
    recorded input rather than CPU-versus-CPU. If that path cannot be driven
    without crashing, it is not a foundation to build on, and the attract-demo
    sizing needs to know.

Not investigated. The probe's `cond`/`cond2` case fields drive this mode, so the
reproduction is a condition-mode case rather than a fixed-present one.

## The alpha rendermode gate landed and held its control (2026-09-09 night)

The Castle roof fix forced every texel opaque, which is correct only where the
rendermode never consumes pixel alpha; where alpha is coverage
(`CVG_X_ALPHA` + `AC_THRESHOLD`), the same forcing fills the cutout. The gate now
tests the rendermode rather than the combine alone.

Source grounding, from the read-only decomp:
`G_CC_MODULATEIA` alpha is `(TEXEL0 - 0) * SHADE + 0` (`gbi.h:511-512`), and
alpha A/B is packed separately from C/D (`:3088-3102`) — so the port's
C/D-only test read "texel modulates" as "texel ignored" for MODULATEIA, where
TEXEL0 sits in slot A. `:673-676` defines the alpha-compare modes and `:690-699`
`CVG_X_ALPHA`; `:773-776` shows TEX_EDGE consuming pixel alpha as coverage, which
the opaque modes carrying the restored Castle surfaces do not.

**Capture confirms both halves of the pair: the red roof is filled while the
bridge lattice is transparent again.** The change is host-only generator code — it
adds zero native runtime field reads, so the field certificate is unaffected, and
ITCM grows by 0 bytes (32,208 of 32,736 on that private ELF, 528 free).

## Audio acceptance is mostly counter-verifiable, not an ears problem (2026-09-09 night)

The SFX trigger column was recorded as entirely unverified, with the implication
that confirming it needed listening. It does not.

**`gNdsSCVSBattleLastFGM` exists in the shipped ELF and pays out** — `022566bc B`,
real storage, checked with `nm` rather than assumed. So a per-trigger test is a
counter read: zero the witness, perform **one** action, read it back, and assert
the triple — cue id matches, miss-ring clean, supported-play count +1. Movement
triggers are 100% probeable this way (`decomp/.../src/ft/ftmain.c:344-398` for the
motion-event PlayFGM/Voice/Loop path).

Character voices are counter-coverable too. What remains there is a
**source-reading gap, not an ears gap**: walking each fighter's `*_MainMotion.c`
enumerator for per-motion trigger reachability beyond damage, smash and deadup.

Note the save/restore of `LastFGM` at
`src/port/reloc_backend_ftmain_damage_proofs.c:375,805` — a probe that does not
account for it will read a restored value rather than the one its action produced.

### Link's absent voice is 4 cues, not ~12

Cues **400, 406, 411, 414**, taking the FGM phase pack from **573 to 577
entries** when they land. The census figure of roughly twelve had been carried
forward and repeated into agent briefs and a status report. The pack loader
rejects the *entire* pack on any entry-count, size or mapping-hash mismatch, so
the re-pin arithmetic follows from the real number rather than the approximate
one.

### What genuinely needs the owner's ears

Narrow: audible tone per stage, and Yoshi's Island's garbled instrument, whose
four ranked causes are all measured and eliminated. Everything else in the audio
column is a counter or a source read.

## Menus

- Boundary's last red is an instrument gap, not a game defect (2026-09-08,
  CONFIDENCE HIGH). `p2_shell_loop` fails on
  `count=11 domain=3 scene=22 identity=0xffffffff status=2 reason=1`. Cause:
  the loop target builds with `NDS_HARNESS_FAST_LOGIC=1` (Makefile:3164) where
  the shipping shell builds with 0 (Makefile:2962), and that flag folds
  `use_realtime_presentation` to zero (`taskman_seam_harness.c:1096-1098`).
  Two battle `gcDrawAll` sites then run OUTSIDE any SObj preview frame — one
  per scene entry at `:1116-1122`, one after the bounded run at `:1553-1557`
  through `reloc_backend_movement.c:13832` — while the only battle path that
  brackets a draw with `ndsSObjPreviewBeginFrame/EndFrame`
  (`taskman_seam_battle_host.c:687/:700`) is skipped. With
  `sNdsSObjFrameActive` FALSE, `lbCommonDrawSObjAttr` cannot reach the layered
  path or any native owner, so every visible battle SObj lands in
  `ndsDrawSObjPreview`, which is now a pure failure recorder — hence the NULL
  GObj identity. The first-cause sprite is the stage wallpaper: RGBA16 with a
  NULL LUT and display link 0 is the battle wallpaper and nothing else in that
  draw (`generate_native_wallpapers.py:158-162` asserts RGBA16 for every
  battle wallpaper; the Results one is I4 and the HUD sprites are CI4/I4).
  My own gdb trap that resolved a bitmap into audio BSS caught a LATER
  occurrence, not the first cause — the record is sticky-first while the count
  keeps climbing — so there is no wild pointer to chase here. Fix: bracket
  those two fast-logic battle draws with the preview frame so the loop
  exercises the same path the shipping ROM does.
- Near-plane census, correction and limit (2026-09-08). Armed on Yoshi's
  Island it reported all 155 non-no-Z vertices with `clip.w == 0`, every one of
  them matrix binding 15 — which is exactly the floor-and-platforms binding the
  owner reports missing. That looked like the root cause and it is not. The
  census transforms by `frame->binding_composed[binding]`, and under
  `NDS_TASK36_HW_COMPOSE` a RIGID binding is composed by the GX, so its
  `binding_composed` slot is deliberately not maintained
  (`renderer_adapter_stage.c:3406` sets `frame.rigid_binding_mask` from the
  runtime rigid mask, and the no-Z path skips exactly those bindings before
  transforming). Reading an unmaintained slot yields w = 0. Castle's non-no-Z
  bindings are dynamic, which is why it read zero crossings and Yoster reads
  all of them. The census must skip rigid bindings the way the no-Z path does,
  and count them separately so the number is interpretable; until it does, its
  Yoster reading says nothing. Castle's result stands, because none of its
  measured bindings were rigid.
- **RETRACTED 2026-09-09 — this row confused two different objects, and acting
  on it would have added geometry the packet already contains.** The 09-08 note
  said Mushroom Kingdom's side platforms "are not in the packet at all" and
  prescribed extending `inishie.py` with the file-155 chain. File 155 root
  0x05F0 is the narrow moving YELLOW SCALE PLATES -- 8 triangles, instantiated
  twice by `grInishieMakeScale` -- and that chain is already complete.
  The owner's report is about the large STATIC side brick platforms, which are
  a different file entirely. MEASURED: they are source file 107. Layer0 root
  0x43A8 contributes 4 platform-top triangles from `gap_0x37A8_sub_0x3C8[4]` /
  `_0x408[4]`, Layer1 root 0x6008 contributes 8 from
  `Layer0MatAnim_MatAnimJoint_data_0x948[8]` / `_0xD88[8]`: 12 floor-top
  triangles total, reaching roughly x=-7642..-878 left and x=863..7633 right at
  y=0. `inishie.py:162-163` already admits file 107 through both layer trees,
  the generator's own `--check` matches the checked-in packet (24 bindings, 324
  source vertices, 334 dense, 65 runs, 54 epochs, 176 triangles), root 0x43A8
  keeps 14 of 14 triangles and root 0x6008 46 of 46, and the runtime owner
  reports zero rejects and accounts all 176.
  So source, generation and admission all hold, and the first unresolved link
  is draw VISIBILITY of specific faces. Do not add geometry for this report.
  The cheap next step is one volatile witness at the post-emit accounting seam
  for runs 11/12/33/46 -- those are the 12 broad floor-top triangles --
  recording expected versus emitted triangles, poly format, resolved texture
  extent and projected depth.
- Saffron door: the head-1 translucent list branches into a bank the repo does
  not contain (branch offset 0x190198 against a 2,796-byte MiscDataBank160),
  so the geometry the owner sees through the door could not be decoded from
  the tree. The head-0 door lists are opaque CI4 by their own palettes, which
  is source-correct. Before any further work here, resolve where that segment-2
  target actually lives; do not bake a door quad from a guess.
- Congo barrel: one print settles it (2026-09-08 probe, CONFIDENCE HIGH).
  Break at `nds_renderer_native_owners.c:159` — a point only an
  all-gates-passed frame reaches — and `p *hierarchy->joint_locals@2`. Read row
  3 of matrix[0] in 20.12: if `m[3][0]` sweeps about plus or minus 14,499,840
  (3540 x 4096) with `m[3][1]` at -6,543,360 (-1597.5 x 4096) and `m[3][2]`
  zero, the barrel is under the platforms where it belongs and the fault is
  presentation — alpha, depth or draw order. If the row is all zeros it is
  pinned at the origin and the owning seam is the animjoint attach/play path
  for the TaruCann GObj, not the matrix code, which the probe found already
  source-correct.
- Zebes acid: the drawn-versus-damage divergence is refuted (2026-09-08 probe,
  CONFIDENCE HIGH). Binding 25 composes its world matrix from the LIVE DObj
  translate every frame — the rigid mask excludes it, so the non-rigid arm
  runs — and the baked matrix row exists but is compiled out by default
  (`NDS_TASK51_STAGE_NATIVE ?= 0`, Makefile:227). Drawn surface and damage
  predicate therefore share one live root-plus-child chain, so the owner's
  "submerged before damage" is not a port offset; the source's root-versus-feet
  distinction is deliberate and must not be changed. That leaves the SHAPE
  question open on its own: the dome look has to come from the geometry or its
  animation, not from a height mismatch.
- Hyrule tornado cycle: the source lifecycle is imported verbatim (2026-09-08
  probe). Position is `1600 + Rand(1200)` and `520 + Rand(600)`, lifetime 80,
  cycle 32, and `syUtilsRandIntRange` is the source function, seeded the same
  way (nothing in the port reseeds it). So "do they spawn randomly and expire"
  is answered yes in code; what is unproven is that a tornado actually
  completes create, capture, release, expiry and recreate on the ROM. The
  probe's ranked risks are the funnel constructor failing (which would show as
  no tornado at all) and the Pupupu-style admission refusing the map object
  count, both of which fail closed rather than misbehave.
- Sector Z lasers: the source muzzle math is exact and worth quoting. The 2D
  laser takes two muzzles from the gun DObjs `map_dobjs[2]` and `[3]`, rotated
  about Z by `map_dobjs[1]->rotate.z`, with `vel_air.x = -230`
  (`grsector.c:674-691`). The 3D laser builds a basis, takes a local forward
  of `(0, 0, 666)` through `gmCollisionGetWorldPosition`, then aims at the
  fighter's TopN with `vel = 230 * dir` (`:839-844`). The port replaces that
  world-position call with `ndsR2SimMacBaseGetWorldPosition`
  (`battleship_gmcollision.c:216`), which is the first thing to check on a
  live run, along with whether the `map_dobjs[0..3]` flight animation ticks on
  the same frame the muzzle is read.
- Yoshi Island transparency, both halves measured (2026-09-08 probe): the
  sparkle particles never reach the converter at all. The particle bank pack
  admits only the P1 seam list (`generate_nds_particle_banks.py:735`), so the
  Yoster scripts are unreachable and fall to an opaque quad before any texture
  conversion runs. Where conversion does run, `ndsRendererHardwareConvertI`
  replicates intensity into RGB and drops the source environment tint, which
  is the clouds' wrong colour as well as their missing coverage. The source
  contract for the sparkle is coverage from the I4 intensity with a white
  prim/env lerp fading alpha to zero. Fixing it needs the Yoster scripts
  admitted (atlas is 31,872 of 32,768 bytes, so the budget has to move) and
  the same graded-alpha path the cloud now takes, keyed on prim/env.
- Mushroom Kingdom frame rate: a runnable recipe now exists instead of a
  theory (2026-09-08 probe). Break at `taskman_seam_battle_host.c:830` after
  present N and again 60 presents later, on the same ROM and configuration,
  for Inishie and Dream Land. Difference the cumulative counters and divide by
  presented frames; read `gNdsRendererProfilePresentActiveTicks` directly at
  the second stop, since it is a last-frame value and must not be differenced.
  The 2-VBlank budget is 1,120,380 ticks (`nds_startup.h:4420`), so margin is
  that minus present-active ticks, and a negative margin predicts a 3-VBlank
  interval. `gNdsBattlePlayablePacingPresentIntervalBucket` says 20 FPS versus
  30 FPS directly: bucket 3 dominating is 20 FPS. Rank the per-present
  subsystem counters between the two stages to find the gap. Do not compare a
  run count to a VBlank.
- Castle roof: the near-plane explanation is measured dead (2026-09-08,
  castle-near). A probe agent pointed out, correctly, that the earlier
  "near-fan refuted" note was void: `gNdsNativeStageNearFanCount` only
  increments inside the no-Z fan, which raw and range runs never enter, so a
  zero reading said nothing about the roof. It also argued that only the no-Z
  class computes `near_inside`, so a raw or range triangle crossing the near
  plane is handed to the hardware whole — true of the code
  (`nds_renderer_native_owners.c`, the class gate in the prepare loop). So the
  premise was measured instead of assumed: a behaviour-neutral census
  (`gNdsNativeStageNearCensus*`, armed by the probe, off and free otherwise)
  transforms every non-no-Z vertex and counts the ones behind the near plane.
  Castle at two cameras: 139 vertices tested, ZERO outside, ZERO with w == 0,
  while the roof holes are in the capture. The roof is not a near-plane loss.
  Still open, and now with two hypotheses spent: the next measurement should
  follow binding 5's triangles all the way to their submitted v16 coordinates
  rather than testing another gate.
- **Castle roof CLOSED (2026-09-09):** `0909-roofalpha2` runs on ROM/ELF
  `A4D30AC1` / `E0C4B191`, which are byte-identical to the rebuilt normal
  `build-p2-shell` outputs. Binding 3 run 9 has `POLY_ALPHA=31`, texture 0x63,
  and `Given=Emitted=9`; `artifacts/visibility/0909-roofalpha2-castle.png`
  shows the upper roof filled. The loss was texture alpha, not GX geometry:
  Castle's steep-roof CI4 contains useful RGB/intensity behind source alpha
  zero while the active N64 combiner takes final alpha from neither TEXEL0 nor
  TEXEL1. The DS conversion now keys that case separately, preserves the RGB,
  and forces the uploaded DS texel alpha opaque so polygon/vertex alpha owns
  the result. A controlled Castle blob `rigid_binding_mask=0xC3F` A/B routed
  run 9 through Task 36 but removed several other Castle surfaces, so that
  experiment was rejected and the production blob remains rigid mask 0.
- Results sprites, full census (2026-09-08 probe, HIGH): the screen needs about
  65 SObjs in a worst-case 4P match — wallpaper, player tags (IA8 19-21x24),
  place arrows (IA8 15x12), stock icons (CI4 8x10), mode and column labels
  (IA8/I4), damage and place digits (IA8), the announce alphabet (IA8 9-39 x
  36-39) and the WINNER plate (RGBA16 42x35) — plus fills that have no SObj at
  all (screen tint, bars, the label line). Only the wallpaper has a native
  path: `ndsSObjWallpaperIsResultsShape` accepts I4 300x220 and draws it as the
  single resident BG2 image. Everything else reaches
  `ndsDrawLayeredSObjFrame` (sprite_preview_backend.c:826) and is recorded as
  a failure, which is why the first native failure of a whole VS run is a
  Results sprite on display link 27. The budget fits: ~75 OBJs after wide-glyph
  splits against 128, three distinct scales against 32, roughly 38 KB of 4bpp
  cells. Ordered plan in the probe: route links 27/29/31 to a new Results OAM
  owner (keeping link 26 on the wallpaper path), bake tags/arrows/digits/
  letters/WINNER with the source pos/scale/colour, and map the fills to
  MASTER_BRIGHT and quads. The battle OAM path cannot be reused as-is: it
  matches assets by (bitmap, width, height) and its player-tag bake is gated on
  battle state.
- KO pillar and shield, source contracts (2026-09-08 probe): the KO particle
  submit passes ONLY the primitive colour
  (`battleship_lbparticle.c:4065-4067`, `:4165-4170`), while the source child
  scripts also set an environment colour and the pixel is
  (PRIM - ENV) * TEXEL + ENV, so the pillar loses its ramp and reads as thin
  streaks. Two more measured gaps on the same path: `QUAD_FRAME_CAP = 6` ships
  frame 0 only for 19 of 32 admitted textures (KO texture_10 has three source
  frames), and the ok/fail/draw diagnostics are gated on `link == 1` while the
  loop walks every allocation link, so link-2 KO particles fail silently.
  Shield: the source is prim/env per player with alpha 0xC0
  (`efmanager.c:450-464`, `:4112-4113`) over an IA8 16x32 half-bubble, and the
  DS blend is the same (PRIM-ENV)*TEXEL+ENV with alpha TEXEL x POLY_ALPHA. The
  entry-effect converter quantises that texture to an EIGHT-entry grayscale
  ramp (`generate_nds_entry_effects.py:604-613`) where the standalone shield
  path keeps all sixteen source intensity levels in 32 entries
  (`generate_nds_particle_banks.py:2460-2462`, `:2507`). Eight weights across a
  white-to-red lerp is exactly the owner's blocky red bands. Fix to try first:
  give the entry-shield texture the finer ramp.
- Results no longer rejects Mario. The reject witness said which check
  declined once it was compiled into the shipping ROM: validate code 4, slot 0,
  high detail, root index 3, observed 0x5300 against the expected 0x18D8. That
  is a source model-part swap, not a missing program. 203_MarioMain.c gives
  joints 10 and 16 a two-part descriptor whose second part is the alternate
  hand (`modelparts_desc_0x05C` 0x18D8/0x3E60 -> 0x5300/0x54E0,
  `modelparts_desc_0x0AC` 0x1DC8/0x4278 -> 0x4F10/0x50F0), and
  ftParamSetModelPartID writes the selected DL straight into the live DObj, so
  the native owner must carry a program for it at the same logical binding.
  Fox already had exactly this for its Results Lose motion; Mario now has the
  same four rows. The Results screen draws both fighters:
  `artifacts/visibility/2026-09-08_results-validate.png`.
  The generator needed one structural change to accept them. Its shared
  Mario+Fox arrays are read as "canonical program first, variants after", which
  held only while the sole variant owner was the LAST one: Mario's variants
  decoded in Mario's own pass displaced Fox's canonical epochs, runs and dense
  vertices, and Fox's packed matrix restores silently went from 14 to 0.
  `_build_source_export_for_owners` now takes a deferred spec set that decodes
  after every owner's pass, so Mario's two roots land in the tail with Fox's
  program untouched, and `build_dense_geometry` walks roots in array order
  instead of assuming one owner's epochs are contiguous. Every frozen pin still
  holds (canonical direct policies, the Task 27 certificate, the dense census
  now expressed as four blocks) and check_nds_native_owner_packet.py and
  check_native_owner_geometry_closure.py both pass.
  NOTE, unrelated and pre-existing: check_nds_native_owner_hierarchy.py is red
  and has been since the DS coverage guard landed (ca217b7f2ab, 2026-09-02).
  Its direct trace packs RAW dense positions while the retained packet carries
  the guard-nudged ones, so every guarded vertex mismatches by one lattice
  unit by construction. Nothing runs that checker; it needs the guard applied
  on both sides before it can be trusted again.
- The next Results failure is a different domain: sprite (domain 3), reason 1
  (no native program), identity 0x1b, status 0x30001, root 0x22cbc58, with the
  fighter validate witness clean at code 0. Total native failures over that
  run fell from 3,250 to 1,570.
- **VS Options crash (2026-09-06):** native round trip passes; owner accepted
  VS Options, Option and Backup Clear visually. Cadence and save-behaviour
  verification stay separate rows.

## CSS

- **Link torso (2026-09-06, `5bc1f461f90`):** camera-correct reflection state
  restores the full native CSS model; evidence
  `artifacts/performance/2026-09-06_css-link-reflection/`.
- **Yoshi invisible (2026-09-06):** three seams. (1) `FTCommonPart.flags` sat
  in the high lane after the file's u32 swap, so every Yoshi joint dispatched
  as case 0 and drew nothing; `ndsRelocNormalizeFighterCommonPartFlags`
  restores the byte (host test `test_common_part_flags.py`). (2) The bake
  published the compiler-extended payload size (0xb1b0) and synthetic weld
  offsets; it now publishes the raw 0xace0 and the source post-list
  identities (`test_native_root_identity.py`). (3) With case 1 live, each
  pre-matrix `dls[0]` became its own contract event: `gcPrepDObjMatrix` now
  credits it to the joint it precedes by identity, and the collection folds
  it into that joint's welded post root. Natural CSS selection validates
  18/18 roots; evidence `artifacts/performance/2026-09-06_css-yoshi-native/`,
  `artifacts/visibility/2026-09-06_css-yoshi-pre-list.png`. Battle/stress
  acceptance and Win2 pose coverage remain open.
- **Pikachu ears (2026-09-06):** not matrices, topology or admission (GX clip
  matrices for the ear roots equal head x ear offset; prepared vertices match
  the bake; a cull-none poke made the ears appear as back faces). The ears
  are a 12x1 CI4 ramp tile (`SETTILESIZE` lrs 11, lrt 0, both axes
  `G_TX_CLAMP`) uploaded as 16x8; the bilinear texcoord offset puts every ear
  texel at t~1.1, which the RDP clamps to row 0 but the DS reads from the
  zeroed padding row, and the PAL16 packer publishes zero as transparent
  colour 0. `ndsRendererHardwareReplicateClampPadding` replicates the tile
  edge across the padding of a clamped axis. Evidence
  `artifacts/performance/2026-09-06_css-pikachu-ears/` (before / generic
  control / after). Battle acceptance and a wider texture regression pass
  remain.
- **Kirby / Jigglypuff / Ness:** `NDS_P2_SHELL_ROSTER` ladder (Makefile:656,
  685-693) stops at rung 7; rung 8 loads ten closures (904,656 B) and hung in
  libfat `get_fat` on the CSS transition. The lazy-load patch hung the shell
  walk and was reversed (`builds/resume-20260905/agents-0906/css_arena_budget.final.md`,
  `css_arena_residency.final.md`).

## Stages

- **Missing BG on all eight (2026-09-06):** every stage wallpaper container
  shares Dream Land's layout, but only Dream Land's Sprite header was
  byte-lane normalized after load (`ndsRelocNormalizeStageDreamLandSprite`
  keyed on one asset id), so the other eight failed the shape gate and seeded
  nothing. The normalizer now covers every wallpaper asset id. Castle and
  Hyrule seed READY (state 2, 42,834 opaque pixels, distinct hashes):
  `artifacts/visibility/2026-09-06_stage-wallpaper-{castle,hyrule}.png`;
  probe `builds/resume-20260905/stage-qa/stage-wallpaper-probe.ps1`.
- **Slow renderer on all eight (`ab3a8f083e4`, `4f8d9a30392`,
  `4099abddd50`, 2026-09-07):** native admission declined every blob stage
  with reason 6: the blob maxima header was never generated (Makefile
  dependency on `nds_renderer_assets.o`, an object no rule builds), the blob
  header's segment-0 flag validated Dream Land's certificate against every
  stage, `layer_mask`/fog/emblem byte lanes were unswapped, and mixed-alpha
  runs could not be submitted. Blob stages have no static texture pin set, so
  PrepareRun step 2 declined forever; the first owner prepare after a blob
  load now uploads once through the live path and guards those entries from
  eviction (`stage_warm`). Two shared texture-state fixes: the render tile
  samples the last LOADBLOCK/LOADTILE at its TMEM address, not the last
  SETTIMG (Zebes palette swaps), and G_SETENVCOLOR is texel state for I/IA
  bakes (Yoshi's Island bushes). Wrapping tiles key without their origin
  (Zebes acid re-upload churn 73 to 24 slots). All eight admit at 29.9 FPS
  entry frames; probe `builds/resume-20260905/stage-qa/stage-admission-all.ps1`,
  shots `artifacts/visibility/2026-09-06_stage-admission-*-shot1.png`.
- **Player tags (2026-09-07, uncommitted):** the five IA8 tag glyphs bake once
  into 4bpp OBJ cells and each frame places one OAM entry from the source's
  own TopN projection (`src/nds/nds_ifcommon_oam.c`,
  `ifCommonPlayerTagMakeInterface` wrapper); the generic compositor is the
  fallback only. The source draws a tag only while `playertag_wait == 1` or
  the camera eye is farther than 6000 units (`if/ifcommon.c:1833`), so tags
  absent in close play is source behaviour. Counters
  `gNdsIFCommonNativeOamFrameRecognizedCalls` / `FrameFallbackCalls`.
- **Congo platforms (2026-09-06):** the layer-1 AObjEvent32 script plans 509
  commands and `NDS_AOBJ_EVENT32_PLAN_MAX` was 128, so the normalizer rejected
  it and the yakumono DObjs never animated. Capacity 640 with a plan
  high-water witness; `gMPCollisionSpeeds[1]` is nonzero. Barrel path open.
- **Zebes entry data abort (2026-09-07, committed):** Mario's event32 Appear
  bound after the 56-frame entry pan while TopN still carried the R2 track
  engine's Q-form TraI AObjs (no interpolate descriptor); the generic walker
  evaluated them through `syInterpCubic(NULL)`. The figatree attach retires
  TopN's and every unscripted joint's AObjs at the event32 hand-over, and
  `ndsTraIDescUsable` guards both interpolation sites with a witness (0 hits
  over 90 presents and a 40 s free run). Earlier `fixupfail=2` readings on
  Zebes/Sector were gdb attached to a stale melonDS left by an aborted probe;
  a fresh emulator reads `openfail=0 fixupfail=0` on both.
- **Stage cull baseline (2026-09-07, all 40 packets regenerated):** the
  generator started every layer walk with cull clear and Z only on link 6,
  but the RSP reset list every task starts from sets `G_ZBUFFER | G_SHADE |
  G_CULL_BACK | G_SHADING_SMOOTH` (sys/rdp.c:26-33) and the grdisplay layer
  procs touch only G_ZBUFFER, so a map list enters with back-face culling ON
  and clears it itself where it wants two sides (Hyrule's platform list does
  exactly that around its two-sided runs). Every run before a list's first
  explicit set therefore drew both sides on the DS: Hyrule's rear roof and
  wall faces over the front ones (owner: "back faces render through"), and
  Dream Land had been patched at the runtime instead (the no-Z CULL_BACK
  force in `ndsRendererNativeOwners`, Pupupu only). `GEOMETRY_LAYER_ENTRY =
  G_CULL_BACK` in the generator and checker; `include_sha` re-pinned on all
  39 descriptors (`repin_stages.py`, hash-only diffs); Dream Land's frozen
  packet opts out (`layer_entry_geometry=0`, its 53 golden pins untouched,
  the Pupupu no-Z force covers it) and the choice rides in the segment's
  `initial_geometry` byte (bit 0 Z, bit 1 cull) so the runtime seeds the
  state the policies were recorded against. Decoder evidence:
  `agents-0906/hyrule_cull_decode.final.md`. Castle roof and Inishie side
  platforms showed NO admission decline (validate/prepare fail steps 0 on
  `castle-w1`, `inishie-w1`), so their loss is at draw time too; re-probe
  under the new baseline before looking further.
- **Castle roof — near-plane fan (2026-09-07, candidate):** every roof
  triangle is in the packet and admitted (validate/prepare fail steps 0,
  `castle-c3`), and the cull baseline changed nothing, so the loss is at
  draw time. `agents-0906/castle_roof_drawtime.final.md`: the native
  near-clip fan (`ndsRendererNativeStageEmitNearClippedTriangle`) had no
  zero-w guard while the generic fan does; a corner whose w rounds to zero
  after the 8-bit shift loads a degenerate clipped-vertex matrix. The
  steep central roof (runs 20-23, y 1320-1410) crosses the near plane during
  the entry pan. Guard added with witnesses `gNdsNativeStageNearFanCount` /
  `gNdsNativeStageNearFanZeroWCount`; owner-visible proof pending.
- **Castle roof — near-plane fan refuted at the probe camera (2026-09-07,
  castle-f1):** on the rebuilt ROM the fan witnesses read
  `near_fan tris=0 zero_w=0` at both shots while the roof still shows the
  strips-and-holes picture (`artifacts/visibility/2026-09-06_stage-admission-castle-f1-shot2.png`);
  validate_full/prepare_run/texreject all 0, so every roof run is admitted
  and submitted. Packet census: exactly five runs exceed the raw v16 range,
  all class 6 range runs — binding 5 runs 12/15/17 (max xz 2397/2791/2220, max
  y 1847/1234/1350, 4+4+2 triangles) and binding 6 runs 27/28; binding 5 is
  the tower. Next candidate is the range path at coordinate shift 1 (mixed
  per-vertex shifts inside one run); probe launched
  (agents-0906/castle_roof_range).
- **Castle roof — the range path is consistent (2026-09-07, probe
  castle-range, CONFIDENCE HIGH):** the class-6 shifted matrix scales x, y, z
  and the translation column together, the v16 written is the shifted
  coordinate, and every corner of a triangle uses the run's matrix, so a
  mixed-shift triangle stays internally consistent to one LSB. No line
  misplaces a vertex. What class 6 does NOT do is the per-corner near test
  the no-Z path runs before emitting (`inside_count`,
  nds_renderer_native_owners.c:3180), so a range run that crosses the near
  plane is handed to the hardware clipper whole. That is the next candidate
  for the roof, and it is a hardware-behaviour claim, so it needs a picture,
  not another census.
- **Saffron white band — identity (2026-09-07):**
  `agents-0906/saffron_wall_projection.final.md` projects the packet's
  white faces at the source zoom (14,000-15,000): the layer-3 backdrop quad
  at z 2296 (binding 16, y -888..-8633, white vertex colours, all-white
  TLUT) lands on screen rows ~115-192 at the wide view, the side walls are
  edge-on and the floor is a thin central band. Why the source shows no
  white there is still open (fog alpha 0; a far-plane or wallpaper-cover
  question); do not blank the quad without that answer.
- **Saffron white band = the source's own haze panel (2026-09-07, two
  probes):** binding 16 of the native packet is DObj 2 / link 1 of the layer-3
  tree (`112_StageYamabukiFile2.c:2156-2171`), a full-width panel at z 2296
  from y -888 (white, alpha 0xff) through -3327 (light cyan 0xdfffff, alpha
  0xdc) to -8633 (pale blue 0x9fcfff, alpha 0xff), combine G_CC_SHADE, drawn
  on display head 1 with G_RM_AA_XLU_SURF and G_ZBUFFER cleared
  (`grdisplay.c:144-155`); the layer-3 map row carries no anim, no matanim,
  no hide flag, and `gryamabuki.c` never touches it. The main platform is at
  y 810, the map bottom bound at -6000, so the panel is the haze that
  swallows fighters falling below the buildings and the original draws it at
  every wide view too. Port delta: the DS carries one polygon alpha per run
  (first corner, `nds_renderer_native_owners.c:1477-1483`) where the N64
  interpolates vertex alpha, so the 0xdc mid row reads opaque here; the
  colour gradient itself is per vertex on both. Owner to compare against an
  N64 capture before any change; if the band is judged too bright, the
  generator's per-run alpha for head-1 runs could take the run's minimum
  corner alpha (agents-0906/saffron_panel_visibility.final.md).
- **Saffron gate display lists decoded (2026-09-07):** MiscDataBank160 DLs
  0x0420/0x04F0 (head 0) are F3DEX2: TLUT RGBA16 16 entries, CI4 32x32
  clamp tiles, combine 0xfc121824/0xff33ffff (texture times shade, texture
  alpha), no render mode of their own (they inherit the head-0 state, and the
  gate GObj draws through `gcDrawDObjTreeDLLinksForGObj` on link 6,
  `gryamabuki.c:252`). Both palettes at File2 0x1898 (US) and 0x1038 (JP) have
  every alpha bit set, so the door texels are opaque on the N64 too; the
  translucency lives on the head-1 list 0x0850, a segment-2 branch
  (`de000000 02190198`) whose target is in another bank, and DL 0x05D0 sets
  G_RM_AA_ZB_XLU_SURF with alpha compare and G_CC_SHADE (a shaded translucent
  quad). Port seam to check next: whether the ground-actor arm emits head-1
  links with the XLU render mode before the run alpha is derived.
- **Zebes crash = event32 ledger exhaustion (2026-09-07):** the probe's crash
  hook caught it: an abort-mode exception (cpsr 0xb7) whose saved return is
  `gcParseDObjAnimJoint` objanim.c:366 (`event32->command.opcode` through a
  garbage joint pointer), then calico's `__excpt_entry` jumped to 0xc68 and
  melonDS raised SIGILL — the visible symptom, not the fault. Ledger census
  (`admission-zebes-l1`): 699 normalized entries carried from the shell at
  battle entry, 3,071 of 3,072 thirty presents in, 27 reason-12 refusals then
  and 46 at present 60 (owners 1 = MObj, 0 = DObj). Zebes' layer-1 material
  animation is eleven 484-word palette scripts (`105_StageZebesFile2.c`),
  ~2,400 entries on their own. A refused attach leaves the GObj without its
  source animation; the refusal that crashed was Mario's entry pipe effect
  (`efManagerMakeEffect(dEFManagerMarioEntryDokanEffectDesc)` eleven presents
  before the fault). `NDS_AOBJ_EVENT32_NORMALIZED_MAX` 3,072 -> 4,096 and its
  index 4,096 -> 8,192 slots (+16,384 B .bss); read
  `gNdsAObjEvent32NormalizedHighWater` over a full Zebes match to right-size.
  The probe gained `-AcidPoke`, a ledger census line at battle entry and per
  shot, `bt 4` on `gcAddAnimAll`, and the `hook-stop` crash printer.
- **Ledger RAM cut (2026-09-07):** the 5,120-entry raise pushed the shell-loop
  arena free floor to 19,220 B (minimum 32,768; the morning run had 48,688).
  The ledger's stored native word was only ever re-checked against the word
  committed in place (reason 3), so each entry is now a pointer plus a one-byte
  fold of that word (`sNdsAObjEvent32NormalizedSig`, 5 B per entry instead of
  8): 25,600 B plus the 16,384 B index. Standalone shell loop after the cut:
  free floor 35,604 B, PlayersVS high-water unchanged, `rej=0`. The remaining
  ~5 KB of the morning-to-evening drop is the Sound Test / VS Record shells and
  the working tree's uncommitted 1P WIP; bake-time pre-normalization still
  retires the whole 41,984 B.
- **Yoster cloud graded alpha — the dedicated upload is never reached
  (2026-09-07, yoster-a1/a2):** the beam's A5I3 path now also accepts the
  cloud's PRIM_ALPHA-over-I4 combine (alpha from the I4 intensity, prim/env
  lerp in the palette), but `gNdsRendererPrimRgbTexel0AlphaPrepareCount` reads
  0 on Yoshi's Island at two cameras while the cloud executor runs
  (cb=1803 tri=10782 rej=0, hwtexrej=0), so the clouds still upload through
  the one-bit converter. First hypothesis was the cache order — the gate sits
  after `ndsRendererHardwareFindTexture` and the stage-source-frame find, and
  the cloud's tile is a stage texture warmed before GO. A build that skipped
  both lookups for a graded-alpha surface read the SAME 0 prepares, so that
  premise is refuted and the hunk was reverted. Next: read the classifier
  result for the cloud's own combine at the bind
  (`ndsRendererHardwarePrimEnvTexel0BlendMode`, textures_effects.c:530) and
  the `format`/`size` the executor's config actually presents, before
  touching the gate again. ANSWERED the same evening by review
  (agents-0906/review_cloud_alpha, CONFIDENCE HIGH): the cloud list is
  2-cycle (`nds_native_actor_yoster_cloud.generated.inc:104` sets
  G_CYC_2CYCLE) and the classifier reads the CYCLE-1 slot when 2-cycle
  (textures_effects.c:552-559). The cloud's PRIM_ALPHA shape lives in cycle 0;
  its cycle 1 is the pass-through (COMBINED, 0, SHADE, 0). So the mode never
  classifies and both gates are dead for it. Fix to try next: when 2-cycle
  and cycle 1 is that pass-through, classify from cycle 0. Two further
  findings from the same review, both open: the graded-alpha surface has ONE
  resident slot, so a beam and a cloud alive together would re-prepare on
  every alternation; and the prepare/bind counters have no reader outside a
  probe, so "prepares 0" cannot fail anything automatically. Same run read 19.9 FPS on Yoshi's Island, the same
  figure as Mushroom Kingdom.
- **Yoster packet coverage is complete (2026-09-07, probe yoster-floor):**
  every source DObj carrying geometry has a binding — the floor is binding
  16, the terrain and platforms binding 15, the backdrop and skirt bindings
  17 and 18 — and the only unbound entries are NULL dummies and the three
  cloud platforms, which are composed dynamically on purpose. No binding
  submits in a class the runtime rejects (the packet uses classes 0, 3 and 6
  and the gate accepts all three). So the owner's missing floor and platforms
  are a draw-time loss at some cameras, not absent geometry, and the
  near-plane reject in `ndsRendererNativeStageEmitNoZTriangle` is the place
  to look.
- **Stage frame rate — the packet terms do not explain 20 FPS (2026-09-07,
  probe inishie-fps):** Mushroom Kingdom and Yoshi's Island run at 20 FPS
  where Dream Land holds 30, on the same ROM with two fighters and no items.
  The census found the packets close: 55/58/54 runs, 44/48/49 texture epochs,
  318/268/423 state events, and the per-run fixed cost (about 3,129 ticks)
  differs by only 3,000 to 12,500 ticks a frame between them. A whole extra
  VBlank is about 560,000 ticks, so the packet terms are two orders of
  magnitude short and the gap is somewhere else; do not spend another cycle
  on run counts. One real finding did come out of it: the generated
  segment-0 fast path is gated on `segment_index == 0`
  (nds_renderer_native_owners.c:3865), so Yoshi Island's validated eleven-run
  segment-3 program is never used. That is a ported-but-unreachable row of
  its own, worth its own measurement rather than a guess. A follow-up probe
  ranked the non-packet candidates with the counter that tests each; the
  top one, a failed fast-wallpaper seed forcing a per-frame software draw, is
  already dead: every stage probe this evening reads `wall_state=2 wall_ok=1
  wall_fail=0`, Dream Land included. Remaining, untested: particle quad
  emission (`gNdsParticleQuadEmitCount`), matrix loads
  (`gNdsRendererProfileMatrixLoadCount`), and the live actors each stage
  composes outside its static packet (Inishie's scale platforms, plants and
  POW block; Yoster's three cloud platforms).
- **Hyrule tornado damage/angle = ll-symbol arithmetic (2026-09-07):** the
  source reads the tornado's FTThrowHitDesc as `gMPCollisionGroundData -
  &llGRHyruleMapMapHeader + &llGRHyruleMapTwisterThrowHitDesc`
  (ftcommontwister.c:92), link-time constants 0x14 and 0xBC there. The port's
  `ll*` symbols are `uintptr_t` objects in .data (nm: 0x0214f6a0 and
  0x0214f69c), so the pointer landed four bytes BEFORE the ground data and
  damage/angle/knockback came from header words: the owner's "too much DMG
  and horizontal throw". Fix: shadow both symbols with their offsets in the
  twister wrapper, the grzebes_ground.c pattern. A census of every decomp file
  the port includes textually (`(u?intptr_t)&ll...` arithmetic without a
  `#define` shadow) leaves ftcommonattack100.c (Kirby, P2-3) and efmanager.c
  (Kirby star, Poke Ball, P2-5) for their rows; file-ID and
  lbRelocGetFileData uses resolve by symbol identity and are safe. Descriptor
  damage 14 / angle 90 is the contract. Verified on the rebuilt shell ROM (hyrule-tw2): the
  probe's file-offset readback gives damage=14 angle=90 kb_scale=60 kb_base=115, and
  `ndsBaseFTCommonTwisterShootFighter` now compiles to `adds r4, #0xa8` on the ground
  pointer. No tornado hit landed in a 900-present fighters run; the `-TornadoBt`
  release trace (release, InitDamageVars) is still owed on a run that meets one.
- **Zebes acid picture (2026-09-07):** at level -3000 (status Wait, child y
  +179.8 then -281.2) the drawn surface still covers the lower cliff faces
  (`zebes-c1` shots 1-2), i.e. the acid overdraws stage geometry that should
  occlude it; the stage's no-Z runs carry one constant depth per run
  (`ndsRendererNativeStageSetNoZColumn`), the acid draws with real depth
  through `ndsRendererAdapterSubmitStageDL` (world via the persistent stage
  cache, whose key includes translate). `-AcidPoke` (root -6000) moved the
  picture down, so the height was right and the depth was wrong: the acid
  DL (`MiscDataBank157` +0x9d8, decoded from the staged bytes) renders
  `AA_ZB_XLU_SURF` with Z_CMP and never clears G_ZBUFFER, but
  `ndsStageGCDrawAllLoopInitialGeometryMode` stripped G_ZBUFFER from every
  generic draw not on display link 6 and the acid draws on link 12, so the
  renderer classified it no-Z and painted it in the FOREGROUND band (the
  band after the stage's first source-Z triangle), over the cliffs. Fixed
  2026-09-07: ground actors keep G_ZBUFFER (`sNdsStageGCDrawAllLoopActor-
  KeepsZBuffer`); `zebes-z1` shots 1-3 show the cliff occluding the pool.
  Left to check: the "sphere-like" look while rising (8-vertex plane under
  affine texturing; tessellation is the candidate).
- **Congo barrel picture (2026-09-07):** the native arm draws 2 triangles a
  frame at the live root translate (`local0` row 3 = the DObj's -2748,-1597;
  `jungle-m1`), and moving the child joint +3000 (`-HideBarrel`, `jungle-hb7`)
  moved that arm's matrix but NOT the barrel the owner sees on the left
  platform. `-BarrelNoDv` (child dv = NULL, `jungle-nodv3`): the native arm
  then draws nothing (fail step 1) and the platform barrel is still there,
  so it is not the cannon GObj; moving the child to (+1000, +1900), which
  puts the native quad beside the platform, showed no second barrel either
  (`jungle-hb91`), so the two triangles the native arm submits every frame
  never reach the screen. The jungle packet's binding offsets do not include
  the barrel DL 0xa08. Open: what draws the platform barrel (layer-0 DObjs
  at x -1681 / +1678, `DL_0x84B8` / `DL_0x8548`, are the candidates) and why
  the native quad is invisible (poly alpha, texture bind, v16 range).
- **Congo barrel — the adapter gate, not the packet (2026-09-07, probe
  congo-barrel):** the native TaruCann packet matches what the source
  hierarchy provides (two bindings root/child, the parent chain, the joint
  kinds and the epoch offsets all line up), so the barrel is refused
  submit-side. `ndsRendererAdapterSubmitNativeTaruCann`
  (renderer_adapter_matrix.c:7176-7205) publishes the reason in
  `gNdsNativeTaruCannFailStep`: step 3 is the liveness test, which requires
  `ndsRelocGetLoadedAssetView(158)` to return the same base that
  `ndsRelocFindLoadedFileContaining(child->dv)` found. That arm was
  unreachable until the 2026-09-07 classifier change, and its first live run
  rejected every frame on a generation mismatch. Read the four witnesses on a
  Jungle run before changing anything: fail step, asset, dv offset, and the
  actor reject/triangle counts. If step 3 stands, the fix is the loader view,
  not the packet and not the orbit (the orbit was already repaired by the
  XObj order change).
- **Congo barrel — the gate passes and the actor draws (2026-09-07,
  jungle-b1):** on the rebuilt ROM `gNdsNativeTaruCannFailStep` reads 0 with
  asset 158 and dv offset 0xA08 at both cameras, and the draw loop counts
  801 actor callbacks emitting 1,602 triangles with zero rejects. So the
  step-3 liveness theory is refuted: the barrel submits two triangles a frame
  and is still not visible. Next question is where those two triangles land —
  read the submitted vertices and the hierarchy matrix for the TaruCann joint
  against the source translate (the barrel travels under the stage at a fixed
  height), rather than anything on the admission side.
- **melonDS host crash under the probe (2026-09-07):** Application log 1000
  `melonDS.exe` exception 0xc000001d at +0x249963 (13:59, 14:02, and every
  `jungle-*` probe after `hb3`), the gdb side reads "Remote communication
  error / Target disconnected" at the VS-mode menu. Zebes runs interleaved
  passed. `Stop-Process -Name melonDS` never matched the owner's `melonDS_4`;
  use `Get-Process melonDS*`. Cause open; rerun on failure.
- **Mushroom Kingdom music — PCM16 path (2026-09-07):** owner accepted ROM
  over RAM; seq 2 now streams signed PCM16 (`bgm_inishie_pcm16.raw`,
  3,918,852 B, rendered with `render-audio-bgm.py --sequence-index 2
  --format pcm16`) through the same two 8,196-byte ring buffers as IMA
  (4,098-sample chunks, refill deadline ~11 frames instead of ~44), zero RAM
  growth; per-track format flag in the BGM table; witness
  `gNdsAudioBgmPcm16UnderrunCount` beside seam-miss/overrun. The shorter
  packet exposed a stall IMA hid: the battle setup frame starts the stage
  track and then outruns one packet (first miss on the setup frame, track 2,
  every entry: `inishie-b6/b8`); the battle start now suspends the stream
  after starting it and resumes on the fourth battle frame, and the menu
  track is suspended across the setup loads (`inishie-b9`: all counters 0
  through present 600). Deadline proof for a full minute still owed (probe
  `bgm` line). Design: `agents-0906/inishie_bgm_pcm16_path.final.md`.
- **Mushroom Kingdom music (2026-09-07):** seq 2 never clips at gain 0.22 and
  an anti-alias low-pass moves its IMA SNR only +0.8 dB; the track's intrinsic
  IMA SNR is 18.6 dB against 25-29 dB for clean tracks (encoder tracking on
  a dense mix); a beam-search encoder gains +0.2 dB. BGM already plays through
  the SPU hardware IMA channel with zero ARM9 decode. A PCM16 stream for this
  one track costs +2,937,640 B ROM and, at the current packet geometry,
  +49,144 B RAM (`builds/resume-20260905/agents-0906/bgm_hw_adpcm.final.md`);
  owner decision needed because RAM is the binding constraint.
- **Sector Z Arwing file:** FoxSpecial3 (0xa1) reaches a Fox-less match through
  the map's extern chain (0x106 -> 0x99 -> 0xa1; the port's
  `MiscDataBank153` header lists 0xa1 and `ndsRelocAssetIDForToken` maps it
  unconditionally), so the load is source-faithful.
- **Sector Z Arwing — the motion data is clean (2026-09-07, probe
  sector-arwing):** all fourteen flight descriptors reach the SYInterp header
  fix exactly once through the DObj normalizer, no flight script contains a
  Jump or SetAnim, the float lanes survive the loader's blanket word swap, and
  the port advances the interpolation on the source's own once-per-frame
  clock with the source's evaluation math. So neither the byte lanes nor the
  clock explain the wrong paths. The remaining mechanism is a REFUSED attach:
  `battleship_sys_objanim.c:2278-2284` skips `gcAddDObjAnimJoint` when
  normalization fails, which leaves the joint stale and the Arwing frozen or
  wrong, exactly the Zebes failure one stage over. Read on a Sector run:
  `gNdsEvent32SYInterpDescFixCount` (expect at least 14), the unresolved
  count and address, and `gNdsAObjEvent32NormalizeFailCount` with its last
  reason. Reason 13 means a validation refusal to trace; reason 12 means the
  ledger cap again.
- **Sector Z — no Arwing reaches the draw arm at all (2026-09-07, sector-d1
  and d2):** across 500 and 1,400 presents the ground-actor arm counts zero
  Arwing callbacks and zero triangles, `gNdsAObjEvent32NormalizeFailCount` is
  0, and `gNdsEvent32SYInterpDescFixCount` stays at 1 where the source carries
  fourteen flight descriptors. Nothing is refused, so this is not the Zebes
  failure repeated: the flight scripts are never normalized because the
  Arwings are never made. Look at the maker and its trigger
  (`grsector.c`, the hazard spawn timer and whatever gates it in the port)
  before touching the interpolation again. The second capture also counted
  473 `unexpected` draw-loop entries after the match ended, which is a
  separate thread to pull.
- **Stage actors swallowed by the classifier (2026-09-07):** `gGRCommonStruct`
  is a union; Dream Land's `pupupu.map_gobj[4]` (bytes 4..19) aliases
  Zebes/Sector `map_gobj`, Jungle `tarucann_gobj` and Saffron `gate_gobj`.
  `ndsStageGCDrawAllLoopClassifyGObj` compared those slots on every stage, so
  the barrel, acid, Arwing and gate classified as Dream Land map pieces and
  went down the layer scan (generic stage DL submit) instead of the native
  barrel arm / ground-actor arm (witness `ground_actor calls=0`, `gatebt2
  rec=30`). The comparison is now gated on `gkind == nGRKindPupupu`. The
  source's own draw loop skips hidden GObjs, so the Arwing hidden flag needs
  no arm-side check.
- **Saffron City:** gate GObj exists with children (`gate root dv=nil
  child=set`); packet covers layers 0/1/3; elevators ride inside layer 1 with
  no yakumono-pose-aware commit (`docs/p2/stages/saffron-city.md`).
- **Hyrule scale (owner, 2026-09-07):** `4f8d9a30392` made range runs carry
  max(1, largest packed vertex shift) and split runs by alpha; Hyrule's
  descriptor moved from 72 to 75 runs. Suspect the shift class for the
  owner's "existing geometry's scale was messed up"; unmeasured.

- **Barrel / clouds native arms (2026-09-07):** both compared
  `owner_generation` against `gNdsTaskmanHeapGeneration` (a different
  counter) and rejected every frame once the classifier let them through; they
  now use `ndsRelocGetLoadedAssetView`. The barrel packet is one billboard
  quad (2 triangles, kind 40 + RotRpyR); the owner still reports it invisible
  in play, so `gNdsNativeTaruCannRouteNative` (gdb-settable, default 1)
  routes it through the ground-actor stage route for a one-binary A/B
  (`stage-admission-probe.ps1 -BarrelGeneric`).
- **Effect sprites opaque (heal sparkle 0x0E, item spawn swirl 0x69):** the
  particle bank pack admits only `P1_PARTICLE_SEAMS`
  (`scripts/generate_nds_particle_banks.py:735`, Mario/Fox/Dream Land items
  off); unreachable scripts fall to an opaque quad. The 128x64x4 atlas is
  31,872 of 32,768 bytes with five animated textures already excluded, so the
  12 unadmitted textures (about 12 KB) need a larger atlas budget (VRAM) before
  the seam list can grow to P2 scope. Decision pending.

- **Hyrule tornado damage/throw (owner 2026-09-07):** the port's Twister
  descriptor arithmetic is source-exact (`&llGRHyruleMapMapHeader` = 0x14,
  `&llGRHyruleMapTwisterThrowHitDesc` = 0xbc via `NDS_RELOC_LVALUE`;
  `FTThrowHitDesc` is all s32, so the u32 swap is right); the routing goes
  through the obstacle registry, not the capacity-1 hazard one. No divergence
  found by reading; needs a measured hit (damage/angle/kb witness) in play.
- **Yoshi CSS preview freezes after select (owner 2026-09-07):** the resident
  Selected figatree tables are answered by `ndsBattleShipLoadCSSSelectedFigatree`
  for Yoshi/Ness/Purin/Kirby, but `ndsBattleShipIsCSSSelectedFigatreeJoint`
  (the predicate the Event32 admission and the pointer resolver consult for
  ROM-resident joints) accumulated only Mario/Fox/Luigi/Donkey/Captain/Samus/
  Link/Pikachu, so Yoshi's Selected joints were rejected as "not a loaded
  reloc range" (reason 2) and the pose never advanced. The four tables are
  now recognised; the CSS warm also preloads each kind's selected row
  (Win1..Win4 = submotion rows 1..4) so selection pays no first-touch load.
- **DATA menu (2026-09-07):** native `src/nds/nds_menu_shell_data.c` (three
  font rows on the Option kit; DATA sprites not yet baked, TODO in the file),
  registry row under the shell, harness native case; its children
  (Characters, VS Record, Sound Test) stay campaign-gated source scenes, so
  A on a row is denied fail-closed until their natives land (agent contracts in
  `agents-0906/{soundtest,vsrecord,characters_screen}_contract.final.md`).
- **Sector Arwing paths:** flight scripts live in map extern 0x99 (file 153)
  as AObjEvent32 + SYInterp blocks; the SYInterp header lane fix exists only
  for fighter AObj16 TraI (`ndsRelocSYInterpDescHeaderNative`). Build agent
  `sector_syinterp_lanes` (worktree `_wt_sector`) extends it to event32.
- **Saffron door alpha:** file 160's gate DObjDesc pointers are unrelocated
  intern-chain words, so the DL bytes need a chain walk before the alpha
  contract can be read; parked behind the actor packet pipeline.

- **Entry-pan frame rate (measured 2026-09-07):** Hyrule reads 19.8 FPS and
  Zebes 22.9 FPS while the entry camera pans the whole stage (presents
  60-90), then 29.9 FPS at GO! (present 200). A one-binary A/B of the XObj
  order, the all-stage stretch, the filesystem lock and the range-run shift
  moved nothing, and hiding the Zebes acid moved nothing, so the pan cost is
  the stages' own range-run geometry in view, not a regression from today's
  fixes. The P2 gate is per presented frame at the screen's cadence, so the
  pan still needs its own measurement (tick-HUD stage bucket) before P2-4
  closes.

## Loading

- **Entry animation open failures — FIXED at the build (2026-09-07):** the
  path was not "an existing nitrofs path": the four loose event32 files
  (FTMarioAnim134/135, FTFoxAnim135/136) were missing from the ROM. The
  `prune-streamed-ftanim` recipe globbed `FT*Anim*` out of the staging tree
  while parallel Make had already judged those four up to date, so about half
  the builds packed a nitrofs without them (`ls nitrofs/reloc/reloc_animations
  | grep FTFox` read 0 after a build; a later build had them). The
  "one run in three" was one BUILD in two. The prune now removes only the
  replaced files (`NDS_FTANIM_STREAM_PRUNE_FILES`). Original observation:
  `fopen` (Mario Appear 0x279, Fox Arwing 0x30a) returned ENOENT after the
  direct nitrorom read failed, on Jungle/Hyrule/Zebes, roughly one run in
  three; the force loader then handed the raw heap back and the entry
  animation did not play. A recursive filesystem mutex (`ndsFsLock`) now
  wraps every reloc, BGM, FGM-pack, hit-spark, preview-pack and backup file
  call. The audit (`agents-0906/fs_reentrancy_audit.final.md`) then showed
  the BGM refill runs on the MAIN thread (the worker only starts channels),
  so the lock is not the fix; nine probe runs since read `openfail=0` but the
  miss is not explained. Witnesses left in place: `gNdsRelocAssetDirectFailStep`
  (which direct step failed), `gNdsRelocAssetOpenFailErrno/Asset`, and a
  4x fopen retry with `gNdsRelocAssetOpenRetryCount/SuccessCount`; the next
  recurrence says whether the miss is transient (retry succeeds) or a poisoned
  directory cache. Uncovered file sites the audit lists (FGM pack reads,
  fenced texture fopen, fighter anim stream) matter only if a second thread
  ever reads.

## HUD

- **Blue A / green B during gameplay:** the pause decals
  (`dIFCommonBattlePauseDecalsSpriteData` ids 1-2) are made on pause and
  ejected on unpause through `lbCommonEjectGObjLinkedList`, which was an empty
  weak stub until `08ce35de928` (2026-09-06 23:22). The root `smash64ds.nds`
  the owner played was built 2026-09-06 21:46, before the fix; rebuild and
  confirm with a pause/unpause probe.

## Native-only adoption and review corrections (2026-09-07)

- The owner explicitly adopted `docs/reviews/NATIVE_ONLY_IMPLEMENTATION_GOAL.md`.
  `02d5a9cdaa8` blocks ROM packaging using actual object/dependency inputs and
  ELF definitions; eight host controls pass, including forbidden references and
  renamed host-unit contamination. The main interpreter now lives outside ROM
  compilation. This is not native gameplay closure: live caller migration and
  the software compositor remain open, and current object compilation refuses
  the remaining forbidden calls. No new ROM has been built after adoption.
- The earlier stage-FPS argument is invalid: a 12,500-tick increase can cross a
  two-VBlank deadline without adding a whole VBlank of CPU work. For example,
  1,115,000 + 12,500 exceeds a 1,120,380-tick budget. These are illustrative
  values, not new project measurements. Packet costs remain a candidate;
  measure actual deadline margin and waits before eliminating them.
- Current owner evidence says Arwings are visible. The old no-spawn probe does
  not establish today's defect. Compare source-gated platform collision and
  separate 2D/3D muzzle transforms against the current owner ROM.
- Saffron haze removal is authorized. The host generator candidate preserves
  topology and removes only the identified panel's four triangles. Shared
  packet regeneration, hash re-pin, native runtime and visual checks remain.
- Haze regeneration now passes: file 112, layer3, DObj 2, head 1, actual DL
  offset 0x8688 (the source symbol's 0x1664 suffix is not its payload offset).
  The packet has 228 triangles, 77 runs, 17 bindings and 19 DObjs; include hash
  d546d1c4ad3c70d963202d8971d71d68bf532b682218f1faafe0f11d5e6bea32.
  Six host tests verify actual sibling geometry/material words and reject a
  stale omission identity. No native-ROM visual closure is claimed.
- Resumed Boundary on a5f2223179d: shell lap passes with 35,604 B free; Mario/Fox
  realtime passes at 212 samples. Four-CPU fails at frame 45, free 12,164 B,
  with a NULL countdown GObj in ifCommonEntryAllThread. Captured separately in
  `builds/resume-20260907/boundary.out.txt` and `boundary.err.txt`.

## Owner rendering regressions — 2026-09-08

- **KO blast pillar — OPEN:** owner reports a regression. The Hyrule screenshot
  shows thin yellow vertical streaks below/right of the stage during the KO
  effect. Preserve source blast direction, placement, lifetime, texture and
  alpha when repairing; no cause or fix is established from this still image.
  Evidence: `artifacts/visibility/2026-09-08_owner-ko-blast-pillar-081953.png`.
  Original: `Screenshot 2026-09-08 081953.png`; SHA-256
  `196c9284c807c29debe0a774d80e7127ac42b9dc2847e37d56c2337b6694a2a9`.
- **Shield texture — OPEN:** owner reports a regression. Mario's shield on
  Hyrule appears as blocky red bands instead of a coherent textured shield.
  Check the live source texture, palette/prim/env, alpha, UVs and attachment
  transform. Current native packet/host tests are separate evidence and do not
  close this visible defect. Evidence:
  `artifacts/visibility/2026-09-08_owner-shield-texture-081933.png`.
  Original: `Screenshot 2026-09-08 081933.png`; SHA-256
  `d55d6b66b0cca207f4d8424af3fbae4f1eb627d58a6268f61ec10507ca991814`.
- The screenshots' ROM/configuration hashes were not supplied. Do not attribute
  them to the new native-only candidate without establishing that provenance.

## Native-only integration — 2026-09-08

- The first native-only lab ROM passes the universal object/compiler-input/ELF
  gate on all 260 link inputs. Removing reference calls changed compiler
  inlining; outlining the LoadTile decoder recovered ITCM space while retaining
  the native state dispatcher there. Linker region has 112 B spare including
  its 32 B vectors. This is a link-fit result, not a measured FPS gain.
- Native wallpaper lookup needed DS registry identities (e.g. 0x10058), distinct
  from source file IDs (0x58). The manifest now carries both, verified against
  the asset registry. CLI regeneration is exercised using the Makefile arguments.
  Dream Land entry capture shows the backdrop, 1 load/15 reuses, zero read or
  native-render failures: `artifacts/visibility/2026-09-08_native-wallpaper-dreamland-alias.png`.
- The second one-minute Mario/Fox run completes 3,600 ticks and reaches Results,
  but is rejected by native-failure evidence: 379 failures, first domain 2,
  scene 22, asset 84 (`EFCommonEffects2`), root 0x2500. Required effect conversion
  remains open; this is not Boundary acceptance. Log:
  `builds/resume-20260907/native-second-realtime.txt`. Lab ROM SHA-256:
  `fb401fe254f14ae749e300950371f190fd3b87a67cffa32e1e7a3c3beb18b459`.
- Shield/reflector packets are compiled from source (2/6 triangles), with actual
  color/OtherMode write masks and five independently resident shield palettes.
  Host checks cover palette reuse, failed preparation/retry, inherited colors
  and partial state writes. Owner shield and KO texture regressions stay OPEN.
- CatchSwirl now has four native roots (0x2500/2588/2610/2698) and retains its
  live source material color/alpha ramp and 13-tick animation. Its I4 texels
  supply coverage only; white DS palette entries preserve source primitive RGB.
  The first conversion draft incorrectly added a grayscale RGB factor; the
  corrected host test checks the source combine and rejects that extra factor.
- Runtime found a shared attachment bypass: `lbCommonAddMObjForTreeDObjs`
  copied word-swapped O2R MObjSub records directly. Source flags 0x0200 arrived
  as live 0x0400. The helper now uses `ndsRelocCopyMObjSubForAttachment`, like
  the other attachment paths; actual-C tests cover flags/colors, native inputs,
  source immutability and failed conversion. After repair, CatchSwirl no longer
  supplies the first native rejection; the run now stops at KO root 0x5218.
  Log: `builds/resume-20260907/native-material-attachment-realtime.txt`;
  candidate SHA-256 `5e53950f2f83c96d218e153fdb6af64d30f8abf1145c4ac70ffe7731f818e494`.
- Verifier e9ec9349fb3 traps native failures inside the existing run, after the
  ARM9 cache flush/write-buffer drain. Raw spilled arguments read by GDB were
  stale; the published record supplies the correct cause. The known failing
  candidate confirmed early rejection with complete evidence and no extra boot.
- KO DeadExplode and ReflectBreak now have source-derived native packets
  (three roots each; 12/6 triangles). KO keeps twelve resident palette bindings
  for the source part/player RGB endpoints, while source MatAnim owns alpha.
  Actual-C tests cover palette allocation/retry/reuse and source color tables;
  packet checks preserve the previous 35 roots and 77 groups unchanged.
  The native-only build passes all 260 link inputs. Candidate SHA-256:
  `3eaaf0ba103712bcd4975a2570344295b39b6f663f9d1e3fbd43901af0067a3d`.
- The next one-minute gate rejects an Interface GObj 1016/link 23 RGBA32 sprite:
  `NATIVE_FAILURE=1,3,22,66584599,3,36500168,0,1`. Its bitmap is 0x022cf2c8;
  source family still needs identification. Log: `builds/resume-20260907/native-ko-first-realtime.txt`.
- Hyrule capture eight presents after the first natural KO:
  `artifacts/visibility/2026-09-08_native-ko-hyrule.png`, SHA-256
  `a138fea9dacd6cf52d0d1518fc825cf6501a5af30ec8d55552b73bd448139aa3`.
  It still shows narrow bright streaks; this does not close the owner regression.
  The capture also reports 54 prior native failures, so it is not a clean match
  acceptance result. Shield and KO remain OPEN; the public ROM is unchanged.
- The next failure is now identified on the same native candidate: Dream Land
  reports GameStatus base 0x022c0e50 and bitmap 0x022cf2c8, offset 0xe478,
  width 36. This is the first blue T in the source TIME UP message, whose
  Sprite descriptor is 0xe4a8. All nine TIME UP/GAME SET glyphs were missing
  from the native admission table. Capture:
  `artifacts/visibility/2026-09-08_native-first-failure-interface23-dreamland.png`.
  A separate Zebes probe stops earlier on stage asset 157/root 0x9d8; retain
  that unsupported actor in the existing stage repair queue.
- Announcement storage constraint: the current OBJ allocation is 31,168 B plus
  22,528 B of spark frames and 3,072 B of tags. The nine blue letters have
  2,266 distinct source RGB555 colors, so an exact shared 256-color palette
  cannot hold them. Source can end a stock match while GO is still alive:
  `ifCommonBattleSetInterface` does not immediately hide it. Keep GO storage
  independent. Tight GO tiles (17,408 B), a separate 20,736 B ending bank,
  sparks and tags total 63,744 B, below the 65,536 B bank. Ending glyphs use
  source-derived 0.8-resolution premultiplied filtering without new color
  quantization; visual acceptance of that measured-budget conversion is owed.
- KO follow-up must cover both the model and its particle half. The packed
  scripts 42–45/60–63 and reachable child textures are present, but the native
  particle submit path currently passes primitive color only; source KO child
  scripts use environment-color blending too. The atlas also freezes several
  multi-frame KO textures at frame zero. Review source color/alpha and frame
  contracts before accepting the pillar; emission counters cannot eliminate
  particle presentation as its cause. KO particles use allocation link 2,
  whereas the existing submit-failure counter only covers link 1. Route required
  particle failures into the shared native-failure record while completing
  those live callers; intentional zero-size/zero-alpha particles remain silent.
- Native endings and the reviewed native player-tag changes are integrated.
  Actual C tests exercise source constructor dispatch, bank writes, GO/spark/tag
  preservation, repeated calls and partial-failure retry. The earlier worker's
  hand-written phase simulation was removed; it did not test the production code.
  Source conversion preview (not emulator acceptance):
  `artifacts/visibility/2026-09-08_announcement-source-conversion.png`.
  Its SHA-256 is `c7feea18603cb72bbb37afbd8b245a478007efca92da02e81fec5d03b989e091`.
- Zebes acid now contributes its sole root 0x9d8, seven triangles and live
  four-frame CI4 material to the stage packet. Its first integration rejected
  the entire stage at topology reason 4, with only the map's 28 DObjs collected:
  admission allowed one TRS XObj but omitted the source acid's one Tra XObj.
  The owning admission now accepts that non-camera shape; the existing matrix
  builder still executes Tra, so rotation/scale fields are not substituted.
  Actual C capture/admission tests and the eight source-packet tests pass.
- Candidate `70a9dbc8f72f1243b5c555be688ddd8faf5197af610d7067e7ed00cf024b6a07`
  passes the native-only gate on 260 link inputs. A parallel nine-stage entry
  wave completes in 62.856 s, with native-zero results for Jungle, Zebes, Sector,
  Yoster, Dream Land and Hyrule. Remaining first rejections: Castle asset 86/root
  0x7558; Saffron asset 160/root 0x420; Inishie asset 155/root 0x1c8.
  This is sampled entry coverage, not stage visual or gameplay acceptance.
  Permanent results: `artifacts/visibility/2026-09-08_native-stage-entry-wave1.json`.
  Isolated Zebes capture now shows the stage and acid surface with zero native
  failures: `artifacts/visibility/20260908-zebes-native-acid-candidate.png`.
  Flatness/contact-height behavior through the full acid cycle remains open.
- The one-minute flow now reaches Results without the TIME UP rejection.
  Results rejects Mario's Lose pose (asset296/status0x10005/root0x1668/reason2).
  Log: `builds/resume-20260907/results-fighter-first-native.txt`. The requested
  `gNdsNativeFighterValidateRejectCode` is absent from the profile0 ELF, so that
  probe ended before its additional diagnostics. Check ELF symbol availability
  before launching another diagnostic; do not infer zero from a missing counter.

## Yoshi Island floor/platforms — drop sites ranked (2026-09-08)

- The packet is not the loss. Bindings 15 (155 vertices, 77 triangles, 22 runs)
  and 16 (20 vertices, 12 triangles, 2 runs) are both present, both segmented
  alone (segments 1 and 2, runs 23-44 and 45-46), and both pass every static
  validate step on their bytes. Source contract: `gryoster.c:216` draws the whole
  DObj tree through display link 6 and only the three clouds are dynamic
  yakumono actors, so the static map has no per-frame code path and must simply
  appear.
- **The near-plane hint in the earlier note is refuted for these two bindings.**
  Both are rigid (`nds_native_stage_select.inc` rigid mask `0x78014`), and
  `nds_renderer_native_owners.c:1604-1613` sets `near_inside = TRUE` for a rigid
  no-Z run before the `clip.w == 0` test can run; binding 15 has zero no-Z runs
  at all, and the raw/range emit path has no near branch. Do not re-probe it.
- Remaining live candidates, ranked: (1) `PrepareRun` step 2, the texture epoch
  resolve, which declines a whole run; (2) `Task36EnsureWorld` / `BeginSegment`
  returning FALSE, which drops the segment tail and bumps
  `gNdsRendererM3PostArmFailureCount`; (3) emit-time cull/winding/depth, the only
  site a counter cannot separate — Yoster keeps its source cull from
  `initial_geometry` (segment 1 word 3 = ZBUFFER|CULL_BACK, segment 2 word 2 =
  CULL_BACK without ZBUFFER) rather than the Pupupu force-cull.
- One probe run separates them: arm the census, then read
  `gNdsNativeStagePrepareRunFailStep` and `...FailRun`,
  `gNdsNativeStageOwnerPrepareFailStep/Segment/GuardMask`, the
  `gNdsRendererM3PostArmFailureCount` delta, and the per-class submit counts
  against the packet census (raw 35, no-Z 87, range/matrix 42). FailStep 2 with
  image/line operands is (1); a BeginRun abort with no PrepareRun fail is (2);
  clean counters with missing pixels leaves (3).

## Near-plane census is invalid for rigid bindings (2026-09-08)

- The census added for the Castle roof question counts any run whose submit
  class is not no-Z, but it does not skip rigid bindings. Under
  `NDS_TASK36_HW_COMPOSE` a rigid binding's world matrix is composed by the GX
  hardware, so the CPU-side clip w is meaningless and every vertex reads
  `w == 0` and outside. Its Yoster reading (155 vertices, all zero-w, all
  binding 15) is an artefact and must not be cited. Castle's zero-outside
  reading stands only for its non-rigid runs.

## Zebes acid is flat on both sides (2026-09-08)

- Source and port agree: 8 vertices, 7 triangles, every port vertex `y == 0`
  (`nds_native_stage_zebes.generated.inc:540-565`), and the only animation on the
  chain is a whole-DObj TRAY translate (`157_StageZebesFile3.c:146-160`) plus the
  material script's own ROTX/TRAY/SCAY channels. No per-vertex Y, no per-vertex
  matrix, and a per-vertex coordinate shift cannot bend `y == 0`. The dome the
  owner sees is therefore not geometry.
- What is left is shading: the acid vertex colours are white with four different
  alphas (`0xff`, `0xdc`, `0xf3`, `0xe8`) arranged as an 8-vertex fan around
  `(0, 0, -3608)`, drawn translucent under a 384x384 affine window over a 32x32
  CI4 image. A centre-bright alpha fan under affine interpolation reads as a
  dome. Next step is the alpha values against the source material, not
  tessellation.

## Item particles draw Whispy's leaves (2026-09-08)

- `gITManagerParticleBankID` is never assigned. The source sets it at
  `decomp/.../src/it/itmanager.c:150` from `efParticleGetLoadBankID` over the
  item script and texture banks, and because `itManagerInitItems` runs before
  `efManagerInitEffects` the source bank id is 0. The port declares the variable
  in `src/import/battleship_item_link_core.c:793` and never writes it.
- Its own comment claims that leaves the id at 0 and produces nothing. **That is
  wrong, and the consequence is worse than nothing.**
  `battleship_lbparticle.c:2549` does `s32 id = bank_id & 7;` with no bank
  identity test, so every item particle resolves against whichever bank happens
  to be 0: on Dream Land that is Whispy's, whose five script offsets are all
  valid, and elsewhere it is efcommon. Item flames and smoke therefore draw
  *another effect's* particles rather than failing. This is the silent-wrong-draw
  case the native-only contract forbids. Derived statically from the quoted
  guards; not yet observed on a ROM.
- The item bank is also never generated:
  `scripts/generate_nds_particle_banks.py:54-124` registers only efcommon,
  grpupupu, gryoster and grhyrule. Full source fidelity needs 5 cells of 32x32
  (5,120 B) against 896 B free in the 32,768 B atlas; even one 32x32 cell does
  not fit. Halving both textures to 16x16 at one frame each is 512 B and fits
  with 384 B spare, at the cost of the flame's 4-frame animation.
- On failure the id must be set to -1, not left at 0: `bank_id & 7 == 7` misses
  `LBPARTICLE_BANKS_NUM_MAX` and reaches the reject path instead of aliasing.

## Saffron: two stale notes corrected (2026-09-08)

- **The white haze is a genuine source element and the port never drew it
  opaque.** Its root is the file-112 layer-1 MatAnim joint at payload 0x8688:
  G_AC_THRESHOLD, G_CC_SHADE, texture off, 6 vertices whose alpha runs 0 -> 220
  -> 255, drawn through display head 1, which `grdisplay.c:82` renders with
  `G_RM_AA_ZB_XLU_SURF`. The generator averages the three corner alphas per
  triangle (`generate_nds_native_stage.py:2803`) and splits runs on the result,
  so the only delta is flat per-run alpha against N64 per-vertex interpolation.
  The earlier note attributing this to "one polygon alpha per run (first
  corner)" is stale. The owner-authorized omission has already landed as
  `yamabuki.py:156 omitted_draw_roots=((112, "layer3", 2, 1, 0x8688),)`.
- **The door's missing transparency is not a texture format.** Its CI4 TLUT
  (`112_StageYamabukiFile2.c:412`) has A=1 in all 16 entries, so there is no
  per-texel alpha to lose and A3I5 or A5I3 would carry nothing. The transparent
  part is the untextured graded shell around the door: DObjDesc slot 3 ->
  DLLink 0x0880 -> DL 0x05D0 -> 0x0660, `G_RM_AA_ZB_XLU_SURF` with G_CC_SHADE
  and per-vertex alphas 0, 30 and 180. The earlier note that this branches into
  a bank the repo does not contain is stale: only the head-1 0x0850 branch is
  out of bank; 0x0660 is in-bank and decodes.
- **There is no admission blocker, and the note that said so was wrong.** The
  `submit=0 reject=0` witness and `d0841c2d7f4`'s comment describe the state
  BEFORE that same commit fixed it. The gate is `gGRCommonStruct.yamabuki.
  gate_gobj` at union byte 12, which is bit-for-bit `pupupu.map_gobj[2]`, so
  `ndsStageGCDrawAllLoopClassifyGObj` claimed it for Dream Land's animated-map
  route and it never reached the ground-actor arm. The `gkind ==
  nGRKindPupupu` guard added there does not exclude Saffron from drawing -- it
  excludes Saffron from a Dream-Land-only comparison, which is what unblocks
  the arm, and it reads the live scene's stage kind through
  `gSCManagerBattleState->gkind` exactly as the nine sibling actor predicates
  do. Both routes end in `ndsRendererAdapterSubmitStageDObjNode`, whose
  `DOBJ_TREE_DLLINKS` case walks the whole `dl_link[]` array including head 1,
  so the door drew on the misclassified route too.
- The remaining question is therefore a material one, inside
  `ndsRendererAdapterSubmitStageDL`: why the graded XLU shell
  (0x05D0 -> 0x0660, `G_CC_SHADE`, vertex alphas 0/30/180) does not appear.
  Do not relax the gate predicate to a bare `dl_link_id == 6`: link 6 is also
  layer 1's link for every stage, and the Sector Arwing, the Inishie scale
  GObjs and the Jungle barrel all sit on it.
- Contract gap found in passing at `reloc_backend_movement.c:13258-13262`: a
  recognised ground actor that scans its tree and emits zero triangles bumps a
  reject counter and returns, which is a silent successful empty draw.

## Congo barrel: three candidates left, none separable by existing counters (2026-09-08)

- Admitted and submitted is now MEASURED, not assumed: `gNdsNativeTaruCannFailStep`
  reads 0 and 801 actor callbacks emit 1,602 triangles with zero rejects. The
  transform is live per frame through the DObj chain, not baked, so there is no
  bake-captured-joint bug to find. Dead with code: absence from the static
  packet (deliberate, `jungle.py:15-20`), the binding never preparing, the
  origin pin (a forced-visible pose showed no second barrel), alpha zero (the
  `poly_alpha != 0u` gate passes), v16 overflow (vertices are +/-318), XObj
  order, and depth occlusion at the forced pose.
- Live, ranked: (1) backface cull or winding, leaving zero covered pixels;
  (2) the CI4 palette or texture epoch resolving transparent; (3) a projection
  or modelview scale collapsing the quad. One behaviour-neutral witness set at
  the barrel submit -- submitted positions after the world unit shift, the
  winding sign, poly_alpha, poly_fmt and the bound texture name -- separates all
  three in a single Jungle run.

## Measured on the ROM, 2026-09-08 (stage witness probe)

`builds/resume-20260908/stage-witness-probe.ps1` walks the shell to one stage,
enters battle and reads that stage's open witnesses at two cameras. Outputs in
`builds/resume-20260908/witness-<stage>.out.txt`, screenshots in
`artifacts/visibility/2026-09-08_witness-<stage>-shot{1,2}.png`.

- **Saffron gate: recognised, reached, and emits nothing.** `GATE seen=480
  failstep=0 submit=0 reject=480` over 480 presents -- the predicate passes on
  every frame and the actor's tree scan produces zero triangles every time.
  This refutes the reading that the gate drew through the animated-map route:
  the ground-actor arm is *reached*, which means the classifier did not claim
  it, and it declines. The sticky first native failure on Saffron is
  `identity=0x3f200a0 status=0x7 root=0x420 reason=1` -- the known file-160
  root 0x420 rejection, now tied to this actor: the gate's display list has no
  native program, so the tree emits nothing. Fix the program, not the alpha.
  Saffron also ran at 19.6 FPS in this probe, beside Mushroom Kingdom's 20.
- **Congo barrel is submitted with everything a visible quad needs.**
  `TARUCANN fail=0 alpha=31 polyfmt=0x11f0080 tex=117 area2=103550976`, object
  space `x=5088,5088,-5088 y=-5088,5088,-5088 z=5504,5504,5504` (the +/-318 quad
  at the world unit shift), and **zero** native failures on Jungle over 480
  presents. Opaque, textured, non-degenerate, back-face culled, POLY_ID 1. The
  witness is object space, so it cannot say where the quad lands on screen --
  that is the next thing to publish. The screenshots show a barrel on the
  platform in both shots; whether that is the cannon actor or a static prop is
  not yet established, so do not close the owner's row on them.
- **Hyrule tornado lifecycle is armed and correct.** `HYRULEBANK id=0 packed=8`
  (the particle bank is resident, so `MakeTwister` is not failing closed) and
  `TWISTER status=1 wait=2449` then `wait=1969` -- the Wait state is counting
  down from inside the source's `Rand(1200)+1600` range. A source-versus-port
  read of the whole lifecycle found the position draw (uniform over four map
  object ids), the `520 + Rand(600)` active lifetime, capture/release and the
  cleanup re-arm all identical, with no per-cycle reseed. The owner's question
  is answered: positions are random per cycle and each cycle has a lifetime.
  Seeing a tornado on screen needs a run longer than the ~2,449 tick wait.

## Castle roof: three probes measured the wrong geometry (2026-09-08)

- **The steep red roof is binding 3, run 9, layer0 DObj 4, source display list
  0x1698, world translate (0, 1815, -840).** Every earlier Castle roof note in
  this file -- the 139-vertex near-plane null, the fan witnesses, the eight
  decoded triangles -- measured bindings 5 and 6, which are a 90-unit lip around
  the 1,206-wide top platform and the platform slab. Those readings say nothing
  about run 9, which has never been instrumented. Treat `:320-341` above as
  retired for this bug.
- Decoded from `ExternDataBank106` with the generator's own loader, reproducing
  the descriptor pins exactly (38 vertex commands, 254 source vertices, 73
  triangle commands, 136 triangles). The source roof is **one G_VTX of 19 and
  nine triangles**: six form the skirt (areas 34k-85k, normals splayed outward)
  and three reference the apex at (0, 1110, 30) (areas 151k-230k). It is an
  **open shell, not a closed pyramid** -- the apex ring is a four-vertex arc and
  the -Z quadrant has no triangle in the source either. Nothing is missing from
  the packet.
- **All nine are carried in one run**: `nds_native_stage_castle.generated.inc:189`
  `{ 0x0042u, 9u, 3u, 7u, 3u, 7u, 0u }` -- first corner 66, nine triangles,
  binding 3, epoch 7, submit class 3, one state, one alpha, and the apex vertex
  is carried verbatim at `:268`.
- **The partition is exact**: the six that render are the skirt -- narrow bands
  210 units tall around a 900-unit base, which is precisely the owner's "narrow
  triangular strips" -- and the three that do not are exactly the three that
  reference the apex. Apex-referencing and missing are the same set.
- Mechanism, not yet measured: the run is class 3 because the source Z is off,
  and binding 3 is rigid (`nds_native_stage_select.inc:1185` mask `0xc3f`), so
  `ndsRendererNativeStageEmitNoZTriangle` takes the Task-36 branch and returns
  before the `inside_count` near test. There is no per-triangle software reject
  on this path, all 19 vertices pack at shift 0, and v16 does not overflow. What
  is left is a hardware-side loss on triangles whose apex sits 1,110 units from
  the run's other corners under one flattened-Z projection matrix. **The next
  probe captures those three triangles' clip coordinates. Not another census.**

## Why Saffron and Mushroom Kingdom run at 20 FPS (2026-09-08)

Measured today: Saffron 19.6 FPS, Congo 29.1 and 27.0, on the same probe and the
same ROM. The owner reports Mushroom Kingdom at 20. Packet census across the four:

| stage | dense verts | tris | runs | epochs | bindings | class 0/3/6 |
|---|---|---|---|---|---|---|
| Dream Land | 312 | 202 | 54 | 49 | 42 | 3 / 48 / 3 |
| Congo | 350 | 182 | 72 | 60 | 30 | 6 / 61 / 5 |
| Saffron | 423 | 228 | 77 | 66 | 17 | 20 / 51 / 6 |
| Mushroom | 334 | 176 | 65 | 54 | 24 | 19 / 33 / 13 |

- **Packet size is refuted as the lever.** Mushroom Kingdom is below Congo in
  triangles, runs and epochs and still runs at 20 FPS.
- **The rigid binding mask is the ranking number.** `nds_native_stage_select.inc`
  gives Jungle `0x3ff9ffff`, and both Yamabuki (`:1481-1484`) and Inishie
  (`:1365-1368`) `0ULL`, with the comment "not derivable here, 0 costs only the
  replay". After masking out camera bindings that is 13 rigid bindings on Congo
  and **none** on Saffron or Mushroom, so triangles on a dynamic binding are
  Congo 72 of 182, Saffron 228 of 228, Mushroom 176 of 176. A rigid binding
  replays a PUSH plus one MULT4x4 of a constant world; a dynamic one issues a
  **LOAD4x4 per triangle** of projection x view x model
  (`nds_renderer_assets.c:5717-5720`). The FIFO words alone price at only
  21K-75K ticks against the ~558K the 30-to-20 move needs, so the unpriced half
  is the per-triangle matrix composition on the ARM9 -- but this is the only
  candidate that orders Congo above the other two.
- **The static texture corpus is Dream Land only.**
  `generate_battle_playable_static_textures.py:42-60` pins 44 keys and 83,840
  bytes, all Dream Land plus Fox and DeadExplode, out of
  `NDS_RENDERER_HW_TEXTURE_CACHE_COUNT 123`. On any other stage those 44 slots
  are dead weight and every eviction bumps the texture epoch, which invalidates
  `ndsRendererNativeStagePreparedTexturesProven` and rebuilds the prepared runs.
  The design note itself says the premise was measured on Dream Land, where the
  allocator runs three times in 1,600 frames. `renderer_adapter_stage.c:3496`
  records this class of failure costing "frame rate fell from 27.9 to 20.0" --
  the observed magnitude.
- **Saffron's zero-triangle gate scan is NOT the cost.** The tree walk is capped
  at 128 nodes and Saffron has 19 DObjs, so the upper bound is about 5,100 ticks
  a frame and the realistic figure about 800 -- under 1% of the gap. Do not
  spend a cycle on it.
- One run separates the two live candidates: sample the STG tick bucket on the
  Saffron route with `gNdsR2StagePrepareBuildCount`, `...ReuseCount`,
  `gNdsR2TexProofSweepCount`, `gNdsR2TextureEpochBumpCount` and the texture
  cache evict count. Rigid-mask predicts Build 0, Reuse about frames, Evict 0
  with the bucket still a third over Congo; texture churn predicts Build about
  frames, Reuse 0, EpochBump above 0.
- **Refuted in passing:** the claim that
  `ndsRendererNativeStagePreparedTexturesProven` sweeps Dream Land's literal run
  count on every stage. `nds_native_stage_select.inc` redefines
  `NDS_NATIVE_STAGE_RUN_COUNT` to the active packet's `run_count`, and it is
  included at `nds_renderer_assets.c:499`, well before that sweep at `:5663`.

## Native-failure baseline for all nine stages (2026-09-08)

Owner goal: every fighter and every stage reaches zero native failures, using the
parallel diagnostic path. This is the stage half's baseline, taken on the
shipping shell ROM `75279407bb64588aa6b80bed970ddb3bab507f0edf9bef280d5932d3889dc695`
through `scripts/diagnostics/probe-native-render-batch.ps1` with nine concurrent
runner slots.

Two runs: `native-stage-entry.json` at 16 presents (66 s wall) and the new
`native-stage-battle.json` at 300 presents (76 s wall). **Both give the same
verdict**, so the three failures are per-frame draws, not entry transients.

| stage | verdict | first failure at 300 presents |
|---|---|---|
| Dream Land, Congo, Zebes, Sector Z, Hyrule, Yoshi's Island | pass | none |
| Peach's Castle | fail | count 300, identity 0x3f50056, status 0, root 0x7558, material 0x2346f40, reason 1 |
| Mushroom Kingdom | fail | count 600, identity 0x3f5009b, status 0x8, root 0xb40, material 0x234c8f0, reason 1 |
| Saffron City | fail | count 1306, identity 0x3f200a0, status 0x7, root 0x420, material 0, reason 1 |

- Identity decodes as `(gobj id << 16) | asset id`, so the three assets are 86,
  155 and 160. All three are domain 2 STAGE, reason 1 NO_PROGRAM, and all three
  fire at least once per present: Castle once, Mushroom Kingdom twice, Saffron
  about four times.
- Mushroom Kingdom's asset 155 is the scale-platform chain, in flight.
  Saffron's asset 160 root 0x420 is the gate hazard, already measured as
  recognised, reached every frame, and emitting zero triangles. Castle's asset
  86 root 0x7558 is **not** the tower roof (that is binding 3, run 9, display
  list 0x1698) and has not been identified yet.
- **Caveat on the six passes.** 300 presents is roughly ten seconds of match, so
  a hazard with a long cycle has not fired -- Hyrule's tornado alone waits about
  2,449 ticks. Zero failures at 300 presents is a floor, not the whole claim; a
  match-length wave is owed before any stage is called clean.
- Freeze the ROM and ELF for a wave. These two runs used the live
  `builds/build-p2-shell` paths, which is only safe while no build is running;
  `builds/resume-20260908/frozen/` holds a copy for waves taken beside a build.

## Match length finds three more stage failures (2026-09-08)

The 300-present wave's own caveat was right. `native-stage-long.json` runs the
six stages that passed at 300 presents for **1,200** presents each -- about
forty seconds of match, long enough for a slow hazard cycle -- on six concurrent
slots, 117 s wall. Three of the six fail:

| stage | count | domain | identity | status | root |
|---|---|---|---|---|---|
| Dream Land, Zebes, Yoshi's Island | 0 | — | — | — | — |
| Hyrule | 54 | 2 STAGE | 0x3f3ffff | 0x4 | 0x236f460 |
| Congo | 56 | 2 STAGE | 0x3f3ffff | 0x2 | 0x23810a0 |
| Sector Z | 110 | 2 STAGE | 0x3f40099 | 0x1 | 0x1c50 |

- Hyrule and Congo share an identity shape: GObj id 0x3f3 with asset id
  **0xffff**, meaning no source asset at all, and a root that is a RAM address
  (0x236f460, 0x23810a0) rather than a bank offset. Those are objects whose
  display list is built at runtime, not loaded -- and the counts are small and
  late, which fits a hazard that only appears after a long wait. Hyrule's
  tornado waits about 2,449 ticks and so cannot appear in a 300-present run at
  all; Congo's is likely the same class.
- Sector Z is different: asset 0x99 = 153 at root 0x1c50, a real bank offset,
  110 times. That is a loaded display list with no native program, like the
  Castle, Mushroom Kingdom and Saffron failures.
- **Consequence for the goal.** "Zero native failures" has to be measured at
  match length, not at entry. The 16-present and 300-present waves are useful
  fast filters, but only the 1,200-present run is evidence. The fighter wave is
  480 presents and owes the same extension.

## The remaining native failures are Items, Weapons and Effects, not stage geometry (2026-09-08)

Decoding the identity's high half against `decomp/BattleShip-main/decomp/src/sys/objdef.h`
reframes every one of them. The failure record's domain 2 is the *recorder's*
domain -- the stage/ground-actor submit path -- not a statement that the object
is stage geometry:

| GObj id | kind | which failures |
|---|---|---|
| 0x3f2 = 1010 | `nGCCommonKindGround` | Saffron gate, asset 160 root 0x420 |
| 0x3f3 = 1011 | `nGCCommonKindEffect` | Pikachu asset 85 root 0x440; Captain, Hyrule and Congo with asset 0xffff |
| 0x3f4 = 1012 | `nGCCommonKindWeapon` | Samus asset 321 root 0x270; Sector Z asset 153 root 0x1c50 |
| 0x3f5 = 1013 | `nGCCommonKindItem` | Castle asset 86 root 0x7558; **Mushroom Kingdom asset 155 root 0xb40** |

Two things follow.

- **Mushroom Kingdom's remaining failure is an ITEM-kind GObj, not a stage
  binding.** The scale-platform work has been carrying file 155 into the stage
  packet, and the packet expansion is real (callbacks 7, DObjs 27, bindings 24,
  runs 65), but the object that records the failure twice per frame is drawn
  through the item path. That is why expanding the stage packet did not clear
  it, and it is the first thing the next pass on that stage should check.
- **A class, not six unrelated bugs.** Asset id 0xffff with a root that is a RAM
  address (Captain 0x2372be0, Hyrule 0x236f460, Congo 0x23810a0) means an Effect
  whose display list is built at runtime rather than loaded from a bank. Three
  different scenes hit it, so one fix plausibly covers all three. The others are
  ordinary loaded display lists with no program, per kind: two Weapons, one
  Effect, two Items and one Ground.

Fighter-domain failures are separate and are genuine fighter programs being
*rejected* rather than missing: Yoshi (identity 0x60152, status 0xdd, root
0x2050) at 8,360 in 1,200 presents and Link (0x50144, status 0xe1, root 0x1d88)
at 3,007.

## Fighter baseline at match length (2026-09-08)

Nine fighters, mirror matches on Dream Land, 1,200 presents each, nine
concurrent slots, 134 s wall. Every case connected attacks on both sides
(damage 13-37), so the runs exercised entry, idle, attack and damage rather
than an idle pose.

| verdict | fighters |
|---|---|
| zero native failures | Mario, Fox, Luigi, Donkey |
| Weapon/Effect with no program | Samus (91), Pikachu (137), Captain (46) |
| fighter program REJECTED | Yoshi (8,360), Link (3,007) |

Captain passes at 480 presents and fails at 1,200, which is the same lesson the
stage wave taught: entry-length runs are a filter, not evidence.

## Castle asset 86 is the bumper, and its two palettes do not share texels (2026-09-08)

- **Identified.** Asset 86 is `MiscData086` = ITCommonObject, and
  `86_ITCommonObject.reloc:183` binds `dITCommonObject_Gfx_0x7558` into
  `dITCommonObject_NBumper_Item_data_DObjDesc+0x30`. `grcastle.c:57` makes
  exactly one `nITKindGBumper` at stage init and never despawns it, which is
  the 300-in-300-presents count. GObj id 0x3f5 is `nGCCommonKindItem`, so it
  arrives on item link 11 and needs no new route.
- The display list is **one unlit, unculled, alpha-tested 360x360 quad**: two
  triangles, four vertices at (+/-180, +/-180, 0), one 32x32 CI4 texture at
  payload 0x7288, and a `G_DL` branch to segment 0xE slot 0 -- which is the
  live MObj material and **the only supplier of the palette SETTIMG**.
  `itgbumper.c:53-61` pulses X/Y scale 2.0 to 1.0 over ten ticks and
  `itgbumper.c:77` flips `palette_id` for three ticks on a hit, so the
  transform and the palette must both stay live.
- The Congo barrel precedent is wrong for it: that adapter requires a two-joint
  tree with **no** MObj on either joint (`renderer_adapter_matrix.c:7370`,
  `:7396-7397`), and the bumper is one DObj **with** an MObj -- the failure
  record's own `material` field proves it live. The entry-effect owner is the
  right shape, since it already carries a live palette image and is not in ITCM.
- **Refuted while implementing, and this is the blocker:** the design assumed
  the two source palettes (file 86 at 0x7260 and 0x7238) reindex to the same
  packed CI4 texels, so one resident image could serve both. They do not. The
  generator's own assertion caught it on the host, exactly as intended. A
  bumper owner therefore needs either two baked images or an index-preserving
  repack for this root; the generator edits were reverted rather than left
  half-applied.

## Saffron gate: decoded, and two of this file's own notes were wrong (2026-09-08)

- `MiscDataBank160`'s pointer words are **unrelocated intern-chain** words of
  the form `(next_slot << 16) | (target / 4)`. Decoding them as addresses is
  what produced two stale readings above: the gate chain does **not** branch
  out of bank at all, and the graded translucent shell is `0x0850 -> 0x0660`
  on head 1 of DObj slot 4, not `0x05D0 -> 0x0660`. `0x05D0` is the third
  opaque door face.
- The chain is four drawn slots off `DObjDesc_0x08A0`: three opaque door faces
  (roots 0x0420, 0x04F0, 0x05D0, five triangles total, one 32x32 CI4 texture
  and TLUT out of file 112) and the shell (root 0x0850, ten triangles, no
  texture, `G_CC_SHADE`, vertex alphas 0/30/180, inheriting
  `G_RM_AA_ZB_XLU_SURF` from head 1).
- The motion is a three-way iris driven by two AnimJoint banks: DObj 1 and 3
  slide +/-330 in Z over ten ticks, DObj 2 drops 420 in Y, and DObj 4 toggles
  `DOBJ_FLAG_NOTEXTURE`, which `objdisplay.c:1714` uses to skip a DObj's whole
  `dl_link` array. **So the shell is drawn only while the doorway is open.**
- The mechanism is the Zebes acid precedent -- a fourth generator owner on the
  existing Saffron packet, whose world matrix is composed from the live DObj
  every frame. The counts were verified by running the generator in process:
  bindings 17 to 21, runs 77 to 92, triangles 228 to 243, all fifteen new runs
  RAW, and every other stage regenerates byte-identically.
- **One genuinely new runtime capability is required.**
  `ndsRendererAdapterCollectNativeStageDObjs` currently rejects the entire
  stage topology when a DLLink DObj carries `DOBJ_FLAG_NOTEXTURE`, and the gate
  carries it for most of a match. Collection is once per scene and the topology
  stamp does not hash `dobj->flags`, so the hidden set has to become a per-frame
  mask on the frame struct and be honoured at commit, not at prepare.

## RETRACTED: both 20 FPS candidates above are refuted (2026-09-08)

The section "Why Saffron and Mushroom Kingdom run at 20 FPS" reasoned from two
numbers that do not reach the ROM. Do not cite it.

- **The rigid binding mask it ranked the stages by is not the live one.**
  `Makefile:7229-7232`: every stage but Dream Land is blob-resident and the ROM
  links only its registry row, so the whole `nds_native_stage_select.inc` packet
  row is not compiled for Congo, Saffron, Mushroom Kingdom or the rest. The live
  value comes from `_BLOB_RIGID_MASKS` in
  `scripts/stages/generate_nds_native_stage.py:4452`, which holds **only**
  `dreamland` and `yoster`. So Congo's runtime mask is 0, exactly like
  Saffron's and Mushroom Kingdom's -- and Yoshi's Island, which is the one blob
  stage that does carry a mask (`0x78014`), runs at 19.9 FPS. Two counterexamples;
  the mask does not order the stages.
- **The Dream Land texture corpus is never prepared on another stage.**
  `src/import/battleship_scvsbattle.c:153-157` gates
  `ndsRendererHardwarePrepareBattleStaticTextures` on
  `gkind == nGRKindPupupu`. On Saffron or Mushroom Kingdom the 44 pinned slots
  are never taken and the whole 123-slot cache is available, so the eviction and
  epoch-churn story cannot apply. A stage packet's texture-epoch count is a
  static table, not a count of evictions.
- What is left unexplained: Congo carries 136 no-Z triangles to Mushroom
  Kingdom's 64, under the same mask 0, and runs 7-9 FPS faster. The per-triangle
  matrix path therefore cannot be the gap either. The separating measurement is
  a `NDS_TASK103_STAGE_RUN_PHASE=1` build read on the Inishie and Congo routes:
  `gNdsTask103NoZPath[0..2]`, `gNdsTask103NoZWorldTicks`,
  `gNdsTask103NoZProjTicks` and `gNdsTask103BeginEndBatchTicks` say which phase
  actually differs.

## Mushroom Kingdom's moving SCALE-platform row is complete (2026-09-08)

Checked against the source in all three halves, not from the screenshot:

- **Geometry**: `grInishieMakeScale` (`grinishie.c:359`) builds the five-DObj
  chain from file-155 `DObjDesc_0x0380` plus two platform DObjs from DL 0x05F0,
  and the packet carries exactly that -- bindings 15-19 at roots
  0x01c8/0x02e8/0x0300/0x0328/0x0340 and bindings 20/21 both at 0x05f0.
- **Motion**: the port includes `grinishie.c` verbatim, and the older copy is
  held out by `#if !NDS_P2_STAGE_INISHIE` guards, so Wait/Fall/Sleep/Retract,
  the string follow and `grInishieScaleProcUpdate` are the source's own.
- **Collision, i.e. standable**: `grcommonsetup.c:26` fills the yakumono table
  from layer 1, and Inishie's `Layer1DObj[1]`/`[2]` are display-list-less
  collision DObjs which the packet carries with `binding_index 0xffff`. The
  `MPLineInfo` ids match `dGRInishieScaleLineGroups`, and the six allocated
  slots are well under the 64-slot cap.

**Scope, added 2026-09-09:** this closes the moving file-155 scale-platform
chain ONLY. It does not close the separate large static side-brick report in
`docs/BUGS.md`; those meshes come from file 107 and need their own visibility
evidence. Reading this section as covering both is what produced the retracted
"not in the packet at all" row near the top of this file.

The stage's remaining defect was never the platforms: it was the Item-kind
Pakkun at root 0x0b40, now natively drawn.

## Mushroom Kingdom's rigid mask IS derivable (2026-09-08)

Worth taking as an optimisation, not as the cadence fix. `grdisplay.c:206-215`
animates a layer only when `gr_desc->anim_joints != NULL`;
`260_GRInishieMap.c:44-47` gives that to layer 0 alone, and
`107_StageInishieFile2.c:777-789` leaves only joints 3, 5 and 7 scripted. In the
file-155 scale chain `grinishie.c` writes `map_dobjs[2]`/`[4]` (bindings 17/19)
and the two platforms (20/21) every tick; `map_dobjs[0]`/`[1]`/`[3]` are never
written. Layers 1-3 are static, and bindings 12/13 are camera-flagged and
already masked out. That gives **0x00C5CF57**, moving 58 of 64 projected-no-Z
triangles onto the cached EnsureWorld arm and skipping 15 of 24 per-frame
composes. The risk is recorded with it: a binding whose own transform moves is
caught by the source key, but one whose ANCESTOR moves is not and would freeze
at the capture-frame world -- every rigid bit here was checked for that.

## Every stage packet promotes source alpha 0 to fully opaque (2026-09-08)

`scripts/stages/generate_nds_native_stage.py:2099-2100`, in the vertex decoder,
verbatim:

```
    if a == 0:
        a = 0xFF
```

So a source vertex whose alpha is 0 -- the *invisible* end of a gradient -- is
baked into the packet as fully opaque. This is a strong candidate for the
owner's standing report that transparency is not being applied, and it is not
stage-specific: a census across the registered stages found Dream Land with 173
such vertices, Saffron's file 112 with 41, Castle 38, Zebes 11, and Saffron's
gate shell with 12 of its 30. On the gate that turns the graded rim into a solid
white edge, which is exactly the shape of both the Saffron and the Yoshi's
Island transparency reports.

Two reasons it has not simply been deleted, both worth respecting:

- Removing it regenerates and re-pins **every** stage packet, including the
  frozen P1 Dream Land include. Scope the first change to one file (Saffron's
  160) so every other packet stays byte-identical, get owner acceptance on the
  visible result, then sweep.
- It needs a paired runtime clamp. `ndsRendererHardwarePolyFmt`
  (`nds_renderer_textures_effects.c:1244`) passes the run alpha straight into
  `POLY_ALPHA()`, where **0 means wireframe** on DS hardware, not invisible. The
  particle path already clamps to 1 at `:6182-6185` for exactly this reason. A
  packet that starts emitting true zero alphas without that clamp would draw
  wireframes where the source draws nothing.

Note this interacts with the per-run alpha flattening recorded above: the
generator averages the three corner alphas per triangle, so even after the zero
is preserved, a gradient becomes a staircase of flat bands rather than a fade.
Both are measured deltas to put in front of the owner, not defects to hide.

## Link's owner is lost to a topology shift his own entry motion causes (2026-09-08)

MEASURED. The validate reject now publishes the whole observed root vector, and
Link's reads:

```
1D88 1F40 21E8 2358 2508 2828 2998 2B78 2C88 81C0 2E08 2EF0 3398 34A0 35B8 36B8 37C0 38E0 39E0
```

against `sNdsNativeLinkRoots`: **0x2630 absent, 0x81C0 inserted at index 9,
count unchanged at 19**. So it was never a root variant -- 0x2828 is Link's own
canonical binding-SIX root, and every ordinal from 5 upward has shifted by one.

The source causes it. `224_LinkMainMotion.c:1915-1920`, his entry motion, sets
model part **-1** on joint 11 (the sword, 0x2630) and **0** on joint 20 (the
scabbard, 0x81C0), waits 87 frames, then restores both; `ftparam.c:811` writes
NULL into a joint's `dl` for id -1. The whole owner is then declined at validate
code 4 and **Link is absent for about 1.45 s of every entry**.

The counts agree and are worth keeping: 3,007 over 1,200 presents is not a whole
number of 19-root draws. Solving 19a + 20b = 3007 gives a = 153 draws of 19 and
b = 5 of 20, and 20 live roots is exactly what Catch produces -- descriptor 12,
the hookshot, turns on. So this is 6.6% of Link's draws, not every frame.

The generator already refuses to model this as a variant
(`generate_nds_native_owners.py:2054-2058`: such cases "alter live topology, not
merely one admitted root"). The fix under review is a canonical-binding
collection -- descriptor order filtered by the commonparts DL that created the
joint -- with an active mask, so a joint whose `dl` goes NULL keeps its binding
and its ordinal and only its draw is skipped. That is under review before it
lands, because it changes the collection path for every fighter and the five
that currently pass must stay passing.

## Yoshi is NOT a topology problem: it declines at the production contract (2026-09-08)

MEASURED, and it refutes the hidden-part theory. With a decline-stage witness at
every site that clears `native_owner_enabled`, a Yoshi mirror match reads
**stage 7** -- the production contract at `renderer_adapter_fighter.c:3494` --
with the validate reject count 0. In production mode `native_owner_hierarchy_mode`
is FALSE, so the failing call is
`ndsRendererAdapterPrepareNativeOwnerMatrices(...)` returning FALSE. Not plan
resolve, not validate, not a root count.

The refuted theory was reasonable and is worth recording as refuted: Yoshi is the
only enabled fighter with a DL-bearing hidden part
(`247_YoshiMain.c:120-126`, joint 9 -> descriptor 5 -> `Joint_0x2800`), his
`OWNER_SETUP_PARTS` mask clears exactly that descriptor so the bake has 18 roots,
and 8,360 factors as 19 x 440. But a live count of 19 against a baked 18 would
have shown validate code 3 at stage 4, and it did not. Note also that 8,360 has
other divisors under 32 -- 20 x 418 and 22 x 380 -- so the factorisation was
never decisive on its own.

**The lesson for the next witness gap:** eight decline sites past plan-resolve
had no stage code, so every one of them read as stage 0, which is
indistinguishable from never declining. That is why Yoshi looked like a mystery
for two rounds. They all carry a stage code now.

## Asset 0xffff is not a broken asset: it is the runtime visual template (2026-09-08)

MEASURED, and one mechanism explains three separate failure rows. Captain
Falcon on Dream Land (46 in 1,200, root 0x2372be0), Hyrule Castle (54, status
0x4, root 0x236f460) and Congo Jungle (56, status 0x2, root 0x23810a0) all
record domain 2 STAGE, reason 1 NO_PROGRAM, GObj id 0x3f3 Effect, **asset id
0xffff**, and a root that is a RAM address rather than a bank offset. The status
field is the stage gkind and nothing else -- `ndsStageRejectNativeRender`
(`renderer_adapter_stage.c:4813`, and that file is moving under concurrent edits
so prefer the symbol) publishes `gSCManagerBattleState->gkind`, and `scene.h:823-831` gives Castle 0,
Sector 1, Jungle 2, Zebes 3, Hyrule 4, Pupupu 6, Inishie 8. Hyrule 0x4 and
Congo 0x2 are exactly those. So the three rows differ by stage, not by effect.

The display list is built into RAM at runtime and was never in a bank.
`battleship_efmanager.c:593` allocates `sNdsVisualTemplates` from
`syTaskmanMalloc`; each `NDSVisualTemplate` carries an inline
`Gfx display_list[NDS_VISUAL_TEMPLATE_COMMANDS]` (`:445`);
`ndsEFManagerBuildStar`, `BuildDust` and `BuildRing` write the commands at
`:483`, `:515` and `:547`; and `:849` attaches that RAM list to a DObj with
`gcAddDObjForGObj`. There are exactly **seven** templates (`:427`) -- Dust,
Normal, Fire, Electric, Sparkle, Wave, Death -- the kind is latched on the GObj
at `:890`, and `ndsEFManagerIsVisualEffectGObj` (`:792-815`) already recognises
one by display-list pointer equality. the same call writes
0xffff into the low half of the identity when `ndsRelocFindLoadedFileContaining` returns NULL and
falls back to the raw pointer for the root, so both fields are behaving
correctly and reporting exactly what is true.

That means the record can name the class but not the kind: it carries no script,
bank or texture id, so Death cannot be told from Sparkle or Dust. The fix does
not need to. Seven display-list addresses are enough to key on, the same way
`ndsEFManagerIsVisualEffectGObj` already does, and ImpactWave is the precedent
for keying an owner on the GObj rather than an asset id
(`nds_renderer_native_common.c:1051`, `battleship_efmanager.c:389`). One owner
closes all three rows.

## Sector Z: both Arwing lasers are one 27-command display list (2026-09-08)

MEASURED. The Sector Z row is identity 0x3f40099, status 0x1, root 0x1c50,
material 0, reason 1 NO_PROGRAM. Asset 153 is `MiscDataBank153`
(`nds_reloc_assets.c:270`), GObj 0x3f4 is Weapon, and root 0x1c50 is
`dStageSectorFile3_AnimJoint_0x1C50`, 27 Gfx commands over a six-vertex pool at
0x1BF0. `262_GRSectorMap.c:94-95` and `:124-125` both point at it: the 2D and
the 3D laser (`nWPKindArwingLaser2D` 0x12, `nWPKindArwingLaser3D` 0x13) share
one display list, so the failure record cannot separate them and an owner does
not have to. `material 0` means the DObj has no MObj at all -- this is an
untextured coloured list, the cheapest owner in the queue.

The spawn side is imported verbatim and is not in question:
`grsector.c:887-896` picks 2D when the laser count is 2 and 3D otherwise,
`:663-719` fires the 2D pair and `:798-884` the single 3D bolt, volleys are 30
ticks apart with a 240-tick cooldown (`:964`, `:974`), and the first Arwing pass
waits 600 ticks (`:1108`). That last value is why a 300-present run records
nothing and a 1,200-present run records a hundred-odd: the stage had not yet
sent an Arwing.

Weapon kinds reach the renderer through one seam,
`reloc_backend_movement.c` around `:12100-12160`, where only Blaster (`:12104`)
and Fireball (`:12131`) have native arms today; everything else falls through to
the generic stage submit and records NO_PROGRAM. Samus bomb, in the fighter
queue, arrives at the same seam.

## The full native-failure ledger, both axes, one build (2026-09-08 22:20)

MEASURED on `builds/build-p2-shell/smash64ds-p2-shell-hwtri.nds` built at 22:13,
nine stages and nine fighters at 1,200 presents each, 206 s and 215 s of wall
time for the two waves. This supersedes every earlier per-row count.

**Stages.** Dream Land, Yoshi's Island and Zebes are clean. The other six:

| stage | count | identity | root | what it is |
| --- | --- | --- | --- | --- |
| Peach's Castle | 1,260 | `0x3f50056` Item asset 86 | 0x7558 | the bumper, one draw a frame |
| Mushroom Kingdom | 161 | `0x3f3ffff` Effect, no file | RAM | runtime visual template |
| Sector Z | 110 | `0x3f40099` Weapon asset 153 | 0x1c50 | Arwing laser, both kinds |
| Saffron City | 59 | `0x3f5009f` Item asset 159 | 0x6a0 | **new row, see below** |
| Congo Jungle | 56 | `0x3f3ffff` Effect, no file | RAM | runtime visual template |
| Hyrule Castle | 54 | `0x3f3ffff` Effect, no file | RAM | runtime visual template |

**Fighters.** Mario, Fox, Luigi and Donkey are clean. The other five:

| fighter | count | identity | root | what it is |
| --- | --- | --- | --- | --- |
| Link | 3,007 | FIGHTER, REJECTED_PROGRAM | 0x1d88 | entry-pose topology shift |
| Yoshi | 350 | FIGHTER, REJECTED_PROGRAM | 0x2050 | **was 8,360** |
| Pikachu | 137 | `0x3f30055` Effect asset 85 | 0x440 | MBallRays entry rays |
| Samus | 91 | `0x3f40141` Weapon asset 321 | 0x270 | **Charge Shot, not the bomb** |
| Captain | 46 | `0x3f3ffff` Effect, no file | RAM | runtime visual template |

Two things fall out of reading the two tables together. The runtime visual
template is **four** rows, not three -- Mushroom Kingdom joins Hyrule, Congo and
Captain, and it only appears past 300 presents, which is why that stage read
clean when the Pakkun owner landed. And **one owner closes all four**, because
the class is keyed on a display-list pointer and not on an asset id.

## Yoshi: 8,360 to 350, and the bound was the whole story (2026-09-08)

MEASURED, before and after, same configuration. Raising
`NDS_RENDERER_NATIVE_FIGHTER_JOINT_MAX` from 27 to 40 took Yoshi's
FIGHTER/REJECTED_PROGRAM count from 8,360 per 1,200 presents to **350** -- 96%
of it. The bound was on the live DObj tree but had been sized from the baked
joint schedule, and Yoshi bakes exactly 27, so any active hidden part made the
topology walk decline; on the animlock arm that decline has no GX fallback.

The residue is real and is being measured with the six new compose-source
witnesses. Do not assume it is the same cause.

Two things worth carrying forward from that fix. Link bakes **30**, above the
old bound, so Link could never source-compose at all -- his 3,007 is a separate
defect but he was also never eligible for this path. And a GLM review of the
change confirmed the sentinel move from 31 to 0xff was required rather than
cosmetic: at a bound of 40, parent index 31 is a real index.

## Saffron's gate row is closed, and what replaced it is unrelated (2026-09-08)

MEASURED. Saffron City recorded 1,306 failures per 300 presents at
identity `0x3f200a0`, root 0x420, asset 160 -- the gate ground actor, reached
every frame and emitting zero triangles. After the native gate owner landed that
row is **gone**. What the stage records now is 59 per 1,200 presents at
identity `0x3f5009f`, status 0x7, root 0x6a0, material 0, reason 1: GObj 0x3f5
is Item, asset 0x9f is 159, and material 0 means no MObj at all, so it is an
untextured coloured item list. That is a different object, roughly one frame in
twenty, and it has never been identified.

## Samus is Charge Shot, and the loader was never wrong (2026-09-08)

MEASURED, and it retires two rounds of suspicion about the relocation chain.
Samus records `0x3f40141` root 0x270, and 0x270 is not a display list: file 321
holds `dSamusSpecial3_JointVerts_Vtx`, seven vertices at 0x230, and
`dSamusSpecial3_BombDL_DisplayList` at 0x2A0, so 0x270 is four vertices into the
pool. That looked like a mis-resolved pointer. It is not.
`218_SamusSpecial1.c:22` sets `dSamusSpecial1_ChargeShot_WeaponAttributes.data`
to `&dSamusSpecial3_JointVerts_Vtx[4]`, commented `321+0x270`, and
`218_SamusSpecial1.reloc:4` carries the matching extern fixup. **The source
itself points Charge Shot's attributes there.** The bomb is a different row
entirely: its data resolves into file 320 at asset 0x140, and nothing live points
at `BombDL`.

So the recorder, `ndsRelocNativeRootOffset` and the extern fixup chain are all
correct and none of them should be touched. The owner to build is Charge Shot
against asset 0x141 root 0x270; Samus's bomb is a future row that has not been
measured yet.

## Both fighter witnesses fired, and both name their cause exactly (2026-09-08)

MEASURED at 480 presents on the 22:13 ROM, one Yoshi mirror and one Link mirror.

**Yoshi.** `DIAG_FTDECLINE=7` as before -- the production matrices contract --
and the new witness says which of that call's six declines fired:
`DIAG_FTCOMPOSE=5,28,23`. Fail 5 is
`ndsRendererAdapterBuildSourceFighterLocalMtx` returning FALSE; the live tree is
**28 joints**, which is past the old bound of 27 and inside the new one of 40;
and it is joint index **23** that fails. So the bound really was the first
defect and is fixed, and what remains is one joint whose local matrix cannot be
built. That function declines four ways -- a zero accumulated scale, an angle at
or past 16 radians, an unsupported XObj kind, and two parts matrices on one
joint -- and fail 5 does not yet separate them. Splitting it is the next step,
and it is four stores.

**Link.** The set-delta witness reads exactly what the design predicted:
`DIAG_FTSETDELTA=0x5,0x9,0x81c0` -- canonical binding **5** has no observed
offset, and observed index **9** carries **0x81C0**, which matches no canonical
binding. One line, no ELF diffing, no arithmetic. The old six words said only
`binding 5 observed 0x2828 expected 0x2630`, which reads like a variant and is
not one.

## Saffron's new row is Electrode, and it shares a class with Sector Z (2026-09-08)

MEASURED. The 59-per-1,200 row at identity `0x3f5009f`, root 0x6a0, material 0
is the Marumine (Electrode) ground monster body.
`159_StageYamabukiFile3.c:115-118` declares `dStageYamabukiFile3_DL_0x06A0[30]`
at `/* @ 0x06A0 */` over `Vtx_0x0660[4]` (`:110-113`), and its mobjlink array at
`:127-132` is three NULLs, which is why the failure record carries material 0.
`264_GRYamabukiMap.reloc:17-18` binds it through
`dGRYamabukiMap_Marumine_ItemAttributes` at 0x790.

The spawn side explains the count. `gryamabuki.c:51-52` sets
`monster_wait = syUtilsRandIntRange(1000) + 1000`, `:89-99` picks uniformly from
the five ground monsters between `nITKindGroundMonsterStart` and
`...End`, and `:208-217` restages both timers. So Electrode is one birth in five
on a mean interval near 1,500 ticks, which lands in the right order for 4.9% of
presents.

It shares one property with the Sector Z Arwing laser -- neither has an MObj --
and a first reading called both "untextured coloured lists" and proposed one
generator for them. **Corrected the same evening:** `material 0` in a failure
record means `dobj->mobj == NULL`, nothing more. The Sector laser turned out to
be fully textured, carrying its own CI4 image and RGBA16 TLUT inline and binding
them through file 153's only two external fixups into file 161. Do not read
"no MObj" as "no texture"; decode the list. Whether Marumine and Charge Shot are
genuinely untextured is unmeasured, so the shared-generator idea stays a
hypothesis until each list is decoded.

## Samus Charge Shot draws vertex bytes as a display list (2026-09-08)

MEASURED, and it explains why the root looked like a mis-resolved pointer.
`wptypes.h:38` says `WPAttributes.data` is a `DObjDesc*` only when
`WEAPON_FLAG_DOBJDESC` is set, and Charge Shot's descriptor
(`wpsamuschargeshot.c:121-124`) has flags `0x00`, so `data` **is the display
list**. `wpmanager.c:268` attaches it with `gcAddDObjForGObj` and `:270` selects
`wpDisplayDLHead1`, which `objdisplay.c:1519` submits as
`gSPDisplayList(dl_head[0]++, dobj->dl)`.

That pointer is `&dSamusSpecial3_JointVerts_Vtx[4]`, three vertex records short
of `BombDL` at 0x2A0. Three `Vtx` records are 48 bytes, which is six GBI words,
so the source is deliberately executing packed vertex bytes as six display-list
commands and then falling into `BombDL`. Nothing is broken: the data is packed
that way on purpose, and any owner must decode those six words from the payload
rather than assume the program starts at 0x2A0.

Charge Shot draws in both states — `wpdisplay.c:161` draws even with
`attack_state == nGMAttackStateOff`, so the held charging ball counts as well as
the fired one — with a translate-only matrix (`nGCMatrixKindTra`), a scale from
`gfx_size`, and a Z spin of 18 degrees a frame. There is no MObj
(`218_SamusSpecial1.c:23` has `p_mobjsubs` NULL), so this is a third member of
the untextured vertex-coloured class above.

## The Sector Z laser decoded: eight triangles, and it carries its own texture (2026-09-08)

MEASURED by running a new generator against the pinned O2R data; every pin,
census and count below passes today. File 153 root 0x1c50 is twenty-seven Gfx
words drawing a **closed six-vertex, eight-triangle spindle** -- apexes at
z +569 and z -564, a four-corner ring between them. It clears
`SHADING_SMOOTH | LIGHTING`, draws, and restores both.

**It is fully textured, and the brief that called it untextured was wrong.**
`material 0` in a failure record is `dobj->mobj == NULL`, nothing more. This
list owns its whole material inline: it sets `TEXTLUT = G_TT_RGBA16`, loads a
sixteen-entry TLUT and a sixteen-by-sixteen CI4 image, and both come from
**file 161, FoxSpecial3** -- 0x10C8 and 0x10F0 -- through file 153's **only two
external fixups**. The combiner is `(TEXEL0 - 0) * SHADE + 0` for colour and
`TEXEL0` for alpha, with lighting cleared, so shade is the raw vertex colour and
all six vertices are white: the drawn pixel is the texel. All sixteen palette
entries are opaque; the CI4 image is a corner-radial green glow drawn with
`G_TX_CLAMP` on both axes.

Three things fall out that an owner must respect. The list emits **no**
`G_SETOTHERMODE_L`, no prim and no env colour, so the blender, colours and cull
state are inherited from the link-14 weapon pass -- inheriting them **is** the
source behaviour and an owner must not seed them. The texture is in another
file, so the admission must **compare** the two relocated SETTIMG pointers
against file 161's base rather than assume the loader's fixup pass ran; an
unrelocated word is still a chain word and binding it would draw garbage. And
the two laser kinds differ in **no drawn respect** -- one list, one DObj, same
transform triple, differing only in damage, element, knockback and collision
width -- so one owner serves both.

The admission is unambiguous, which is the property the Castle bumper lacked. A
whole-image pointer census finds exactly two pointers to file 153 root 0x1c50 in
the entire game: GRSectorMap external fixups 0x00BC and 0x00F0, the
ArwingLaser2D and ArwingLaser3D `WPAttributes.data` fields. No internal fixup
inside file 153 targets it. The generator asserts that census and fails if it
ever changes.

## Sizing P2 step 5: forty-one item owners, not forty-five (2026-09-08)

An inventory of all 45 item kinds, read from the source spawn tables. Only three
pairs genuinely share a shape -- the green and red shells, the two bumpers, and
the two Chansey entries -- so the 45 kinds reduce to **42 distinct owner
shapes**, of which exactly one is landed: the Mushroom Kingdom Pakkun. That
leaves 41.

Worth knowing before any of it is scheduled. Thirteen of the kinds are Poke Ball
Pokemon and are gated behind a Ball being thrown and opened, with an anti-repeat
rule and a one-in-151 Mew that also needs a newcomer unlock; they cannot appear
until the Ball itself does. The five Saffron ground monsters are **not** Poke
Ball content -- they live under `itground/`, are gated by the Yamabuki doors, and
cost no Ball work. `PROJECT_GOAL.md` counts both sets in step 5.

By ordinary items-on VS frequency the uncovered order starts Capsule, Box,
Barrel, Star Rod, Tomato, Master Ball, and every kind carries the same 1,400-tick
pickup lifetime with a spawn interval of ten to thirty seconds by rate. Egg
weights zero on Dream Land and never spawns there, which is worth stating because
the opposite is widely assumed.

None of this needs a runtime interpreter: every kind reduces to the Pakkun shape,
a fixed owner consuming a typed material snapshot.

## Yoshi's Island transparency is a texel-alpha problem, and the code says so (2026-09-08)

The vertex-alpha promotion is eliminated with evidence: on yoster it moves zero
runs whose material can read shade alpha. Every run it touches is on a material
the runtime ignores vertex alpha for, or on a combine whose alpha output is
TEXEL0 alone. So the transparency this stage is missing is carried by TEXEL
alpha, and the defect is on the conversion or residency side.

The stage binds three render-tile families. **CI4 with an RGBA16 palette** is
the dominant one, and it is lossless: the N64 palette entry carries one alpha
bit and the DS COLOR0-transparent mode reproduces it exactly. **RGBA32** appears
once and collapses eight-bit alpha to one bit -- anything not fully transparent
becomes fully opaque. And **I4**, the minority family, is the interesting one.

I4 has no alpha channel of its own; its sixteen intensity levels *are* the fade.
The dedicated A5I3 path exists precisely to carry that, but it only engages
under a specific prim/env blend mode, and everything that misses it falls back
to a one-bit conversion that thresholds at `coverage >= 16` -- sixteen levels
cut to two. The comment beside that fallback already records the symptom in
another context: *"threshold sixteen levels down to two, and the beam drew
hard-edged where the source fades."* Hard-edged where the source fades is
exactly what the sun's rays look like in
`artifacts/visibility/wave-0908s-yoster.png`.

That is a mechanism, not yet an identification: which of the stage's 48 texture
epochs draws the sun, and whether it reaches the A5I3 path or the fallback, is
unmeasured. **The probe that settles it** reads, at the sun's bind:
`ndsRendererHardwarePrimEnvTexel0BlendMode`, the format and size, width and
height against the upload dimensions, the wrap bits, and the PrepareRun fail
step. Do not widen the A5I3 blend-mode test or force COLOR0 globally to make it
go away -- the Saffron door already recorded that per-palette global
transparency is the wrong lever.

## The owner wave that closed five rows, and what it exposed (2026-09-08 23:35)

MEASURED, both axes, before and after on the same build.

**Stages: six failing to three.** Hyrule Castle 54 to 0, Congo Jungle 56 to 0,
Sector Z 110 to 0. Dream Land, Yoshi's Island and Zebes stay clean, so six of
nine stages are clean. **Fighters: five failing to four**, Captain Falcon 46 to
0, so five of nine are clean.

Three owners did it. The runtime visual-template owner closed **four** rows at
once -- Hyrule, Congo, Captain, and Mushroom Kingdom's share -- because the
class is one bug on four stages. The Sector Z Arwing laser owner closed its own.

Two rows moved rather than closed, and that is the expected unmasking, not a
regression: the first-failure record is sticky, so removing the row that held it
reveals the next one.

- Mushroom Kingdom 161 to **105 at a new identity**: `0x3f5009b`, Item asset
  155, root 0x10d0. Same file as the Pakkun, different root.
- Peach's Castle 1,260 to **1,200**, the bumper alone now.
- Link 3,007 FIGHTER rejects to **2,000 at `0x3f50161`**, Item asset 353 root
  0x16f8. His owner draws; his item does not.
- Samus 91 to 70 and Pikachu 137 to 123, both from the visual-template share.

## Yoshi's last 342 is a Z rotation past sixteen radians (2026-09-08)

MEASURED with the split witness. `DIAG_FTCOMPOSE=1035,28,23` decodes as low byte
**11** and high byte **4**: route 11 is `ndsFighterMatrixAngleToIndexExact`
refusing an angle, and mask 4 is the **Z** component. Joint 23 of a 28-joint
live tree has a `rotate.vec.f.z` at or past 16 radians, or NaN or infinite.

That retires the zero-scale theory, which was reasonable and is worth recording
as refuted: a uniform zero accumulated scale really is how source collapses a
subtree -- `lbCommonMatrixTraRotScaInv` multiplies every row by zero before the
saturated reciprocal reaches it -- and declining it as a corrupt chain really
was wrong. Accepting it is now correct behaviour and it stays. It just was not
Yoshi's cause: the count moved 350 to 342.

The bound itself is `nds_fighter_matrix_index.h`, which refuses a biased
exponent above 130, i.e. any magnitude in `[16, 32)` or beyond. Source has no
such bound -- it calls the trig functions, which accept any magnitude, and
angles are periodic. So the fix is a range reduction rather than a wider table,
and it has to preserve the exactness the index path exists to provide.

**And Link's compose is clean.** The same run reads `DIAG_FTCOMPOSE=0,0,0` and
`DIAG_FTDECLINE=0,0,0` for Link, so the second owner program resolves without
declining anywhere on that path.

## The item DL head never advances, so every item reads head 0's env (2026-09-09)

MEASURED, found while landing Link's Bomb. `sNdsRendererAdapterItemSubmitHead`
is assigned in exactly two places, both `0u`, both in
`ndsRendererAdapterSubmitItemDObjTree` (`renderer_adapter_stage.c:7113` on the
way in and `:7117` on the way out). The DObj tree walk between them never
touches it. So the field reads **0 for every list of every item**, however many
display lists that item's tree offers.

Two consequences, one avoided and one still open.

**Avoided.** The Link Bomb admission was drafted to gate the fuse list on
`head == 1`, matching the source's own `DObjDLLink` assignment (0x18b8 is
list_id 0, 0x18c8 is list_id 1). That test can never pass. It would have
declined every fuse draw at candidate step 3 while the body drew normally --
the item half-drawn, its count falling to about half rather than to zero, and
nothing saying why. The root alone already discriminates the two lists, so the
landed arm does not gate on the head at all and records the observed value in
`gNdsLinkBombHead` instead. **A redundant guard is not free: a redundant guard
built on a field the port does not maintain is a silent half-failure.**

**Still open.** `renderer_adapter_stage.c:6275-6283` selects the ColAnim
damage-flash colour as `sNdsRendererAdapterItemEnvColor[head]` with the same
always-zero field. Any item whose source emits a *different* env into head 1
than into head 0 therefore flashes the head-0 colour on both lists. Link's Bomb
is not affected -- its fuse list carries its own `SETENVCOLOR` word and
overrides whatever the layer seeded -- so this was invisible here. It needs a
census of which items write two different env values before it can be sized;
fixing it means teaching the tree walk to carry the `DObjDLLink` list_id, which
is wider than any one owner.

## The KO pillar's env-drop theory is not supported by the first measurement (2026-09-09)

MEASURED. `DIAG_PARTICLE_ENV` is new: it prints
`gNdsParticleEnvVariantBakeCount, HitCount, FallbackCount`, the three counters
the particle env-colour variant cache has always maintained and which **nobody
had ever read live**. On Dream Land, 1,200 presents, `smash64ds-p2-shell-hwtri`:

    DIAG_PARTICLE_ENV=4,98,0

Four bakes, ninety-eight hits, **zero fallbacks**. The standing theory for the
owner's "thin yellow streaks instead of a blast pillar" was that the submitter
drops the env colour, so the pixel `(PRIM - ENV) * TEXEL + ENV` collapses. That
theory requires the fallback path; the fallback path was not taken once.

Two corrections fall out of this.

**The BUG_NOTES line numbers were stale.** The earlier note claimed submit
"passes ONLY the primitive colour" at `battleship_lbparticle.c:4065-4067` and
`:4165-4170`. The current tree passes prim AND env at `:4248-4262`, gated on the
source's own `LBPARTICLE_FLAG_ENVCOLOR` write flag, with a comment naming the KO
pillar scripts explicitly. That half of the fix already landed; the note
outlived it and would have sent the next reader to re-fix a fixed thing.

**What this does NOT establish.** The counters are shared across every particle
that uses the variant cache, and this window may contain no KO at all — the case
is a one-human-one-CPU Dream Land match and nothing forced a blast-off. So
`fallback=0` refutes the general env-drop mechanism for the particles that ran;
it does not by itself clear the pillar. The cheap next step is a probe that
forces a KO and reads the same three counters, not another static reading of the
submitter. Do not record the pillar as explained until that runs.

A separate diagnostics gap is worth carrying: KO pillar particles are link 2,
and the two existing particle counters are gated on `link == 1`, so they cannot
see a KO submit failure at all. That gap is why this row survived several
investigations.

## The shell arena floor is 1,968 B, and the 74,612 B closes exactly (2026-09-09)

MEASURED, `verify-p2-shell-loop.ps1` run alone:

    ARENA FREE FLOOR: 1968 B (minimum required 32768)

That is 6% of the required reserve, and it fails the arm on its own assertion.
The board still describes this arm as "red only on a Dream Land MObjSub
attachment decline", which is stale by a wide margin.

**It is not a mystery, and it is not a leak.** Diffed against the last green
run of the same arm, `artifacts/performance/2026-09-06_shell-results-floor/fixed-run.txt`,
which read 76,580 B:

| scene kind | 09-06 | 09-09 | delta |
|---|---|---|---|
| Title 1 | 506,736 | 506,736 | 0 |
| ModeSelect 7 | 248,992 | 248,992 | 0 |
| VSMode 9 | 387,928 | 387,928 | 0 |
| **PlayersVS 16** | 1,275,100 | **1,321,040** | **+45,940** |
| Maps 21 | 154,800 | 154,800 | 0 |
| VSBattle 22 | 1,237,000 | 1,208,328 | -28,672 |
| VSResults 24 | 1,078,628 | 916,316 | -162,312 |
| Startup 27 | 160,460 | 160,460 | 0 |
| **free floor** | **76,580** | **1,968** | **-74,612** |

**The arena really did shrink 28,672 B, and the run prints it.** I asserted
that by back-solving, then retracted it as an unmeasured inference, then found
that both runs record the arena size directly on a line I had not read:

    2026-09-06  LOOPARENA base=0229c3a0 size=1351680
    2026-09-09  LOOPARENA base=022a31e0 size=1323008

Both satisfy floor = size - peak exactly (76,580 + 1,275,100 and
1,968 + 1,321,040), so the relationship holds and the delta is real. The
retraction's reasoning was wrong on both legs: the two rings are related after
all, and the 1,548,288 B figure in `P2-2-four-fighters.md:288` is the
FOUR-FIGHTER battle arena, a different configuration from the shell loop. The
original claim was right and under-evidenced at the same time -- the evidence
was sitting in the artefact.

**And the mechanism is better than "a sizing constant".** 28,672 is exactly
7 x 0x1000. `ndsTaskmanArenaBytes` in `src/port/diagnostics_taskman_heap.c:23-60`
does not take a constant: it starts at `NDS_TASKMAN_ARENA_SIZE` and walks DOWN
one 4,096-byte page at a time until `calloc` succeeds. So the arena is
best-effort, and losing exactly seven pages means the DS heap could no longer
satisfy the larger request -- something OUTSIDE the arena grew by about 28 KB
and pushed the negotiation down seven notches. Nobody edited a size.

That reframes the second question. It is not "who took 28 KB from the arena" but
"what started allocating ~28 KB from the libc heap between 09-06 and 09-09".
`gNdsTaskmanArenaChosenSize` and `gNdsTaskmanArenaAllocFailCount` are live
globals (`nds_startup.h:732-733`) and both are in the shell-loop ELF, so the
negotiated size and the number of failed attempts can be read directly rather
than inferred again.

So: what added 45,940 B to the character select, and what added ~28 KB of libc
heap pressure that cost the arena seven pages.

Note the two scenes that got CHEAPER, because they will mislead anyone reading
only the totals: VSBattle is down 28,672 and VSResults down 162,312. Neither
helps the floor, because neither is the peak.

**This is not today's five native owners.** They add static ROM packets, a
handful of `volatile u32` witnesses in BSS, and no taskman-arena allocation at
all; and every one of the unchanged scene high-waters above is bit-identical
across the two runs, which would not be true if a shared allocation had moved.

## Both remaining fighter rows are live-material siblings (2026-09-09)

MEASURED, fighter wave on `smash64ds-p2-shell-hwtri` at 1,200 presents after the
Thunder Jolt owner landed:

    pikachu cnt=22  Weapon asset 342 root 0x1660 material=0x23c6900
    link    cnt=144 Effect asset  85 root 0x2ef0 material=0x23c6928

Pikachu moved 23 to 22 and its ROOT moved, 0x0270 to 0x1660, inside the same
file. So the air Thunder Jolt owner works and contributed exactly one of the 23;
the other 22 are a sibling root that was masked behind it. `nWPKindThunderJoltAir`
and `nWPKindThunderJoltGround` are adjacent in `wp/wpdef.h:52-53`, so the ground
variant is the obvious candidate for 0x1660 -- to be confirmed, not assumed.

**The shape has changed and this is the useful part.** Every owner landed today
had `material 0`, i.e. `dobj->mobj == NULL`, and could bake its whole material.
Both remaining rows report a non-NULL MObj pointer, so both take live material
state per frame and want the Mushroom Kingdom Pakkun shape -- a segment-0xE hook
with the live half taken from the MObj snapshot -- not the Bomb or Charge Shot
shape. Anyone picking these up should read
`scripts/stages/generate_nds_native_inishie_pakkun.py` first, not the five
owners landed today.

Expect a row per ROOT, not a row per file. Asset 342 and asset 85 each hold
several sibling display lists, the failure record latches only the first, and
each owner reveals the next.

## Zebes' "low-poly dome" is an alpha collapse, not geometry (2026-09-09)

MEASURED, and it falsifies every geometry hypothesis including the one this
project would normally reach for first.

The acid is a flat fan. Decoding `MiscDataBank157` root 0x09D8 gives eight
vertices with **Y = 0 exactly** at every one, and the native packet emits all
seven source triangles. So there is no dome in the source, no missing-triangle
subset, no curving transform, and no fixed-point curvature: the DS 1.12 path
does quantize X and Z, with a maximum error of 4 source units over a 9,876-unit
extent (0.0405%, or 0.0156 DS world units), but zero stays zero exactly, so the
vertical error is **0 at every acid vertex**.

The defect is that the N64 interpolated alpha PER VERTEX and the DS carries it
PER POLYGON. The source has alpha 220 at the centre and two far vertices and 0
at the other five, and the generator averages each triangle's three values, so
the seven triangles become flat facets of 73, 73, 73, 147, 73, 147, 220 -- DS
polygon alpha 9, 9, 9, 18, 9, 18, 27 after the `>> 3`. Seven broad radial
opacity bands over a flat plane is exactly what reads as a low-poly dome. The
combiner really does consume shade alpha (`combine_alpha_reads_shade` true on
all five acid runs, combine words 0xfc272c04 / 0x1f1093ff), so this is a live
field and not a dormant one.

Fixed by subdividing **only** that root once at edge midpoints, opt-in through a
new `alpha_subdivide_roots` descriptor field: 7 triangles to 28, stage 151 to
172, slab 12,226 to 13,696 bytes. Every midpoint is an exact average of two
source vertices, so the plane stays planar and no geometry is invented, and the
collapse now quantizes four small facets where it had one large one. Exactly one
of 39 stage packets changed.

**Defect B, the fighter sinking before damage, is REFUTED as a port bug.**
`src/import/battleship_grzebes_ground.c` literally `#include`s the source
`grzebes.c`, so the active predicate IS the source's own
`fighter_root_y < acid_root_y + acid_child_y` at grzebes.c:233. Wrong fighter
point, wrong root/child term, sign or offset error, a frozen acid transform and
a reduced check cadence are all separately falsified -- the acid binding is live
(`rigid_binding_mask = 0x0`), and the 30 Hz presentation still runs the source
predicate once per 60 Hz source tick. Source `ftMainUpdateDamageStatGround` sets
`fp->acid_wait = 30` after a hit, so **repeat** acid damage is deliberately
delayed thirty ticks while the fighter stays submerged. That is original
behaviour. Do not "fix" it with a Y offset or by switching to a collision
bottom; either would diverge from the specification on purpose.

## Pikachu's last row is one weapon with six live-material lists (2026-09-09)

MEASURED, decoded from the pinned payload. The row is
`Weapon asset 342 root 0x1660, count 22, material non-NULL`, and it is the last
native failure anywhere: nine of nine stages and eight of nine fighters are
clean.

Asset 342 is `PikachuSpecial3`. `DObjDesc 0x1888` -- reached from
`PikachuSpecial1` (file 244) slot 0x0034 -- has two non-drawable entries and
then **six drawable children, all id 2**, whose DObjDLLinks at
0x1828/0x1838/0x1848/0x1858/0x1868/0x1878 resolve to six roots:

    0x1490  0x1528  0x15c0  0x1660  0x16f8  0x1790

Every one is 19 or 20 Gfx words, draws exactly **one triangle**, and carries
exactly **one segment-0xE call**. The attribute's `p_mobjsubs` at 342:0x1018
has NULL in heads 0 and 1 -- matching the two non-drawable DObjs -- and six
MObjSub lists at 0x1350, 0x1358, 0x1360, 0x1368, 0x1370, 0x1378, one per
drawable child. So this is the Mushroom Kingdom Pakkun shape six times over:
image taken live through segment E, everything else baked. Root 0x1660 also
bakes its own SETPRIMCOLOR and SETENVCOLOR at words 2 and 3.

**Do not model it on the five owners landed today.** Every one of those had
`material 0` and could bake its whole material; this cannot. Read
`scripts/stages/generate_nds_native_inishie_pakkun.py`.

**Identified: it is the GROUND Thunder Jolt.**
`llPikachuSpecial1ThunderJoltGroundWeaponAttributes` is 0x34
(reloc_data.us.h:3726), which is exactly the slot whose `data` resolves to
DObjDesc 342:0x1888. So this is the sibling of the AIR jolt at 0x0270 whose
owner landed earlier today from `...ThunderJoltAirWeaponAttributes` at 0x0.
`nWPKindThunderJoltAir` and `nWPKindThunderJoltGround` are adjacent at
`wp/wpdef.h:52-53`. Six one-triangle children is the ground spark's segmented
trail.

**And the latch order is explained: DOBJ_FLAG_HIDDEN.**
`ndsRendererAdapterSubmitStageDObjTreeDepth` (renderer_adapter_stage.c:7516)
puts BOTH the node's own draw and its child walk inside
`if ((dobj->flags & DOBJ_FLAG_HIDDEN) == 0u)`, while the sibling walk sits
outside it. A hidden child is therefore skipped entirely -- no draw, no submit,
no reject, nothing recorded -- and its siblings still run. So the record latching
the FOURTH child is what you get when the first three are hidden at that moment,
which is exactly how a segmented spark that grows along the ground behaves. It
also explains why 22 is not a multiple of six: the visible subset changes frame
to frame, so the reject count tracks visible segments, not segments.

That resolves the open question without a runtime witness. It does NOT tell you
how many of the six ever become visible, so the owner should cover all six
roots: they are the same 19-or-20-word shape and owning five would leave the
same class of half-drawn object the Link Bomb review rejected.

**The live material is a texture index.** `wppikachuthunder.c:172` and `:196`
write `DObjGetStruct(weapon_gobj)->mobj->texture_id_curr`, once to a fixed 3 and
once to `syUtilsRandIntRange(WPPIKACHUTHUNDER_TEXTURES_NUM - 1)` over the four
textures at `wpvars.h:50`. That is precisely the Pakkun contract -- image chosen
live through segment 0xE, everything else baked -- and it is why each of the six
children carries its own MObjSub list.

## WRONG: Saffron's gate was never silent -- I misread the counter (2026-09-09)

**This whole row is retracted. There is no silent-empty-draw defect on Saffron's
gate, and there never was.** I read `DIAG_OWNERTRI={228, 320, 306, 0, ...}` as
"three stage layers draw and the fourth stage owner draws nothing", matched slot
3 against `yamabuki.py`'s fourth `owner_spec` -- which is named `gate` -- and
built a native-contract violation on top of that coincidence.

`sNdsRendererFastOwnerTriangleCount` is indexed by `NDSRendererProfileOwner`
(`nds_renderer.h:536-540`), not by stage `owner_spec`. Slot 0 is STAGE, slot 1
MARIO, slot 2 FOX, slot 3 LUIGI. A stage case walks Mario against Fox, so
`{228, 320, 306, 0}` means the stage drew 228 triangles, Mario 320, Fox 306, and
Luigi 0 because Luigi is not in the match. That is exactly correct output. The
alignment between "slot 3" and "the fourth owner_spec" was pure coincidence, and
`gate` being that spec's name is what made it convincing.

The board's 09-08 reading of `seen=480 reject=480` may still describe something
real; it came from a different instrument and is not evidence for or against
this. Saffron's door transparency stays open on the owner's own report.

What survives is the binding census, which is worth keeping because it was
measured properly and shows the gate geometry IS present:

    idx asset root      runs tris
     17     1 0x0420     2     2
     18     1 0x04f0     1     1
     19     1 0x05d0     2     2
     20     1 0x0850    10    10

So the two gate-door roots the descriptor names, 0x0420 and 0x04f0, carry runs
and triangles, and asset 1 contributes 15 runs and 15 triangles across four
bindings. The packet is not the problem.

Binding 16, root 0x8688, does carry 0 runs and 0 triangles, and that is also
correct: it is the OWNER-AUTHORIZED haze-panel omission, declared through
`omitted_draw_roots=((112, "layer3", 2, 1, 0x8688),)` at `yamabuki.py:166`,
which deliberately keeps the binding and DObj identity so runtime topology still
validates while emitting nothing. A zero-run binding is a supported, documented
state.

**The lesson is the one this repository keeps paying for: an array index is not
an identity.** `DIAG_OWNERTRI` names no owner_spec, and I never opened the enum
before building three documents and an agent brief on what its fourth slot
meant. The counter was printing exactly what it should. Check what the harness
already prints, and check what its indices MEAN, before treating a zero as a
defect.

## Yoster and Inishie BGM: three measured defects, none of them sanctioned (2026-09-09)

MEASURED. The owner's "part of the BGM sounds garbled, but like only one
instrument" on Yoshi's Island is attributable, and it is conversion damage
rather than a deliberate DS-budget compromise. PROJECT_GOAL puts audio first in
the sacrifice order, but only for MEASURED conflicts; none of these three has
one, and the first two cost nothing at DS runtime at all.

**One: the renderer resamples 32 kHz source straight to 22.05 kHz with per-voice
linear interpolation and no antialias filter.** That damages Inishie and Yoster
markedly more than the accepted Dream Land. Mixing at the bank's own 32 kHz
first and band-limiting the finished mix to the same 22.05 kHz output removes
most of the error for **zero ROM growth, zero DS RAM, zero DS runtime CPU** --
it is all offline.

**Two: CSEQ controller 21 is ignored, and BattleShip uses it as the compressed
sequence player's master volume.** Dream Land sets 127, Inishie 99, Inishie
Hurry 99, Yoster 86 before their first notes. So the port plays Inishie about
+2.16 dB and Yoster about +3.39 dB louder than source semantics. It does not
clip, so it is a fidelity bug rather than an artefact, and it is baked offline.

**Three: Mushroom Kingdom's second track is still IMA ADPCM.** The stage swaps
`gMPCollisionBGMDefault` to `nSYAudioBGMInishieHurry` at `time_remain <=
I_SEC_TO_TICS(30)` (`if/ifcommon.c:2500`), which is source behaviour. The port
stores sequence 2 as PCM16 but sequence 3 as IMA, whose measured codec SNR is
**18.98 dB against 28.80 dB for the accepted Dream Land**. Nothing measured
justifies keeping the worse codec.

Two traps worth carrying. The container names are transposed:
`dSYAudioPublicSettings` orders its fields so `bank1` points at `B1_sounds2_*`
and `bank2` at `B1_sounds1_*`, and `syAudioMakeBGMPlayers` explicitly binds
`sSYAudioSequenceBank2` -- so a generator keying on the name picks the wrong
bank. And the IDs must come from `gm/gmsound.h` as sequence indices into
`S1_music_sbk`, never inferred from `relocData` container names.

**FIXED (2026-09-09).** The generator now mixes these tracks at 32 kHz, applies
controller 21 from the actually-bound `B1_sounds1_*` bank, then band-limits the
finished mix to 22.05 kHz. Hurry is PCM16 through the existing ring. Dream Land
remains byte-identical at SHA-256
`431298f12745f3bde9801fb010e76fe5bc658c570267b4d2ba08703426f98d91`;
the derived-asset checker passes. Yoster IMA codec SNR moved 25.87 -> 26.17 dB;
Hurry moved 18.98 dB IMA -> lossless PCM16. The staged audio payload grows
1,352,180 bytes, with no DS RAM or runtime CPU cost.

## Ground Thunder Jolt: the material contract is measured, not guessed (2026-09-09)

MEASURED with a reconnaissance-only witness that owns nothing and draws nothing,
on a Pikachu mirror at 1,200 presents:

    DIAG_THUNDERGROUND=0x200,0x3f,<residue>

`0x3f` is the root mask: **all six children are walked**, so all six need owners.
Owning five would leave the half-drawn object the Link Bomb review rejected.

`0x200` is `NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE` (`nds_renderer.h:977`)
and it is the ONLY effect bit observed across all six roots. So the contract is
exactly the Mushroom Kingdom Pakkun's: the live half is the current image chosen
by `mobj->texture_id_curr`, everything else bakes. The owner should assert
`effects == NDS_RENDERER_NATIVE_MATERIAL_CURRENT_IMAGE` and refuse anything
broader, as Pakkun does.

**The third field is residue and must not be quoted.** It read 3,934,256,618
where the maximum possible is six increments per present over 1,200 presents,
about 7,200. These three globals are new and are NOT in a
`NDS_PUBLISH_DEBUGGER_GROUP`, so a debugger read can catch a dirty cache line.
The two fields that ARE trustworthy here are trustworthy because they are
self-consistent -- a six-bit mask reading exactly `0x3f`, and a single clean
effect bit -- not because the read is sound in general. Any new witness this
harness must read should join a published group.

## Zero native failures, everywhere (2026-09-09)

Both waves, 1,200 presents each, on one ROM -- shell probe
`smash64ds-p2-shell-hwtri` SHA-256 `5203F631...F317`, and every case in both
`summary.json` files carries that same `rom_sha256`, which is the only place a
split across two ROMs would show:

    native-fighter-long.json   9 cases, 269.7 s, transport failures=0, native failures=0
    native-stage-all.json      9 cases, 309.6 s, transport failures=0, native failures=0

That is the owner's standing goal met. The day started at about 2,629 native
failures with 3 of 9 stages and 4 of 9 fighters clean.

The last row closed was Pikachu's, and the record named it as an EFFECT
(identity kind 0x3f3) in asset 342 at root 0x2170 with a non-NULL material --
the same file as both Thunder Jolts but reached from
`dEFManagerThunderJoltEffectDesc` (efmanager.c:670) rather than from any
WPAttributes. It is the air jolt's own twenty-nine-word shell with word 17
turned from a baked SETTIMG into a segment-0xE call, so it takes the Pakkun
live-material shape where its sibling bakes everything. One word of difference
between two owner shapes is why the two are separate generators.

Ten owners now stand: Link's Bomb, Saffron's Marumine, the Poke Ball entry
rays, the Mushroom Kingdom POW block, the item-get swirl, both Thunder Jolts,
the Thunder Jolt effect, Samus's Charge Shot and the Castle bumper -- plus the
fighter angle range reduction and the Zebes acid alpha subdivision.

**Zero native failures is not zero owner bugs, and the difference is now the
whole remaining question.** Castle's roof and Yoster's floor are both admitted,
emitted and drawn by a packet that records no failure at all, so whatever loses
them is downstream of admission. See the entry below.

## Castle roof and Yoster floor are the same defect shape (2026-09-09)

Three independent investigations, all host-side, no ROM:

  - Generation is LOSSLESS on both. Castle roof root 0x2240 emits 56 triangles
    over 16 runs, 136 over the stage's 12 roots, source count equal to emitted
    count for every root, and no drop policy is reachable -- every TRI emits,
    run splitting preserves, an unloaded slot fails closed rather than skipping,
    zero alpha promotes rather than drops, and both the omission and subdivide
    tables are empty for Castle.
  - Yoster is the same: all 19 drawable roots listed in the descriptor and
    emitted 1:1, and the missing floor root 0x49A0 carries 155 commands, 155
    vertices, 77 triangles and 22 runs.
  - Every one of those 38 runs passes EVERY STATIC decline gate: bindings
    resolve, alphas are all 0xff, submit classes are only 0 and 6, shifted
    coordinates stay inside +/-2048, corner and epoch indices stay in range.
    So they do not decline at different static sites either -- they decline at
    none.

And the stage wave above reads zero native failures for both stages, which
retires the whole-packet candidates outright: an unresolved kind, a blob load
failure, an owner guard or a ValidateFull decline would each have recorded a
failure, and none did.

What is left is runtime-only, and the candidates are few:
`PrepareRun` step 1 policy mismatch (`nds_renderer_native_owners.c:1437-1456`),
step 2 texture-resolve miss (`:1470-1512`), step 6 alpha UINT_MAX
(`:1754-1763`), the owner 200+/300+ run declines (`:4164-4175`, `:4221-4232`),
the commit-time `inside_count == 0` cull (`:3396-3401`), and the packet
unselected path (`:3770-3774`).

**Watch the cull.** A run culled at `inside_count == 0` draws nothing and, as
far as this investigation could establish, records nothing -- a skipped draw
that reads as success is exactly the failure mode this project treats as a bug,
and it is the one candidate here that would leave every counter happy. Confirm
whether a witness exists before assuming one does.

The witnesses to read on a Castle and a Yoster frame, by their true names:
`gNdsNativeStageValidateFullFailStep` (`:722`),
`gNdsNativeStagePrepareRunFailStep` (`:1357`) and `...FailRun` (`:1363`),
`gNdsNativeStagePacketUnresolvedKind`/`Count`
(`nds_native_stage_select.inc:3432-3433`), and the blob `ReadFailCount` /
`HashMismatchCount` (`nds_native_stage_blob.c:37,42`). Three earlier citations
had drifted: `owners.c:3405-3418` is now near-clip emit and declines nothing,
`:3773-3784` is now an R2 reuse memo, and the blob file is
`src/nds/nds_native_stage_blob.c`.

## The downstream candidates, ranked — and the family is not one bug (2026-09-09 evening)

With submission eliminated by measurement, the surviving space was enumerated
against the DS hardware and the two DS reference decompilations
(`scratch/gx_downstream_candidates.md`).

**The orchestrator's own "decisive" candidate is dead on arithmetic.** The idea
was that Castle's frame fills the DS polygon or vertex list and the roof apex
triangles are simply late. It does not:

    contributor                       triangles
    Castle stage packet, whole              136
    Mario                                   320
    Fox                                     306
    frame total                             762   of 2048   (37%)
    vertices                               2286   of 6144   (37%)

Even at a pathological 2x clip expansion the frame stays 500 under the cap, and
Mushroom Kingdom (176) and Yoshi's Island (164) have *larger* headroom, not
smaller. List-full also drops at run granularity, and run 9 sits mid-packet --
run 9 of 40 -- so it is the wrong position as well as the wrong budget.

**New top suspect: the painter-depth band exit.** A class-3 `PROJECTED_NO_Z` run
does not carry camera depth; the projection's Z column is overwritten from a
single `projected_z` (`owners.c:2602-2622` via `:3116-3127`), so clip-Z semantics
are the painter band. `include/nds/nds_startup.h:4750-4759` records that past 128
depths a foreground primitive passes v16 -4096 and leaves the `|z| <= w` clip
volume outright. Consumption is monotonic and run 9's apex triangles are the last
three submitted -- so a band boundary falling inside run 9 takes exactly the tail.
Six skirt triangles draw, three apex triangles do not. That is the symptom.

**And then the painter band died too, an hour later, on its own arithmetic.**

Consumption is per triangle -- confirmed at `owners.c:5170-5176`, and the per-run
matrix load spends a caller-allocated depth (`:3133-3156`) rather than allocating
one, which resolves the evidence that pointed the other way. So the *shape* of the
candidate survives: a band boundary inside a run could take its tail. The numbers
do not.

    runs 0-8 before run 9 (1+2+1+4+5+2+3+2+2)          22 depths
    run 9's apex triangles, stage-local ordinals       29, 30, 31
    whole Castle stage, every NoZ triangle                    62
    band depth per endpoint                                  128
    headroom at the last apex triangle                        97

Pushing the apex out would take another 98 same-band depths ahead of it, and the
complete consumer list -- Fox blaster per shot, the rebirth halo on respawn frames,
the entry effect, source-fallback triangles -- cannot supply them in a Mario-vs-Fox
frame. The whole stage fits inside one band with room to spare.

There is a second, independent kill. Castle segment 0 carries **zero** source-Z
triangles, so no foreground flip happens inside it, and run 9 therefore consumes
*background*-band depths. `nds_startup.h:4754-4757` is explicit that a background
primitive past 128 descends into the source-Z range while only a **foreground**
primitive passes v16 -4096 and leaves the clip volume. A clip-style disappearance
cannot come out of the background band at any count.

**And then the side plane died too, by logic rather than by margin.**

Each of the three missing triangles carries **two low skirt corners** that must
test inside under the same transform. So every case where the apex tests `y > w`
is a *crossing* triangle, and the hardware truncates a crossing triangle into a
smaller polygon rather than erasing it -- the port's own cull agrees, culling only
when `inside_count == 0` and fanning anything partial (`owners.c:3672-3683`).
Three entirely missing triangles require wholly-outside status, and that would
force the low skirt corners outside with them. The skirt draws. The margin never
had to be computed; the verdict does not depend on it.

### So the enumeration is incomplete, and that is the finding

Dead with numbers or logic, all of them: submission itself, the polygon and vertex
list, the painter-depth band, the side-plane clip, alpha, collapse, winding, and
cull derivation. C5, the depth tie, survives only as named and the candidate study
already prices it as pixel-scale and unable to erase three non-overlapping
triangles. **Ranked first is now "something not yet enumerated."** Six candidates
generated from the outside have all died, which usually means the model of the
system is missing a step rather than that the candidates were poorly chosen.

### Two "corrections" recorded here were themselves wrong — decoded from the packet

An agent report gave run 9 a corner list shifted by two triangles, and this file
briefly carried two conclusions drawn from it: that the apex triangles are t4-t6
in the middle of the run, and that run 9 spans two matrix bindings. **Both are
false.** Decoded directly from `nds_native_stage_castle.generated.inc` -- run 9 is
`{first_corner 0x42 = 66, triangle_count 9, binding_index 3, epoch 7, class 3,
policy 7}`, and corners 66..92 are:

    t0 63,62,61   t1 60,63,61   t2 59,61,58
    t3 57,59,58   t4 56,55,54   t5 53,56,54
    t6 52,53,51   t7 50,49,48   t8 47,46,45

The apex dense vertices `(0, 1110, 30)` are 45, 50 and 51, and they sit in **t6,
t7 and t8 -- the last three submitted.** The original framing was right and the
"correction" inverted it.

Every vertex the run touches carries `matrix_binding` **3**, the run's own; the
highest index it references is 63. Dense 64-67 belong to a different run. **Run 9
spans one binding**, always takes the rigid single-matrix arm, and cannot exhaust
the matrix stack. Mushroom Kingdom's brick runs are single-binding too.

The structural argument that killed the side-plane candidate is unaffected, and
the real vertices confirm it: each missing triangle carries **one apex corner and
two low ones** -- t6 is `(-132,-6,278) (133,-6,278) apex`, t7 is
`apex (133,-6,278) (255,0,-109)`, t8 is `(-255,0,-109) (-132,-6,278) apex`. Every
side-clip-positive case is therefore a crossing triangle, and crossing triangles
truncate rather than vanish. C2 stays dead.

### It is three stages, across three different submit classes

    stage              surface              in packet   valid   Given == Emitted
    Peach's Castle     roof, run 9              yes       yes         9 = 9
    Mushroom Kingdom   both brick slabs         yes       yes        identical
    Yoshi's Island     layer-1 floor, b15       yes       yes        identical

Yoshi's Island's floor is binding **b15**, root `0x49A0` -- 155 source commands,
155 source vertices, 77 triangles in 39 triangle commands, 22 runs (global
g23-g44), 14 texture epochs (`nds_native_stage_yoster.generated.inc:233`, runs
table `:263-284`). The descriptor admits it deliberately -- layer-1 owner at
`native_stage_descriptors/yoster.py:279`, partition at `:296`, and
`omitted_draw_roots` empty -- so generation is exonerated there as well.

**And its runs are 8 of class 0 and 14 of class 6 -- no class 3 at all.** Castle's
run 9 is class 3. So the loss spans `SUBMIT_RAW_CURRENT`,
`SUBMIT_PROJECTED_NO_Z` and `SUBMIT_PROJECTED_RANGE_OR_MATRIX`, which retires
every NoZ-specific explanation generically: the flattened-Z matrix, the painter
band and the projected-depth counter cannot reach Yoshi's Island at all.

The scale argues the same way. Yoshi's Island loses **77 triangles across 22
runs** where Castle loses 3 of 9. A single cause that covers both is not
boundary-shaped -- it is categorical. Something that was never bound, never set,
or never made visible, rather than something that ran out.

### The two stages measured first

Mushroom Kingdom's brick slabs were investigated in parallel: **both slabs -- the
right one under the right pipe and the left one the owner reported -- are in the
generated packet, in the 65 valid runs, with `Given == Emitted`.** Generation is
exonerated there too and the loss is downstream of the stage committer, the same
place as Castle's. This supersedes the earlier census note that separated Inishie
b12 as a compact whole-run loss; an explanation that only works for one steep roof
is now probably the wrong explanation.

**The confirming counters are not in the binary.** `gNdsPainterSlotFgMax` and
`gNdsPainterSlotFgOverBand` are declared at `nds_startup.h:4762`/`:4767` and
defined at `diagnostics_collision_runtime.c:551`/`:556`, and `nm` on the probed
ELF finds neither. Neither carries `__attribute__((used))`. This is the second
witness today found missing from a shipped ELF.

**And the three stages are not one bug.** `scratch/tall_geometry_census.md:64-98`
already separated them: Inishie b12 is a compact whole-run loss at class 0, Yoster
b15 is a spread-independent whole-binding loss, and neither has a within-run
partial for a clip to select. Congo submits two triangles a frame through an actor
path with no far vertex. Whatever the painter band proves on Castle does not
transfer without its own per-stage reading.

## The Castle clip witness is armed and never fires (2026-09-09 evening)

The Castle roof clip witness printed immediately after `DIAG_STAGE_SHORTFALL`,
whose counters do not exist in every ELF, and the whole tail of the capture was
silently dropped -- that is why the witness "had not been read". The shortfall
printf now runs last (`probe-native-render-scene.ps1`, commit `ba16dc450fc`).

**The mechanism is narrower than first stated, and worth stating correctly.** Four
`arm-none-eabi-gdb --batch -x` experiments show a *file-mode* command list does
NOT abort on an unresolvable symbol: it prints an error for that line and keeps
going, exit 0. But the live-remote transcript is unambiguous -- 
`emitwitness4-castle.gdb.txt` carries zero `DIAG_STAGE_SHORTFALL` lines, zero
`ROOF_CLIP` lines, and ends at `DIAG_STAGE_RUN_EMITTED` followed straight by
`[Inferior 1 detached]`, while the post-reorder `roofclip1` transcript carries all
seven `ROOF_CLIP` lines. Something stopped the list at that point against a live
target. So the reorder was the right change on the evidence; the general claim
"a GDB command list stops at the first unresolvable command" is false in file
mode and true, by observation, against the remote. Do not generalise it further
without measuring the remote case directly.

With it unblocked, a fresh single-case Castle run (`native-stage-roofclip.json`,
600 presents, 53 s, `builds/diagnostics/roofclip1/`) reads:

    DIAG_CASTLE_ROOF_CLIP_META=600,0,0,0,1
    DIAG_CASTLE_ROOF_CLIP_PROJECTED_Z / SHIFT / DENSE / SUBMIT_V16 / RESULT / FLAGS
        -- every element 0x0

META is `serial, Valid, Run, CornerCount, Arm`. So serial 600, **Arm 1, Valid 0,
CornerCount 0**, every array untouched -- in the same frame where
`DIAG_STAGE_RUN_EMITTED[9]` read 9. Run 9 emitted all nine triangles while the
witness watching it captured nothing, and the probe arms it at line 150 long
before the walk reaches the battle, so this is not a missed arm.

That makes the next question narrow: which of the eight guards in
`ndsRendererNativeStageCaptureCastleRoofClip` (`owners.c:3406`) rejects run 9, or
is the call site at `:3576` not reached. Two of them can be wrong on a premise --
`sNdsNativeStagePacketActive->gkind` may not be `CASTLE` during a battle, and
`run->triangle_count` may not carry what `DIAG_STAGE_RUN_GIVEN` counts.

**The three transform outcomes below are all still unselected.** Nothing about
the clip volume, the w plane, or the polygon state has been read yet.

## MEASURED: the geometry does reach the GX FIFO (2026-09-09 evening)

The per-run emit witness has been read, on a clean-checkout ROM, and it closes
the question this afternoon's retraction reopened.

Probe `emitwitness4`, 600 presents, `smash64ds-p2-shell-hwtri` built from a clean
worktree, three stages:

    stage             valid runs   Emitted vs Given
    Castle                    40   identical, element for element
    Mushroom Kingdom          65   identical, element for element
    Yoshi's Island            58   identical, element for element

`DIAG_STAGE_RUN_SERIAL=600` and `DIAG_STAGE_RUN_VALID={600 <repeats N times>, 0
...}` on every stage, so every compared slot is the current frame's rather than a
stale leftover -- which is exactly what the serial-and-valid discipline was added
for.

**Castle run 9 -- the steep roof -- reads `GIVEN=9, EMITTED=9`.** All nine of its
triangles reach the hardware, including the three apex triangles at object-space
`(0, 1110, 30)` that the owner cannot see.

So the loss is **downstream of commit**. It is not generation, not the static
gates, not the runtime decline paths, not the `inside_count == 0` cull, and not
the emit loop. Every candidate that operates at or before submission is now dead
with a measurement rather than an argument, and the surviving space is what the
hardware does with the triangles or where they land -- the GX-side clip or
w-plane behaviour under the flattened-Z matrix that the NoZ path loads, on a
vertex 1,110 units above the rest of its run.

This also retires the wrong version of the earlier claim properly. In the
morning the table read 136/164/176 "submitted" by comparing `DIAG_OWNERTRI`
against the packet's own declared counts -- a number against itself. The claim
was right by accident and the evidence was worthless. It is now right on
evidence.

### Two other witnesses paid out in the same run

`DIAG_ENTRY_EFFECT_NOZ=1745` -- the shield's NO-Z painter path executes; it is
not inert.

**RETRACTED, 2026-09-09 later the same evening.** `DIAG_SHIELD_POLYFMT=0xea7ffd11`
was recorded here as alpha field 15, translucent, killing the coupled risk that
`ndsRendererHardwareAlpha` returning 31 would put the quad in the OPAQUE pass.
**It is not a reading of anything.** `gNdsEntryShieldWitnessPolyFmt` has no
storage in the probed ELF -- `nm` finds zero symbols -- and its only writer,
`nds_renderer_native_common.c:5673`, is inside `#if NDS_ENTRY_EFFECT_DIAG`, which
`Makefile:1238` defaults to 0. Nothing in that build ever writes it and nothing
allocates it. GDB resolved the DWARF entry the compiler still emits, read the
address it implies, and printed what happened to be there. The value repeats
across four runs because the stale address is stable, not because it is true.

So the opaque-pass risk is **open again**, and the shield's alpha is unmeasured.
`__attribute__((used))` was on that definition and did not save it; the map file
lists `.bss.gNdsEntryShieldWitnessPolyFmt` under discarded input sections. The
retention rule this build actually follows is *referenced*, not *attributed* --
a same-file witness with no attribute at all survives because something reads it.

The other witnesses from the same run were checked and all have real storage:
`gNdsEntryEffectNativeNoZGroupDraws`, `gNdsRendererZebesAcidBindCount`,
`gNdsCameraFrameCenterX`, `gNdsRendererAdapterSectorArwingMtxCount` and
`gNdsEntryShieldTexturePrepareDeclineCount`. Only the poly format was fabricated.

`DIAG_CAMFRAM=322.444092,1547.682617,1505.699585,937.018677,1080` -- the camera
framing witness works, so "is this surface in frame" is now a reading.

### One caveat about the run

All three cases report `transport: failed`, exit 1. The substance completed --
every DIAG line printed -- and the failure is a trailing GDB warning: *could not
convert 'ndsSceneManagerEnter' from the host encoding (CP1252) to UTF-32*. It is
a cosmetic encoding fault at teardown, not a measurement failure, but it means
`summary.json` counts three transport failures for a run that succeeded. Fix
before the next run, so a real transport failure is distinguishable.

### And the ROM this can be read on

The published `smash64ds` **cannot be probed**. It is built with
`NDS_P2_MENU_WALK` at the Makefile default 0, and the Makefile's own comment says
0 "is not merely inert, it is ABSENT" -- the dwell scripts, the tap injector
(`nds_menu_shell.c:485`) and the Results auto-START (`:580`) do not exist in that
translation unit, so the ROM waits at the title screen. GDB attaches, the scene
marker never arrives, and the case times out. Two probe cycles were lost to that
today. Use `smash64ds-p2-shell-hwtri`, which overrides the flag to 1.

## "Native failures = 0" does not mean everything drew (2026-09-09)

This phrase appears on the board, in HANDOFF and in a dozen agent briefs as
though it were a statement about what reached the screen. It is not, and the two
reasons are both structural.

**`ndsRendererRecordNativeFailure` is a first-failure LATCH.**
`src/nds/nds_renderer_dispatch_profile.c:6-29` writes domain, scene, identity,
status, root, material and reason only when `count == 0`, then increments a
saturating counter and flushes one cache line. Every later failure is counted and
discarded. So a zero count means literally *nobody called the recorder* -- not
that nothing failed.

**And NO_PROGRAM only fires when no native owner claims the display list.** It
sits in two tail arms of `ndsRendererAdapterSubmitStageDL`
(`src/port/renderer_adapter_stage.c:8666-8674` and its impact-wave-OFF twin), and
both mean the same thing: every `*_native_handled` flag was FALSE. An owner that
claims a display list, prepares its runs, commits them and puts nothing on screen
satisfies the contract completely.

That is exactly the state of all five submitted-but-invisible surfaces, and it is
why nine stages read zero while the owner can see holes. **The contract forbids a
silent empty draw and cannot currently detect one.**

The candidate detector is already in the tree and is frame-local and stale-proof:
`gNdsNativeStageRoofSnapGiven[run]` on the prepare-success path
(`nds_renderer_native_owners.c:1815`), `...Emitted[run]` plus `...Valid[run]`
after the commit loop accounts the run (`:5007-5009`), with the serial bumped and
every Valid cleared once per owner preparation (`:3823-3835`, wrap guarded at
`:3826-3829`). Valid publishes the current serial only after commit captures
Emitted, so a skipped run cannot make a prior frame's count read as current.

It cannot simply be promoted to a recorded failure, because `Emitted != Given` is
legitimate in both directions: near-plane clipping returns 0 for a fully clipped
triangle (`:3211-3215`), zero-w fan corners are skipped (`:3224-3225`), and the
NoZ path culls all-outside triangles (`:3447-3453`). Any contract check has to
separate those from the pathological case first, or it will fire on ordinary
culling and be worse than no check at all.

What this changes immediately: every "zero native failures" claim in the docs
should be read as "no owner declined", and no stronger. The phrase has been doing
work it cannot support.

## master does not build, and has not since 2026-09-06 (2026-09-09)

Every build in this campaign has been incremental, in the one working tree, on
top of a large body of uncommitted work. A clean checkout of `master` was never
tried. It was tried today, in a detached worktree at HEAD with the gitignored
inputs junctioned in, and it fails four separate ways -- each of which had been
invisible because an incremental build finds its output newer than its
prerequisites and skips the step entirely.

In the order they surface:

 1. **The stage generator's field certificate is stale.**
    `M3_STAGE_FALSIFIER: tracked pointer bases are no longer read ['stats']`.
    `b42d8ac9c07` split `ndsRendererHardwareTextureFilterOffset` in two: the
    reading half became `...ForSourceFrame(stats, live_source_frame)` and the
    original became a wrapper that forwards `stats` without dereferencing it.
    The certificate follows NAMED CLOSURES, so it must name the half that reads
    the field. Fixed here by pointing it at `...ForSourceFrame`.
 2. **A tracked generated header is stale against its committed consumer.**
    `nds_renderer_assets.c:3632` reads `.prepared_dense`, and the committed
    `include/nds/generated/nds_native_fighter_image.generated.h` has no such
    member -- eight fighters' image structs fail to compile. The generator
    changes that emit it live uncommitted in
    `scripts/fighters/generate_nds_native_owner_images.py` (+84 lines) and
    `native_owner_image_arrays.py` (+20). Committed consumer, uncommitted
    producer: the exact inverse of the usual "generator edit needs regen"
    trap, and it fails the same way.
 3. **A linked translation unit is untracked.**
    `cannot find battleship_ftcommon_dokan.o`. `src/import/battleship_ftcommon_dokan.c`
    has been on disk since 09-06 and has never been `git add`ed, while the
    Makefile's link inputs at HEAD name it.
 4. **Three symbols the committed code links against are only defined in
    uncommitted files**: `ndsGRYosterCloudGObj`, `ndsGRYamabukiGateGObj` and
    `mpCollisionCheckProjectRWall`, from the modified-but-uncommitted
    `battleship_gryoster_ground.c`, `battleship_gryamabuki_ground.c` and the
    collision TU.

### Three more, found by fixing the first four (2026-09-09)

Each of the four above let the build reach further, and each new reach found
another break. The count is seven, not four, and `docs/HANDOFF.md` says seven on
the strength of these three:

 5. **The four Saffron owners' executors were never included.**
    `src/nds/nds_renderer.c` includes each owner's generated packet and its
    `.exec.inc` beside it; the four new ones were absent, so the link ended in
    four undefined references to `ndsRendererSubmitNativeItemGLucky`,
    `...Porygon`, `...Hitokage` and `...Fushigibana` -- entry points the adapter
    arms call unconditionally.
 6. **Their generators had no Makefile rules or object prerequisites.** The
    `.generated.inc` packets live under `src/nds/generated/`, which is gitignored
    and produced at build time, so with no rule they are never emitted and the
    compile dies on a missing include. The object prerequisites on
    `nds_renderer.o` and `scene_backend.o` are the part that actually forces the
    rule to run -- the same shape as the recorded failure where a generated
    maxima header depended on an object no rule built.
 7. **The live pair's shared helper was untracked.**
    `scripts/stages/generate_nds_native_yamabuki_live_item.py` carries the
    conversion and validation discipline for Hitokage and Fushigibana and was
    never `git add`ed, so both generators died on `ModuleNotFoundError`.

An eighth was mine and is recorded separately: I added a second, unguarded copy
of the Saffron includes without seeing the agent's correctly guarded block, and
committed both, producing eight redefinitions.

All seven are fixed. The five item owners that landed after them arrived with the
same wiring missing and it was added before committing rather than discovered by
a build -- which is the first time this class was caught in advance.

There are **41 untracked files** under `src/`, `include/` and `scripts/`, plus
dozens of modified tracked ones. Some of the untracked set is deliberate -- the
Muse-written item generators are reference-only by owner rule and must not land
as they are -- but the four above are not deliberate, and together they mean the
published artifact cannot currently be reproduced from the repository.

Only item 1 is fixed in this commit, because it is the only one that is purely
mine and verifiable without a build. The rest need the dependency-complete
subsets committed in themed slices, each verified by a build, and that has to
happen in the main tree during a window when no agent is mid-edit.

The lesson generalises past this repository: an incremental build is not
evidence that a commit builds. Nothing in the campaign's verifier fleet does a
clean checkout, so nothing was ever going to catch this.

## The missing stage geometry is SUBMITTED, all of it (2026-09-09) -- RETRACTED, see below

The Castle roof, the Yoster floor and the Mushroom Kingdom side bricks are not
lost anywhere in the stage pipeline. Measured on the witness ROM, three stages
at 900 presents each:

    stage     emitted triangles   DIAG_OWNERTRI[STAGE]
    Castle          136                  136
    Yoster          164                  164
    Inishie         176                  176

One hundred percent, on all three. And every decline witness reads zero:

    DIAG_WITNESS = PrepareRunFailStep 0, FailRun 0, ValidateFullFailStep 0,
                   PacketUnresolvedCount 0, BlobReadFail 0, BlobHashMismatch 0,
                   NoZInsideCullCount 0
    DIAG_STAGE_PREP = 0,0,0,0,0,0,0

`NoZInsideCullCount` is new and exists because of this measurement. The
commit-time `inside_count == 0` cull recorded only a shared profile counter that
the two clipper-path rejects and the generic pipeline also increment, so a run
culled entirely there read as a SUCCESS with every fail step at 0. It now has
its own counter, and that counter reads ZERO -- so the cull is not it either.

**Every submission-side hypothesis for these three bugs is therefore dead**, and
that includes a whole class the project has spent several cycles on: generation
loss (refuted first -- source triangle count equals emitted count on all three),
static run rejection (refuted -- all 38 Castle and Yoster runs pass every gate),
whole-packet decline (refuted by zero native failures), runtime per-run decline
(refuted here), and the silent cull (refuted here, by a counter built for it).

The triangles reach the hardware. So the defect is what the hardware does with
them, or where they land: polygon attributes -- culling, depth, alpha -- or the
transform that places them. Backface culling is the strongest single candidate
and fits all three symptoms at once: with inverted winding a viewer sees only
the faces that happen to wind correctly, which is precisely the Castle's
"narrow triangular strips and edges with large transparent holes", and a floor
seen from above with inverted winding is simply absent, which is Yoster and
Inishie. It is also cheap to test: compare the emitted winding against the
source's, and read the POLY_CULL bits the stage runs actually set.

One more reading worth keeping: Yoster's `NearFanCount` is 3600 over 900
presents, exactly four clipped triangles per present, while Castle and Inishie
read 0. That is a real per-frame near-plane fan on Yoster alone and is not
explained by anything above.

Note also that all three missing surfaces are LAYER 1 -- Castle root 0x2240,
Yoster root 0x49A0, Inishie roots 0x5C70/0x5E40 -- which is worth carrying into
the winding check, though the owner triangle counts above already prove layer 1
is submitted rather than skipped.

### RETRACTION: that table never measured emission (2026-09-09)

The heading above is wrong and so is the last sentence. Both columns of the
136/164/176 table are DECLARED counts, not emitted ones, so the table compares a
number against itself and proves only that the runtime read the packet's own
triangle field.

Verified in the tree rather than argued:

  - `nds_renderer_native_owners.c:4989-4990` -- the commit loop does
    `stats->triangle_count += run->triangle_count` and
    `segment_triangles += run->triangle_count`, both the DECLARED field.
  - `:5019-5022` -- `sNdsRendererFastOwnerTriangleCount[STAGE] +=
    segment_triangles`, and that array is exactly what the probe prints as
    `DIAG_OWNERTRI` (`probe-native-render-scene.ps1:296`).
  - The real emitted count exists but is never read: `emitted_triangles` is
    passed to `ndsRendererNativeStageAccountRun` (`:4982-4988`), which
    accumulates it into `stats->hardware_triangle_count` (`:2050-2076`). Nothing
    under `scripts/diagnostics/` prints that for the STAGE owner. The fighter
    owner does have one -- `gNdsFighterDLAllDrawP0HardwareTriangleCount` -- which
    is why the same question is answerable there and not here.

So "the missing stage geometry is SUBMITTED, all of it" is withdrawn. The
correct statement is that all of it is DECLARED and passes every static and
runtime decline gate. Whether it reaches the GX FIFO is untested, and a run that
declines inside the emit loop after `BeginRun` succeeded would look exactly like
this: every fail step 0, `DIAG_OWNERTRI` full, nothing on screen.

This does not resurrect the refuted candidates -- generation loss, static run
rejection, whole-packet decline, per-run decline and the `inside_count == 0` cull
each died on their own evidence. It reopens one specific question the project
believed closed, and it is the cheapest remaining one: **store `emitted_triangles`
per run and compare it against `run->triangle_count` per run.** A per-run
comparison also localises the loss, which the totals could never do even had they
been the right numbers.

The lesson is the same one the texture-phase change taught two hours earlier: a
counter whose name matches the question is not a counter that answers it. This is
the second time today (`DIAG_OWNERTRI` is also indexed by profile owner, not by
stage owner_spec) that this one array has produced a false conclusion.

## Look at the picture before theorising about it (2026-09-09)

Three screenshots from the witness run, 900 presents each, at
`artifacts/visibility/0909-witness3-{castle,yoster,inishie}.png`. They correct
two of today's three "submitted but not drawn" claims and add a fourth finding
nobody was looking for.

**Castle reproduces exactly.** The upper tower's steep roof draws as red EDGES
-- an outline of the spire with sky visible through it -- against a tower body
that renders normally. Owner's description confirmed verbatim.

**Yoster: the FLOOR draws, the PLATFORMS are unestablished.** The green top
surface and its layered dirt sides are both present and both fighters stand on
them, so the "missing main floor path" half is not visible here. The "missing
main platforms" half is NOT settled either way -- the camera may simply not
frame them, and treating a stage that has geometry as a stage that has THIS
geometry is precisely the error retracted on Inishie below.

**Inishie DOES reproduce -- the reading above was wrong and the owner caught
it.** RETRACTED 2026-09-09, same day: what was called "the grey stepped brick
platforms" is the CENTRAL tower structure, not the large side brick platforms
the owner reported missing. The tell is the green warp pipe on the left: it
hangs in the air on a thin pale column with nothing beneath it, where the source
stands it on a brick platform. Read the picture for what is ABSENT, not only for
what is present -- a stage full of geometry can still be missing the specific
surface the report names, and naming the wrong geometry is exactly how the
earlier Mushroom Kingdom row got retracted for describing the scale plates.

That is worth saying plainly because two entries above -- and the commit message
that carried them -- treated all three as one shared "submitted but not drawn"
defect on the strength of counters alone. The counters were right about what
they measured (100% of triangles submitted, every decline witness zero) and
wrong as evidence for the owner's symptom. A counter says a triangle was
submitted; only the picture says what it looks like -- and reading the picture
wrong is its own failure mode, which is what happened to Inishie above.

**And the cadence, unasked for:** Castle 29.0 FPS, Inishie 21.7, Yoster 19.9.
The owner reported "renders at 20FPS" against Mushroom Kingdom and it is real,
but it is not Mushroom Kingdom's alone -- Yoster is worse. Two of nine stages
sit a third below the 30 FPS requirement in an ordinary two-fighter match, which
is a P2 gate problem and not a stage-geometry one.

### What the Castle picture argues

Edges without fill, on triangles that are provably submitted, correctly wound
and unculled, is what **POLY_ALPHA 0** looks like: on DS that is WIREFRAME, not
invisible. This repository already states that rule in several of its own
executors and guards against it there. The runtime polygon alpha does not come
from vertex alpha -- the emitted per-run vertex alphas for the roof are all
0xff -- it is derived from the combine and other-mode state by
`ndsRendererHardwareAlpha`, which is where a zero would come from.

The alternative if alpha is not zero: the runs collapse to zero screen area
after the baked world matrix, or a POLY_MODE/depth setting suppresses fill.

## Three P2 blockers finally have numbers (2026-09-09)

None of these are new problems. All three had board rows describing a symptom
and no arithmetic behind them. Now they have arithmetic.

### The gate is 2.5x over, and the animation lane is why

Four-CPU stress against the two-fighter Boundary arm, by profile lane:

    lane      2-ftr mean   4-ftr mean   x      2-ftr P95    4-ftr P95    x
    WORK-H     1,143,972    1,893,541   1.66    1,463,104   2,808,768   1.92
    SRC          329,667      708,441   2.15      554,176   1,366,528   2.47
    STG          197,817      380,662   1.92      205,312     426,240   2.08
    FTR          378,662      504,819   1.33      392,000     795,776   2.03
    MISC         206,272      252,115   1.22      471,616     467,456   0.99
    OTHR/WAIT          --           --  1.14           --          --   ~1.00

Two fighters to four is 2x, so 2x is the expected slope and **SRC's 2.47x at
P95 is the finding**. Inside it, SINT's P95 of 890,176 is 31.7% of WORK-H P95
here against 27.0% banked on the two-fighter arm, and GCRA's P95 is 48.5% of
WORK-H P95. The gate is P95 <= 1.12M; SRC alone exceeds the entire budget.

MISC, OTHR and WAIT are FLAT (0.99x-1.00x at P95), which is worth as much as
the finding: whatever is wrong is not everywhere.

**STG doubled while the stage did not change** -- both arms are Dream Land. A
stage lane that scales with fighter count is either mis-bracketed
fighter-adjacent work or a real per-fighter stage cost, and which one it is
decides what to optimise. HUD's 15.7x is the instrument and is not a finding.

Carry this caveat with any conclusion drawn from the table: the two arms are
cross-build and cross-content, a July one-versus-one tree against a September
four-kind tree with items and a stream loader. The ratios are indicative, not a
clean A/B.

### P2-5 is forty owners short, not "runtime acceptance open"

Exactly FIVE item kinds have native draw owners: Pakkun, the POW block, the
Green Bumper via the Castle owner, Link's Bomb and the Marumine. The kind
arithmetic is 4 container + 16 utility + 2 fighter + 5 ground + 5 Saffron + 13
Pokemon = 45, so **forty kinds have none**, and under the native-only contract
each of their spawns records a loud NO_PROGRAM rather than drawing. The
Nintendo Bumper is explicitly REFUSED by the Castle owner and needs its own.

The forty split by OWNER SHAPE, which is what decides the work:
**22 bake-everything, 13 live-material (a segment-0xE call in the root), 4 that
branch on a non-segment 0xDE, and 1 unlocated** (Target, which has no
ITAttributes). Ranked by how often a kind appears in ordinary VS play against
how cheap its owner would be, the top ten are Tomato, Star, Sword, Bat, MBall,
Hammer, BombHei, the Green and Red Shells (shared geometry, one owner), LGun and
Harisen. The full census with each kind's asset, root, child count, triangles
and segment-0xE verdict is at
`builds/resume-20260905/agents-0906/scratch/item_owner_census.md`.

One correction that matters for anyone writing these: **the Marumine is
BAKE-everything, not live-material.** It was cited as the live-material example
in three places today, and it is not one -- 159:0x06A0 is 30 words with no 0xDE
and a NULL MObj, exactly as its own generator says. Both existing item
generators are bake-everything; the live-material examples are Star, BombHei and
the Pakkun-shaped followers.

The rest of the item system is in better shape than that sounds. All 13
Pokemon are in the maker table unconditionally and can appear; the spawn law,
the anti-repeat rule and Mew's 1-in-151 are wired with witnesses; and pickup,
hold, swing and throw are one shared seam behind `NDS_P2_ITEM_CORE` that every
landed fighter uses. What is missing is draw, and the per-fighter pickup
animation FileIDs, which are stubbed 0 for every fighter.

### The unselectable roster does not fit, by 21x

Kirby, Jigglypuff and Ness cost, as compact preview packs:

    Kirby   17,564 B     Purin  12,152 B     Ness  13,028 B     total 42,744 B

Derived twice -- once from each generated pack's own header
(`data_bytes + 32*section_count + 12*span_count`, the loader's own formula) and
once independently from the generator's source metadata maps -- and the two
agree exactly on all three.

The arena free floor is **1,968 bytes** against a 32,768-byte minimum. Even the
cheapest single pack exceeds the floor about sixfold and the three together
exceed it twenty-onefold, so any of them at the character-select peak overflows
into `ndsSyMallocOverflowHalt`. The answer is no, and it is not close.

Two corrections to the standing story. The roster ladder's own reasoning
contains NO arena measurement: the number that blocked Jigglypuff was a libfat
hang, and Kirby and Ness were never ladder candidates at all. And these pack
bytes are the 1P comparator -- the pack loader runs only in the 1P
character-select scene. In VS, where `docs/BUGS.md` files all three, the binding
blocker is the eager full-roster closure load, not these packs. The answer is
still no against either scene; the reason differs by scene.

## The animation lane is quadratic, and the stage lane never was (2026-09-09)

Two follow-ups to the lane table above, both landing at CONFIDENCE LOW for
honest reasons, and both changing what should be worked on.

### SRC is super-linear because two things inside it are O(n^2)

The mechanism is not a cache, an allocator or a memo, which were the three
guesses. It is list walks:

  - **Fighter jostle.** `ftMainProcessAll`'s inner walk (ftmain.c:1522-1523,
    :1563) runs once per fighter per tick and walks every other fighter, so the
    total is n^2.
  - **CPU AI target scans.** `ftComputerProcessAll` (ftmain.c:1268-1270,
    bracketed SCPU at battleship_ftcomputer.c:167-210) reaches
    `ftComputerCheckFindTarget`, `ftComputerCheckEvadeDistance` and
    `ftComputerWaitGetTarget`, each of which walks the fighter list
    (ftcomputer.c:3716-3780, :3803-3832, :3837-3845). Each CPU fighter scans
    n-1 others.

Directed pairs go from 2 to 12 between two and four fighters -- a 6x term --
mixed with linear per-fighter animation at 2x, which is how a lane lands at
2.47x rather than 2x.

**Explicitly NOT found**, in bounded reading: shared-cache thrash, fragmenting
allocation, and falling memo hit rate. The pose pool is a fixed per-fighter 128
(`include/nds/nds_ft_pose.h:63-71`) and the Q12 conversion cache was measured,
rejected for footprint, and spends nothing today. So do not go looking for a
cache; the attribution is list walks.

Worth holding against the temptation to attack the n^2 directly: twelve
directed pairs is a tiny number. If twelve pair evaluations cost this much, the
price is in the PER-PAIR work, not in the pair count, and an algorithmic fix to
the walk would buy very little.

The ranked lever is the one PROJECT_GOAL already sanctions outright -- skeletal
poses at 30 Hz, and reduced animation update rates -- and the pose engine was
already designed for it: evaluate common body joints on the last logic tick of a
presented frame plus the attach tick, keep `TransN`/`XRotN`/`YRotN` every tick,
and skip invalidation through `ndsFtPoseBodyChangedThisTick`
(`include/nds/nds_ft_pose.h:31-40`, :254-257). Projected, not measured: about
340-390K off WORK-H P95. **That does not reach the gate** from 2,808,768, so it
is a step and not the answer. The cost is body hurtboxes one tick stale on a
held tick; transitions are preserved by the attach-tick evaluation.

Lowering the AI scan rate is ranked LAST on purpose: it changes CPU behaviour,
which is gameplay fidelity, third in the sacrifice order and requiring owner
approval.

### STG's doubling is not fighter work, and the premise was wrong

The brief that chased it assumed fighter-adjacent work was being billed inside
the stage bracket. **Refuted.** The tick-HUD STG bucket by construction excludes
fighter, weapon, effect and item work: fighter GObjs are rejected by
`ndsStageGCDrawAllLoopClassifyGObj` and returned before the bracket opens
(`reloc_backend_movement.c:13559-13561`, :13610); shadows have their own SHDT
bucket; the per-fighter collision and platform queries live in SRC and GCRA.

And the distributions do not overlap -- the four-fighter STG minimum, 365,760,
is above the two-fighter maximum, 212,992, with both spreads tight at about
1.1x. That is a uniform floor shift of roughly 183,000, not fighter-driven
variance. The honest attribution is the one the caveat already warned about:
different ROMs, different builds, different windows. Two targets, one dirty path
against 164, frames 440-2040 against 2-1973.

**But a real mis-bracketing does exist, one level up.** Four non-stage submits
-- `reloc_backend_movement.c:12153`, `:12289`, `:12489`, `:13249` -- set the
profile owner to STAGE while their wall time bills MISC. Their names say what
they are: weapon draw and misc-effect draw. So the profile OWNER and the tick
bucket disagree about the same code, which is the same class of trap as
`DIAG_OWNERTRI` being indexed by profile owner rather than by stage owner_spec.
There is no WEAPON, ITEM or EFFECT member in the owner enum
(`reloc_backend_movement.c:536-612`) for them to use, so the correction is
either `NDS_RENDERER_PROFILE_OWNER_NONE` or new members -- keeping the state
reset (`renderer_adapter_stage.c:1105-1119`) separate from the owner set
(`:1122`).

Neither of these figures is in `docs/PERF_LEDGER.md`; both were read out of the
banked run JSON directly (`artifacts/performance/r207-boundary-match-1600.json`
and `artifacts/performance/2026-09-06_fourcpu-real-items-memory/full-stress.json`).

## The shield's alpha was never flat, and the symptom was never banding (2026-09-09)

RETRACTION, same day, of a claim that reached a commit message and two docs.

Commit `65629710bc0` moved the shield's entry-effect texture from A5I3 to A3I5,
arguing that the palette entry count is what bands and that three alpha bits
cost nothing "because the shield's source alpha is FLAT".

**The source alpha is not flat.** Read out of the actual IA8 bytes for file 163
root 0x0248, the shield region carries **11 distinct alpha levels and 13
intensity levels**:

    alpha nibbles:     0, 1, 4, 6, 8, 10, 11, 12, 13, 14, 15
    intensity nibbles: 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15

Both are now pinned in `scripts/3d_vfx/test_native_shield_reflector_packets.py`
so the false premise cannot quietly become accepted again. A3I5 therefore
quantizes a real graded alpha channel from four bits to three; it does not get
those bits for free.

**And the symptom was never banding.** The owner's description, given after that
change landed: *"Shield texture slices are rendered in the wrong order, causing
the shield image to be horizontally scrambled instead of forming a continuous
circular texture."* Scrambled slices are a TEXEL ORDER defect. The palette
widening does nothing for it.

So the position now is: A3I5 is a defensible change to the colour ramp made for
a partly wrong reason, aimed at a symptom the owner was not reporting. It stays
pending the picture, because reverting it on the same quality of reasoning that
landed it would not be an improvement.

The order defect has three candidate seams and four prior recurrences in this
repository's interleave/byte-lane class:

  - **the source decode.** The converter's 8-bit branch reads
    `payload[offset + (source_index ^ 3)]`, a byte-lane correction for a
    word-swapped O2R payload. A wrong lane rule permutes texels within each
    group of four -- horizontal scrambling, with every count, size and checksum
    still valid. This is the leading candidate and it is decidable on the host:
    decode under each candidate rule and score for the radial symmetry a bubble
    must have.
  - **the quad's texture coordinates**, which could flip or wrap the mapping.
  - **the atlas upload**, if the fill writes rows strided by the texture width
    rather than the atlas width -- which slices output diagonally.

What the verification pass DID establish and is worth keeping: the A3I5 row
reaches the runtime prepare path end to end (root 0x0248 to group 71 to slot 44,
32 entries, format 3, a 1024-byte LZ10 stream for 32x32 at one byte per texel),
and the fail-closed guard I added does NOT trip on the current packet. It can
trip if the generated contract changes, so it now has a witness:
`gNdsEntryShieldTexturePrepareDeclineCount`, printed as `DIAG_SHIELD_PREP`.

There is still no probe case that photographs a shielding fighter, which is why
none of this has been settled by the oracle that matters. The recipe now exists:
a `mario-shield` case on Dream Land holding the DS L button, which maps to the
source's `Z_TRIG` 0x2000, with the capture gated on `mario->is_shield != 0`
rather than on a frame count -- so the shot is provably of a shielding fighter
instead of a guess at animation timing.

---

## Character select's worst frame is a synchronous fighter load (2026-09-10)

The owner reported character select below 30 FPS. The tail was analysed and then
bisected, both by reading, without building a single ROM. Recording the verdict
here because the analysis lives under `builds/`, which is gitignored.

**The tracked worst-frame series**, each figure read from the artifact named:

    08-18   439,488   artifacts/verification/2026-08-18_p2-1j-shell.txt
    08-25 2,315,072   .../2026-08-25_p2-3f8/probe-p2-shell-roster2-control.txt
    08-25 4,590,784   .../probe-p2-shell-roster3-capture.txt      (same day, bigger roster)
    09-09 10,712,832  artifacts/performance/2026-09-09_p2-shell_css-cadence-post-animcache.txt

Two intermediate figures that circulated -- 519K on 08-18 and 874K on 08-19 --
exist in no tracked artifact and reached an agent brief before anyone checked.
The 08-18 shell runs actually read 439,488 and 425,472; no 08-19 shell probe is
tracked at all. Do not bisect on them.

**First bad change, for the shape:** `d99a89f8741`. It replaced the eager roster
preload with four refcounted 156 KiB slot arenas, which moved the fighter closure
load off the uncounted scene-entry frame and onto counted browse frames. Its own
message records the arithmetic (eager 802,744 B, slotted 704,512 B). The worst
frame is now a browse-load tic by construction.

`435ebf00d6d` landed the same morning and mitigates the rate -- a 13-tic dwell,
deferred release, one physical residency action per tic -- while keeping the
magnitude by design. Its message says so: still one synchronous load tick,
bounded by a 155,888-byte closure. That is the cost this bug is about.

**For the tail's existence:** roster admission under the eager preload across
08-19 to 08-25. No single commit owns it, and the same-day roster3 capture prices
it directly at about 2x. Per-commit A/B through that window is archaeology.

**Exonerated, with reasons, so nobody re-opens them:**

  - `11014557337`, the animation-cache sizing fix. Its reservation runs inside
    entry setup, which the shell instrument excludes from the distribution by
    construction (`src/nds/nds_menu_shell_core.c:274-276`, `:332-334`).
    `CSSIO arena=39072/11088 reservefail=0` confirms it paid once. The 39,072 B
    reservation cannot be inside the 10.7M counted frame. Its on-frame cost is
    warm loads, 6 per pass at 11,088 B, each a header read -- mid-tail colour at
    most.
  - Audio. `MSMAXAT f3=660/c0` records zero FGM play calls on the worst frame,
    the cue counter has storage, and CSS-level audio agrees (39 cues over 3,302
    frames, BGM fence clean).
  - The fighter-packet partition flatten. It is not in the numbers because it is
    not in the tree: HEAD still carries the detail-conditional half-arena at
    `src/nds/nds_renderer_native_common.c:8911-8917`. It remains a pending
    behaviour-change risk, not a cause.

**Two rows in the pinned capture were uninterpretable.** `CSSPHASE` and
`MSFPSHUD` printed values for witnesses with no storage at HEAD -- the
character-select FPS HUD had been written but never committed, so every capture
carrying those rows came off a private ROM and a clean checkout had no CSS FPS
HUD at all. Landed as `b242a60acaa`; those rows now mean something.

**What is still not known:** which browse action produced frame 660. Every
residency counter is cumulative per scene stop, so no artifact field attributes
one input to one frame, and the VBlank histogram's top bucket saturates at
interval four or more, so nothing separates the 20-VBlank frame from the 94
mid-tail frames. Seven full loads against 114 tail frames rules out one-to-one
attribution: the tail is a class -- commit, retire and load spread over three tics
by construction -- and the worst frame is one load tic within it. A per-frame
attribution latch is specified and queued.

**8 KiB / 4-node slice verification, 2026-09-10: RED.** The uncommitted
resumable extern-tree implementation compiles and the latch proves it removed
the atomic closure tic: stop 5 is now `MSMAX w3=4409600`, with
`MSMAXRES load=0 finish=0 retire=0 dwell=0 payload=3`. The exact slice byte
counter reads 522,064 B total and 8,192 B max per tic, so the configured cap is
honoured. It does not meet cadence: `MSVB3 5 1482 110 24 35 max=8` versus the
pinned pre-slice `1510 87 44 10 max=20`; the tail is 59 frames versus 54 and
the <=2-VBlank share falls from 96.73% to 96.43%. Seven loads still finish with
zero failures, but acquire/retry grows from 10/3 to 91/84, about twelve retry
tics per completed load, not the predicted 2-5-tic span. Evidence:
`artifacts/performance/2026-09-10_p2-shell_css-load-slice-8k.txt` and
`artifacts/performance/2026-09-10_p2-shell_css-load-slice-bytecap.txt`.

Do not tune K in isolation from this point. Smaller chunks trade the still-red
4.41M-tick peak for an even longer already-red span/tail; larger chunks shorten
the span by putting more work on a continuation tic already 3.94x over the
1.12M budget. The next measurement should price the blocking work inside one
`ndsRelocExternTreeSliceStep` (stream read versus publish/finalize/fixup) before
another implementation change.


---

## The low-only pack lever is worth zero, not 112,388 B (2026-09-10)

Two documents priced "four fighters means Low detail, so stop packing High" at
**112,388 B** and ranked it the largest remaining step on the pack gate. A third
investigation killed it, and the kill was then verified independently against
source before it was recorded here.

**The invariant is true of the wrong field.** `detail_base` is Low for all four
slots in any three- or four-fighter VS match (`scvsbattle.c:188`, `:460`). But
every draw reads `detail_curr` -- the `use_low_detail` assignment in
`src/port/renderer_adapter_fighter.c` tests `fp->detail_curr`, not the base --
and two paths raise `detail_curr` to High inside an ordinary match:

  - the dead-up-fall, `ftParamSetModelPartDetailAll(fighter_gobj,
    nFTPartsDetailHigh)` at `ftcommondead.c:529`;
  - the pause zoom, which saves the old value into
    `sIFCommonBattlePausePlayerDetail` and then sets High
    (`ifcommon.c:2955`) -- the save-and-restore is what makes it deliberately
    temporary rather than a leak.

Both reach the port: `src/import/battleship_ftcommon_dead.c:44` includes that
decomp translation unit directly, and `battleship_ifcommon.c` does the same for
the other. A KO happens in essentially every real match and a pause is one
button press away, so **High is reachable in every four-fighter configuration**.

If High images stop being packed, the first KO or pause issues a High request
against an absent image, at exactly the moment the camera is closest to the
fighter. Failing closed there means a visible hitch or a wrong-looking fighter;
failing open means drawing something that is not the fighter. There is no
residency-side enforcement point that can see it coming, because these are
status callbacks, not creation policy.

**What this does to the arithmetic.** The ranked stack, with the dead lever
removed:

    402,984  worst four-fighter set (Captain+Link+Pikachu+Kirby)
    - 27,044 retained texels moved to VRAM (VRAM-fit proof still owed)
    - 15,012 quantization Q-a + Q-b
    - 20,120 conditional Special geometry deferred
    - 12,026 lever 7.1 remainder resolved favourably
    = 328,782  against a 175,604 allowance -- about 153,178 over

and the growth deductions tighten it further. The document that stacked these
reported about 40,790 over; that figure assumed the 112,388.

**Two neighbouring premises died in the same pass**, both worth recording so
they are not re-proposed: quantization does not close the gate in the large
(positions, normals and state are already at GX width, so the free bytes total
roughly 15 K rather than 80 K), and the 175,604 ceiling is an **optimistic**
bound rather than a conservative one, so there is no slack to be recovered by
re-deriving it.

**What is left is census-scale**, outside every lever: a vertex and run budget
for the base images -- Kirby's base carries about 1,325 vertices against Link's
781, for the roundest fighter in the game -- the Yoshi-donor second copy, and
retained semantics beyond texels. Two of those are sanctioned fallback space
under `PROJECT_GOAL.md`; the low-only route survives only as an owner decision,
either pinning the death and pause selectors to Low (a behaviour change) or
streaming High at the KO zoom (a synchronous load on a counted frame, which is
the exact shape of the character-select bug).

**The lesson worth keeping:** both documents that priced this lever measured the
bytes correctly and never checked whether the state they assumed was reachable.
A residency lever is a claim about reachability first and bytes second.


---

## What the stage visibility counters prove, and what they do not (2026-09-10)

A Codex agent working the seven absent stage cells reported three of the four
SUBMITTED-BUT-INVISIBLE surfaces fixed by the alpha and rendermode correction,
citing per-surface counts: Yoster 67 of 67 emitted, Inishie 16 of 16, the barrel
quad at 2 triangles per callback with alpha 31 and zero rejects. A recheck found
that claim over-read, and the board was not updated from it.

**`Emitted == Given` proves the emit loop ran. It proves nothing about a pixel.**
Everything downstream of the emit loop is untouched by that counter: COLOR0
cutout discard, alpha-test discard, Z-buffer loss, clipped or off-screen
projection, camera framing, occlusion, and a right key bound to wrong texture
content. `DIAG_NATIVE=0` proves no owner declined loudly -- and the board's own
header rule says NO_PROGRAM fires only when no owner claims a display list, so an
owner that draws nothing passes it. There is precedent in this tree for a silent
commit-time failure that returns TRUE after incrementing a counter compiled out
at the default profile, and for write-on-failure-only latches that read
all-zeros on success, where a zero is absence of evidence rather than evidence.

**Per-surface verdicts after the recheck:**

  - **Yoster** -- the main floor is present and visible in
    `artifacts/visibility/alpha-gate-land-0909-b-yoster.png`. The root-0x49A0
    platform subset is *not isolated at that camera*: nothing names which pixels
    are runs 23-32, 36-41, 43, 44. Floor closed; platform subset submit-proved
    only.
  - **Inishie bricks** -- the capture shows brown cracked floor and grey block
    walls, which is suggestive, but no binding-to-screen mapping certifies the
    visible blocks are the b12/b13 runs 27 and 30. Unproven.
  - **Congo barrel** -- a 2-triangle live quad is indistinguishable from stage
    dressing at the capture's framing, and the landing record itself declined to
    claim it as a second proof. Unproven on screen.
  - **Zebes shafts -- NOT a missing-draw case at all.** Five single-quad display
    lists become five bindings, five runs, ten triangles, submit class 0, poly
    alpha 31. They draw. The defect is appearance: a hard stair edge with no
    fade. The gated correction deliberately does not touch them -- their runs
    classify preserve=0, and the ungated variant would have filled the
    silhouettes. Recommended disposition is fallback acceptance with no shipped
    code change; an owner look is needed only if a non-source translucent look is
    wanted. **Listing this beside three unproven surfaces has been miscategorising
    an appearance question as a missing-geometry bug.**

**Every existing stage capture predates the correct textures.** The generated
static-texture corpus was still at its pre-fix hash until 2026-09-10, when it was
regenerated to 44 keys, 42 outputs and 82,760 payload bytes. The newest stage
captures are the 09-09 alpha-gate sets, all taken before that. Any disagreement
between a capture and a counter that predates the regeneration is unresolvable
without a new capture.

**The structural problem, which is why this argument keeps recurring:** nothing
maps a run or binding to the pixels it should have produced, so a full gameplay
screenshot cannot answer whether a specific small surface drew. This is the same
shape as the full-mesh comparison that scored 0.9976 while hiding a 70%-wrong
island, and it was only found by cropping the changed subset and scoring it
alone. A per-run screen-space crop is specified separately.


---

## Three premises killed, so nobody re-investigates them (2026-09-10)

Each of these was a plausible theory that a targeted reading disproved. They are
recorded here because the investigations live under `builds/`, which is
gitignored, and an unrecorded kill gets re-opened by the next person who has the
same idea.

### CPU level is not dropped anywhere -- the gap is coverage

The suspicion was the same defect shape that killed the low-only pack lever: a
value set correctly at one end and read from the wrong place at the other. The
entire campaign has measured exactly one CPU configuration, a level-3 Fox, so if
the selected level never reached the decision code nothing would have noticed.

It reaches it. The character select's arrows write it, the commit carries it,
apply copies it, battle setup and fighter creation forward it, and the decision
code reads `fp->level` in roughly forty places. `FTCOMPUTER_LEVEL_MAX` is 9
(`decomp/.../src/ft/ftdef.h:4`), stored as `u8 level` on both `FTDesc` and
`FTStruct` (`.../fttypes.h:551`, `:988`).

**The more useful finding underneath it:** the level is a scalar everywhere,
with no per-level tables. Every use is a formula in `(9 - level)` or a
threshold, so the thresholds produce *bands* rather than nine distinct
characters -- L1-L2 share the timid-attack gate, L3 unlocks counterattack, L4
getup-escape and shield-dodge, L5 dash and meteor-dive, L7 the far dodge. That
collapses acceptance from nine runs to five, each with a named discrete
behaviour. The deterministic half is the cleaner evidence: reaction delay and
the getup, cliff and mash-out gates are pure functions of level, where L1 waits
roughly 120-200 ticks and L9 waits none. The probabilistic gates need a
sample-size argument -- the close dodge is about 14% at L4, so a single match
can show zero of it and prove nothing.

### There is no shared item branch executor to widen

Capsule was held out of item batch 5 because its display list branches and
carries more sibling roots than the existing executors admit, and the framing
was that a shared branch shape needed widening.

No such shared unit exists. Bat, StarRod and FFlower each own their own triple:
a per-kind generator function, a generated header, a hand-written executor
include, and an adapter admission case. The "shared seam" is a pattern, not
code. So the work is additive -- a fixed two-sibling Capsule row touching no
existing kind -- and the only genuinely shared file is the wave-1 core
generator, which is append-only.

A census of all twenty remaining unowned kinds found **Capsule is the only one
needing more than one sibling root**, so a generic N-sibling row would be
speculative generality for a single consumer and would add branches to the
submit path for nothing.

### Kirby's vertex density is genuine source, and the conversion multiplication is already gone

The attractive theory was that Kirby's outsized census came from the converter
multiplying geometry -- which would have been a bug worth more than any budget.
The answer is both, in sequence, and the useful half is already fixed.

Source is genuinely dense: `328_KirbyModel.c` carries 3,160 `Vtx` entries
against Link's 1,094 and Captain's 705, and one display list holds 272 verts
where **no other fighter has a display list over 32**. The sphere is
tessellated at source.

The conversion had also been multiplying it: the old monolithic image baked
2,012 + 1,740 = 3,752 dense verts, *more than the source contains*, because each
of the fourteen joint-6 appendix variants re-baked overlapping context. The
copy-hat deferral (`4d8d9d27179`) removed that -- the post-defer base is 1,325
dense verts, well under source -- and the bake is confirmed to dedup rather than
multiply.

So the remaining density is real, and decimation is a genuine visual trade
needing owner judgement rather than a defect anyone can quietly fix.


---

## Nine figures that disagreed across documents, and which one is right (2026-09-10)

A consistency audit swept the campaign's studies against each other, prompted by
one contradiction that had already been expensive: two documents priced the
low-only pack lever at 112,388 B and published stacked arithmetic assuming it,
while a third had proved it worth zero. It found eight more. The authoritative
value for each contested quantity is recorded here so an implementer has one
place to look.

**Two of these were being repeated by the orchestrator**, into agent briefs and
into a commit message, which is how a stale figure reaches many documents at
once rather than one.

### The particle atlas is FULL, not 31,872 of 32,768

Authoritative: **32,768 of 32,768, zero free**, 39 textures admitted, 5
quad-excluded (indices 28, 30, 31, 35, 36) needing 1,024 B each, so 5,120 B of
demand against no supply. Pinned by `scripts/check-nds-particle-banks.ps1:507-548`
and `docs/optimization/NDS_PARTICLE_BANKS.generated.json`.

The "31,872 of 32,768 with 12 unadmitted textures, roughly 12 KB" reading is
brief-era and stale, and so is the "33 new drawables" premise built on it. The
badly-packed era ended on 2026-08-14, when a first-fit-decreasing repack
recovered 5,248 texels -- so the earlier finding that "no room" meant packer
waste has been acted on and does not apply again. The HUD never competes for
this atlas.

This matters for the item work specifically: thirteen Poké Ball Pokémon and the
remaining kinds cannot assume atlas space exists. It is a budget decision now,
not a packing problem.

### ITCM has zero free, not 864 B

Authoritative: `linker/nds_hot_text.ld:18` gives the region as 32,736 B and the
ledger reads **32,736 of 32,736, zero free**. The 864 B headroom figure is a
message-era reading that several documents and one commit message repeated.

The consequence is procedural rather than numeric: **any change that places code
in ITCM must now name what it evicts.** There is nothing to spend.

### The other seven, briefly

  - **Worst pack set** is 402,984 B short 227,380, not 506,636 short 331,032 --
    the latter is pre-Kirby-hat-deferral.
  - **Character select's worst frame** is 11,701,888 at frame 219 on the current
    configuration. The 10,712,832 at frame 660 belongs to a ROM without the
    attribution latch and cannot be attributed retroactively; 2,315,072 at frame
    419 is the August roster2 control.
  - **Item owners** are 25 of 45. Both 21 of 45 and "39 ownerless" are earlier
    counts.
  - **The four-fighter crash frame** is 45, not 256 -- already retracted, but
    still quoted in places.
  - **Kirby's bytes** differ legitimately by what is being measured: 178,072 was
    the pre-deferral image census, 61,668 + 12,752 the post-deferral base plus
    reserve, 273,271 and 169,619 the profile-A W before and after. Not a
    contradiction, but four numbers for one fighter needs the qualifier attached
    every time.
  - **WORK-H P95** has three published values from three configurations; the
    configuration must be named with the number.
  - **The low-only lever's variants** -- 150,572, 167,944, 54,148 -- are the same
    kill computed at different baselines (pre-hat, pre-hat plus donor on an
    optimistic baseline, and a route-(c) staging figure). All are superseded by
    the verdict that the lever is worth zero.

### The lesson, which is the same one twice

Both figures the orchestrator repeated were correct when first measured and
became wrong when the thing underneath them changed. Neither was re-checked
because both were being quoted rather than measured. The standing instruction
given to every agent -- *every number you report must name its source* -- applies
to the briefs themselves, and a figure carried in a brief template propagates
faster than one in any single document.

---

## The owner's 2026-09-10 regression report, diagnosed (2026-09-10)

The owner played a ROM built 08:25:27 and updated `docs/BUGS.md` from live play.
That ROM carried uncommitted in-flight work -- roughly 776 insertions of an
unfinished character-select load slice -- so the first question for every symptom
was whether it lived on the committed side or in the tree. Three investigations
answered that and killed four premises on the way.

### Seven fighters with no character-select preview: delayed-empty, not missing

The previews appear. A first visit to a kind leaves the panel **empty for about
25 tics** -- thirteen dwell tics, then roughly twelve load tics -- and then fills.
The dwell commit destroys the old fighter and forces the kind to null *before any
byte moves*, so the gap reads as blank rather than stale, which is what makes it
look like a missing preview rather than a slow one.

**Killed: "four slots for twelve fighters".** `d99a89f8741` did not build that.
The blocks are a per-*kind* cache with eviction serving four live *instance*
slots (`NDS_PLAYERS_VS_RESIDENT_BLOCKS = GMCOMMON_PLAYERS_MAX`), so the design is
not short by construction.

**The working set is exactly Mario and Fox**, not five. Of twelve kinds: seven
delayed, Mario and Fox instant, and Kirby, Jigglypuff and Ness compiled out at
roster rung 7 showing the baked question-mark by design -- those three are not
previews and cannot be missing ones. The asymmetry that explains Mario and Fox is
structural: `ndsMNPlayersVSPreviewPrepareResidentKind` demands two native owner
images for every other kind while those two fall through to `Ready` with no image
gate. So the defect is per-kind acquisition cost, not rendering -- the draw path
is shared and proven.

**Both baselines delay; the tree delays longer.** The empty window exists at the
committed baseline. The uncommitted slice roughly doubles it, because its
acquire/retry went from 10/3 to 91/84 -- about twelve retry tics per completed
load. An unchanged `CSSFTR draws=826` initially suggested the slice was innocent;
a frame count cannot see latency, and that reading was wrong.

**Killed: the slice can stick permanently IN_PROGRESS.** Every `IN_PROGRESS`
return resumes with advanced state, the only exits are `DONE` at depth zero or
`FAIL`, popped frames run the same finalize the atomic loader runs, and cancel is
closed. Measured 7 finishes and 0 fails. It is a work-in-progress *performance*
artifact, not a correctness trap.

**One real defect, at its owning seam.** Retire, victim selection and the slice
continuation all gate on `sNdsPlayersVSPreviewResidencyActionBudget` and
decrement it. The **bind stage does neither**, despite a comment claiming it owns
"its own final residency action" -- so it can stack a second heavy BGM-fenced I/O
action onto a tic whose budget another slot or a cancel-service already spent.
That is the continuation-tic stacking the residual 4.41M-tick peak implicates.
The repair is to take a budget unit at the `load_tree_done` stage exactly as the
neighbouring stages do.

### Sector Z crashes: committed side, not work in progress

Every Sector Z-touching file is byte-identical between HEAD and the working
tree -- the Arwing matrix case, the laser owner, the stage owner. The crash
shipped; it is not an artifact of the in-flight slice.

**Killed: the matrix-stack imbalance theory.** The push and pop are balanced on
every path including the early returns, so a GX stack pointer leak is not the
mechanism. A scripted reproduction case exists, so this needs no owner time.

### Newly visible geometry with no texture: two of my leads were wrong

Peach's Castle renders all its roof geometry now and Mushroom Kingdom's side
platforms are present, both untextured.

**Killed: the two-missing-corpus-outputs theory.** The regenerated static texture
corpus produces 42 outputs from 44 keys, and the two keys without outputs are not
these surfaces.

**Killed: the alpha and rendermode gate as the cause.** It provably cannot
produce an untextured surface. The texture bind gates on `use_texture`, which
requires texture state on, a non-zero combine count, not PRIMITIVE_DECAL, and
TEXEL0 appearing in the colour or alpha output -- and that last clause is where
the answer lives.

**The white cards and the opaque sparkles on Yoshi's Island are two different
bugs**, and neither is the untextured-geometry cause. Filing them together was
wrong.


---

## 1P unpaused: the campaign is mostly written and gated, not unwritten (2026-09-10)

The owner unpaused 1P today. Its thirteen plan items had been pause-owned for the
whole campaign, so none had a study. Six surveys ran; this is what they found.

### Eleven of thirteen items are GATED, not absent

"Gated" means the code is present and largely complete behind
`NDS_P2_1P_GAME=0`, which is what ships. Only the every-character campaigns are
genuinely absent, and those are content batch rows -- substitutions, endings and
boards for the eleven fighters after Mario -- rather than new systems.

  - **Campaign driver:** whole translation units compiled in, and local
    integration already reaches the Intro and Link/Hyrule play after GO. The
    runtime bridge reports zero refusals through Giant DK.
  - **Scoring and bonuses:** a large stage-clear translation unit is imported,
    and several counters are already half-live even with the flag off. ROM
    acceptance of the tally is what remains.
  - **Variants:** the data slots are compiled in and the file IDs are present.
    Metal and Giant carry their resist and weight arms behind the flag, and the
    bridge admits both Metal Mario and Giant DK today.
  - **Master Hand: gated AND unreachable.** The bridge refuses the Boss stage and
    every Boss-kind fighter outright, so unlike the variants it cannot be reached
    even with the flag on. Its owner export, wallpaper enums and bullet rows are
    open, and it needs the Last venue. **This is the one boss-side blocker worth
    naming separately.**
  - **1P venues and bonus boards:** five arenas, twenty-five bonus boards and the
    Race are wired as native packets; they are enum-only in the shipping build.
    The Target item stays unlinked until the bonus phase lands.
  - **Team and ally fights** ride the four-fighter engine and therefore wait on
    P2-2 rather than on anything 1P-specific.

So the phase's size is much smaller than "thirteen unowned items" suggested. Most
of the work is acceptance and reachability rather than implementation.

### The shipping flag should stay off for now

Unpausing development and flipping `NDS_P2_1P_GAME=1` in the shipping build are
separable, and the recommendation is to separate them. Lab flag-on builds are
already the campaign instrument -- `probe-p2-campaign.ps1` contracts on the flag
and `docs/VERIFYING.md` carries flag-on checks -- so every campaign item is
measurable without touching what ships.

The reason not to flip the default yet is arithmetic rather than caution:

  - The pack gate is short **227,380 B**, and flag-on *adds* the ten-roster and
    twelve-polygon forced residency to the failing side.
  - The campaign's own margin is **8,356 B free** at the first ladder fight in
    the lab configuration, against a shell floor of 1,968 B versus a 32,768 B
    minimum. There is no slack to absorb growth.
  - A four-fighter battle cannot currently start at all, so flipping now would
    also risk conflating campaign residency with the VS pack measurement the
    critical path is trying to close.

Order: keep the default off, work the campaign in lab builds, and flip after the
startup fix and the pack gate.

### Keep the two RAM constraints separate

Repeated here because conflating them has cost this project real time: the
frame-45 latch deficit is **13,436 B** and is expected to clear with roughly
5,000 B to spare, while the pack shortfall is **227,380 B**. They are different
problems with different fixes, and treating them as one is what kept the small
blocker invisible behind the large one.


---

## ITCM has 392 bytes free, measured from a freshly linked ELF (2026-09-10)

This quantity has now been wrong twice in one day, in opposite directions, and
both errors had the same root: it was being **quoted** rather than **measured**.

  - "864 B free" -- a message-era figure, repeated into briefs and one commit
    message.
  - "32,736 of 32,736, zero free" -- taken from the ITCM ledger document. That
    correction was recorded this morning and is also wrong.
  - **392 B free** -- `.itcm = 0x7e58 = 32,344 B` against the linker region
    `0x7fe0 = 32,736 B` (`linker/nds_hot_text.ld:18`), read with
    `arm-none-eabi-nm` from an ELF linked today in an isolated build.

The measured figure is authoritative because it came from a link of the current
tree. The ledger is a document; the ELF is the binary. **When those disagree, the
binary wins**, and the correct move is to link and measure rather than to pick
whichever document reads more recently.

The procedural consequence recorded earlier still stands and is unaffected by the
number: a change that places code in ITCM should name what it evicts. 392 B is
not room for anything meaningful, and the region has been within a few hundred
bytes of full all week.

Capsule itself is the worked example of the right answer: its submit function is
1,984 B and lives in `.main`, every emitted Capsule symbol in the ELF is `.main`,
and its ITCM delta is zero with no eviction required.

## Capsule's own specification carried one wrong detail

The dispatch specification gave the callee pointer words as `4/9/12/26`. The
checked display-list sequence and the generator implementation both use
**`4/10/14/26`** -- words 9 and 12 are non-pointer commands. Everything else in
that specification held: the `0x03E0` header, the 32-command `0x0440` callee, and
the sibling roots `0x0540` and `0x05E0`.


---

## Why Codex agents die mid-task, measured rather than guessed (2026-09-10)

Four agents stopped mid-task in one session, each after 50 to 80 minutes with
transcripts between 2 and 9 MB. One printed the cause verbatim: *Codex ran out of
room in the model's context window.* Every one had already written real work, so
each cost a second agent slot to recover by reading the diff and relaunching.

An investigation sampled the transcripts. **Prompt length is not the cause, and
scope is only half of it.** What dominates is **tool-call volume**: dead and
finished sessions alike run 100 to 180 calls, and every call's output re-enters
context.

Two things fill it:

  - **Repeated full-diff echoes.** One transcript was 53.4% diff-line characters
    with 1,234 `diff --git` lines; another repeated the same probe-script diff
    header roughly 165 times.
  - **Compiler-warning floods.** One 13 MB transcript carried 17,487 `warning:`
    lines making up 44.9% of its characters, almost all of them include-chain
    notes from the BattleShip `decomp/` tree. Notably that session *finished* --
    warnings do not kill on their own, they spend budget that is then unavailable
    when something else needs it. **The decomp include chain is quietly taxing
    every build-running agent**, and that is a build-hygiene problem with a real
    payoff.

### The three boundaries, all of which must hold

Any two are not enough -- two of the dead sessions satisfied two each:

 1. **One verb per session: implement XOR measure.** Build-and-probe-and-report
    must be split, with the confirming run belonging to a separate verify task
    rather than to the implementer.
 2. **At most three named files**, no broad sweeps, and no "read this document in
    full" beyond the two root documents. The surviving comparison ran 54 tool
    calls; every death ran over 100. Treat ~100 calls, or any second full-tree
    read, as the abort line.
 3. **No red-build retry.** Stop after two consecutive red builds and hand back
    the last thirty log lines plus `git diff --stat` -- never the full diff and
    never the full warning stream.

The agents that finished cleanly share a shape: report-only or single-artifact,
host-side, no `make`, no emulator, a bounded file list, and verdict-shaped
output. The collision-parity checker, the pose clock work and the stage emit
witness are the positive examples, at 0.6 KB to 307 KB of transcript rather than
several megabytes.

### What this changes

Briefs already carry a bounded-reading instruction, which was added after two
probes died at 7.1M tokens from being told to sweep broadly. That stays. What is
added is the build-versus-probe split, the explicit abort line, and a prohibition
on re-printing a full diff or full build log into the transcript.

The permanent home for this is the agent-workflow guidance in `AGENTS.md`, which
requires owner permission to edit; it is recorded here in the meantime so the
next session does not rediscover it at the cost of four more agents.


---

## The bind-stage budget fix was right about the defect and wrong as written (2026-09-10)

The diagnosis stands: retire, victim selection and the slice continuation all
gate on `sNdsPlayersVSPreviewResidencyActionBudget` and decrement it, while the
bind stage does neither despite a comment claiming it takes its own final
residency action. That asymmetry is real and it lets a second heavy BGM-fenced
action stack onto a tic whose budget something else already spent.

**Taking a budget unit at the bind stage, exactly as its neighbours do, breaks
the screen.** Measured, both runs on the same route and configuration:

                                 slice only        slice + bind budget
    MSMAX w3                      4,409,600            4,073,984
    MSVB3                 1482 110 24 35 max=8   1510 108 10 23 max=8
    three-or-more tail                   59                   33
    CSSRESACT acquire / retry         91 / 84          3,272 / 3,272
    CSSRESACT load / finish              7 / 7                6 / 6
    CSSFTR sel / vis                      3 / 3                0 / 0

The peak improved slightly and the tail nearly halved, which is why the numbers
look attractive at a glance. But `sel=0 vis=0` means **no fighter was ever
selected or made visible** -- the owner's exact reported symptom, taken from
"delayed by about twenty-five tics" to "never". And 3,272 retries against 3,272
acquires is one retry per acquire with essentially nothing getting through: the
bind stage is starved rather than paced.

So the budget is not simply a resource the bind stage forgot to charge for. The
bind is the stage that *completes* a load, and refusing it when the budget is
spent means a load that has already done its I/O never publishes, so the next
tic re-acquires from the start. The neighbouring stages can be refused safely
because each is a discrete step that resumes; the bind is a commit, and commits
cannot be starved the same way.

**What would actually work is not specified here**, and the honest next step is
to price the bind's own work rather than to gate it -- the residual 4.07M-tick
peak on a continuation tic is still 3.6x the budget, and the slice measurement
already recommended splitting one continuation into stream-read cost versus
publish, finalize and fixup cost before any further implementation change. That
recommendation now has a second reason behind it.

The experiment was applied, measured, and reverted by the agent that ran it, so
the tree carries the slice only. The evidence is permanent at
`artifacts/performance/2026-09-10_p2-shell_css-bind-budget.txt` and
`...-restored-slice.txt`.

**Do not re-try this change as written.** It is the kind that looks like a clean
win on the headline numbers and fails on the one counter nobody was watching.


---

## The four-fighter startup deficit is 2,348 bytes, not twenty (2026-09-10)

The frame-0 startup fix was verified and the answer is **no**: a four-fighter
battle still does not start.

What the fix did accomplish is real. The player-tag OOM is gone -- startup now
proceeds past `ifCommonPlayerTagMakeInterface`, which was where it previously
died on roughly a 108-byte request. So pre-seeding the four tag SObjs and funding
the pool by halving a display-list buffer both worked as designed.

**The deficit simply moved to the next allocation, which is two orders of
magnitude larger.** Startup now reaches fighter pose creation and fails there:

    ndsFtPoseOpen requests   3,072 B
    free at failure            724 B
    deficit                  2,348 B

The measurement is sound in the way that matters here. The guest publishes its
fault witnesses and executes `armDCacheFlushAll` before halting, and `r5 = 0xC00`
independently confirms the 3,072-byte request -- so no raw heap-table read through
gdb was used, which is the trap that once turned one written-back cache line into
an "8 of 18 slots patched" finding.

**This retires the "roughly twenty bytes" framing.** That figure described the
tag allocation only, and it was carried into the board, the restart surface and
several briefs as though it described the four-fighter blocker. It never did.
The original follow-through investigation had warned exactly this -- that
pre-seeding without a real rebate "merely moves the same OOM earlier" -- and the
warning was right in shape while understating the scale.

The 592 B of remaining margin is therefore not thin, it is absent: the next
allocation alone wants four times it. `PROJECT_GOAL.md` makes stability
mandatory, so this configuration does not ship regardless of what else is green.

**What this means for the critical path.** The startup fix is necessary and not
sufficient. The next question is not "is 592 B enough" but "where do 2,348 B come
from", and that sits much closer to the pack-gate problem than to the
startup-sized rebate the tag fix represented. The two constraints were carefully
separated earlier in the day for good reason; this finding does not merge them,
but it does move the startup blocker out of the trivial category it was filed in.

## The two-fighter guard arm is also not clean

Mode 163 -- the standing regression guard and one of the three publication gates
-- **starts but does not complete**. It reaches `scVSBattleStartBattle` and the
runtime verifier then aborts on `NATIVE_FAILURE=1,2,22,66125928,6,1728,0,1`.

That is a separate finding from the four-fighter deficit and it matters more for
shipping: the four-fighter arm is future content, while mode 163 covers the
configuration that actually ships. Publication was already held pending mode 163
being green on the clean-built ROM; this is that gate failing rather than
passing, and the failure is a native record rather than a timeout or a crash.


---

## The acid subdivision does not fit the vertex format (2026-09-10)

The owner reports Zebes' acid edges as "too HARD -- edges too defined instead of
a gradient". The specified repair was to recurse the midpoint subdivision once
more on root `0x9D8`. It was attempted and **correctly refused**, with no change
made and nothing re-pinned.

Root `0x9D8` is 101 dense vertices, 28 triangles, 17 runs. Of the 84 candidate
second-level edge instances, **28 have odd X sums and 44 have odd Z sums** (32
odd S, 44 odd T), so their exact midpoints are half-integers.
`NDSNativeStageDenseVertex` stores `x/y/z/s/t` as `s16`, and the coordinate-shift
mechanism reduces integers for GX range without providing fractional source
storage. The existing midpoint helper would therefore round -- and a rounded
midpoint puts a fold in a plane that must stay planar.

The brief required every new vertex to be an exact midpoint average precisely so
this could not be papered over. Without that requirement the rounding would have
produced geometry that passes every count check and is subtly wrong, which is the
hardest kind of defect to trace back to its cause.

Baseline is unchanged and green: `--stage zebes --check` gives
`dense_vertices=431 runs=72 triangles=172`.

**What this leaves open.** Softening the acid needs a different route: a
coordinate-basis change that makes the midpoints integral, a wider vertex format
-- which would grow every stage using it, against a pack gate already short
227,380 B -- or, most likely, no geometry change at all if the source carries the
softness in a texture alpha ramp or the blend rather than in vertex
interpolation. That question should have been settled before a subdivision was
specified, and the neighbouring light shafts are the cautionary precedent: they
also looked wrong and turned out to be exactly source-faithful.

## The DTCM dense-normals pin is stale, not broken (2026-09-10)

Mode 163 fails before gameplay on `DTCM owner 'sNdsNativeFighterDenseNormals' is
2452 bytes, expected 2268`. The growth is **legitimate** -- landed Mario hand
variants -- and not a duplication or double-emit.

The evidence is structural rather than circumstantial: the generator enforces
per-owner block ownership rather than only a total, with fail-closed errors, so a
duplicated or twice-decoded root would have broken a block count or a per-detail
total instead of quietly growing the size. High and low details also grew
consistently, by 46 and 36, with matching block shape -- which fits two roots per
detail per fighter rather than an accidental double of one root.

So the pin is a measurement that fell behind its subject. It is being re-measured
from the generator's own output rather than transcribed from the assertion
message, which is the distinction that made three other figures wrong today.

## Four-fighter startup and frame-45 latch are closed locally (2026-09-10)

The current four-distinct-kind tick-HUD build now crosses both RAM failures that
were blocking every useful P2-2 measurement. `probe-p2-fourcpu-sparse.ps1` at
present 32 reached four live pose owners with `POSE=4,0,4,4,4,0`; at present 64
the same configuration reported:

    MEM=53128,1490944,57
    GOBJ=-1,60,0,0,0
    GFXHEAP=1536,16,0,0
    POSE=4,0,4,4,4,0

So the historical player-tag / `ndsFtPoseOpen` frame-0 OOM is no longer the first
failure, and the frame-45 GObj latch did not arm: the general-heap low-water is
53,128 B, comfortably above `ifCommonSetMaxNumGObj`'s 25,600 B trigger, while
`sGCCommonsMaxNum` remains -1 with 60 active GObjs. `gNdsObjmanPanicCount`, its
mask, and `gNdsSyMallocOverflowCount` are all zero.

The scene budget producing that result is still bounded by the DS renderer's
measured/source writer limits: 16 and 4 Gfx entries for the two retained display
list buffers, 1,536 B per graphics heap, and a nonzero 16 B RDP output block. At
present 64 the graphics heap has used only 16 B and both the overflow and
checked-no-room counters are zero. The player-tag overlay pre-seeds four SObjs so
the four 108 B tags do not depend on late heap allocation.

The sparse probe now prints the GObj cap/panic/allocator tuple and graphics-heap
capacity/peak/failure tuple every run. That makes the latch acceptance criterion
directly visible instead of asking a future investigator to infer it from
survival. `scripts/menus/test_taskman_battle_display.py` was also stale against
the current taskman wrapper; its mock now covers the 1P enum checks and scene
memory recorder, and passes again.

This closes the two small RAM constraints only. It does **not** change the
402,984 B worst-set pack estimate or the 175,604 B optimistic allowance; the
remaining pack shortfall is still 227,380 B and is now the first RAM problem on
the P2 critical path.

## Pack-disabled skeleton falsifies the old exact ceiling (2026-09-10)

The queued four-slot skeleton measurement was run on the shipping shell instead
of being left as another inferred budget. Build identity and permanent evidence
are in `artifacts/performance/2026-09-10_pack-skeleton-ceiling/CEILING.md`.
The important result is that this control cannot reach battle:

    FOURKIND SETUP kind=3 free=418920   raw tree=83200
    FOURKIND SETUP kind=1 free=297820   raw tree=116944
    FOURKIND SETUP kind=7 free=162548   raw tree=100160
    FOURKIND SETUP kind=2 free=23732    raw tree request=77360
    FOURKIND HALT request=77360 headroom=23732
    FOURKIND ARENA chosen=1294336 overflow=1

So `generalfreemin` never initializes; substituting its sentinel into the review
equation is invalid. This means the historical 175,604 B allowance cannot be
promoted to a current exact shell ceiling by this run.

There is still a hard current RED proof without guessing `D_other`. The first
three resident raw trees total 300,304 B. Give the future pack every impossible
advantage: delete all three for free, charge **zero** for fighter 4, everything
after the stop, and `D_binder`, then preserve the required 32,768 B floor:

    W <= 23,732 + 300,304 - 32,768 = 291,268 B

This is only a relaxed upper bound; omitted real costs lower it. The current
estimator worst set is 402,984 B, therefore the current shell proves a minimum
shortfall of **111,716 B** (`402,984 - 291,268`) even without trusting the stale
historical equation. Pack residency remains the first RAM problem, but future
agents must quote the two numbers with their qualifiers: >=111,716 B is the
current direct lower bound on the gap; 227,380 B is the historical-model gap.

The >=16 KiB allocation census also explains where the arena is going before the
raw trees: 153,600 B scene/cache, 208,672 B battle files, 185,920 B collision,
82,976 B items, 33,152 B additional battle state, and 52,736 + 28,352 B effects.
Those are the concrete secondary-recovery targets once semantic pack reduction
has exhausted no-fidelity-loss closure compaction.

## Foreign fighter-model liveness is object-granular (2026-09-10)

The estimator was still treating file closure as object liveness for foreign
native-owner model files. Kirby pulls `338_YoshiModel.c` into its source closure,
so the old ledger retained 11,690 B of Yoshi records and charged Yoshi's 29,948 B
native image even though Kirby has no path to Yoshi body geometry.

The source relocations make the exact slice explicit. Kirby's closure contains
`328_KirbyModel.c` and `338_YoshiModel.c`, but not `247_YoshiMain.c`; the only
cross-file Yoshi targets are:

    328_KirbyModel.reloc:1006 -> YoshiModel + 0x9EC8 (palette)
    328_KirbyModel.reloc:1007 -> YoshiModel + 0x9EF0 (texel)

Starting at those external entries and following YoshiModel's internal readers
reaches exactly two objects / 296 source bytes. The other 470 indexed donor
objects are now `UNREACHABLE_DONOR_DROP`. Both live banks are costume-common and
VRAM-resolved, so Kirby no longer charges a Yoshi native-owner image.

Kirby falls from 169,619 to 127,997 B raw and 167,059 to 127,885 B VRAM-bound.
Across all 793 one-through-four-kind sets, the new worst is
Captain+Link+Yoshi+Kirby: 399,416 B raw / 371,444 B VRAM-bound. Against the
current relaxed 291,268 B shell ceiling, the defensible minimum gap is therefore
**80,176 B**. The exact ceiling remains unknown until a complete four-slot
skeleton reaches battle.
