# Four-Fighter 30 FPS Architecture (P2-2p8)

Owner request, 2026-09-22: any four fighters on any stage at a stable **30 FPS**,
using the compromise list in the owner's message of that date ("Change the
implementation as aggressively as necessary. Preserve the game."). The owner's
rulings on this design (D1-D7, same day) are in section 8; the gate stays P95.
This document is the architecture that request needs. It is a design, not a measurement. Every
saving below is an ESTIMATE until its phase gate measures it on the four-CPU
stress run. The measured basis is
`artifacts/performance/2026-09-22_p2-2p8-architecture-baseline/`.

Units: timer ticks at 33.513982 MHz (1 tick = 2 ARM9 cycles). The budget for a
30 FPS frame is two refreshes, 1,120,000 ticks of WORK.

## 0. Summary

**Where we are.** Four CPUs (Donkey/Samus/Link/Kirby), Dream Land, items on:
mean presentation **16.8 FPS**; **0 of 1,535 frames** present in two VBlanks;
WORK-H P50 **1.59M**, P95 **2.30M**, P99 **2.92M**.

**Why the 09-17 ledger found no lever.** Every candidate it priced was a leaf
inside the same machinery, and the machinery is the cost:

1. **Rendering is 61% of the median frame** (FTR 344K + STG 326K + MISC 296K).
   Only ~1,100 ticks per fighter joint are matrix math; ~2,200 are plumbing
   (display-head capture, draw plan, material refresh, packet key and precheck,
   patching). Dream Land's Whispy eyes/mouth and flower beds cost **43,998 ticks
   for 21 triangles** (`src/nds/nds_renderer_native_owners.c:1317-1320`).
2. **P99 frames are event frames, and events do storage I/O.** From median to the
   top 1%: hit detection +386K, status/param change +306K, catch +93K, fighter
   packet re-production +441K. In the stress roster **every motion change reads
   storage** (681 reads, 0 cache hits: the animation cache is never reserved), sound
   effects add 326 synchronous reads, and BGM refills cost ~123K every ~13 frames.
3. **The working set does not fit the machine.** Each frame executes ~675
   functions / ~284 KB of code (~114 KB of distinct instructions) through an 8 KB
   I-cache and a full 32 KB ITCM; 64% of cycles are memory stall. No leaf change
   shrinks a working set; deleting machinery does.

**The architecture: keep the rules, replace the machinery.** The source's rules
(status procedures, CPU AI decisions, physics and damage/knockback formulas) stay
decomp C at 60 Hz and remain the gameplay authority. Everything around them
becomes a compiled, DS-native runtime:

| # | Pillar | Replaces | Main effect (ESTIMATE) |
|---|---|---|---|
| A1 | Compiled render: host-built per-instance GX lists patched at matrix/tint sites, one DMA per program, native per-frame draw list | display-tree walks, packet record/replay/production, CPU stage vertex emission, per-frame render caches | render 966K -> ~255K (P50) |
| A2 | Full motion residency in a new compact format (D6); ARM7-streamed audio (D7) | on-demand NitroFS/FatFs reads inside gameplay frames | removes I/O from the tail |
| A3 | Shared pose, per-consumer composition; native clips | figatree interpretation | pose/anim -40..-80K |
| A4 | Events cost O(1): pre-bound motions, pre-resolved event operands, pooled effects | per-status lookup/parse/bind/alloc/re-production | SPRM P99 311K -> <=40K |
| A5 | Batched fixed-point hurtbox kernel (guarded, shadow-proven); stage-compiled map collision; exact AI perception memos | per-victim soft-float chains, per-query collision overhead | SHDT+SCAT P99 512K -> ~100K |
| A6 | Rates: 60 Hz only for gameplay truth (no run-ahead, D4) | 60 Hz visual work | camera matrices, HUD, particles |
| A7 | Memory: overlays, render-code retirement, per-match TCM | front-end code resident in battle; generic code in ITCM | funds A2; cuts stall |
| A8 | ARM7 owns BGM streaming and FGM voices | ARM9 refills, timer IRQ, synchronous SFX reads | AUD spikes gone |
| A9 | An instrument that does not move the gate | tick-HUD refresh inside measured frames | honest P99 |
| A10 | Visual reserve, used only on a measured residual | — | LOD tiers, 15 Hz effects |

**Gate (owner ruling D1, 2026-09-22): P95 WORK <= 1,120,000 and >= 95% of all
presented frames in two VBlanks**, items on, every legal roster and stage, on the
shipping configuration; P99 is reported, not gated. **Design target** on the
measured roster: WORK P50 ~0.7M, P95 ~0.9M, P99 ~1.0M. The tail margin depends
on two unsized items (section 2), so Phase 0 measures them before any phase is
promised.

## 1. The frame today (measured)

Whole-match tick-HUD rows, four CPUs, Dream Land, items on, frames 440-1973
(`…/2026-09-17_p2-2p8-dtcm-hot-scalars/fourcpu-rows.csv`, summarised in
`…/2026-09-22_p2-2p8-architecture-baseline/bands_dtcm.txt`):

| band (by WORK-H) | WORK-H | SRC | SINT | SPHD | SHDT | SPRM | SCAT | FTR | STG | MISC | AUD |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| P40-60 | 1,584K | 579K | 287K | 120K | 30K | 5K | 2K | 344K | 326K | 296K | 12K |
| P90-95 | 2,192K | 942K | 482K | 170K | 110K | 23K | 5K | 560K | 327K | 311K | 25K |
| P95-99 | 2,497K | 1,127K | 473K | 180K | 219K | 78K | 13K | 627K | 327K | 358K | 31K |
| P99+ | 3,236K | 1,671K | 443K | 217K | 416K | 311K | 96K | 785K | 327K | 404K | 21K |

VBlanks per presented frame: 3: 832, 4: 560, 5: 118, 6: 19, 7: 5, 8: 1.

- **STG is a constant** (326K in every band); **FTR and MISC grow in the tail**
  (packet re-production, effect spawns); **SRC's tail is hit/status/catch events**
  and physics substeps under knockback. The CPU AI (SCPU, inside SINT) is flat.
- The **HUD** bucket spikes ~450K every 9-10 frames: that is the tick-HUD console
  refresh, an instrument cost the shipping ROM does not pay (A9). **AUD** spikes
  ~123K every ~13 frames: the BGM refill.

Mechanism attribution (per-PC profile, self time, `classes.txt`): RENDER_STAGE
213K, RENDER_COMMON 210K, RENDER_FIGHTER 203K, SOFTFLOAT 150K, MAP_COLLISION 97K,
FT_LOGIC 97K, POSE 78K, MEM 60K, ANIM 47K, CAMERA 46K, RENDER_EFFECT 44K, SYSLIB
41K (FatFs, libnds GL wrappers), INSTRUMENT 37K, OBJMAN 33K, RELOC 29K (runtime
asset resolution), FT_AI 29K, PARTICLE 23K, HUD_LOGIC 22K. The highest CPIs are
dispatchers (OBJMAN 6.5, FT_AI 6.1, FT_LOGIC 5.8): a working-set signature.

## 2. Target budget

Band means, ESTIMATE, on the measured roster. "P99+" is the mean of the top 1%;
the P99 value itself sits below it.

| Component | now P50 | now P99+ | target P50 | target P99+ | Pillar |
|---|---:|---:|---:|---:|---|
| FTR fighters | 344K | 785K | 70K | 90K | A1, A4 |
| STG stage | 326K | 327K | 35K | 40K | A1 |
| MISC effects/weapons/items/particles | 296K | 404K | 150K | 200K | A1 (needs Phase 0 split) |
| SINT update + interrupt (incl. AI) | 287K | 443K | 230K | 330K | A2, A3, A5 |
| SPHD physics + map collision | 120K | 217K | 80K | 140K | A5 |
| SHDT + SCAT hit/catch | 33K | 512K | 30K | 100K | A5 |
| SPRM params/status | 5K | 311K | 5K | 40K | A2, A4 |
| other SRC (effects/items/stage logic, camera) | 133K | 187K | 90K | 120K | A4, A6 |
| AUD | 12K | 21K | 3K | 5K | A8 |
| HUD + instrument | 68K | 26K | 10K | 10K | A9 |
| residual (OTHR - WAIT) | 27K | 27K | 25K | 25K | — |
| **WORK** | **1.58M** | **3.24M** | **~0.73M** | **~1.10M** | |

Two items are unsized and carry the P99 margin:

- **MISC residual.** ~200K of MISC matches no top-400 symbol; it is a residual of
  DRAW minus the named buckets (`src/port/taskman_seam_battle_host.c:949-956`),
  holding whole-GObj display traversal, classification, weapon/item/effect
  submits, the particle pass and the damage-slash texel fill. The
  `gNdsMisc{Weapon,Effect,Particle,TexUpload}DrawTicks` counters exist but are not
  sampled into the rows. Phase 0 samples them.
- **SRC "other".** Effects, items, stage logic and camera procs inside GCRA are
  not bracketed per owner. Phase 0 brackets them.

If the gate still misses after A1-A5, the remaining option is A10 visual
reserve, case by case with the owner (D5); run-ahead is refused (D4).

## 3. Architecture

### A1. Compiled render pipeline

**The design is an evolution of what exists.** The fighter generator already emits
whole-owner GX FIFO templates with patch tables, kept host-only under `#if 0`
(`src/nds/nds_native_fighter_owner.generated.inc:7938`: Mario 4,034 words for 320
triangles). Fighters and Task 36 stage runs already DMA recorded packets to the
GX FIFO on DMA0. What remains is to stop *recording* at runtime and stop *walking*
to decide what to draw.

**Fighters.**

- *Offline:* per fighter x detail x program (default, and each model-part program:
  Samus Catch/FSmash/Morph, Link Entry/SpecialN/Catch/Claps, Kirby hats and
  copies, hand variants...), emit a list with NORMAL + strips, DIF_AMB per epoch,
  TEXIMAGE/PLTT patch sites, and one **LOAD4x3 patch site per drawn binding**.
  Cross-part corners (9 of 12 fighters: N64 vertex-cache sharing across DObjs)
  keep today's MATRIX_STORE/per-corner RESTORE, which is static in a compiled list.
- *Load:* copy each fighter's lists into a **per-instance mutable buffer**
  (52-80 KB for four) with VRAM texture/palette words resolved once. Per-costume
  words are baked in.
- *Per event (status change):* rewrite program selection, hidden parts and part
  variants in the instance buffer (copy from the template, NOP-pad). No
  re-production, no packet keys.
- *Per presented frame:* run the display head (its side effects are real:
  off-screen arrow HUD, `gLBCommonScale`, fog statics, scene light,
  `src/port/renderer_adapter_fighter.c:593-610`); compose one 4x3 world per joint
  in an ARM-mode ITCM kernel from the pose (A3); write the LOAD4x3, tint and
  texture-animation words; flush the patched lines; one DMA per program list.
- *Matrices stay CPU-composed.* Engaging GX-side hierarchy composition cost
  +22,848 P50 / +67,456 P95 at four fighters
  (`…/2026-09-16_p2-2p8-gx-compose-decline/`). The matrix stack holds only the
  bindings that cross corners reference (<=14, Yoshi), with slots 0-7 left to
  existing PUSH users; gate on a flat GXSTAT stack level.
- *4x3 trap:* the modelview row 3 carries the 2^-8 world-unit shift including
  m33. Load P' = P with row 3 scaled by 2^-8 once per owner and use
  `[R; T * 2^-8]` 4x3 matrices; x/w, y/w, z/w are unchanged. Proven with the
  Task 49 GX differ before use.
- *Special cases* (31 enumerated): texgen via normal-source TEXGEN plus a per-root
  texture matrix; hurt/colanim flash via DIF_AMB/SPE_EMI words (today's
  approximation, frame-global fog is the limit); light direction per fighter;
  texture animation via TEXIMAGE/PLTT words from prebuilt tables; hidden parts,
  Entry flips, detail hi/lo, Kirby hats, Fox gun overlay, electric skeleton, held
  items, Giant (root scale), Metal, costumes: list selection + matrices + words.
  **Not yet native:** linear texgen (N-Link/N-Captain/N-Ness; not affine in the
  normal), shadows (stubbed today), afterimage trails (not drawn today), Captain's
  high-detail alpha test (a frame-global register). Packed 1P builds halt on any
  native decline (`renderer_adapter_fighter.c:4405-4460`), so coverage must be
  total.

ESTIMATE: 103 joints x ~450 ticks + patches + ~20-25 DMA jobs + heads ~12K:
**FTR ~70K**, and the +441K tail disappears with re-production.

**Stage.**

- No-Z layers keep the constant-depth projection per painter slot
  (`ndsRendererNativeStageSetNoZColumn`, `nds_renderer_native_owners.c:3296-3308`),
  but as **baked LOAD4x4 commands inside the compiled list**, repatched only on FOV
  change. With z = c*w both clip planes collapse onto the eye plane: triangles that
  never cross it (every stage backdrop) need no CPU clip, and Dream Land's 99 rigid
  no-Z triangles already rely on the hardware clip.
- Raw/range runs: static lists with one modelview patch per binding. Dynamic
  bindings (Whispy, flowers, platforms, bumpers, Arwings, Pokemon) recompose per
  frame; material animation writes words.
- Non-rigid no-Z triangles stay CPU-emitted where they genuinely deform (Dream Land
  27, but Hyrule 128, Saffron 134, Mushroom Kingdom 64): those stages need their
  own specialised actors (compromise item 5).
- No stage has camera-surrounding geometry: every VS sky except Dream Land's is
  already a BG2 wallpaper, so "backdrop to BG" is low value (A10 at most).

ESTIMATE: Dream Land **STG ~20-40K** (GE ~15K cycles), **if** the 326K is executor
work: `nds_renderer_native_owners.c:3046-3048` records ~331K ticks/frame of STG
as unattributed, and Task 53 saw removed stage prep reappear as OTHR. Phase 2
measures it.

**Effects, weapons, items, particles (MISC).** A native per-frame draw list
replaces `gcDrawAll` display-proc traversal for everything that is not a fighter
or the stage. Z-buffered effect models (ImpactWave, DamageSlash, items, weapons)
become compiled lists with per-instance patches; particles become view-space
billboards batched per (alpha, sheet, palette) run with one camera context per
frame. There is no texture sort, because submission order is blend order under
manual translucent sorting. DamageSlash textures are resident, so there is no
per-frame texel fill and no mid-draw `glTexImage2D`. Named savings are 36-48K
plus ~24K of traversal; the ~200K residual is sized in Phase 0 before any MISC
promise.

**DMA is a transfer, not free time.** GXFIFO DMA bursts block ARM9 main-RAM access
(single loads stall 1,022-1,660 cycles behind them today), and the geometry engine
looks ~10% busy (~70-90K of 1,120K GE cycles per frame, ESTIMATE). Overlapping
submission with the next simulation tick would move bus stalls into SRC, so it is
**not** a lever until GE-busy and DMA-stall are measured. The CPU simply never
waits on DMA except before `glFlush`. The July claim that the stage is
GE-throughput-bound (`…/2026-07-24_task54-stage-dma-e0.md`) disagrees 10x with
today's backpressure data; Phase 0 resolves it.

**Polygon RAM** is not binding: two-fighter frames use P50 465 / max 510 polygons
(`…/2026-08-15_gxstack-io-draw/gxstat-c183-rows.csv`); four fighters plus stage
are estimated at 850-1,100 of 2,048.

**Retired by A1** (MEASURED `nm -S`, sums ESTIMATE): ~98 KB fighter BSS, ~70 KB
stage BSS, ~19 KB effect packets, up to ~72 KB of texture scratch/refresh/key pools
once all battle textures are pre-converted, and ~180 KB of executor `.text`
(native common, stage adapter, native owners, matrix adapter, fighter adapter,
fighter production). New cost: per-instance fighter lists 52-80 KB (they can live
in the 141,440 B of `gSYFramebufferSets` the packet regions use today) and ~30 KB
for Dream Land.

### A2. Tiered residency; storage off the ARM9 critical path

