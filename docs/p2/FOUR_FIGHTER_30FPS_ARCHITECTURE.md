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
    doctrine.
- **Visual.** Synchronised A/B captures and crops of changed geometry, with the
  owner as oracle.
- **Memory.** Heap low-water, arena and pool high-waters per roster, at GO and at
  the worst tick.

## 6. Delivery sequence

Each phase is one coherent Codex batch: one integrator, measured once at the end.
A phase that misses its estimate by more than half stops for re-planning.
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
  reshape Phase 1: (1) a **re-record episode** (r49 re-records a tinted packet
  whenever its prim changes; a fighter whose prim changes every frame re-runs
  production and re-resolves its textures, ~1.16M/frame, for 246 frames) owns
  P95 -- without it P95 is 2.64M; the tint tile set itself is stable (9 builds,
  0 evictions per match); (2) **native failures at
  match start** (Link AppearL, texture could not be bound). Replay digest
  proven (deterministic across builds; an item-rate poke diverges). Owed:
  shipping-config heap census (before Phase 3), MF experiment result.

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
- **The shipping build's four-fighter heap has never been measured** (A7).

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

## 9. The owner's compromise list, mapped

| Item | Where it lands | Weight for the gate |
|---|---|---|
| 1 Core performance model | A6 | Medium |
| 2 DS-native implementation | A1, A3, A4, A8 | High |
| 3 Numeric precision | A3/A5 whole chains, never leaves | Medium-high |
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
