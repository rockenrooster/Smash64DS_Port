# Smash64DS Main-RAM Recovery and Cache-Residency Plan

## Goal

Recover enough Nintendo DS main RAM to turn the battle runtime into a bounded,
pre-resident working set instead of paying gameplay-time asset streaming costs.

Immediate target:

- recover **at least 96 KiB** of persistent main-RAM footprint;
- prefer **128 KiB or more** if correctness-preserving candidates exist;
- preserve a **minimum 32 KiB general-heap safety floor** throughout the full
  one-minute Mario-vs-Fox Dream Land Boundary match;
- convert recovered RAM into taskman/general-heap capacity and then into enough
  animation residency to eliminate gameplay-time animation ROM reads;
- re-profile after streaming is gone and let the new measured P95 owner choose
  the next CPU optimization.

Do not solve this by merely increasing heap/cache constants until the ROM barely
boots. Remove or overlap unnecessary resident memory, prove the bytes are really
available at runtime, then spend them deliberately.

Owner performs final playtest verification.

---

## Starting snapshot

Treat these as the starting snapshot, not eternal constants. Re-measure on current
HEAD before making acceptance claims.

### c118 gate-build static image

- main-RAM static/reserved image reaches approximately **2,691,696 bytes**;
- .main.bss: **1,623,824 bytes**;
- writable initialized data: **136,996 bytes**;
- remaining address space after the static image: approximately **1,502,608
  bytes**;
- taskman currently obtains **0x130000 / 1,245,184 bytes**;
- taskman wants up to **0x150000 / 1,376,256 bytes**;
- runtime is therefore already **128 KiB short** of the desired taskman arena.

The non-HUD proof build is roughly 33 KiB smaller statically. Keep profiling
instrumentation overhead separate from shipping RAM decisions.

### Current runtime pressure

Latest measured values before this plan:

- general-heap low-water: approximately **42,136 bytes**;
- required safety reserve: **32,768 bytes**;
- discretionary low-water slack: only approximately **9,368 bytes**;
- animation cache arena: **200,704 bytes**;
- animation cache used: **200,400 bytes**;
- animation cache internal free: **304 bytes**;
- animation cache rejects: **38**;
- missing animation working set was estimated at roughly **82 KiB**.

The 82 KiB figure must be re-derived from current HEAD before final sizing. Do not
hard-code an old estimate into a permanent cache size.

### Largest current .bss owners

| object | bytes | current interpretation |
|---|---:|---|
| gSYFramebufferSets | 441,600 | live source-era framebuffer storage; biggest architectural target |
| sNdsAudioFgmCache | 204,800 | live audio cache; do not shrink casually |
| sNdsRelocSceneFileBuffer | 185,696 | live battle asset staging; already optimized once |
| sOriginalSpritePreview | 153,600 | live |
| sOriginalSpriteDisplayPreview | 153,600 | live |
| sNdsRendererHardwareTextureScratch | 32,768 | scratch/lifetime-sharing candidate |
| sNdsRendererTask36ReplayOwner | 30,944 | lifetime/engagement candidate |
| sNdsRelocLoadedFiles | 29,184 | live relocation metadata |
| sNdsFighterDLAllDrawStates | 27,136 | live fighter state |
| sNdsAudioBgmBuffers | 16,392 | live BGM storage |
| sNdsRendererHardwareTextureRefreshLarge | 16,384 | scratch/lifetime-sharing candidate |
| sOriginalDLPreview | 13,824 | known battle-inactive candidate; needs final proof |
| sSYMainThread5Stack | 13,312 | stack; measure before touching |
| sNdsRendererAdapterNativeOwnerMaterials | 12,800 | renderer working set |
| sNdsFighterStructPool | 12,048 | fighter pool |
| sNdsRendererAdapterNativeStageWorkspace | 11,680 | renderer working set |
| sNdsRendererHardwareTextureKeyPool | 10,620 | texture metadata |
| sBattleTickHudRing | 10,240 | profiling-only; never count as shipping recovery |
| sNdsAObjEvent32Normalized | 8,192 | animation working data |
| sOriginalDLDisplayPreview | 7,776 | known battle-inactive candidate; needs final proof |