**Today, every motion change in a four-kind match is a storage read.** The canon
stress run records 681 acquisitions, **0 cache hits** and a **0-byte** animation
cache (`…/2026-09-17_p2-2p8-roster-variance/canon-regression-memory.json`). The
cache needs free heap above pending fighter bytes plus a fixed 128 KiB keep-free
(`src/port/reloc_backend_assets.c:45`, `:13572-13583`). The match's low-water is
111,680, so it never reserves. The warm list covers Mario and Fox only, and warms
nothing when more than two kinds are present (`:13123-13138`, `:14319-14379`).
Sound effects add 326 synchronous reads per match: every cache miss, and every play
of an enveloped cue (`src/nds/nds_audio_fgm.c:1204-1211, 2133-2142`). BGM refills
run on the main thread (`src/nds/nds_audio_bgm.c:2050`). Each read walks the ROM
file's FAT chain from its head: 447 steps at 12 MB, while the shipping ROM is
62.7 MB, so the cost grows with content. DLDI runs on the ARM7 (calico default) and
the ARM9 waits.

**Owner ruling D6 (2026-09-22): every gameplay motion is resident; no motion is
read after GO.** N02.04 stands. The tight motion set is 1,220,912 B for the stress
roster (MEASURED) and ~1.58 MB for the worst four kinds (ESTIMATE), against
~0.9-1.0 MB of battle RAM after A7 (ESTIMATE). Generic LZ reaches only 0.78, so a
**new compact motion format** is required: at most ~0.45x of today's BPS1 bytes
for the worst four kinds (whole-kind LZMA reaches 0.45, so the information content
allows it; the question is a form the ARM9 can use at bind time).

**Compact motion format (MF).** Requirements, in order:

1. **Lossless** for every value the pose engine produces: gameplay reads the
   joints (hurtboxes, hitboxes, attach points), so the decoded pose must be
   bit-identical to today's Q12 output, proven by the existing pose oracle over
   every clip of every kind.
2. **Random access per clip**: a status change binds one clip in O(clip), with no
   stream-wide state. Decode either at bind into a small per-fighter working buffer
   (LZ-class ~5-11K ticks per 2.2 KB clip, ESTIMATE) or, better, evaluate directly
   from the compact form.
3. **Exploit the redundancy the corpus actually has**: constant and
   hold-dominated tracks, keys the interpolation reproduces exactly, shared track
   segments across clips of one kind (the cross-clip redundancy whole-kind LZMA
   finds), narrow deltas, per-kind dictionaries. Build-time encoder, one checker
   that decodes every clip and compares against today's bake.
4. **Worst-case budget**, not typical: the worst four kinds plus Kirby's copy
   clips for the opponents present must fit the resident region with the
   25,600 B floor intact, on the **shipping** configuration.

Phase 0 runs the host-side experiment (achievable ratio per kind, decode cost
modelled on ARM9) before any runtime work. If MF cannot reach the budget, that is
a STOP for the owner, not a quiet fallback to demand reads.

| Tier | Contents | How served | ARM9 cost per use |
|---|---|---|---|
| resident motions | every gameplay-reachable clip of the four kinds (+ Kirby copies for present opponents), ShieldPose, common data, in MF | one match bank built before GO, served by pointer (generalise the Fox BattlePack path, `reloc_backend_assets.c:14941-14949`), pre-bound into `FTMotionDesc` (A4) | bind/decode only |
| resident heads | first 512 B of each reachable SFX cue; BGM ring | ARM7 starts the voice from the head and fills the rest itself (D7) | one PXI word |
| ARM7 streams | SFX tails; BGM | **extent map**: a boot-time LBA list of the ROM file, `blkDevReadSectors` (`calico/dev/blk.h:57-58`) on the ARM7: no FAT walk, no fopen | 0 |

- **Admission.** Replace the 128 KiB keep-free constant with admission from the
  measured low-water plus the 25,600 B GObj floor.
- **Match load builds the bank** (seconds are acceptable, compromise item 28):
  bulk-read the four kinds' MF blocks, relocate, bind `FTMotionDesc`.

### A3. Shared pose, per-consumer composition

A single world matrix per joint **cannot** serve both gameplay and rendering. The
renderer composes from the source's 16.16-quantised N64 locals into Q43.20 worlds
because that is what Fast3D did. The collision world (`FTParts::mtx_translate`)
"was tested and is observably different"
(`src/port/renderer_adapter_matrix.c:6548-6573`). The **local pose** (the pose
engine's Q12 TRS per joint) and the **topology** are shared; each consumer
composes its own world from them.

- The pose clock runs every tick. Body joints are evaluated on the presented tick
  and held on the other, while TransN/XRotN/YRotN and hidden-part joints run every
  tick because physics reads them (`include/nds/nds_ft_pose.h:31-42`). The hold is
  an **accepted deviation** today ("body hurtboxes read one tick stale on the held
  tick"). It stays by default. Exact 60 Hz body hurtboxes cost ~+15-20K and are
  owner decision D3.
- Native clips replace figatree interpretation (`ndsFtPoseParse` 21.5K,
  `ndsFtPosePlay` 36K, `ndsFtPoseUpdate` 17K self): key times, values and tangents
  in the engine's existing Q forms, with operands and bind maps pre-built. The
  binary32 clock stays float-exact, because non-integer `anim_speed` exists and
  events read it.
- The render side reads the same locals once per presented frame (A1). Today it
  pays a float->fixed edge per joint because the pose output is f32; producing Q
  directly removes that edge.

This is the least certain pillar. A lab arm that skipped 94.7% of pose-entry
evaluation saved nothing, but its arms diverged
(`…/2026-09-16_p2-2p8-joint-cap-ladder/`). Its saving is measured, not assumed.

### A4. Events cost O(1)

A status change today (`ftMainSetStatus`, `decomp/.../ft/ftmain.c:4365-4823`, plus
the port hooks in `src/import/battleship_ftmain.c:191-236`) runs forward effects,
detail swap, hitbox clear, hurtbox and part resets, colanim, loop-SFX stop and
DL-link moves. It then runs the figatree **acquisition**: a linear token scan of
~690 rows, a loaded-file table memmove, `ndsAObjEvent32ForgetRange` over a ledger
with high-water 1,623, and a cache copy or storage read. After that come
hidden-part DObj creation, a TRS reset of every joint, the bind, the first play,
and a renderer invalidation that forces the next draw to re-produce packets.
Acquisition, ledger and I/O alone account for ~83K of the P99-class excess.

- **Pre-bound motions.** Rewrite each fighter's `FTMotionDesc` entries at match
  load with tagged pointers to Tier-0 clips, so the decomp `ftMainSetStatus` runs
  unchanged and the port's force-load becomes a tag test. Exact by construction;
  the pose oracle (`NDS_FT_POSE_ORACLE`) proves it.
- **Keep the gameplay half:** the first play on the transition tick is source
  behaviour.
- **Pre-created hidden parts** at load, relinked on change.
- **Motion events: pre-resolve, don't compile.** The interpreter costs ~4K per
  frame (`ftMainUpdateMotionEventsAll` 2,082 + `...ForwardEffect` 1,546), so
  generated C is not worth it. What pays is pre-resolving operands (joint index,
  effect/SFX descriptors, absolute jump targets) and making their callees O(1):
  pooled, pre-initialised effect records, and resident or ARM7-served SFX.
- **No render re-production:** A1 has no packets to invalidate.

### A5. Combat and collision kernels

**Hit detection is expensive per engaged victim, not per pair.** When any live
hitbox is inside a victim's range box (`hit_detect_range`, +-1200 wide for
DK/Samus/Link), each of that victim's hurtbox joints rebuilds a soft-float world
chain, a 3x3 inverse and three `sqrtf`. That costs **7,377 ticks per joint**, about
81K per engaged victim-tick (MEASURED 1v1,
`artifacts/performance/2026-08-16_shdt-mechanism/SHDT_MECHANISM.md:109-165`).
SHDT's P99+ 416K is about five engaged victim-ticks per frame. A pair-level broad
phase cannot help: >=97% of pair evaluations already exit before any geometry
(`artifacts/performance/2026-08-13_shdt-broadphase/REFUTED_PAIR_REJECT.md`). Catch
search uses the same gateway.

- **Kernel.** On a victim's first engagement in a tick, prepare **all** its
  hurtbox joints in one ARM-mode ITCM pass from the pose engine's Q12 locals (no
  f32 edge). Use a Q30 basis and Q20 s64 translation (the renderer's proven SMULL
  form), with the hardware divider and square root: ~48 B per joint in a port-side
  store. Do **not** write the `FTParts` latches; capture reads them
  (`ft/ftcommon/ftcommoncapturepulled.c:31`).
- **Exact decisions by construction.** The fixed narrow phase decides only when
  its margin clears an analytic error bound. Inside the bound it falls back to the
  decomp float test for that pair. Source order semantics are kept: the first
  hurtbox in array order wins (`ftmain.c:3197-3213`), hit-log order holds, and the
  knockback winner uses strict `<` (`:2845`).
- **Proof.** A same-ROM shadow arm (one `.data` word) runs both paths and counts
  decision flips. The gate is 0 flips plus identical whole-match witnesses.
- **Why this is not the lane that lost.** The fixed-point collision lane
  (exchange rate 2.68,
  `artifacts/performance/2026-08-15_cfx-narrow-exchange/EXCHANGE.md`) was entered
  about once per frame, cold in the I-cache, and paid f32 edges and a libgcc 64-bit
  divide. This kernel is batched per tick, sits in ITCM, reads Q locals and uses
  the hardware divider.

ESTIMATE: ~600 cycles per joint instead of ~14,750. SHDT P99+ 416K -> 60-100K,
SCAT P99+ 96K -> ~20K, P50 unchanged. Fallback if the guard band misbehaves: a
bit-exact incremental float chain that reuses rotation blocks while ancestor locals
are bit-identical (held pose, hitlag), at 25-40% of today's chain.

**Map collision is a median cost** (97K, flat in the tail). Stages have 5-19
lines, so spatial bins buy nothing, and endpoints, kinds and yakumono are already
memoised per line. What remains is **per-query** overhead: readiness checks
(`ndsStageCollisionLoopGeometryReady` 9.2K), route tests, O2R halfword reads, lab
proof hooks inside each floor query
(`src/port/reloc_backend_mp_collision.c:1552,1655,1677`) and soft-float compares.
Knockback multiplies queries (`mpProcessUpdateMain` substeps up to ~10 per tick),
which is the SPHD tail. Design: build-time per-stage tables indexed by line id,
one validity check per scene, a per-tick yakumono snapshot, lab hooks compiled
out. ESTIMATE 40-50K with float-exact numerics (the board freezes collision
numerics), or 70-80K with a proven fixed-point core behind the same guard/shadow
method.

**CPU AI** stays source. Exact shared perception only: memoise
`func_ovl2_800F8FFC` (one floor query per target per CPU,
`ft/ftcomputer.c:3744`) and per-fighter "targetable" facts, keyed by position bits
and stage generation. Never cache across `syUtilsRandFloat` calls or reorder them.
ESTIMATE <=10-20K.

**Port machinery deleted outright** (MEASURED average ~58K/frame, plus ~40K in
tail frames): `ndsFTParamsInvalidateSubtree` 20.3K, `ndsRelocGetFileData` 10.0K,
`ftGetStruct` 10.0K (provenance checks inside an accessor that the source defines as
a macro), readiness checks 9.2K, and acquisition/ledger lookups.

### A6. Rates and scheduling

A system runs at 60 Hz only if a gameplay consumer reads its output between two
presented frames.

| System | Rate | Why |
|---|---|---|
| Status procs, physics, AI, hit/catch, items, weapons, hazards, RNG | 60 Hz | gameplay truth |
| Pose clock, TransN/rotation joints, hurtbox locals | 60 Hz (body hold: A3) | read by physics and A5 |
| Camera logic and `gGMCameraMatrix` | 60 Hz | gameplay readers: Link's boomerang off-screen return, the Star item, screen KO placement |
| GX view/projection | 30 Hz | render only |
| Visual joints, part matrices, material animation | 30 Hz | already the case for body joints |
| Particles, decorative stage animation | 30 Hz, 15 Hz where authored motion is slow | compromise items 1, 24 |
| Battle HUD | on change | compromise item 19 |
| Lighting | on change | compromise item 23 |

**Existing camera divergence.** `gGMCameraMatrix` is written only at draw
(`src/import/battleship_gmcamera.c:1092-1098`), once per presented frame, while
Link's boomerang projects through it every ninth tick
(`src/import/battleship_link_weapons.c:43-90` compiles in
`wp/wplink/wplinkboomerang.c`). The port comment "There is no simulation reader"
(`battleship_gmcamera.c:1085-1091`) is out of date. Fix: produce the matrix per tick
while a reader can fire. This must land before the replay digest is used as a gate.

**No run-ahead (owner ruling D4, 2026-09-22).** Starting the next frame's first
tick in the previous frame's idle time would absorb isolated spikes but adds up to
one refresh of input latency; the owner refused it. Every frame must fit its own
two VBlanks, so A1-A5 carry the whole requirement.

### A7. Memory, overlays and TCM

**RAM ledger (ESTIMATE; the shipping heap is unmeasured).** Every P2-2p8 figure so
far comes from the four-CPU lab ELF. The shipping `smash64ds.elf` carries
**+430,928 B** more static image, and ~491 KB of it is front-end code and data
resident during battle (CSS 143K, 1P 134K, menus 101K, opening 48K, diagnostics
38K, Results 27K).

| Supply | Bytes |
|---|---:|
| Front-end overlay region reused by battle (shipping) | ~453K |
| General-heap margin above the 25,600 floor (lab; shipping unknown) | 86K |
| A1 retirement: renderer BSS + executor `.text` + texture scratch, net of new lists | ~260-360K |
| FGM cache 237,568 repurposed to SFX heads (~100-130K needed) | ~100K |
| **Total** | **~0.9-1.0M** |

| Demand | Bytes |
|---|---:|
| All gameplay motions in MF, worst four kinds (at <=0.45x of 1.58 MB) | <=~710K |
| SFX heads | 100-130K |
| Compiled lists not placed in the framebuffer region | 0-30K |
| **Total** | **~0.46-0.76M** |

With MF at 0.45x the worst four kinds need ~710K and the demand total is
~0.81-0.87M against ~0.9-1.0M of supply: it fits, with little margin, only if
every supply row is realised and the shipping heap matches the lab estimate. Both
are Phase 0 measurements; generic LZ (0.78) does not fit.

- **Scene overlays** (calico `ovl.h`; none are used today). Front-end TUs move to
  `ovl_frontend`. The overlay area sits after BSS and shrinks the heap by its size,
  so battle must **reuse that range as its scene region** or nothing is gained. An
  ELF-relocation check for battle -> overlay references is part of the change;
  reload is ~0.2 s per transition.
- **Pools, not heaps**, for effects, particles, hidden parts and motion banks, with
  fixed capacities from the roster census.
- **ITCM** (32 KB, full): as A1/A3/A5 delete generic code, reassign it to the new
  per-frame kernels (joint compose, list patcher, hurtbox pass, collision queries,
  particle update; ~10 KB, ARM mode), then to the hottest remaining source procs
  by profile.
- **DTCM**: 8,582 B of the 10,828 B DTCM image hold `sNdsNativeFighterDenseNormals`
  and `sNdsNativeFighterPreparedDense` (`0x02ff0000`), reported as Mario's
  canonical tables, while the stress roster never draws Mario (verify no other
  consumer). Make DTCM per-match: the current roster's hot tables and the
  per-tick hurtbox store (~2 KB). DTCM is never a DMA source.
- **Layout**: SoA hot records, 8/16-bit indices, no per-tick pointer chasing.
  Moving data without shrinking the working set only re-phases the cache.

### A8. ARM7 audio

The ARM7 already runs DLDI and the sound engine under calico's `ds7_maine`, with
~17 KB of WRAM free (a custom ARM7 without wireless frees more).

- **BGM stream on the ARM7** from the extent map: 0 ARM9 ticks, which removes the
  ~123K spike every ~13 frames (~9.5K average).
- **FGM voices, envelopes, release ramps and cue fill on the ARM7.** The ARM9
  posts one PXI word per play (cue, pan, handle). Resident 512 B heads cover an ARM7
  read, so a miss costs <=1 frame of audio latency instead of a synchronous ARM9
  read. This removes `ndsAudioFgmPlayAtPan`/`Update` (4-8K/frame, including a u64
  divide per handle) and the ARM9's 5.75 ms envelope timer.
- There is no sequencer to move: BGM is pre-rendered ADPCM.
- Constraints: the IPC FIFO is 16 words each way (single-word events; bulk data by
  cache-line-owned descriptors); flush before ARM7 reads, invalidate before ARM9
  reads, never share a line between writers, never a TCM payload.

