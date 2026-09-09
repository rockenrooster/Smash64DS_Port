# Bug investigation notes (P2 repair queue)

Working notes behind `docs/BUGS.md`, which stays the lean owner-facing queue
(`docs/BUG_FIXING_PROCESS.md`). One entry per queue row that has evidence
worth keeping; append, do not rewrite history.

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
- Mushroom Kingdom side platforms: the geometry is not in the packet at all
  (2026-09-08 probe, CONFIDENCE HIGH). The scale platforms live in stage file
  155, and that file is not among the generator inputs for Inishie, so the
  packet has zero bindings and zero runs for `dStageInishieFile3_DL_0x05F0` /
  `DObjDesc_0x0380`. The ground-actor arm admits the scale actor, but nothing
  pins its display lists or textures. This is not a transform putting them off
  camera and not a binding admitted then dropped: the data was never carried.
  Fix: extend `scripts/stages/native_stage_descriptors/inishie.py` with the
  file-155 chain (desc 0x0380, DL 0x05F0, the 0x01C8/0x02E8/0x0300/0x0328/
  0x0340 chain and textures 0x0498/0x04B0), pin its textures in the stage
  corpus, and drive the bindings live from the scale platform state.
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

## Mushroom Kingdom's platform row is complete (2026-09-08)

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