The top five BSS objects total **1,139,296 bytes**, about 70% of current BSS.
Prefer a few architectural wins over hundreds of tiny cuts.

---

## Non-negotiable invariants

1. **No fidelity regression hidden as RAM optimization.** Gameplay, collision,
   rules, camera meaning, results flow, VFX/SFX telegraphs, and recognizable SSB64
   presentation remain correct under PROJECT_GOAL.md.
2. **No new gameplay-time general-heap dependency.** RAM recovery must move the
   runtime toward a fixed/bounded post-setup memory state, not create allocator
   churn.
3. **Keep at least 32 KiB general-heap low-water.** Do not consume the safety
   reserve to make a cache benchmark pass.
4. **Do not simply raise NDS_TASKMAN_ARENA_SIZE or the animation-cache size and
   call the job done.** Prove available bytes first.
5. **Do not globally return NULL from syMallocSet as an OOM strategy.** Legacy
   callers still exist that assume successful allocation.
6. **Do not delete a buffer based on its name.** Prove writers, readers, lifetime,
   scene/configuration reachability, and pointer arithmetic that depends on its
   address or extent.
7. **Do not count profiling-only BSS as shipping recovery.** The tick-HUD ring and
   similar instrumentation explain profiling overhead but are not product wins.
8. **No concurrent ROM builds.** Let the Makefile choose parallelism; never pass
   -j and never override MAKEFLAGS.
9. **Use CodeGraph before grep/read for code relationships.** Inspect BattleShip,
   decomp/sm64-nds, and decomp/sm64ds-decomp before substantial DS memory,
   framebuffer, or renderer architecture changes.
10. **Preserve unrelated dirty-tree work.** This plan is RAM-focused.

---

## Definition of success

This campaign is complete only when all are true:

- [ ] current shipping/proof configuration recovers **>= 96 KiB** of persistent
      main-RAM footprint relative to the re-banked starting point;
- [ ] stretch: recover **>= 128 KiB** if correctness-preserving candidates exist;
- [ ] taskman arena growth is observed in the real ROM, not inferred only from
      linker bytes;
- [ ] general-heap low-water remains **>= 32 KiB** over the one-minute Boundary
      match, preferably with additional margin;
- [ ] animation residency is re-sized from measured current working-set demand;
- [ ] gameplay-time animation cache rejects are **0** for Mario/Fox Dream Land;
- [ ] gameplay-time animation ROM/FAT reads after battle setup are **0**;
- [ ] no new resource-exhaustion spins/freezes are introduced;
- [ ] Results transition still works;
- [ ] visible battle output remains correct;
- [ ] current both-CPU 1600-frame WORK-H P95 is re-banked after streaming is gone;
- [ ] the next CPU target is selected from the new measured P95 owner.

---

# Execution order

## Phase 0 — Re-bank RAM before touching anything

### 0.1 Record build identity and static image

On current HEAD:

1. Run the normal repo startup commands from AGENTS.md.
2. Record git HEAD and dirty paths.
3. Use the current Boundary/proof build matching the shipping battle
   configuration; do not rely on a stale c118 ELF if HEAD has moved.
4. Record linker section sizes:
   - .main
   - .main.rw
   - .main.bss
   - .itcm
   - .dtcm
   - .dtcm.bss
   - fake_heap_start / __heap_start_ntr
5. Generate a sorted symbol-size list for BSS/RW objects.
6. Store concise permanent evidence in the owning artifact/doc. Do not dump raw
   symbol output into HANDOFF.md.

### 0.2 Record runtime allocation state

Using the current one-minute battle ROM, read at minimum:

- gNdsTaskmanArenaChosenSize
- gNdsTaskmanArenaAllocFailCount
- gNdsTaskmanGeneralHeapFreeMin
- gNdsR2AnimCacheArenaReservedBytes
- gNdsR2AnimCacheArenaUsedBytes
- gNdsR2AnimCacheArenaOverflows
- gNdsR2AnimCacheRejects
- graphics/DL overflow counters
- objman panic counters/mask
- general-heap overflow counters

If a needed counter is absent, prefer an existing diagnostic reader or one cheap
persistent counter over a large census buffer.