### A9. An instrument that does not move the gate

- Measured frames carry only the ring sample. The on-screen tick-HUD text renders
  while paused or after the window. RingDump already reads everything over GDB.
- Sample into the ring per frame: GE busy (GXSTAT bit 27) and FIFO level at owner
  boundaries, DMA0 wait, `gNdsMisc*DrawTicks`, polygon/vertex RAM at swap, storage
  reads (must be 0 after GO for Tier 0 content), pool high-waters.
- Report WORK-H P50/P95/P99/max and the two-VBlank share. Parameterise the stress
  harness's pinned-roster residency asserts
  (`scripts/verify-p2-four-fighter-stress.ps1:575-585`) so other rosters can be
  measured at all.

### A10. Visual reserve

Used only when a phase gate shows a residual: fighter very-low detail when four
fighters are zoomed out; decorative particles and stage animation at 15 Hz; simpler
impact effects; Dream Land's backdrop as a BG layer. Polygon RAM does not require
any of them (A1).

## 4. What stays source

| Stays decomp C (60 Hz authority) | Becomes native |
|---|---|
| status procs, interrupts, physics equations, damage/knockback/hitlag formulas | render lists, frame builder, matrices for GX |
| CPU AI decisions and RNG order | AI perception memos (exact) |
| motion-event interpreter and its float clock | pre-resolved operands, pooled callees |
| `ftMainSetStatus` including the first play | motion acquisition (pre-bound pointers) |
| hit-resolution order and results | hurtbox preparation and the guarded narrow phase |
| map-collision decisions | per-stage tables and query plumbing |
| camera logic | GX camera matrices |

## 5. Verification

- **Performance, every phase.** `scripts/verify-p2-four-fighter-stress.ps1` alone
  on the stress target, whole match, items on, with the phase's engagement counters
  in the run. A phase banks only what that run measures.
- **Coverage before any universal claim.** A worst-case search: every stage with
  its heaviest roster, every fighter on its heaviest stage, Kirby with copies,
  item-dense moments. The release matrix in
  `native-optimization/16_ALL_ROSTERS_ALL_STAGES.md` stays the closure contract.
  The shipping ROM's heap is measured, not inferred from the lab ELF.
- **Gameplay equivalence (A3-A5).**
  - *Replay digest:* a deterministic four-CPU match with a per-tick digest of
    positions and velocities (quantised to 1/16), status and motion ids, motion
    frame, damage, stocks, hitlag, shield, hit events (attacker, victim, hitbox),
    RNG, and item/weapon state. It must match control for the whole match, and is
    mutation-tested.
  - *Shadow oracles:* each kernel runs beside the source routine in a lab arm and
    counts divergences.
  - *Tolerance (D3):* exact for every discrete outcome. Continuous values fed back
    into simulation within 1/256 unit. Render-only values follow the visual
    doctrine. *Exception (D13, 2026-10-04):* a deliberate float -> fixed
    conversion of simulation math is held to mechanical equivalence instead and
    re-baselines the digest (section 8).
- **Visual.** Synchronised A/B captures and crops of changed geometry, with the
  owner as oracle.
- **Memory.** Heap low-water, arena and pool high-waters per roster, at GO and at
  the worst tick.

## 6. Delivery sequence

Each phase is one coherent Codex batch: one integrator, measured once at the end.
A phase that misses its estimate records the shortfall in the phase log and the
work continues into the next phase, banking every measured win (owner, D9) --
it no longer stops for re-planning.
Estimates are cumulative WORK-H P50 / P99 on the measured roster.

| Phase | Content | Exit gate | P50 / P99 |
|---|---|---|---:|
| 0 Truth | A9 instrument and counters; MISC and SRC-other splits; shipping-config heap and arena census; MF host experiment (ratio per kind, decode cost); replay digest + shadow scaffolding; camera-matrix fix; harness roster parameterisation | digest mutation-tested; unsized items sized; MF budget met or STOP | 1.57M / 2.85M |
| 1 Fighters (A1) | first slice Samus LOW default program (word-compare against a recorded packet), then DK (cross slots), Link (texgen, programs), Kirby (hats), then all admitted kinds; retire packets/production/replay | FTR <= 90K P99; native failures 0; stop if FTR falls but WORK-H does not | 1.30M / 2.25M |
| 2 Stage + MISC (A1) | stage compiler per run class; native draw list replacing display-proc traversal; effect lists; particle batcher; resident DamageSlash | STG <= 40K; MISC per Phase 0 sizing; every stage A/B | 0.85M / 1.75M |
| 3 Residency + audio (A2, A7, A8) | MF encoder + checker + runtime bind; full-roster match bank + admission rule; custom ARM7 (extent map, BGM stream, FGM voices with heads); front-end overlay | 0 motion reads after GO, every roster; pose oracle bit-identical; AUD P99 <= 10K | 0.82M / 1.45M |
| 4 Events + pose (A3, A4) | tagged `FTMotionDesc`; pre-created hidden parts; pooled effects; pre-resolved events; native clips if the pose cost is confirmed | digest identical; SPRM P99 <= 40K | 0.77M / 1.20M |
| 5 Combat + collision (A5) | hurtbox kernel + guarded narrow phase; stage collision tables; AI memos; delete port machinery | 0 flips; SHDT P99 <= 100K | 0.73M / 1.0M |
| 6 Layout (A7) | ITCM/DTCM reassignment to the new kernels; per-match DTCM tables | two-VBlank share >= 95% gate with margin | — |
| 7 Coverage | worst-case search across stages, rosters and items; A10 only for a measured residual | release matrix | — |

Phases 1 and 2 share renderer files and run in sequence. Phase 0's census and
Phase 3's residency work touch disjoint files and may run alongside Phase 1 within
the three-subagent cap. Phase 5's kernel reads the Q locals Phase 4 produces.

### Phase log

- **Phase 0 (2026-09-23), instrument landed** (`3d62c6abf26`, `552f795752f`;
  `artifacts/performance/2026-09-23_p2-2p8-phase0-baseline/`). New baseline on the
  current tree, instrument out of the gate: WORK-H P50 **1,689,088**, P95
  **4,207,488**, P99 **4,753,152**; two-VBlank **5.7%**; 14.4 FPS. Two findings
  reshape Phase 1: (1) a **re-record episode** owns P95 (frames 798-1043,
  ~1.16M/frame of texture re-resolution and packet production; without it P95 is
  2.64M) -- its mechanism, first blamed on tint, is a texture-upload fence storm
  (Phase 1 slice 1 entry below); (2) **native failures at match start** (Link
  AppearL, texture could not be bound). Replay digest
  proven (deterministic across builds; an item-rate poke diverges). Owed:
  shipping-config heap census (before Phase 3), MF experiment result.
- **Phase 0 (2026-09-23), MF verdict: yes** (`artifacts/performance/2026-09-23_p2-2p8-mf-experiment/`).
  Candidate B (structural re-encoding + static Huffman, one global table set)
  is lossless on 1,570 / 1,570 clips and reaches **0.382x** on the worst four-kind
  roster of all 495 (Yoshi + Captain + Pikachu + Ness: 579,904 B; 599,116 B with
  tables and the non-AObj16 clips, ~111 KB under 710,000 B). LZ-class formats
  reach only 0.53-0.73x. Risk carried into Phase 3: bind cost ~21-26K ticks per
  2.2 KB clip (ESTIMATE; ~3x the A2 planning range, P95 clip 44-50K); the
  mitigation is to spend the headroom keeping each kind's most-bound clips raw.
- **Phase specs (2026-09-23)**: `artifacts/performance/2026-09-23_p2-2p8-phase-specs/`
  (Phase 1 fighters, Phase 2 stage + MISC). Phase 1 started: slice 1 = Samus LOW
  program 0 on an adopted packet, exact oracle, texture admission before GO.
  Decided without an owner question: menus, CSS, Results and the 1P intro go lean
  inside Phase 1 before its deletion step, so no dual path survives the phase.
- **Phase 0 closed (2026-09-23): shipping heap census**
  (`artifacts/performance/2026-09-23_p2-2p8-shipping-heap-census/`). The shipping
  shell with the heaviest reachable roster (Captain human; Link, Pikachu, Kirby
  CPUs) **halts at battle load**: arena 1,138,432 B, 178,784 B free at the first
  fighter, `ndsSyMallocOverflowHalt` on Kirby's 35,272 B battle pack with 3,676 B
  left -- about 130 KB short of a running match with the 25,600 B floor. Stage,
  common and item files hold ~546 KB before any fighter (Dream Land's whole ground
  file alone 202,816 B). The shipping image is 190,728 B larger than the four-CPU
  gate ROM's, so the gate ROM measures a machine with more arena than ships.
  Consequence: Phase 3's RAM work (A7 overlays, A8 FGM cache, A2 admission) is a
  correctness prerequisite for "any four fighters", and every phase report adds
  the shipping arena and this roster's load margin.
- **Phase 1 slice 1 (2026-09-23, `c33274f8345`,
  `artifacts/performance/2026-09-23_p2-2p8-phase1-slice1/`)**: lean path for Samus
  LOW program 0 on an adopted packet; ARM joint kernel bit-exact with the source
  compose (35,772 joints); route 2 oracle 0 mismatches over 1,217 replay hits;
  digests identical across routes. Route 1 vs 0: FTR P95 -11.0%, WORK-H P95
  -6.6%, P50 flat (one fighter of four). Three findings reorder the phase:
  (1) the P95 episode is a **texture-VRAM fence storm** -- 408 fighter texture
  uploads after GO, each moving the global fence so every fence-dependent packet
  re-records (tint re-records: 0); (2) Link's AppearL failure is **texture VRAM
  exhaustion** at the entry burst (frame 156, nothing evictable); (3) the live
  record path never applies the colanim flash to shade words while replay does
  (the lean path follows replay). A naive pin-on-record cut FTR P95 -39% but
  starved later frames (+51%) and raised native failures, so texture admission
  needs a **VRAM budget**: slice 2a measures VRAM and proposes a lossless battle
  VRAM plan.
- **Phase 1 slice 2a (2026-09-23, `f1476de32dd`)**: VRAM census. Texture VRAM
  A+B is full all match (247-262 KB of 256 KB); fighters hold only 40.5 KB at GO
  (the rest: battle static 66 KB, IFCommon clouds/traffic 57 KB, particle atlas
  42 KB, entry textures kept after GO 30 KB); Link's entry failure and the P95
  episode are **fragmentation** (largest free run 2.2-2.6 KB); BG3 (bank D,
  128 KB) is **empty in a VS battle**; the wallpapers are 16-bit sources (BG2
  stays). The reachable fighter texture set, enumerated on the host without
  drawing, contains every runtime-recorded key. Plan (approved): bank D as
  texture slot 3 in battles with an empty BG3 (393,216 B), scene + admitted
  fighters pinned in A+B, D the only region that allocates during the match;
  slice 2b implements it with host-generated admission lists.
- **Phase 1 slice 2b (2026-09-23, `bd29b08e282`,
  `artifacts/performance/2026-09-23_p2-2p8-phase1-slice2b/`)**: the plan behind
  runtime word `gNdsFtrLeanAdmit` = 2 (default 0). Four-CPU stress, word 2 vs 0,
  same ROM: WORK-H P50 1,690,816 -> 1,648,576, **P95 4,207,754 -> 2,527,139**,
  P99 4,742,153 -> 3,162,301; FTR P95 2,616,477 -> 944,074; frames of 5+
  VBlanks 23.6% -> 10.6% (two-VBlank share flat at 5.6%: the median is still
  1.65M); Link's entry failures 254 -> 0; digest identical. Not yet whole: the
  admission stops at the libc floor (texture cache slots carry 236 B keys and
  libnds mallocs a record per texture), so 18 fighter uploads remain after GO;
  319 Kirby JumpAerialF1 native-program rejects are pre-existing. Slice 2c
  makes the admission fit with no new RAM and proves the exit and
  creation-time paths.
- **Phase 1 slice 2c (2026-09-23, `b20b8f0f2b2`)**: the whole admission fits
  with no new RAM -- the texture cache stores a compact identity instead of a
  236 B key (124 -> 254 slots in less storage; 0 collisions), admitted textures
  are carved without libnds records; shipping static RAM -9,068 B. Word 2 vs 0:
  fighter uploads after GO 458 -> 0, native failures 293 -> 39 (the rest is one
  pre-existing effect class; 280 came from a battle-pack bug fixed here), WORK-H
  P50 / P95 / P99 1,634,304 / **2,449,779** / 2,996,454, FTR P95 907,763;
  battle exit and creation-time admission proven; 1P open. Found: the stress
  target's Sudden Death never ends (pre-existing; separate task). Next, slice 3:
  lean for all four stress kinds and a cheap lean draw (FTR P99 <= 90K is the
  phase gate; today P50 358K).
- **Phase 1 slice 3 (2026-09-23, `08736558f3b`)**: lean for all four stress
  kinds on adopted packets (96.4-99.0% of draws; route 2 oracle 0 mismatches over
  15.6M words); slow kernel joints 34-51% -> 0-0.4%; per-draw cost DK 57.5 ->
  41.4K, Samus 50.0 -> 34.7K, Link 68.4 -> 50.1K, Kirby 45.9 -> 29.6K. Admit 2,
  route 1 vs 0: FTR P50 / P95 / P99 358,080 / 915,638 / 1,170,867 -> **229,056 /
  435,251 / 1,025,600**; WORK-H 1,642,592 / 2,474,362 / 3,034,156 -> **1,512,928 /
  2,284,672 / 2,896,626**, following FTR one for one. The kernel still runs
  855-954 ticks/joint (cache misses; the spec budgets ~450). **Found: the first
  1P battle runs out of general heap while loading** (pre-existing, ~15 KB short;
  the slice's code adds 8 KB) -- memory work starts now in parallel (battle HUD
  file bake, A7), and the phase's deletion step returns production's RAM. Next,
  slice 4: host-generated fighter lists (LOAD4x3 + P'), the path to deletion.
- **Phase 3 prep (A7), 2026-09-23 (`002e5999244`, `b0a0b1a851f`,
  `artifacts/performance/2026-09-23_p2-2p8-if-gamestatus-compact/`)**: the battle
  HUD file IFCommonGameStatus (152,288 B) is kept as a 21,056 B image after load
  -- the letters' pixels are baked (GO into OBJ VRAM, TIME UP / GAME SET into
  22,104 B of run-length streams) and dropped. Four-CPU lab: general-heap
  low-water 54,020 -> 122,412 B, OBJ VRAM byte-identical, replay digest
  identical. Shipping shell 1P: the first battle now loads and completes; the next
  blocker is stage 2's intro loading full fighter files. Flag default 0 until
  slice 4 lands, then on.
