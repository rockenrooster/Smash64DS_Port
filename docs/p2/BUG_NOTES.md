# Bug investigation notes (P2 repair queue)

Working notes behind `docs/BUGS.md`, which stays the lean owner-facing queue
(`docs/BUG_FIXING_PROCESS.md`). One entry per queue row that has evidence
worth keeping; append, do not rewrite history.

## Menus

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