### 0.3 Re-derive the animation working-set deficit

Do not carry forward “82 KiB” blindly.

For every animation request in the one-minute Boundary match classify:

- asset/file ID;
- requested bytes and aligned bytes;
- resident hit;
- initial miss that becomes resident;
- rejection because the arena is full;
- repeated request of an already rejected asset;
- first frame/status that requests it.

Produce:

- total unique animation bytes actually needed by the match;
- bytes resident at GO;
- bytes first demanded after GO;
- bytes repeatedly streamed because they could not become resident;
- exact cache size needed for zero rejects with alignment/metadata;
- a small safety margin justified by data.

Call that measured requirement ANIM_REQUIRED_BYTES.

**Phase-0 exit:** one current, reproducible baseline exists for static bytes,
taskman arena, heap low-water, and animation residency demand.

---

## Phase 1 — Recover the known battle-inactive DL preview pair

Targets:

- sOriginalDLPreview — 13,824 bytes
- sOriginalDLDisplayPreview — 7,776 bytes
- combined opportunity — **21,600 bytes**

They were previously observed unpopulated during battle, but a ready flag at one
instant is not lifetime-wide proof.

### 1.1 Prove configuration-level non-use

Trace with CodeGraph:

- declarations;
- every writer;
- every reader;
- ready flags;
- addresses passed through generic helpers;
- aliasing through preview pointers;
- startup, battle, pause, KO/respawn, end-of-match, Results handoff, rematch, and
  Sudden Death paths.

Required proof:

> In the battle/proof configuration, no reachable path reads or writes these
> arrays before they are compiled out.

If another scene/configuration needs them, compile storage only for that owner
rather than deleting the feature globally.

### 1.2 Remove their battle BSS

Preferred implementations, in order:

1. compile-time configuration guard excluding the arrays and dead battle path;
2. move storage into the scene/configuration that owns it;
3. lifetime-overlay into an existing scratch arena only if compile-time exclusion
   is impossible and non-overlap is mechanically proven.

Do not replace them with heap allocation.

### 1.3 Verify

Prove:

- .main.bss decreases by approximately 21,600 bytes minus small metadata;
- battle startup succeeds;
- taskman chosen arena rises if allocator headroom permits;
- Results handoff still works;
- no preview/readiness path unexpectedly engages.

**KEEP gate:** >=20 KiB real static recovery with zero behavior regression.

---

## Phase 2 — Re-architect gSYFramebufferSets instead of accepting 441.6 KiB

This is the highest-value RAM target.

Current object:

**gSYFramebufferSets[3][230][320] as 16-bit pixels = 441,600 bytes.**

It is live because the imported transition/Results path references
gSYSchedulerCurrentFramebuffer, and the DS compatibility seam can point that at
gSYFramebufferSets[0]. Therefore **do not delete it outright**.

The correct question is:

> What pixel history does the DS Results/photo-wipe path actually require, and
> what is the smallest DS-native representation that supplies exactly that
> history?

### 2.1 Trace the complete consumer contract

Use CodeGraph first. Inspect:

- gSYFramebufferSets
- gSYSchedulerCurrentFramebuffer
- lbtransition.c consumers
- src/import/battleship_lbtransition.c
- scene-manager clear/init logic
- Results/photo-wipe setup and draw
- any three-buffer index/rotation semantic
- raw dimensions/stride assumptions.

Also inspect corresponding framebuffer/capture architecture in:

- decomp/sm64-nds
- decomp/sm64ds-decomp

Use those only to validate DS-native ownership choices; do not copy blindly.

### 2.2 Determine what the current DS path actually contains

Instrument at scene boundaries, not every frame.

Answer:

1. Which of the three buffers are ever read after battle starts?
2. Which are ever written with meaningful pixels rather than the initial clear?
3. Does Results need the immediately previous displayed frame, one fixed battle-end
   snapshot, multiple historical frames, or merely a compatibility pointer?
4. What exact dimensions and stride does the transition consumer read?
5. Can one DS-native capture happen once at the battle-to-Results boundary?
6. Can the transition consume a 256x192 representation directly?