- **Phase 1 slice 4 (2026-09-23, `3fd3262357c`,
  `artifacts/performance/2026-09-23_p2-2p8-phase1-slice4/`)**: the lean path no
  longer adopts recorded packets -- it materializes each list on the device from
  the generated owner tables in the LOAD4x3 + P' layout, keyed by content (lists
  survive model-part swaps and rebuilt MObjs), with fold-free tint repatch and
  small-diff variant records. Route 1: 0 recordings, 0 declines, 100% lean share
  for the four kinds; route 2 oracle 0 mismatches over 13.6M words; digest
  identical; native failures 39 = 39. Admit 2, route 1 vs slice 3: FTR P50 / P95 /
  P99 229,056 / 435,251 / 1,025,600 -> **227,488 / 318,557 / 812,284**; WORK-H
  1,512,928 / 2,284,672 / 2,896,626 -> **1,507,808 / 2,138,586 / 2,763,228**.
  Host proof: every recorded packet reproduced word-exact after masking; the
  matrix layout moves clip row 3 by at most 1 LSB (0.006 px). Cost: shipping static
  +11,312 B (lists are built at the first draw from owner tables that stay
  resident; deletion returns production's RAM), gate-ROM heap low-water 54,020 ->
  37,636 B (4 arena pages). Open: DK's shape-changing hand swaps still materialize
  (26 x 537K ticks); high-detail DK/Link lists exceed an entry (Capacity decline);
  Samus's key carries the Kirby trio head key. Found alongside: **the all-content
  VS character select had switched every 3D preview off** -- static growth took the
  heap under its animation reservation (fixed `050b7c18db4`,
  `artifacts/performance/2026-09-23_css-preview-heap/`); with slice 4 it keeps
  192,464 B against 183,072 required, 9,392 B of margin that every static byte
  spends. Next: flip `NDS_IF_GAMESTATUS_COMPACT` on after its 1P/menu-loop proofs,
  then the 1P intro packs and the phase's deletion step.
- **A7 compaction on by default (2026-09-23)**: GAME SET proven at runtime (the
  announcement forced to GAME SET on the four-CPU ROMs: OBJ end bank
  byte-identical between the pixel conversion and the stream decode), the shell
  loop passes one lap with it on (compaction ran: count 1, stage 0), the first
  1P battle loads. `NDS_IF_GAMESTATUS_COMPACT ?= 1`. Its 4,416 B of image costs
  the all-content character select one arena page: 188,368 B at the animation
  reservation, 5,296 B of margin -- the same margin the 2026-09-22 published ROM
  shipped with. Next: the 1P stage-2 intro packs, then the phase's deletion step.
- **Phase 1 slice 5 (2026-09-23,
  `artifacts/performance/2026-09-23_p2-2p8-phase1-slice5/`)**: lean draw cost. The
  camera LookAt runs once a frame instead of once per fighter (exact: input and
  output hashes, graphics-heap footprint kept; the decomp control arm, level 0,
  always calls); the patch reads a per-draw view; material events keep their
  plan when a held list matches (verify arm: 140 kept plans re-resolved, 0
  differences); the kernel and the per-draw list code (3,328 B) run from ITCM,
  funded by moving the old path's production execute (7.9 KB) to main RAM. Digest
  identical, oracle 0 mismatches, native failures 39 in routes 0-2. Route 1 (admit
  2, compaction on): FTR P50 / P95 / P99 228,992 / 321,715 / 812,019 -> **183,552 /
  268,333 / 766,360**; WORK-H 1,556,096 / 2,190,358 / 2,815,923 -> **1,404,128 /
  2,031,187 / 2,690,529** (WORK-H less STG fell 41,756 with FTR's 41,929; STG's
  own drop, seen in route 0 too, is layout and not claimed). Per draw DK / Samus /
  Link / Kirby 50.5 / 35.5 / 58.2 / 32.3K -> 42.0 / 28.6 / 48.5 / 24.9K; kernel
  704-822 ticks/joint (target ~450: ~250 is data stall on DObj lines). Shipping
  static -136 B. **Accepted cost**: route 0's record frames lost ITCM (+88K P95,
  +154K P99 on the stress match) -- the old path the phase deletes, but also what
  non-lean kinds draw with until coverage lands. Next: coverage (all kinds, both
  details) so lean can be the default, then deletion.
- **Phase 1 slice 6 (2026-09-24,
  `artifacts/performance/2026-09-23_p2-2p8-phase1-slice6/`)**: VS coverage. All
  twelve kinds draw lean in VS battles at both details (electric skeletons, Fox's
  blaster, Yoshi's programs, Pikachu/Purin accessories, Captain's HIGH alpha --
  exact, not an approximation: the test's reference is 0 and the DS never draws an
  alpha-0 texel; DK/Link HIGH lists span the slot's whole region). The one
  remaining decline is Ness's yo-yo smashes (no generated model-part program;
  93-97% lean for Ness). Per roster (route 1 vs 0, digest identical, oracle 0,
  native failures equal): default DK/Samus/Link/Kirby FTR P50 343K -> 185K, WORK-H
  P95 2.46M -> 2.01M; Mario/Fox/Luigi/Yoshi FTR P50 343K -> 182K, WORK-H P95 2.54M
  -> 2.19M; Captain/Pikachu/Purin/Ness 316K -> 170K, 3.04M -> 2.71M; two-fighter
  HIGH rosters FTR P50 81-109K. Old-path fix found by the oracle: the draw plan
  now keys on the electric skeleton (shared code, 1P included). Captures route 0
  vs 1 differ by 0-206 edge pixels a frame (the accepted LOAD4x3/P' rounding);
  Mario's skeleton frames are wrong on route 0 (texture cache ignores the program)
  and right on route 1. Shipping static -32 B. Next: lean as the default, then
  deletion.

- **Phase 1 slice 7 (2026-09-24,
  `artifacts/performance/2026-09-24_p2-2p8-phase1-slice7/`)**: lean route 1 /
  admission 2 are now the boot defaults. Ness's yo-yo and bat programs cover
  the remaining smashes (LOW 1,739/1,739 draws; HIGH 1,771/1,771, including
  51 yo-yo and six bat draws). The final default stress ROM has WORK-H P50 /
  P95 / P99 **1,393,664 / 2,021,027 / 2,665,534**, FTR **184,448 / 269,651 /
  767,704**, 19.21 FPS and 10.59% two-VBlank presents; the gate remains RED.
  This qualifies the combined default, not a new gain over slice 6. Main and
  both Ness rosters have zero oracle mismatches and strictly identical replay
  sequences (all 1,972 pairs, no skipped rows or resynchronization). Native
  failures remain 39 on main and 166 on Captain/Pikachu/Purin/Ness; HIGH
  Purin/Ness has zero. The owner-playtest regression was bank-D tint lifetime:
  release tint tiles in D at battle exit and delay its BG3 remap until the
  new frame or an actual BG3 write. Natural Results/CSS probes complete and
  captures retain Mario/Fox's parts. Final shipping static +104 B; CSS keeps
  188,368 B at its reservation. **IMPLEMENTED_NOT_ACCEPTED**: remaining D8
  scene coverage, retirement, integration and owner look are due. Next:
  camera-modelview and transient Intro support, then variant kinds and the
  remaining scene checks, before deleting production everywhere.

- **Phase 2 stage compiler (2026-09-24), IMPLEMENTED_NOT_ACCEPTED.** All 202
  Dream Land triangles and their material commands compile to native GX lists;
  adjacent visible runs share 27 DMAs/frame instead of 54. Source-depth dispatch
  in the old reference is repaired. Five host checks pass; all 1,972 replay pairs
  and geometry counts match the preceding compiled control. Final `49263A2E`
  WORK-H P50/P95/P99 1,391,936 / 2,026,259 / 2,656,071; STG 316,864 / 325,184 /
  327,315. Mean-ALL 19.20 FPS; two-VBlank 215/1,973 (10.90%). No overall win
  banked. Lab heap 88,148 B, arena 1,249,024 B; shipping heavy-roster/CSS checks
  remain due. Next: physical replay retirement, remaining preparation, other VS
  stages and MISC. Receipt: `artifacts/performance/2026-09-24_p2-2p8-phase2-stage/`.

- **Phase 2 Task36 retirement (2026-09-24), IMPLEMENTED_NOT_ACCEPTED.** Removed
  the recorder, replay consumer, 24,256 B owner buffer, routing flag and copied
  certificates. Native-only guard forbids their return. `0A14A9F9`: static
  -35,652 B; lab heap +32,768 B to 120,916. WORK-H P50/P95/P99 1,378,016 /
  1,990,128 / 2,612,394; 19.41 FPS, two-VBlank 242/1,973. All 1,972 replay and
  geometry-count pairs match; both captures are pixel-identical. Native failures
  remain 39. Shipping `0879E550` CSS reserve is 221,136 B, margin 38,064 B.
  The all-content heavy-roster diagnostic `BB81E3B0` still halts loading Link
  dependency 224: arena 916,992 B, free 4,222 B, zero battle frames. This replaces
  the older restricted-shell memory estimate. Next: remaining preparation,
  VS-stage/MISC compilation and cache retirement. Same phase-2 receipt.

- **Phase 2 shared stage camera (2026-09-24), IMPLEMENTED_NOT_ACCEPTED.** A
  current diagnostic attributed 135,635 ticks/frame to matrix preparation.
  Stage bindings now share frame camera/billboard operands, retaining live
  world transforms and multiplication order. Removed the obsolete dynamic
  binding list. `8E6E7D8D`: WORK-H P50/P95/P99 1,337,280 / 1,962,762 /
  2,587,860; STG 267,520 / 276,224 / 278,419. Mean-ALL 19.81 FPS; two-VBlank
  317/1,973. WORK-H median/P95 improve 40,736/27,366 ticks against retirement.
  Both 42-matrix comparisons, all 1,972 replay/count pairs and both captures
  match. Static -312 B; lab heap/arena unchanged. Shipping `B99B32FB` static
  -320 B and CSS margin 38,064 B; all three preview captures match. Prior heavy
  capacity failure remains open. Next: remaining transform/preparation machinery,
  other VS stages and MISC; no further profiling of the settled camera change.

- **Compact packet layout reverted (2026-09-26), BANKED.** Placing the stage GX
  body in a framebuffer tail (`c116fffa03e`) cut fighter regions to 6,528 words;
  lean entries (2,304 list words) could no longer hold DK/Samus/Link low-detail
  lists, which went wide (double walk, no variants): FTR P95 1.20M. Regions are
  8,840 again; the body is heap with the measured in-match growth margin.
  `BC3500EA` WORK-H P50/P95/P99 1,332,160 / 1,955,392 / 2,786,944, 19.72 FPS,
  replay identical. Lesson: a fighter-list budget is per entry, not per region.
  Receipt: `artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/`.

- **A8 storage and FGM (2026-09-26/27), BANKED.** ROM reads bounce only their
  unaligned head/tail lines; FGM misses fill asynchronously through the ARM7
  (storage op 5, reply in the request line, voice starts next update); the FGM
  cache is a 160 KiB ring arena instead of 232 KiB of fixed slots (arena +73,728
  B). `18A992BB` WORK-H P50/P95/P99 1,328,512 / 1,839,488 / 2,263,936, replay
  identical. Receipts `2026-09-26_p2-2p8-{storage-bounce,fgm-cache}`.

- **A9 fast cpuGetTiming (2026-09-27), BANKED.** The tick-HUD's ~500 clock reads
  per frame cost ~32K inside WORK-H. An ITCM `--wrap` reads the same Calico
  clock without IME masking: `E6DB1E1A` WORK-H P50/P95/P99 1,314,880 /
  1,822,144 / 2,232,704, replay identical. Receipt `2026-09-27_p2-2p8-fast-timing`.

- **A5 step 1: conservative hurtbox reject (2026-09-27), BANKED.** Not the
  guarded narrow phase: a fixed-point world chain from the source's cached
  locals feeds only a margin-guarded world-AABB separation test; anything not
  provably a miss takes the decomp float test unchanged, and no latch is
  written. Shadow oracle 0 flips over 14,153 tests (13,499 rejected); replay
  identical. `56AF4CCB` WORK-H P50/P95/P99 1,257,472 / 1,725,376 / 2,127,744,
  SHDT P95 216,640 -> 154,624. Receipt `2026-09-27_p2-2p8-a5-hurtbox-reject`.

- **Fast memcpy/memset, FGM id map, stage matrix leaves (2026-09-27), BANKED.**
  newlib's Thumb memcpy/memset (~49K/frame) become ARM LDM/STM routines in ITCM
  (boot self-test 0 failures); FGM lookup is an id map; the 4x4 fixed multiply is
  unrolled; the shifted-raw matrix range check is 32-bit. `F6CE3DAB` WORK-H
  P50/P95/P99 1,215,616 / 1,657,152 / 2,056,192, two-VBlank 556/1,973, replay
  identical. Priced and refuted: a 200 KB anim cache (motion reads 362 -> 275,
  P95 -2.7K: reads are not a tail lever). Receipt `2026-09-27_p2-2p8-fast-mem`.

- **09-28/29 solo, BANKED** (all replay identical): elastic motion cache, compact
  ground maps, R1 retirement, hit fetch (09-28, gate 934,272 / 1,284,864); spline
  bisection reuse (Sector Z P95 -88K); stage GX spans overlap the DMA; pose clock
  whole-frame steps; integer sin/cos index for the hurtbox kernel and
  `lbCommonSin/Cos`; A5 kernel 1/s table, unit-scale and zero-angle local builds,
  and a local-frame reject that cuts the float tests 659 -> 286 (0 flips). Gate
  `22c41b9718e` WORK-H P50/P95 928,000 / 1,263,808, two-VBlank 84.3%.
  A fresh whole-match profile (`2026-09-29_p2-2p8-hurtbox-box`) puts the P90-98
  premium over the median at 316K: A5 kernel ~37K before these cuts, soft float
  ~29K (the source's float collision chain), pose attach ~32K, lean
  materialisation ~9K (39 a match, 145K-683K each), shield pose ~5K. Tried and
  reverted: a per-box hurtbox record, a stage DISP3DCNT shadow (the stall moved
  to the next bus access: the stage draw is GX-DMA-bound).
- **09-29 Sector Z entry owner, BANKED** (replay identical): Sector Z's Arwing
  is drawn by the entry-effect owner (FoxSpecial3's lists, ~94K ticks a frame
  while it flies). Its static fence is proven once per root (`8bc4684098a`,
  lab P95 1,711,488 -> 1,698,944) and its lit groups are lit by the geometry
  engine, one five-bit step from the CPU shade (`a7f6aefa490`, 1,703,488 ->
  1,681,664; gate flat). A fresh profile put the rest at ~247 cycles a corner
  and ~2,500 a group: fast corners from the const tables (`f51600051ec`,
  1,683,584 -> 1,657,984) and FoxSpecial3's resolved group state kept across
  draws while the inherited state holds (`28aaf4f9df8`, 1,653,248 ->
  1,617,792). Sector Z lab P95 1,711,488 -> ~1,618K over the four; all exact
  except the engine light. Then the composed CPU matrix only for roots a
  CPU-projected corner reads (`f0434c38944`, -2.5K) and entry models admitted
  ahead of the general stage submit (`3d87fe00a10`, -2K, inside spread).
  Receipt `2026-09-29_p2-2p8-sector-z-arwing`.
- **09-29 priced and refuted** (same-ROM A/Bs, all reverted): three lean
  spares (materializations 39 -> 33, P95 flat, heap -124 KB), a per-box
  hurtbox memo (40% hits, P95 flat: the first test per joint is the cost), a
  lean-kernel skip for unchanged fighters (5% of draws), and a Q12 kernel
  compose (FTR +6.6K: slower than the exact form). Attribution kept: a lean
  materialization is ~386K ticks (-35K gate P95 if removed, diffuse), Dream
  Land's stage is prepare 80K + commits 105K for 6,914 GX words, Sector Z and
  Saffron have no rigid stage set. Receipt `2026-09-29_p2-2p8-refuted-levers`.
- **09-29 stage GX words kept, BANKED** (replay identical): the compiled stage
  program rewrote its MATERIAL words (three a run, ~130 ticks each, 53 runs a
  frame on Dream Land) and its NOZ/PROJECTION patches every frame. Each run now
  records what its words hold (texture-key generation, projection serial
  checked once per stage-matrix generation) and skips the patch while that
  holds (`f6e8cda2a75`: STG median -4.7K, P50 -5.0K, P95 -4.4K; lab verify
  mode: 0 of 235K-2.3M kept words differ on all nine stages). Billboard
  bindings compose only the translation row `ApplyMvpRecalc` keeps
  (`f7793ae9996`: STG -2.7K, P50 -1.7K to -2.8K). Gate WORK-H P50/P95
  ~922,800 / ~1,256,600. Receipt `2026-09-29_p2-2p8-stage-gx-memo`.
- **09-29 object draws priced; the stage-DL fast lane, BANKED** (replay
  identical): items, weapons and effect models drew through the generic
  stage-DL submit at 22K-78K ticks a list whatever their size (a Charge Shot
  42K, a Beam Sword root 29K; 36K a frame on Dream Land, 72K Yoshi's Island,
  88K Saffron, 117K Sector Z in the lab census). The body now records the
  route when an owner draws a list; later draws of that list go straight to
  the owner with the same inputs (Charge Shot, the thirteen MObj-less item
  owners, the procedural visual templates), and the generic-cache quads bind
  through the owner texture memo (verify mode 0 differences). Gate WORK-H
  P50/P95 ~924K/1,256.5K -> ~918.5K/~1,249K; Yoshi's Island P95 -31K,
  Saffron -24K (`cde0ccea3f2`, `e5c64860e14`, `008b5429544`). Receipt
  `2026-09-29_p2-2p8-object-fast-lane`.
- **09-29 status-change costs priced; SMULL for the Q12 cubic, BANKED** (replay
  identical): a status change is ~111K ticks on the gate profile (pose first
  play 36K, lean events 23K, motion fetch 17K plus the ARM7 read wait, install
  10K). The pose evaluator's cubic compiled to 64 x 64 multiplies (GCC proved
  its truncations no-ops and kept wide values); an asm barrier makes every
  product one SMULL (190 -> 119 instructions, host-equal over 300M inputs).
  Also: parser reciprocals from a table, the flattened walk kept across
  status changes that touch no hidden part (511 of 702), ShieldPose base rows
  decoded once per blob, the hurtbox local build's trig products 32-bit, the
  stage span state written before its DMA. Refuted: an LRU refresh of the
  motion ring (the gate's ring wraps once a match; 194 of its 220 reads are a
  clip's first use). Gate WORK-H P50/P95 ~918.5K/~1,249K -> ~910.7K/~1,232K;
  Saffron lab P95 -8.4K (`45d6b475fe4`, `8597d6be8a8`). Receipt
  `2026-09-29_p2-2p8-sim-codegen`.
- **09-29 hurtbox walk and re-test box, AObj ledger hash, BANKED** (replay
  identical, shadow 0 flips): in the heaviest non-materialization frames the
  hurtbox kernel was the largest compute delta (+53K over a median frame). Its
  walk now checks the slot before FTParts and composes in place (P95 -2.5K
  same-ROM), and a re-test in the epoch reuses the coll's separation pieces
  (5,324 a match, -1.9K). The AObj ledger's xor-fold hash clustered a script's
  adjacent words (23.6 probes a lookup); a multiplicative hash takes 1.2
  (-3.3K). Cross-build the shipped build reads ~911.5K/~1,235K: layout drift
  of +3K against the per-change same-ROM pairs. Refuted: a TRS-keyed trig
  memo (no gain), more spare lean lists (39 -> 33 materializations, Donkey's
  states are not an LRU problem). A materialization log puts Donkey at 13
  in-match lists a match (~460K each), Link at 5 (~700K); removing Donkey's
  would be P95 -14K (`964229de7f9`, `7df0e823bfb`).
- **09-30 map-collision group rejects, BANKED** (replay identical per stage):
  the lab stages' premium over Dream Land is mostly flat sim time (SRC medians
  483-562K vs 408K); on Peach's Castle the wall sweep alone made 1,850
  soft-float calls a frame. The wall, floor and ceiling sweeps now skip a
  line group whose lines' span misses a truncated superset of the sweep's
  range (exact; backoff for groups that never reject), compute a moving
  group's offsets once per group, and store the extent reject's bounds at
  fill. Same-ROM lab P95: Peach's Castle 1,401K -> 1,340K (two-VBlank
  59.7% -> 70.7%), Saffron -11.8K, Yoshi's Island -9.0K, Mushroom -8.6K,
  Dream Land -5.4K, Sector Z -2.4K; gate ROM neutral (`444044b9052`). Found:
  the simulation's digest depends on the build's heap layout (two layouts
  differ on Dream Land and Saffron with every new word off; kernel modes
  agree within each) -- open. Receipt `2026-09-30_p2-2p8-map-collision`.
- **09-30 build-dependent digest: a stale loaded-file pointer, FIXED** (every
  layout now agrees on all nine stages; gate digest unchanged): a fighter
  clip's full load registered its record, stored the raw template in the
  animation cache, then finalized the record -- but the store can wrap the
  cache ring over another fighter's pinned zero-copy clip, whose rescue
  compacts the loaded-file table, so finalize fixed up a different file and
  the clip kept its relocation chain (Samus's back roll played no animation
  and ended at once). The arena's size, set by the build's heap, decided
  whether it fired; `-ftrivial-auto-var-init` "fixed" it only by moving the
  heap. The path now re-finds its record after the store, as does
  registration after it allocates. Receipt `2026-09-30_p2-2p8-layout-digest`.
- **09-30 object-animation sentinel tests as bit compares, BANKED** (replay
  identical): ~580 `__aeabi_fcmpeq` calls a frame were the generic animator's
  `anim_wait` tests against the AOBJ_ANIM_* sentinels and its payload-vs-0.0F
  tests; the objanim import patch compares bits (exact for every input).
  Same-ROM gate P50 -4.7K, P95 -4.2K. A fresh gate profile puts the tail
  (P93-97 frames, +321K over the median) on the status-change pose cluster
  (~40K), the hurtbox kernel (~31K) and soft float (~27K). Receipt
  `2026-09-30_p2-2p8-anim-bitcmp`.
- **09-30 floor/ceiling sweeps reject a segment by its x span, BANKED**
  (replay identical): Sector Z is soft-float bound (fadd 91K a frame) and its
  floor sweep ran the segment kernel ~810 times a frame; the kernel's
  non-flat branch rejects a segment whose x span misses the sweep's, and the
  loops now make that same bit-compare test before the call. Same-ROM Sector
  Z P50 -22.3K, P95 1,621K -> 1,584K (-37.5K), two-VBlank 41.5% -> 44.5%;
  Castle and Saffron P95 -2.9K; Dream Land's floors are flat (gate neutral).
  Refuted: a per-binding stage near-plane test (unions straddle the plane).
  Receipt `2026-09-30_p2-2p8-segment-xreject`.
- **09-30 fighter clip misses read whole sectors into the ring, BANKED**
  (replay identical): a status change costs ~66-69K on the lab and 36-44% of
  changes read a clip at 43-51K, 27-37K of it the ARM9 blocked on the ARM7.
  A miss now reads the clip's whole sectors in one storage request straight
  into a ring slot and pins it like a hit (no heap copy, no second request).
  Same-ROM per read 43-51K -> ~31K on four stages; lab P95 -3K..-18K; gate
  P50 -2.6K, P95 -1.4K. A clip trace sizes the next step: a ring that steps
  over hit clips plus successor prefetch halves the reads in simulation.
  Receipt `2026-09-30_p2-2p8-direct-clip-read`.
- **09-30 fighter clip prefetch in the idle time + a stepping ring, BANKED**
  (replay identical on nine stages, three rosters, gate): a table from 27 lab
  clip traces names each clip's two likeliest successors; they are read by
  the ARM7 asynchronously in the frame's idle time before its presentation
  VBlank, and the ring steps over clips fetched since its last pass. Lab P95
  -1.5K..-24.8K on all 13 stage/roster pairs (mean -7.5K), blocking reads
  -41..-54%; gate P95 -5.2K/-6.2K. Refuted: issuing at the install (P95
  +5..16K, ARM7 queueing), a two-queue ring. Dream Land P99 +53K is the
  event-32 ledger ForgetRange (~150K) landing on a heavier frame: owed.
  Receipt `2026-09-30_p2-2p8-clip-prefetch`.
- **09-30 the event-32 ledger forgets a range by leaving holes, BANKED**
  (replay identical): ForgetRange's ~4 real removals a match each compacted
  the ledger and rebuilt the 8,192-slot index (~150-200K). A removed entry now
  leaves a NULL command and an index tombstone; holes compact only when an
  append would not fit. Lab P95 -1.4..-2.8K on six stages, Dream Land P99
  -38K; gate flat. Refuted: swap-remove with backward-shift deletion (slower).
  Receipt `2026-09-30_p2-2p8-ledger-holes`.
- **09-30 carrying hurtbox worlds across epochs, REFUTED** (exact: 0
  mismatches on nine stages): a held tick's world is the last one moved by the
  parent's translation. Carries reached 39-47% of builds and same-ROM lab P95
  -6..-19K, but only against the machinery's own overhead; cross-build against
  the HEAD kernel the gate P95 is +4.7K and Saffron/Zebes flat/+5.6K. Patch and
  data: `2026-09-30_p2-2p8-hurtbox-carry-refuted`.
- **09-30 the damage meter's source display callback is not called, BANKED**
  (replay identical, lower-screen pixels identical): with the lower-screen HUD
  its SObj work (~180 soft-float calls a frame) had no reader. Gate P50 -10.0K,
  P95 -9.2K (4/4 pairs, 1,221.6K -> 1,212.4K); lab P95 -12..-14K. Also the
  meter colour kept per (damage, colour id). Receipt
  `2026-09-30_p2-2p8-hud-damage-display`.
- **09-30 the spline's arc-length bisection on bit patterns in ARM state,
  BANKED** (host-proven exact over 164.9M checks, device oracle 0
  mismatches): owner: the Sector Z Arwing is "the massive P95 hit" (~230K a
  flight frame). The kernel keeps libgcc's float ops (an inlined integer
  version grew 12.5 KB and cost +48K at P95: the I-cache). SZ P95 -8.8K.
  Receipt `2026-09-30_p2-2p8-interp-kernel`.
- **09-30 the Arwing answers its spline from a flight table, BANKED** (replay
  identical, oracle 0 mismatches): eight authored patterns, each started from
  frame 0, ask the same (segment, t) questions every flight; a lab capture
  recorded the device's answers, keyed on everything the function reads
  (`assets/stages/sector_arwing_frac.bin`, 103,336 B, SZ heap only). SZ owner
  roster P95 -65.5K, default roster -78.6K. Receipt
  `2026-09-30_p2-2p8-arwing-frac`.
- **09-30 the Arwing's eight roots sent by DMA, BANKED** (replay identical,
  DS top screen pixel-identical on five flight frames): the state cache's
  replayed group loop is recorded once as a packed GX list and sent by DMA,
  with the lit-matrix load a CPU write between two segments. Same-ROM SZ P95
  -29.9K (owner roster), -27.5K (default); cross-build vs the table build
  -14.3K / -0.8K (the packet-off arm moved +16..27K with the build's layout).
  Receipt `2026-09-30_p2-2p8-arwing-packet`.
- **09-30 the event-32 ledger covers event32-motion rosters, BANKED** (a
  crash): Jungle Captain/Yoshi/Kirby/DK died at f937 (the ledger full, a Poke
  Ball's rays lost their animation, the ball wrote through the ejected
  effect). The margin adds 768 per Ness/Yoshi/Pikachu/Purin; 0 refusals on
  12 rosters, replay identical where nothing refused before; the ball
  follows only a live effect. Receipt `2026-09-30_p2-2p8-event32-margin`.
- **09-30 scene transitions: a black cover from the last frame to the next
  scene's first** (owner r58; `52a026d86a9`, gate digest identical): no old
  stage under Results, no Results fighters over the CSS, no half-built
  screens, no tic-80 wallpaper flash. Evidence
  `artifacts/bugs/2026-09-30_scene-transitions/VERIFIED-r63.md`.
- **09-30 scene transitions hold the last frame** (owner r64: "keep what's
  currently on the screen until the next screen is ready, then switch"; r65a
  "much better"): BG0 keeps re-rendering the retained GX list and the 2D
  layers scan VRAM, so the old frame stays while the layer owners record the
  old scene's display teardown instead of doing it; the next scene's first
  display write (a 2D layer, an OBJ tenant, bank D under BG3, a texture or
  palette upload under BG0) thaws it into the r63 cover, which now lasts only
  while the first frame builds. Results' leaving frame is no longer drawn
  (taskman.c:994 breaks before it; drawn, it lost its text and tint). Gate
  +4.4K P50 / +4.7K P95 with no transition in the run (placement), digest
  identical. Evidence
  `artifacts/bugs/2026-09-30_scene-transitions/VERIFIED-hold.md`.
- **09-30 native owners for the roots drawn with none**: a root-keyed lab
  census (25 arms) found PK Fire's pillar, Bob-omb walking left, the placed
  Bumper, the Ray Gun's shot, Goldeen, Koffing and its smog, Razor Leaf and
  a Ness effect drawn by no owner; ITCommonData adds most Poke Ball Pokemon
  and their weapons. One generator compiles 32 such roots to native state
  deltas, material hooks and geometry groups (`generate_nds_native_item_baked.py`),
  one adapter lookup runs them: 0 native failures on the census arms and on
  13 forced-Pokemon arms. Samus's entry effect drew three frames past GO
  into retired textures: the retirement now waits for an idle frame. The
  image growth (+13.3 KB) tipped Sector Z's default roster under the world
  caches' first-frame reserve (STG +23.5K a frame); a missed attempt now
  retries at GO keeping 48 KB. Gate (draws no baked root) P50 +2.0K, P95
  +2.1K, digest identical: carrying cost. Open: Link's 3 entry frames on
  Sector Z/Saffron (a texture bind fails: VRAM at the entry burst) and
  Hitmonlee (made, never reaches the adapter).
  Receipt `2026-09-30_p2-2p8-native-owners`.
- **10-03 VS stage bindings pinned**: the blob stages shipped rigid mask 0
  except Dream Land and Yoshi's Island, so the other seven venues composed
  every static binding on the CPU each frame and baked no static world. A lab
  census (seen & ~moved over 1,900 match frames, with the yakumono and
  anim-joint maps) pins castle 6, Sector Z 2, Jungle 13, Zebes 10, Hyrule 13,
  Saffron 2 and Mushroom Kingdom 13 bindings. A binding a cross-matrix run
  reads stays live (a rigid binding's composed matrix is never built), and
  Hyrule/Saffron stop at BODY_MAX. Same code, default roster: mean -20K
  (Hyrule), -16K (Jungle), -14K (Zebes), -13K (Mushroom), -7K (castle), share
  in two VBlanks +1..+3 points; no declines; heap low-water >= 76.9K except
  Sector Z (binding 10 left live for its 4.2 KB of body). Receipt
  `2026-10-03_vs-stage-pins`.
- **10-04 stage segments in one pass; the gate's tail is the item phase.**
  Gate at HEAD `3472eaf646a`: WORK P50/P95 960,064/1,324,672, 384 of 1,960
  over 1.12M, 344 of them after presented frame 800 when the item spawns pile
  up. Against frames under 1M, the over-gate frames carry SRC +222K, MISC
  +139K (items 38K, the camera/draw-shell remainder 41K, particles 26K,
  weapons 26K) and FTR +69K; STG is flat. `ndsStageGxCommitFast` commits a
  compiled stage segment in one patch pass and one DMA (runs contiguous in
  words and patches, no hidden binding, no 1P cull, union near boxes per
  binding; every FIFO word, span state and closing painter depth hashes
  identical on 901 lab frames): STG P50 170,560 -> 154,688, WORK P50/P95
  965,440/1,327,872 -> 948,288/1,316,224, two VBlanks 1,556 -> 1,597 of
  1,961, digest identical (`fa35dd61607`). The stage prepare takes the
  battle camera's split matrices from the frame camera cache instead of
  building them a second time (`cbbe041ca76`, STG P50 -1.3K, digest
  identical). Rejected: skipping a held item's attach at draw time -- the
  latch walk writes the FTParts latches the simulation reads, and the digest
  diverged at frame 879 -- and culling the VS venues' stage segments
  (STG +17K). Receipts `2026-10-04_gate-head`, `2026-10-04_stage-segment-fast`,
  `2026-10-04_stage-camera-share`.
- **10-04 the stage DL fast lane on the DTCM hot stack.** A late-window lab
  profile (frames 1,200-1,840, `artifacts/task37-census/sz-lateprof01`)
  charged the fast lane ~10 cycles an instruction: its owners' 3,000-byte
  traversal states and its own frames lived on the main-RAM stack, and its
  costliest rows were stack pops and frame loads. The lane now enters
  through `ndsDtcmHotStackRun` (static reach 5,728 B of the 6,144 B stack,
  lab high-water 5,136 B; calico runs IRQ handlers on their own stack):
  gate P50/P95 946,304/1,309,184 -> 938,368/1,302,464, over 355 -> 330,
  two VBlanks 1,597 -> 1,623 of 1,961, digest identical (`debe30ee031`;
  lab item phase WORK mean -20.6K). The same profile's over-gate split puts
  the remaining premium in hurtbox rejects (77K cycles in each of the 137
  attack frames), soft float (~2.9K fadd and 2.2K fmul calls a frame),
  status-change pose parses and effects; the frames that draw Samus's
  Charge Shot (22% of the gate, 46% of them over) carry the combat around
  it, not the shot. Receipt `2026-10-04_stage-dl-fast-hot`.
- **10-04 more draw on the hot stack; camera-loop trims.** G_MTX push and
  G_POPMTX are interpreted only by the host reference, so the DS
  `NDS_RENDERER_MODELVIEW_STACK_SIZE` is 1 and `NDSRendererTraversalState`
  is 888 B (was 3,000): the fast lane's static reach drops to 3,632 B and the
  NDL dispatch (impact waves, Link bomb, damage slashes) and
  `TryNativeEntryEffect` fit on the hot stack too (`2ae07aeb07a`: same ROM
  P50/P95 938,304/1,295,936 -> 934,144/1,280,448, over 334 -> 311). The
  battle camera loop no longer runs five recognisers per display GObj for a
  diagnostic count, and the stage display commit pre-tests GObjs against a
  32-bit bloom of its segment pointers (`0ac832f3bd7`: P50 -4.9K, P95
  -8.4K across ROMs). The collision readiness check (~1,000 interworking
  calls a frame into ITCM) is inline (`e89136078b8`: P50 -4.6K). Rejected:
  inline 16-word copies for the fast pass's memcpy calls (STG +8.7K) and an
  inline `ftGetStruct` fast path at ~1,370 call sites (P50 +3.8K, SRC
  +2.8K): small ITCM leaves beat inline main-RAM code. Gate now 925,888 /
  1,271,488, 288 over, 1,666/1,961 in two VBlanks. Receipts
  `2026-10-04_ndl-entry-hot`, `2026-10-04_capture-trim`,
  `2026-10-04_micro-copies`.
- **10-04 ITCM fourth pack.** The wall sweep's slow path
  (`ndsStageMPAdjustFloorLoopWallSweep`, 2,016 B, admitted 09-27 when every
  sweep ran it) rents ~840 cycles a byte now that the all-reject fast path
  answers first; it returns to main RAM and the fast path
  (`ndsMPWallSweepStaticMiss`, census section D's first at ~11,200 non-mem
  stall cycles a byte) and 24 more section D functions take the bytes by
  name (`09dc70e015f`: P50/P95 925,376/1,273,664 -> 920,704/1,259,712, over
  286 -> 280, 1,678/1,961 in two VBlanks, same-session baseline). Thunks
  whose bodies the shipping ROM inlines stay off the name list (the first
  pack overflowed the shipping link by 1,680 B). Receipt
  `2026-10-04_itcm-pack4`.
- **10-04 item draws replay their recorded emits.** 247 of the gate's 280
  over-gate frames follow frame 800, and the only items alive then are Beam
  Swords (one lying, one held by Link, a second lying from ~1440): MITM read
  ~33K a lying sword, ~62K a held one. A lab split put each root (an
  11-triangle blade, a 2-triangle hilt) at ~15.6K of per-root machinery
  (emit 5.7K, generated setup 2.9K, matrix prep 3.8K, fast-lane prep and
  tail 2.0K) plus the held item's exact attach (~21K a frame, kept: its
  latch walk is gameplay). An item draw whose lists are all replayable
  owners now records once (`ndsNativeItemWave1Emit` leaves each emit's
  bound texture, batch format and alpha-test words and per-vertex words)
  and replays under freshly prepared matrices while a verbatim key holds
  (`ndsRendererAdapterSubmitItemDObjTreeReplay`); a lab verify mode compared
  1,548 fresh recordings with the cached ones, 0 mismatches. Sword route
  only for now (`cacd4fd246f`): P50/P95 920,704/1,259,712 -> 906,432/1,247,232, over 280 ->
  241, 1,714/1,961 in two VBlanks; frames 1400-2000 MITM 85.8K -> 60.5K,
  window P95 1,331,968 -> 1,295,808; digest identical. Receipt
  `2026-10-04_item-replay`.
  Then every MObj-less item route replays (thirteen owners; a key whose
  recording failed is remembered) and a keyed node under a moving parent
  keeps its local (`3911899aa71`; Castle all-items verify 204/0); the
  held item's integer render tail priced at no gain (its latch walk is the
  cost).
  Rejected the same day: exact unpacked binary32 chains for the latch walk's
  compose, point transform and local (digest identical, P95 +68K: 11.5 KB of
  main-RAM ARM against libgcc in ITCM; receipt `2026-10-04_exact-f32`).