Capture the existing Results transition as visibility evidence before changing it.

### 2.3 Candidate implementations, cheapest first

#### Candidate A — One source-format buffer

If only one historical frame is required:

- current: 441,600 B
- one 320x230x16 buffer: 147,200 B
- theoretical recovery: **294,400 B**

This alone exceeds the campaign target.

#### Candidate B — One DS-screen-sized RGB555 snapshot

If only displayed DS pixels matter:

- 256 x 192 x 2 = **98,304 B**
- theoretical recovery: **343,296 B**

Only choose this if scaling/cropping and Results fidelity remain correct.

#### Candidate C — Hardware/VRAM-owned snapshot

If a VRAM bank can safely retain the needed snapshot without conflicting with
battle/Results textures, investigate moving pixel ownership out of main RAM. This
requires a complete VRAM-bank ownership/lifetime audit and is not automatically
better than A or B.

#### Candidate D — Remove a dummy compatibility surface

If the current buffer is never populated with meaningful battle pixels, replace
the N64-era dependency at the transition seam instead of preserving a huge dummy
surface.

Do not accept a visibly broken/black wipe simply because the old compatibility
path also happened to sample cleared memory.

### 2.4 Verification gate

- Results transition screenshot remains acceptable and correctly timed;
- no out-of-bounds pointer arithmetic remains;
- battle and Results both enter/exit correctly;
- rematch/re-entry is unaffected;
- .main.bss recovery is measured;
- taskman arena increase is measured;
- no new per-frame copies are introduced.

Attempt this before micro-optimizing dozens of tiny buffers.

---

## Phase 3 — Remove one copy from the 307.2 KiB sprite-preview pair if possible

Targets:

- sOriginalSpritePreview — 153,600 B
- sOriginalSpriteDisplayPreview — 153,600 B

Both have been observed live, so simple deletion is already refuted.

**MEASURED 2026-08-12 — this phase's premise is WRONG on the shipped build, and
the real opportunity is smaller but concrete.**

These are not a redundant pair on `NDS_RENDERER_HW_TRIANGLES` (what P1 ships).
`nds_platform.c:763` says so: *"HW consumes staging directly into BG VRAM and
owns no retained frame copy"*, and downscaling in place *"leaves the retained
display buffer free for immutable decoded sprite data"*. So
`sOriginalSpritePreview` is STAGING and `sOriginalSpriteDisplayPreview` is a
DECODE CACHE (hence `sOriginalSpriteDecodeCacheEpoch`) — two jobs, not two
copies. Both are read under `#if NDS_RENDERER_HW_TRIANGLES` (line 730; the
getter at 538). **Removing either is refuted; the ~150 KiB target is not
available.**

What IS available is reduction #4, with the extent measured rather than
guessed. Read on the gate arm, no build:

```
gNdsOriginalSpritePreviewDisplayWidth  256
gNdsOriginalSpritePreviewDisplayHeight 192
gNdsOriginalSpritePreviewCommitCount     2   (a WHOLE match)
```

Both arrays are `NDS_ORIGINAL_SPRITE_PREVIEW_MAX_WIDTH * ..._MAX_HEIGHT` =
320x240x2 = 153,600 B, and `ndsPlatformCommitOriginalSpritePreviewLayer`
**clamps `dst_w`/`dst_h` to `SCREEN_WIDTH`/`SCREEN_HEIGHT` before writing**, so
the destination can never exceed 256x192 = 98,304 B. That is **55,296 B of dead
space per buffer**.

**The gate before editing:** `ndsPlatformGetOriginalSpriteOverlayLayer` hands
the decode cache out advertising `*out_pitch = 320` and `*out_height = 240`, so
its consumer may legitimately write the full extent. Trace that consumer first;
shrinking the array without shrinking what the getter promises is a buffer
overrun, not a saving. Staging may genuinely need 320 wide — it is written at
SOURCE (N64) resolution before the clamp — so the decode cache is the honest
candidate, worth 55,296 B alone.

Incidental proof the software path really is compiled out: the harness rejected
`gNdsOriginalSpritePreviewDrawCount` as absent from the hwtri ELF, i.e.
`--gc-sections` dropped it — the same mechanism Phase 1 used for the DL pair.