- **10-04 the instrument leaves the gate.** The FPS console's periodic text
  is compiled out of the gate ROM (`d03478ba889`, A9 class) and the display
  capture takes four exact cuts (`8aa86b448fd`: 897,408/1,236,352, 234
  over). Under owner ruling D12a WORK now excludes the replay digest
  (~4.0K a frame) and the gate ROM boots with the tick HUD's fine span
  clocks off (`NDS_TICK_HUD_SPANS_DEFAULT=0`; FTR/STG/MISC read 0 there,
  `-BootSetGlobals 'gNdsTickHudSpans=1'` restores them): 895,360/1,232,576,
  226 over, 1,722/1,961 in two VBlanks, digest identical. Priced, no
  effect: stage binding bookkeeping, a fifth ITCM pack. Receipts
  `2026-10-04_fps-console`, `2026-10-04_capture-cuts`,
  `2026-10-04_instrument-out`, `2026-10-04_stage-bind-cuts`,
  `2026-10-04_itcm-pack5`.
- **10-04 painter depth by MTX_TRANS.** Every no-Z stage triangle loaded its
  own 16-word projection to carry its painter depth (Dream Land: 126 of
  them, ~2,400 of 7,009 words a frame). Template version 6 loads one clip
  transform a run into the position matrix (view x projection, or the
  composed matrix) with the z column at w x the run's first depth, keeps the
  projection identity, and steps each later triangle with MTX_TRANS(0, 0,
  -1) -- a clip-space z translation, x/y/w untouched; the run consumes its
  depths at the load. Dream Land 7,009 -> 5,589 words, 297 -> 202 patches;
  composed runs exact in x/y/w, static runs within 17 LSB (~0.02 px), painter
  ordering preserved (fixed-point model over 11 stages); gate captures
  pixel-identical at two of four frames, isolated texels at the others.
  Gate 884,224/1,223,680, 214 over, 1,732/1,961 in two VBlanks, paired
  median -10.8K, digest identical. Receipt `2026-10-04_painter-trans`.
  Then a run sends a baked COLOR or TEXCOORD only when it changes (508 of
  Dream Land's 606 COLOR commands repeated the last value; 5,589 -> 4,890
  words): gate 882,496/1,222,848, 209 over, paired median -2.1K, captures
  pixel-identical (`5c4c43cc242`, receipt `2026-10-04_attr-dedup`).
- **10-04 development tallies out of the shipped configuration.** A late-window
  lab profile (`sz-lateprof05`) put ~50.8K cycles a frame on lines that update
  `gNds*` tallies read only by probes and verifiers, each a main-RAM
  read-modify-write the 4 KB D-cache had evicted. 106 write-only sites go
  through `NDS_DIAG(...)`, compiled out where `NDS_DIAG_COUNTERS` is 0 (the
  published ROM and the gate; 1 elsewhere): gate 873,344/1,215,616, 199 over,
  1,754/1,961 in two VBlanks, paired median -7.9K, digest identical
  (`29b1e46c3ef`, receipt `2026-10-04_diag-out`). A second batch of 28 sites
  whose readers were only declarations, other tallies or lab tours followed
  (`545f4c30d98`: 870,592/1,210,752, 192 over).
- **10-04 empty particle passes and one MObj walk.** The four particle passes a
  frame each ran the atlas/camera/Whispy setup before finding their links
  empty; passes without link 0 (which also owns the Fox glow and FireGrind
  pools) now return first (`28b8c40df08`: 864,192/1,204,480, 179 over,
  paired median -6.7K). `gcPlayAnimAll` collects live MObjs in one walk
  instead of count-then-collect (`f66920bfe76`: median -0.8K). Rejected: a
  stage texture-entry stamp dedupe (+0.8K). Parked: stage animation at a
  reduced rate (a ground GObj's animation feeds Dream Land's gameplay; the
  digest diverges at sample 130; receipt `2026-10-04_stage-anim-rate`). The
  fixed-point fighter-part chain (D13) needs residency to pay: the decomp's
  DObj TRS are floats, so a fixed answer still converts them at entry (slice
  52 measured the producer swap at 1.001).
- **10-04 quiet dynamic stage bindings (D12c class, render only).** A dynamic
  stage binding whose world did not rebuild for eight validations is checked
  one frame in four (`gNdsStageDynStride`; bindings a yakumono DObj names stay
  every frame): gate 855,744/1,192,064, 169 over, 1,778/1,961 in two VBlanks,
  digest identical, captures pixel-identical (`71a93c22d59`, receipt
  `2026-10-04_stage-dyn-stride`). A spans-on breakdown of that ROM
  (`artifacts/performance/2026-10-04_breakdown`): the P90-95 band over the
  median is SRC +185K, MISC +77K (camera remainder +22K, weapons +18K,
  particles +16K, items +12K, effects +9K), FTR +18K, STG +2K.
- **10-04 lean fighter kernel at render precision.** The kernel's outputs are
  list words only, so it no longer reproduces the old compose's
  round-half-away steps or the float-rounded angle index
  (`NDS_FTR_LEAN_RELAXED`, default 1; 0 is the exact kernel the lab oracle
  routes grade): gate 850,176/1,184,960, 160 over, 1,788/1,961 in two
  VBlanks, paired median -6.7K, digest identical; captures differ by edge
  pixels only (receipt `2026-10-04_lean-relaxed`).
- **10-04 lean instruments out of the gate and the owner validation pool.**
  The lean renderer's lab counters and the VRAM census followed NDS_TICK_HUD
  alone, so the gate ROM carried them; they now also need NDS_DIAG_COUNTERS
  (`2bc60fe00ce`: 845,504/1,181,952, 157 over). Lean events (47
  materializations at 150-850K ticks and 196 entry switches at 25-73K a
  match, 14.2K ticks a frame on average, all spikes) are the FTR tail:
  priced on the gate rows, free events would be P95 -33K, a 2-4 frame split
  only -7K. The owner validation cache held one root set per owner slot, so
  alternating model parts re-ran the full validation each switch; a 12-entry
  pool keyed by the root set (`be6fc76cc27`: 845,952/1,178,752, P99 -27K,
  151 over). Rejected: the D13 latch walk (func_ovl2_800EDBA4 and its local,
  compose and point carry) in the hurtbox kernel's fixed arithmetic with
  float latches -- worse on all four seeds (P95 +2.1K, mean +1.3K): it runs
  about once a frame, so its main-RAM code is cold while the float path is
  small decomp code over ITCM soft float (receipt `2026-10-04_latch-fixed`).
- **10-04 fighter joint worlds resident in fixed point (owner: "fixed point
  the whole way").** The hurtbox reject's per-epoch world cache is the one
  place a fighter joint's world is built: `gmCollisionGetFighterPartsWorldPosition`
  reads it (point in, one fixed transform, point out) and the held item's
  0x52 matrix reads the parent's world and writes the Q20.12 directly; neither
  writes an FTParts latch, and the float walk/local/compose leave those paths
  (gdb, 100 late frames: 958 composes and 1,186 locals -> 14 and 20). The local
  builder is the relaxed form (truncating index, cosine a quarter turn on).
  Digest re-baselined (D13, frame 221); same-ROM 4 seeds P95 -0.8K mean,
  official seed 1,178,048 -> 1,169,536 (`97701743bdf`); the walk then reads
  DTCM sines and composes with truncating reductions: 834,880/1,163,328, 131
  over (`8d02498a7d8`, receipt `2026-10-04_joint-resident`). The late-window
  profile shows the fixed walk memory-bound (~1.1K a local, ~0.9K a compose,
  ~96K a tail frame). Rejected: the walk in ITCM for five low-rent residents
  (paired +4.2K a frame) and body-joint locals kept across the pose-hold tick
  in their FTParts (digest identical, 21% fewer local builds, paired -128).
- **10-04 the worst-case search, all nine VS stages (lab sweep ROM with every
  `NDS_P2_STAGE_*` on; the gate target stages Dream Land only, and the lab
  stage word on it faulted at frame 0 on the other maps -- no wallpaper
  sprite).** Preset roster, items on, 1,960 frames, lab instrument included
  (~6% over the gate ROM): P50/P95 Castle 1,002,752/1,346,944, Sector Z
  1,036,672/1,389,504, Jungle 1,010,368/1,427,520, Zebes 1,012,672/1,333,632,
  Hyrule 896,512/1,153,024, Yoshi's Island 1,045,248/1,396,480, Dream Land
  898,048/1,238,336, Saffron 1,090,880/1,456,576, Mushroom Kingdom
  1,060,288/1,360,192 (`artifacts/performance/2026-10-04_stage-sweep/a1-*`).
  Saffron's SRC runs +114K and MISC +77K over Dream Land's.
- **10-04/05 the all-stage worst case, first cuts (digest identical on every
  stage, so frames pair one to one).** Per-stage profiles
  (`artifacts/task37-census/p2-g0..8`, categories by scratchpad `catsum.py`)
  put the excess over Dream Land in map collision and the stage draw. (1) The
  wall sweep's all-reject fast path refused any kind with a dynamic group;
  it now applies the group reject's own yakumono shift (`7c7949e0f40`:
  Castle -36.9K, Saffron -23.9K). (2) DLLink stages (Sector Z, Hyrule,
  Saffron, Mushroom Kingdom, Zebes) drew every segment run by run in four
  head passes; GX programs are now compiled in head-pass order (format 7)
  and take the one-pass commit (`72565c2ce9a`: Saffron -41.9K, Hyrule
  -40.2K, Mushroom Kingdom -30.8K, Zebes -24.1K, Sector Z -8.9K; captures
  pixel-identical). (3) `ndsStageGxDraw`'s now-cold 3,576 B of ITCM went to
  the joint walk and the wall edge memo (`56f8903d10e`: every stage -2.3K to
  -6K; official gate 831,744/1,154,496, 123 over). (4) 271 write-only
  collision tallies behind NDS_DIAG (`88e9ea006dc`: -0.1K to -5.3K; gate
  829,824/1,152,256). Lab P95 now: Castle 1,286,208, Sector Z 1,370,944,
  Jungle 1,406,016, Zebes 1,290,816, Hyrule 1,098,304, Yoshi's Island
  1,385,088, Dream Land 1,224,832, Saffron 1,365,504, Mushroom Kingdom
  1,315,072. Left per stage: Yoshi's Island's cloud owner (~120K a frame of
  per-draw setup for 18 triangles), Jungle's stage DL lane (~60K), the floor
  sweeps (37-53K), the pose parser's combat spikes (+100K in tail frames),
  and the lean kernel's DObj reads (~95K everywhere).