### 3.1 Establish producer/consumer lifetime

Trace:

- who writes each buffer;
- who reads each buffer;
- whether one is only a staging copy;
- whether reads overlap writes;
- whether both are required in the same frame;
- whether one converts source/N64 presentation into final DS presentation;
- whether the final display copy can be produced directly into its true consumer.

Record each lifetime as:

**first-write -> last-read**.

### 3.2 Preferred reductions

1. direct producer-to-final-surface; eliminate intermediate storage;
2. single-buffer discipline only if producer/consumer cannot overlap;
3. lifetime overlay with another large scratch owner only with proven non-overlap;
4. smaller pixel format/dimensions only when visual quality is preserved and no
   runtime conversion is introduced.

Do not replace static double buffering with general-heap allocation each frame.

### 3.3 Gate

A successful one-copy design is worth about **150 KiB**. If both buffers are truly
simultaneous and necessary, document the proof and stop rather than force an
unsafe alias.

---

## Phase 4 — Build an explicit renderer scratch-lifetime map

Several medium fixed arrays may be separately sized for peaks that never overlap.
Do not individually shave constants before checking whether they can share one
arena.

Start with:

- sNdsRendererHardwareTextureScratch — 32,768 B
- sNdsRendererHardwareTextureRefreshLarge — 16,384 B
- sNdsRendererHardwareTextureRefreshSmall — 4,096 B
- sNdsRendererTask36ReplayOwner — 30,944 B
- native owner/stage workspaces
- fighter draw-plan/state temporary storage
- scene-only preview scratch proven not concurrent with battle renderer work.

### 4.1 For every candidate record

- owner function/subsystem;
- first write;
- last read;
- maximum bytes actually touched;
- alignment;
- whether DMA/GX can still consume it asynchronously;
- whether it survives across frames;
- scene/configuration;
- cache flush/invalidate requirements;
- whether fallback paths can use it in the same frame as another candidate.

### 4.2 Overlay only proven-disjoint lifetimes

If A and B are mutually exclusive, create one explicitly owned scratch arena with
compile-time size/alignment assertions and clear ownership transitions.

Do not use incompatible typed aliases or undefined behavior. Prefer byte/word
storage with aligned typed accessors.

### 4.3 Measure peak usage before shrinking capacities

For large fixed arrays with unused tails, add cheap high-water counters and run
the full Boundary match. Shrink only when:

- peak is measured;
- meaningful safety margin remains;
- overflow has a fail-soft policy;
- fault injection can prove failure behavior.

Target another **32-64 KiB** if the lifetime graph supports it; zero is acceptable
if the buffers are truly concurrent.

---

## Phase 5 — Audit taskman arena consumers and finite pools

Static recovery is only half the job. The 1.245 MiB taskman arena should be spent
on measured high-water, not source-era maximums.

### 5.1 Produce an absolute setup allocation ledger

The existing optional ledger warns that intra-TU taskman allocations can bypass
its wrapper. Do not report it as an absolute total without closing that gap.

Account for at least:

- SYTaskGfx / gfx-end / VI structs;
- both task contexts' DL buffers;
- per-context graphics heaps;
- RDP output compatibility allocation;
- GObj/DObj/MObj/AObj/XObj/CObj pools;
- effect pool;
- weapon pool;
- particle support;
- fighter figatree/struct heaps;
- reloc/collision allocations;
- animation cache arena;
- renderer auxiliary heaps.

For each publish:

**requested bytes | aligned bytes | lifetime | measured peak entries | reserve**

### 5.2 DL/graphics buffers

Current DS VSBattle rebudget is approximately:

- DL0: sizeof(Gfx) * 2048 = 16 KiB per context;
- DL1: sizeof(Gfx) * 512 = 4 KiB per context;
- 2 contexts;
- graphics arena: 0xD000 per context.

Do not cut from source constants alone. Read existing overflow/high-water counters
over the full match and identify actual peak plus safety margin.

Graphics heap previously had a real 16-byte/frame leak. Do not confuse a leak
with oversized capacity: monotonically growing usage is a bug, not a RAM budget.

### 5.3 Object/effect/weapon pools