- **10-05 render state at the presentation rate; fetch-frame bookkeeping.**
  An official-gate profile (`artifacts/task37-census/gp2-official`, frames
  1,100-1,900, aligned to the gate rows at r = frame - 1,101) puts the frames
  just over 1.12M at +33K soft-float adds, +65K hurtbox reject and walk, +67K
  effects (entry effects, impact waves, CPU-projected corners, particle
  quads) and +28K pose parse/play over the median frame. (1) Material
  animations (MObj colours, texture/palette indices, scroll) are render state
  the replay digest does not fold, so they step once a presented frame, on
  the drawn tick, by the ticks it stands for (`b3887a3bfb5`, word
  `gNdsMObjTick30Hz`; costume bake and Yoshi's clouds keep the source rate):
  gate same ROM P95 1,149,376 -> 1,140,992, paired -4.8K; lab paired -1.8K
  (Hyrule) to -9.0K (Zebes) on all nine stages, digest identical. (2) Particle
  draw lookups memoized -- per-transform axis magnitudes, the ENV variant and
  packet binding searches, the view-space axis conversions (`44094ba0257`):
  -0.6K paired, -1.1K in the top 5%. (3) Frames that fetch a clip (7.5% of
  the gate's, mean 2.14M cycles vs 1.82M) spent ~41K in the ring's overlap
  scans, a line fill per 32-byte entry per scan; the scans read compact
  extents now (`1102299cdbb`): single fetch frames -87K at best, P95 -1.4K.
  Measured and not kept: using streamed clips in place of the memmove
  compaction (lost ring bytes cost re-reads; P95 +2.3K). Gate after the three:
  ~1,139K, 110 over.
- **10-05 FTParts latch clears skip clean subtrees.** The two per-tick latch
  clears a fighter walked every FTParts of its subtree, and 20,799 of 21,437
  found nothing set (gdb census). Every latch writer now bumps
  `gNdsFtPartsLatchWrites`, and a clear with the counter unchanged since the
  subtree's last clear is skipped (word `gNdsFtPartsCleanSkip`); a batch's
  held tick also skips the fighter MObj loops. Gate same ROM P95 1,148,160 ->
  1,141,696, paired -6.7K; shipped build 820,544/1,137,344, 107 over; digest
  identical on the gate and all nine lab stages (receipt
  `2026-10-05_ftparts-cleanskip`).
- **10-05 stage commit accounting, wall-sweep readiness.** The stage fast
  commit sums its per-run texture accounting once per segment and adds only
  the classes a segment holds (word `gNdsStageGxFastLean`, paired -1.5K); the
  wall sweep's fast path proves its geometry checks once per geometry and heap
  generation (word `gNdsMPWallMissReadyCache`, -1.9K). Together paired -3.4K,
  gate 817,024/1,130,560, 105 over, digest identical (receipt
  `2026-10-05_stage-commit-lean`). Measured and set aside: CPU composition of
  Dream Land's dynamic no-Z bindings (11 billboards, 10 swaying parts with
  cross corners, Whispy) would leave ~5K to GX composition.
- **10-05 the GX list DMAs priced; lean lists with VTX_10 corners.** A lab
  word skipping GXFIFO DMA starts (experiment ROM, digest identical) put the
  DMAs at paired -25.4K a frame on the gate: stage segments -10.4K, the four
  lean fighter lists -14.8K, the rest -0.3K -- CPU bus stalls behind the
  112-word bursts, so the cost follows list words (receipt
  `2026-10-05_dma-stall`). A live dump of the lean lists (8,197 words at frame
  1,200) put VTX_16 at a third of them; every corner is a whole source unit
  (|unit| <= 292), so route-1 lists now take VTX_10 corners, one parameter
  word, at 4x the VTX_16 value -- the per-root LOAD4x3 translation rows and P'
  row 3 take the same 4x, a uniform clip scale the divide, the homogeneous
  clip and the z/w depth buffer cancel (lighting unscaled). A list with a
  corner past +/-511 units walks again at VTX_16. Gate same ROM paired -2.6K,
  814,656/1,127,552, digest identical; captures differ in isolated fighter
  edge pixels (receipt `2026-10-05_lean-vtx10`, word `gNdsFtrLeanVtx10`).
  Stage templates v8 send every (affine) baked world as MTX_MULT_4x3, 12
  words instead of 16 (Dream Land 4,890 -> 4,782 words a frame, the other VS
  stages -4 to -192); the validator checks column 3. Gate 813,568/1,125,568,
  100 over (receipt `2026-10-05_stage-mult43`). Tried and reverted: making the
  DMA starts' bookkeeping stores first (paired +0.3K: the stall moves to the
  next bus access).
- **10-05 KO pillar palettes rewritten at the swap VBlank.** The KO burst
  carries up to 13 distinct ENVCOLOR keys a frame (44 over the burst), so the
  eight-entry variant round robin rebaked 124 times in 19 frames, each a
  `glColorTableEXT` on a live name (libnds free + alloc, banks F/G mapped to
  LCD mid-frame, and a palette the on-screen frame still read): 357 of the
  match's 394 mid-match GL uploads. Now 16 entries keep their palettes; a miss
  takes the LRU entry the frame being built does not reference, bakes into its
  staging copy, and `ndsRendererParticleEnvVariantCommit` copies the queued
  palettes in at the first VBlank after the flush. Same ROM: P95 1,125,184 ->
  1,118,528 (paired -7.9K), KO frames -47K to -146K, 101 -> 97 over, digest
  identical; two-VBlank presents 1,857/1,961 (94.7%) (receipt
  `2026-10-05_env-defer`, word `gNdsParticleEnvVariantDeferred`). This also
  removes the commonest mid-frame VRAM remap, a candidate for the KO-burst
  screen corruption in BUGS.md.
- **10-05 entry ramps deferred too; no-Z entry groups through the GX.** The
  entry ramp palettes (Link's spin oranges, glows) now rewrite the same way
  (`gNdsEntryRampDeferred`; digest identical, cost neutral on the gate, receipt
  `2026-10-05_ramp-defer`). A no-Z entry group's CPU painter submitted
  z = depth * 4096 / w, a constant clip z; the GX now draws such a group with
  the root's composed matrix and that constant in the z column instead of a
  CPU transform and divide per corner (`gNdsEntryEffectGxPainter`). Same ROM:
  P95 1,116,416 -> 1,111,552, over 97 -> 92, two-VBlank presents 1,857 ->
  1,861, Link's spin frames -19K to -28K, digest identical; the KO frame
  capture is pixel-identical (receipt `2026-10-05_entry-gx-painter`).
  Measured and rejected: the effect submitters built Thumb (profile, all
  frames +11K cycles, KO frames +38K; the impact wave grew 10% and slowed
  30%). Found: a present with WORK above ~1,110K misses its second VBlank
  (the replay digest's ~3.9K and ~5.5K of frame-boundary time sit outside
  WORK), so the two-VBlank share tracks WORK <= ~1,110K, not 1,120K.
- **10-05 a clean worst-case ROM; Yoshi's Island clouds; Fire Flower routes.**
  The lab sweep ROM's instruments (stage/Fox GX FIFO hashes, item
  accumulators, the Fire Flower recorder) inflated the heavy stages by 93K-125K
  P95. `NDS_LAB_FOURCPU_WORDS=1` compiles the stage/roster words alone; on it
  (P50/P95): Castle 888,000/1,206,272, Sector Z 931,584/1,278,016, Jungle
  913,280/1,280,832, Zebes 890,880/1,195,328, Hyrule 767,552/1,012,288,
  Yoshi's Island 947,840/1,274,688, Dream Land 815,616/1,124,992, Saffron
  924,544/1,249,280, Mushroom Kingdom 936,448/1,218,560
  (`artifacts/performance/2026-10-05_clean-sweep`); the excess over Dream
  Land is simulation (+69K to +122K in the P92-98 band) and, on Sector Z,
  Jungle and Yoshi's Island, the draw (+75K to +87K). (1) Yoshi's Island's
  cloud locals are all Tra builds, so a drawable's chain is its three
  translations summed and the kind-48 recalc needs only row 3 of chain x
  camera: 7 local builds and 9 multiplies a cloud become integer
  conversions and 3 row products, bit for bit (`gNdsYosterCloudFast`, paired
  -15.3K); the three drawables' shared corner words are kept with their
  inputs (`gNdsYosterCloudCornerMemo`, -5.3K). (2) The Fire Flower's two lists
  reached their owner through the whole stage-DL body every draw; the body now
  records fast-lane routes for both roots (`gNdsStageDLFastFFlower`; Jungle
  P95 1,310,272 -> 1,277,376, paired -27.3K). Replay digest identical on every
  pair and on the official gate (1,111,168, 1,861 two-VBlank presents,
  neutral: neither owner draws in its match). Receipts
  `2026-10-05_cloud-fast`, `2026-10-05_fflower-route`. Found: Sector Z's
  segment 0 declines the GX fast commit every frame for the owner-hidden
  wing-platform proxy binding (lab counters `gNdsLabStageGxFastWhy`); a
  degenerate-binding exemption measured nothing and was reverted
  (`2026-10-05_sz-degenerate`). A body-submit census (clean ROM, frames
  100-1,900) puts the remaining body lists at the rebirth halo's three roots
  (105-163 frames a match), the N Bumper, the Sector Z Arwing laser, the
  damage-fly dust and Saffron's Pokemon.
- **10-05 the body's remaining owners get fast-lane routes; the halo goes
  first.** Each list the census found now takes a route the body records when
  its owner draws it, with the body's admission tested again every draw: the N
  Bumper (one palette-image MObj), the Arwing laser (a weapon; its texture
  file re-proved from the list's relocated words), Saffron's Marumine, GLucky
  and Porygon (item routes) and Hitokage and Fushigibana (a CURRENT_IMAGE
  snapshot), and the damage-fly dust (an effect: the effect layer's seeds and
  witnesses, then the body's own helper). The RebirthHalo arm runs from the
  dispatcher ahead of the entry probe and the body's prologue. On the frames
  each owner draws (clean ROM, same-ROM words, digest identical): halo -5.4K,
  N Bumper -15.9K, laser -22.6K, dust -25.6K to -29.6K, Saffron's Pokemon
  -7.9K; Saffron P95 1,269,312 -> 1,261,504. Official gate 1,112,960, 1,860
  two-VBlank presents (neutral). Mushroom Kingdom's two Pakkun and its POW
  block were drawn through the body on every frame; with their routes
  (palette and image files re-proved each draw) its P95 is 1,224,256 ->
  1,197,056, paired -24.6K (`7c590b1e478`). Receipts
  `2026-10-05_halo-first`, `2026-10-05_more-routes`.
- **10-05 the stage GX fast commit across hidden bindings.** Sector Z's
  segment 0 holds the owner-hidden wing-platform proxy and declined the
  one-pass commit on every frame, drawing run by run. A segment is now built
  for the hidden subset in force: the left-out runs' patches are skipped and
  their words cut out of the DMA as span breaks (`gNdsStageGxFastHidden`).
  Sector Z P95 1,303,232 -> 1,283,904, paired -23.2K, digest identical; the
  lab FIFO-word hash matches the per-run path on all 201 sampled frames
  (receipt `2026-10-05_stage-hidden-spans`).
- **10-05 Kirby's star quad on the fast lane; a parked-list pool measured and
  parked.** The star quad the Star Rod's weapons and Kirby's stars share takes
  a fast-lane route (`2cc13c51bad`; Castle P50 -2.4K, digest identical). A
  pool of compact slots holding evicted lean lists for any fighter (copied
  out, copied back in on a key hit) cut Sector Z's P95 1,273,408 ->
  1,236,928 with six 16 KB slots (45 revivals of 80 materializations) but
  Dream Land's only -3.6K: it pays where the general heap is too tight for
  the per-slot spare, and those stages have no room for it (Sector Z's heap
  low-water 65,208 against Dream Land's 182,744, same roster; the VS overlay
  loan is full; the FGM arena pins up to 133,424 of its 163,840 B). The
  patch is kept with the receipt (`2026-10-05_lean-park`) for when deleted
  machinery frees main RAM.
- **10-05 collision segment windows; the wall static miss in one pass;
  heavy-frame particle LOD.** (1) The floor and ceiling sweeps bound the x
  interval a segment must meet: the sweep's truncated span, widened by
  |dx| + 2 units when the motion falls (rises, for a ceiling) more than one
  unit -- a flat segment's extrapolated crossing lands outside the span by at
  most 0.001 |dx| / dy -- and not widened when the flat branch cannot hit. On
  a line of at least 12 vertices whose x never reverses, a line whose extent
  misses the interval is skipped and two binary searches give the run of
  segments to walk; the point queries start where the point can first be
  bracketed. The wall sweep's all-reject fast path proves and books each
  group in one pass (`06e295b1313`). Same ROM, digest identical: Sector Z
  paired -5.4K (P95 -5.7K), Yoshi's Island -1.5K, Saffron -1.0K, the rest
  flat; bounding every line had cost Yoshi's Island +2.9K. (2) Owner ruling
  D12b: a frame whose elapsed work has reached 1.0M ticks at its first
  particle pass draws half of its generic particles (odd pool slots, stable
  across consecutive heavy frames; their transforms still run;
  `f310d5435f7`): P95 Sector Z -8.1K, Jungle -7.2K, Yoshi's Island -7.0K,
  Dream Land -1.7K, digest identical. Official gate (`build-gate-1005u`, both,
  same ROM): P95 1,124,992 -> 1,121,728, two-VBlank 1,854 -> 1,858; the
  build reads +5.4K P95 against the morning's by layout alone. (3) A lab
  census (`99b344558de`) found 56% of a fresh lean list's roots
  word-identical to the held list's (half the lists differ in at most two
  roots): materializations mostly re-emit words already held. Receipts
  `2026-10-05_collision-window5`, `2026-10-05_particle-lod`,
  `2026-10-05_lean-root-census`.
- **10-05 owner playtest fixes; specials on the fast lane; the float census.**
  Yoshi's Island spawn (floor projection in each platform's frame,
  `9dc4f0f67fb`), hatted Pikachus/Jigglypuffs back on the lean path (accessory
  alias root, `0deb4522bc2`: 4 x Pikachu costumes 0-3 P50 1.72M -> 0.83M), the
  unlit GO lettering (`70ec049d455`), PSI Magnet over Ness (`67a1f15ac84`). A lab
  input knob forces every CPU to perform a special together: Thunder and PK
  Thunder drew every list through the stage DL body; fast-lane routes
  (`dac7fea37d8`) took forced 4 x Thunder P95 1.62M -> 1.43M and forced PK
  Thunder 1.42M -> 1.30M, and retrying deferred effect descs only when the
  loaded-file table changes took Thunder to 1.38M (digest identical). Owner
  ruling the same day: software float must go (fixed point only) and the old
  machinery must be deleted, both top priority. The first per-call-site census
  (`2026-10-05_float-census`) puts soft float at 249K ticks a frame (gate
  roster), 310K (Sector Z) and 391K (4 x Yoshi); the hurtbox test now decides
  in fixed point (gate digest identical, P95 -4.3K), and Yoshi's animation-lock
  chains -- the hurtbox walk and the lean renderer's local -- followed (4 x
  Yoshi P95 1,332K -> 1,275K, misses 317 -> 248; gate digest identical; Yoshi's
  digest re-baselined on low-order bits, same positions and damage on screen).
  Receipts `2026-10-05_vfx-specials`, `2026-10-05_fixed-hurtbox`.