For every finite pool:

- read peak live count;
- keep justified spare entries for same-frame overlap;
- ensure exhaustion returns failure/skips optional work rather than spinning;
- use fault-injection/smaller-capacity builds to prove the match continues.

Do not cut pools below gameplay-semantic needs merely to hit the RAM target.

---

## Phase 6 — Convert recovered RAM into animation residency

Do this only after Phases 0-5 produce real reclaimed bytes.

### 6.1 Recompute available budget

Define:

- STATIC_RECOVERED = measured decrease in shipping/proof static main-RAM image;
- TASKMAN_GAIN = measured increase in gNdsTaskmanArenaChosenSize;
- HEAP_LOW_WATER = full-match minimum after setup/runtime allocations;
- ANIM_REQUIRED_BYTES = measured zero-reject animation cache requirement.

Animation cache may grow only if:

**post-growth HEAP_LOW_WATER >= 32768 bytes.**

Prefer substantially more margin when available.

### 6.2 Make the match working set resident before GO

Desired architecture:

1. derive the exact Mario/Fox animation asset set reachable in the VS match;
2. preload/admit it during battle setup;
3. finish file reads, normalization/fixup, and allocation before gameplay;
4. seal residency at GO;
5. gameplay animation lookup becomes RAM-only;
6. unexpected post-GO requests produce diagnostics and an explicit correctness
   policy rather than silently streaming forever.

Prefer an exact match/fighter residency set over caching every animation in ROM.

### 6.3 Do not add LRU eviction unless exact residency truly cannot fit

An eviction cache is second choice because it allows file reads back into P95.

Only implement eviction if all are proven:

- exact reachable set cannot fit after reasonable RAM recovery;
- some residents are provably cold before others are needed;
- reload can happen outside gameplay or its cost is acceptable;
- eviction cannot invalidate live AObj/track pointers.

Otherwise keep the simpler invariant:

> Battle setup determines the complete working set; gameplay does not stream it.

### 6.4 Permanent low-cost diagnostics

Keep counters for:

- arena bytes reserved;
- arena bytes used/high-water;
- unique residents;
- rejects;
- post-GO load attempts;
- post-GO animation file bytes read;
- unexpected asset ID/first frame when preload coverage fails.

Boundary success is rejects=0 and post-GO animation file reads=0.

---

## Phase 7 — Enforce the no-gameplay-allocation/no-streaming contract

The RAM campaign should prevent regression permanently.

### 7.1 General heap becomes setup-only for battle

Add a clear runtime phase:

- PREPARE: setup allocations allowed;
- SEALED/GO: gameplay-reachable general-heap allocation forbidden except a tiny
  explicit allowlist that should trend to zero.

In diagnostic/verifier builds, a post-GO syTaskmanMalloc should record caller LR,
size, frame, and count. Do not add a production infinite loop.

### 7.2 Build-time/source checker

Add or extend a checker rejecting newly introduced gameplay-reachable:

- syTaskmanMalloc/general-heap allocation;
- filesystem reads in fighter animation/status gameplay paths;
- resource-exhaustion while(TRUE) / for(;;) loops outside the tiny intentional
  scheduler/idle allowlist.

Use an explicit setup-only whitelist rather than banning the allocator globally.

### 7.3 Fault injection

For each bounded arena/pool touched here, have one cheap fault path that reduces
capacity enough to trigger exhaustion and proves:

- diagnostic counter increments;
- optional spawn/effect/cache admission is rejected cleanly;
- no infinite ARM9 stall occurs;
- match continues or startup fails cleanly according to owner semantics.

This keeps the recurring resource-exhaustion/shield-freeze class from simply
moving to a different pool.

---

## Phase 8 — Final verification and CPU re-bank

### 8.1 RAM acceptance table

Publish before/after:

| metric | baseline | final | delta |
|---|---:|---:|---:|
| .main.bss | | | |
| .main.rw | | | |
| __heap_start_ntr | | | |
| taskman chosen arena | | | |
| taskman allocation probe failures | | | |
| general-heap low-water | | | |
| animation cache reserved | | | |
| animation cache used high-water | | | |
| animation rejects | | | |
| post-GO animation file reads | | | |
| objman panic count/mask | | | |
| graphics/DL overflow counts | | | |