- **10-05 night: float leaves to fixed point; the old executor's last users.**
  Soft float measured by the PC profile is ~11% of busy cycles (the census's
  per-call ticks read 3-4x high: its wrapper times itself). Two batches moved
  the arctangent family, sinf/cosf, the Mtx builders' conversions, guMtxCatF's
  zero terms, stage/item animation (fixed cubic for every DObj), the map
  normals (an exact per-slope memo), the CPU target search and ftparam's
  projection to fixed point: census-estimated soft float gate 65K -> 50K,
  Sector Z 84K -> 69K, 4 x Yoshi 102K -> 54K ticks a frame, with WORK flat on
  identical game states (paired before the digests part: gate +0.8K / +0.1K,
  Sector Z -1.2K / -2.9K, Yoshi -2.0K). Code size decides it: inlined
  converters grew syMatrixF2L to 1.6 KB and the fixed projection is entered
  cold once per fighter draw (+2-3K MCAM on Sector Z). The 1P scenes' Demo
  actors (ending figure, challenger, continue) now draw on the lean path's
  scratch slot. A campaign-walk census of every old-executor draw
  (`gNdsOldExecutorDraws`, `gNdsFtrLeanDeclineReasons`) leaves Metal Mario and
  the Polygon team: lean declines them on Capacity, being environment-mapped
  all over (a Polygon: 28 texgen groups, 484 sites; a lean list holds 8 and
  256, and the recorder's GX texgen is not yet in the lean materializer).
  Master Hand and the ending figure draw lean. Receipt
  `2026-10-05_fixed-leaves`.
- **10-05 late: GX texgen in lean lists; the old executor compiled out.**
  Lean lists for the environment-mapped 1P owners (Metal Mario slot onward)
  let the geometry engine derive texture coordinates from the normals
  (TEXIMAGE_PARAM texgen mode 2, the texture matrix patched per frame; the
  header holds 32 groups): the late campaign walk drew every fighter lean,
  0 declines. The old fighter executor is then compiled only where the lean
  path is not (`include/nds/nds_ftr_lean_live.h`); a lean decline skips the
  draw and counts it. Text -34.9 KB, BSS -19.2 KB; gate P95 1,118,592 ->
  1,105,344 (paired -2.4K), Sector Z 1,268,992 -> 1,236,672, 4 x Yoshi
  1,267,520 -> 1,265,280, every digest identical. The VS Results
  out-of-memory (bug Y1) no longer reproduces: 48.5 KB free after the
  Results audio thread. Receipt `2026-10-05_lean-texgen`.
- **10-05 late: the packet recorder and the lean oracle deleted.** With the old
  executor compiled out, the recorder and its replay had no caller in any
  configuration; they are deleted from the source with every hook site, the
  record twins of the production emitters, the texgen overflow store and the
  18 KB packet array (heap +18,176 B), and the lab oracle routes that compared
  the lean lists against them went too (-2.9K lines). Digests identical; gate
  P95 1,108,928, Sector Z 1,228,032 (paired drift +1.5K to +4K from layout,
  the hooks' 472 ITCM bytes not yet re-admitted). Receipt
  `2026-10-05_recorder-deleted`.
- **10-05 late: stage animation in fixed point; the wall sweep's integer box.**
  The event32 parser writes stage and item joint tracks in the fighters' Q form
  (Requirement 4), so the player's length add, Linear, Step and cubic run in
  integers (the DObj clock and Sector Z's path parameter stay f32): Sector Z
  paired -5.1K, gate -2.0K, digests identical (4 x Yoshi's moves on 146
  transient frames). The wall sweep then skips a segment whose integer box
  misses the motion's before any float compare (exact): Sector Z -1.7K, gate
  -0.2K. Gate P95 1,107,328, Sector Z 1,219,584, 4 x Yoshi 1,261,184. Receipt
  `2026-10-05_stage-anim-q`.
  Per stage (one ROM, A/B word): every stage flat or better, Peach's Castle,
  Jungle, Saffron and Mushroom Kingdom's digests move (their platforms' tracks;
  Peach's Castle's moving platform within 0.003 units of the float arm).
- **10-05 late: the camera's interest box in fixed point; integer floor
  brackets.** gmCameraUpdateInterests (~150 soft-float calls a frame) takes a
  fixed-point strong definition; the floor projection's bracket tests run on
  floor/ceil integers (exact). Sector Z -2.1K, 4 x Yoshi -0.7K, gate flat,
  digests identical. Census after Q2: 3,332 soft-float calls a frame (gate),
  4,101 (Sector Z), 3,544 (4 x Yoshi). Receipt `2026-10-05_camera-fixed`.
- **10-05 late: the CPU attack pick in fixed point; particle centres refuted.**
  ftComputerCheckDetectTarget's per-attack prediction runs at Q12 (inputs once
  a call, 32-bit products: with int64 products the Thumb TU called
  `__aeabi_lmul` 17 times and ran +0.8-1.0K slower than float). Sector Z
  +0.3K, 4 x Yoshi -0.6K; the gate's AI decides differently from frame 424 and
  that match reads P95 1,087,616. Particle centres in Q8 cost +2-3K (64-bit
  products, an extra submit layer) and were reverted. Receipt
  `2026-10-05_cpu-detect-fixed`.
- **10-05 night: path arc length in fixed point; the float kernel and its
  tables deleted.** syInterpGetFracFrame's Bezier/Catrom arm (Sector Z's
  Arwing, Samus's rolls, the Board the Platforms boards) keeps the source's
  bisection and Simpson integrals in integers (`include/nds/nds_interp_fixed.h`:
  Q22 frames, a Q30 Horner, the math unit's sqrt pipelined behind the next
  sample, one int64 unit, the final divide rounded to nearest even); 99.7% of
  calls are bit-identical to the float source and every digest is unchanged.
  The bit-exact soft-float kernel, its oracle and capture, the 103 KB Arwing
  flight table and the 64 KB scene-heap result table are deleted. Sector Z
  paired P95 -26.9K (1,188,032), gate -2.1K (1,086,016). Over-gate Sector Z
  frames are now texture upload, lean re-materialisation and status-change
  setup; soft float is ~15K cycles of their premium. Receipt
  `2026-10-05_interp-fixed`.
- **10-06: the hurtbox joint cache doubled and the narrow-test frame cached.**
  The 4 x Yoshi over-gate profile put hit detection first (cofactor frames,
  lock-chain locals, rejects). The joint-world cache had 64 direct-mapped
  slots for ~100 joints, so chains evicted each other inside a tick; it has
  128 now (+14.6 KB BSS), and each slot keeps its world's cofactor frame.
  The reflection light is memoised by its two angles. All exact: digests
  identical. 4 x Yoshi P95 -15.9K (1,238,464 / 1,234,368), gate -4.8K
  (1,079,936), Sector Z flat. Receipt `2026-10-06_hurtbox-cache`.
- **10-06: 52 concluded A/B words deleted with their losing branches**
  (owner: "Delete Old machinery"). Hurtbox and map-collision words
  (`0a41e57a743`), stage GX, object animation, renderer, shims and the float
  lbCommonSin/Cos (`36d7cd0a0b2`), the stage DL adapter (`e7479f8b565`); the
  unshipped 30 Hz CPU arm went with its word. Every digest identical. Paired
  medians fell 1-5K per batch; at q15 the gate reads P95 1,073,664, Sector Z
  1,173,632, 4 x Yoshi 1,221,504 (WORK-H). Left: words whose old branch is a
  lab verifier or a config default (`gNdsFtPartsCleanSkip`,
  `gNdsStageDLHaloFirst`, `gNdsObjAnimBitCompare` in the objanim patch), and
  about forty in other declaration forms. Runs `2026-10-06_ab-cleanup`.
- **10-06: libgcc helpers out of the hot fixed-point code.** Thumb-1 has no
  SMULL or CLZ, so a Thumb int64 product or `__builtin_clz` is a libgcc call.
  The hot callers moved to ARM state (`c6c5ed936ee`, `6e7e354f98c`) and 64-bit
  divides went to the math unit. GCC inlines a static `target("arm")`
  function into a Thumb caller *as Thumb*, which had kept all nine
  `__aeabi_lmul` calls of the animation-lock local in place; `noinline` fixed
  it (`a465ea01751`, 4 x Yoshi P50/P95 -11K). ARM state for big main-RAM
  functions without 64-bit math does not pay: three map-collision sweeps read
  Sector Z +4.9K (I-cache) and were dropped. Receipts `2026-10-06_lmul-arm`,
  `_clz-div`, `_particle-fixed`.
- **10-06: particle centres in fixed point** (owner: "fixed point only"): a
  transformed particle's centre is three 64-bit dot products of the
  transform's Q16/Q8 form, straight into the submitter's Q8
  (`ndsRendererSubmitParticleQuadQ8`); nine float products and nine adds a
  particle before. Render only.
- **10-06: exact cuts in collision and the lean path.** A flat segment's
  height is `v1y` bit for bit, so flat floor/ceiling queries skip the
  interpolation's six float operations; the lean variant learn diffs two lists
  in one early-exit pass (`45db4a84823`). Lean events check the old path's
  per-row material keys before rebuilding a snapshot (99.9% of rebuilds were
  identical), and the AI floor memo's snapshot reads only the yakumono groups
  that own floor lines (`ff9dc8ee0ca`, Castle -7.4K paired). All digests
  identical. Receipts `2026-10-06_flat-learn`, `_lean-matkey`.
- **10-06 (afternoon): owner playtest rows.** The fixed-point
  `func_ovl2_800EB924` saturated perspective scales in [4, 8) (the shared
  float-to-fixed converter declines a left shift of 7), so narrow cameras --
  Master Hand's intro, the close-up entries -- culled the fighter as
  magnified (`a0a4cafe982`). Motion-script effect kinds that still drew
  stand-ins take their source particle makers (`9c897c32647`): the replay
  digest re-baselines, because particle scripts draw from the game's one
  random stream as on N64 (gate diverges three frames after the first
  HealSparkles). The item draw replay had admitted Saffron's three gate
  Pokemon, whose owners never call its sink, so it replayed them as nothing
  (`f8e22c5a117`); DObjDesc weapons without DL links reached a port no-op
  display and never drew -- Blastoise's water, Onix's rocks, Meowth's coins,
  Beedrill's swarm, Master Hand's bullets (`4e01f85a9c8`); and the
  flying-dust bands that do not fit beside Dream Land's pinned set are
  prepared again after GO (`e585e7e82f4`, the last native failure in a Poke
  Ball match). Gate after these: P50/P95 763,392 / 1,074,496, Castle
  800,000 / 1,096,384, Sector Z 852,544 / 1,182,144 (lab ring dump).

## 7. Found along the way

- **Camera-matrix staleness** for the boomerang (A6): an existing one-tick
  divergence.
- **The anim cache is disabled for every four-kind roster** by the 128 KiB
  keep-free rule (A2): 681 in-frame storage reads per match.
- **FAT walks grow with ROM size**: every remaining read gets slower as content
  lands.
- **Possible stage bug** (inferred, unmeasured): `near_inside` is camera-dependent
  (`nds_renderer_native_owners.c:1308-1311`), but `PrepareRun` is skipped when the
  R2 reuse key matches, and that key has no camera input (`:4263-4285`,
  `:4706-4712`).
- **DTCM holds Mario's tables** for rosters without Mario (A7).
- **The shipping build cannot load the heaviest four-kind roster** (A7; measured
  2026-09-23, Phase 0 log): Captain/Link/Pikachu/Kirby halts ~130 KB short.

- **Phase 2 world-pass experiment (2026-09-24), REJECTED.** Replacing the
  persistent stage-world cache with a preorder pass cost more than cache reuse.
  Static-affine refinement still gives WORK-H P50/P95/P99 1,351,680 / 1,980,259 /
  2,619,994 (14,400 / 17,498 / 32,134 worse than 8E6E7D8D), 19.65 FPS and
  289/1,973 two-VBlank presents. Replay/counts and paired matrices match.
  Removing 8,960 B of cache allocations was outweighed by a 16 KB startup
  allocator-placement loss; usable heap -7,424 B. Both attempts and source
  diff are preserved in the phase-2 receipt; production changes reverted.
  Shared-camera control remains current. Next: other VS-stage compilation
  and MISC; world-cache retirement is still owed, without another leaf retry.

## 8. Owner rulings (2026-09-22)

- **D1 Contract: P95 stays.** Gate = P95 WORK <= 1,120,000 and >= 95% two-VBlank
  presents over all presented frames (PROJECT_GOAL unchanged); P99 is reported.
- **D2 Replace, don't wrap: yes.** Packet record/replay/production, Task 36
  replay, CPU stage emission and their caches are deleted as compiled paths cover
  the content. No dual paths survive a phase.
- **D3 Equivalence: yes.** Tolerance classes as in section 5. The accepted
  body-hurtbox one-tick hold stays (A3 default); exact 60 Hz is not requested.
- **D4 Run-ahead: no.** Every frame fits its own two VBlanks.
- **D5 Visual reserve: case by case.** A10 items and approximations for the
  natively-missing items (linear texgen on the Polygon fighters, shadows,
  afterimages, Captain's high-detail alpha test) go to the owner with A/B captures
  when a phase needs them.
- **D6 Residency: new compact motion format.** Every gameplay motion resident;
  N02.04 (no post-GO demand reads) stands; MF is required (A2).
- **D7 ARM7 audio: yes.** A custom ARM7 binary owns BGM streaming and FGM voices.
- **D8 (2026-09-24) No dual paths includes the menus and 1P: yes.** Before the
  fighter production path is deleted, the lean path must also draw the character
  selects, VS Results, autodemo and the 1P scenes (intro, battles and their
  variant kinds); production is then deleted everywhere, not only in VS battles.
  This is coverage of those scenes' fighter rendering -- 1P campaign bugs outside
  it stay out of scope (owner, 2026-09-23: optimization work only).
- **D9 (2026-09-24) Keep going: yes.** A phase that falls short of its estimate
  or gate does not stop for re-planning; its shortfall is recorded and the next
  phase starts, accumulating every measured win. A bucket that falls while WORK-H
  does not is still not banked as a win.

**Owner clarification, 2026-09-24 (D10-D11):** pre-stage 1P intros use static
images, not live 3D. Campaign work is deferred; at least 95% of current effort
goes to making four concurrent VS fighters playable. Continue battle rendering,
memory and frame-cost work from the slice-7 checkpoint. The uncommitted live
Intro experiment is withdrawn. Deferred campaign coverage and global retirement
remain recorded obligations, not prerequisites for the next VS optimization
batch or claims of completion.

**Owner ruling, 2026-10-04 (D12):** with the gate ~116K short and exact cuts
yielding 1-3K each, four classes are approved for the four-CPU gate -- measure
each, ship the ones that pay: (a) instrument out of WORK (measured replay-digest
time and fine tick-HUD timer reads); (b) particle/effect LOD in heavy frames,
render only; (c) stage background LOD on VS stages; (d) CPU AI decisions at
30 Hz (changes CPU behaviour and the replay digest; humans unaffected).

**Owner ruling, 2026-10-04 (D13): float -> fixed point is an acceptable
compromise, game-rule code included** (physics, animation, collision, AI).
PROJECT_GOAL's mechanical equivalence governs ("bit-exact or numerically
identical execution is not" required). This resolves three conflicting
statements: the /goal guardrail "game rules stay source code" (ACTIVE_GOAL.md),
the done criterion "replay digest identical", and section 5's D3 tolerance
"exact for every discrete outcome". For a float -> fixed conversion: the
converted math lives in port code (decomp/ stays read-only); the conversion
re-baselines the replay digest, which then stays the exact gate for every later
change; discrete outcomes need mechanical equivalence, not identity. As with
every lane, the conversion ships only if it measures cheaper (the losing pattern
is a leaf swap that pays float<->fixed edges at its boundary; whole chains with
fixed-point state are the candidates).

**Owner ruling, 2026-10-04 (D14): fighter LOD in four-fighter matches.**
Approved: very-low detail meshes (build-time per-fighter LOD: tiny parts merged
into their parents, fewer joints and parts composed and drawn; hitboxes and
gameplay untouched) and skipping parts whose projected size is under about a
pixel. Declined: 15 Hz limb poses (fighter visual poses stay 30 Hz). Very-low
meshes get A/B captures for the owner.

## 9. The owner's compromise list, mapped

| Item | Where it lands | Weight for the gate |
|---|---|---|
| 1 Core performance model | A6 | Medium |
| 2 DS-native implementation | A1, A3, A4, A8 | High |
| 3 Numeric precision | A3/A5 whole chains, never leaves; D13: simulation math may go fixed point (digest re-baselined) | Medium-high |
| 4 Precomputation/baking | A1, A2, A4, A5 | High |
| 5 Runtime specialization | per-fighter/stage lists; per-stage collision and actors | Medium |
| 6 Reduced genericity | fixed pools and slot limits | Medium |
| 7 ROM philosophy | pre-expanded lists and pre-normalised clips | Medium |
| 8 RAM bandwidth/layout | A7 and every new kernel | High (64% of cycles are stall) |
| 9 Dynamic allocation | A4 pools | Medium (tail) |
| 10 Scene overlays | A7, funds A2 | High enabler |
| 11 ITCM/DTCM | A7 | High |
| 12 Hardware offload | GXFIFO DMA, texgen, BG/OAM | High |
| 13 ARM7 offload | A8 | Medium (tail) |
| 14 Render batching/lists | A1 | High |
| 15 Geometry compromises | A1 compiler | Low for CPU |
| 16 Fighter LOD | source hi/lo; very-low in A10 | Low (polygon RAM not binding) |
| 17 Skeletal animation | A3 | Medium, least certain |
| 18 Collision | A5 | Medium-high |
| 19 Event-driven systems | A4, A6 | Medium |
| 20 2D substitution | A10 (Dream Land backdrop), sprite particles | Low |
| 21 Texture compromises | A1 compiler, VRAM admission | Low for CPU |
| 22 Transparency/effects | A1 effects | Low-medium |
| 23 Lighting | GX lighting, on-change words | Low |
| 24 Particles | A1/A6 | Low-medium |
| 25-27 Audio assets/runtime/DSP | A8 | Low CPU; removes tail spikes |
| 28 Loading-time trade | A2, A4 | High |
| 29 ROM duplication | A1, A2 | Medium |
| 30 C/ARM assembly | A1/A5 kernels in ARM mode | Medium |
| 31 Branch/state specialization | A4 | Medium |

**Not in this plan:** a 30 Hz simulation (owner-refused 2026-09-16), a reduced
skeleton (the hurtbox and effect tables name the joints), leaf-by-leaf float
conversions (each f32<->Q edge costs 31-42 cycles), GX-side hierarchy composition,
and another cache layered on today's machinery.

## Evidence

- `artifacts/performance/2026-09-22_p2-2p8-architecture-baseline/` — bands,
  mechanism classes, tail symbols, working-set footprint; scripts to reproduce.
- `INVESTIGATION_RENDER.md`, `INVESTIGATION_RESIDENCY.md`, `INVESTIGATION_SIM.md`
  in the same folder: the read-only investigations behind A1-A8, file:line
  throughout; key figures re-checked against the tree before inclusion. Codex
  briefs for each phase start from the matching report's "RECOMMENDED FIRST SLICE".
- Prior artifacts cited inline: `2026-09-16_p2-2p8-*`, `2026-09-17_p2-2p8-*`,
  `2026-08-16_shdt-mechanism`, `2026-08-13_shdt-broadphase`,
  `2026-08-15_cfx-narrow-exchange`, `2026-08-15_gxstack-io-draw`.