### 8.2 Correctness coverage

At minimum verify:

- normal battle startup;
- Mario human vs level-3 Fox CPU on Dream Land;
- repeated shield impacts in both directions;
- Mario fireball and Fox projectile/effects;
- KO and respawn;
- pause/unpause when covered by Boundary capability;
- match timeout;
- Results transition/photo wipe;
- rematch/re-entry where currently supported;
- no resource-exhaustion freeze regression.

Owner performs final playtest verification; automated evidence still covers all
mechanically checkable invariants.

### 8.3 Performance re-bank

Once post-GO animation streaming is gone:

1. run the canonical one-minute both-CPU gate;
2. collect 1600 representative samples/ring dump;
3. report WORK-H P50/P95, not quantized ALL alone;
4. re-profile the top 5% frames;
5. compare SRC/MISC/FTR and the SINT subtree against pre-RAM baseline;
6. select the next CPU task from the new measured owner.

Do not assume ftComputerProcessAll collision is next simply because it was a
runner-up before streaming was removed.

---

# Coding-agent decision tree

Continue across successive work cycles without waiting for a new user prompt after
every slice:

1. **Re-bank current RAM.**
2. **Prove and remove/guard the 21.6 KiB DL preview pair.**
3. **Investigate gSYFramebufferSets fully.**
   - If one-buffer/DS-native replacement is correct, implement it and likely meet
     the whole RAM target immediately.
   - If not, record the exact measured reason and continue.
4. **Investigate the sprite-preview double buffer.**
   - If one copy can be deleted, implement it.
5. **Build the scratch lifetime graph and overlay/shrink only proven-disjoint
   storage.**
6. **Audit taskman/pool high-water for additional safe recovery.**
7. Stop RAM hunting when both are true:
   - enough real bytes exist for measured zero-reject animation residency; and
   - >=32 KiB heap safety floor remains after cache growth.
8. **Grow/preload/seal animation residency.**
9. **Prove zero post-GO animation streaming.**
10. **Re-bank P95 and move to the next measured CPU owner.**

Do not spend days shaving 1-2 KiB entries while an unrefuted 150-440 KiB
architectural target remains.

---

# Revert/stop rules

Immediately revert or stop a candidate when:

- visual output corrupts or Results changes incorrectly;
- a pointer/address contract becomes uncertain;
- an overflow/panic counter appears where baseline had none;
- heap low-water falls below 32 KiB;
- post-GO allocations/file reads increase;
- a RAM win increases CPU work enough to materially worsen the 1.12M P95 goal;
- a candidate requires runtime conversion that could be AOT;
- apparent recovery exists only in the tick-HUD build and not shipping/proof.

For a failed architectural candidate, record the measured reason once so another
agent does not repeat it.

---

# Commit/slice guidance

Keep slices separable enough to identify the byte source:

1. census/instrumentation only if needed;
2. DL preview configuration removal;
3. framebuffer architecture;
4. sprite-preview storage architecture;
5. scratch lifetime sharing/pool sizing;
6. animation residency enlargement/preload;
7. no-gameplay-allocation checker/guard;
8. final evidence/docs cleanup.

Each kept slice states:

- static bytes before/after;
- taskman chosen bytes before/after;
- heap low-water before/after;
- semantic/visual path tested;
- whether recovered bytes are already spent on a cache;
- whether post-GO animation file reads changed.

Do not mix unrelated CPU optimization into a RAM-recovery commit unless required
by the new DS-native memory representation.

---

## End state

The desired architecture is:

> **After battle setup, memory demand is fixed and bounded. Source-era software
> framebuffer/staging baggage that the DS does not need has been removed or
> replaced with DS-native ownership. The taskman arena has measured safety
> headroom. The complete Mario/Fox battle animation working set is resident before
> GO. Gameplay performs no animation ROM streaming and does not rely on the shared
> general heap. Every finite pool has measured capacity and a fail-soft policy.**

That condition turns RAM recovery into both a P95 performance win and a permanent
reduction in the resource-exhaustion freeze class.
