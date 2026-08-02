# Handoff

Updated: 2026-08-01. **Boundary green; published battle ROM + tick-HUD sibling rebuilt;
`smash64ds.nds` untouched (owner: not needed for P1).**
**THREE ROOT CAUSES CLOSED, all in the particle path, all one line each.**
**(1) KO FREEZE — one macro.** `LBPARTICLE_MASK_GENLINK` was `(link) << 16`; the decode is `bank_id >> 3`
into a SIXTEEN-entry array, so the burst wrote index 8192. `((link)+1)*8` is confirmed independently by
`efdisplay.c:79-98`, which gives each slot its own draw GObj and render mode.
**(2) HEADER `size` WAS NEVER BYTE-SWAPPED.** `ndsParticleNormalizeHeader` swapped nine of `LBScript`'s
TEN prefix words; the tenth is `size` at 0x2C. Unswapped `20.0f` is a positive DENORMAL of 5.7e-41 —
it passes `pc->size == 0.0F`, submits a real quad, costs no QuadMiss, and prints as `0.000000`. Every
particle sized by its header drew sub-pixel while every counter read clean. Confetti `maxsize`
0.000000 → **20.000000**; scripts that size in BYTECODE (`lbpSetSize*`) were always right, which is why
Whispy measured 260.0 and hid it. Guarded now by `_Static_assert` on `offsetof`.
**(3) FOUR LIVE SEAMS HAD NO PACKED SCRIPTS.** My own `ftParamMakeEffect` routing made DustLight,
DustHeavy, DustHeavyDouble and MusicNote live; the generator's seam list did not know. Rejects 49 → 0,
starts 137 → 226, roots 251 → 340 (+89 on both — every rejected start became a real one).
**The DS path also never applied the LBTransform**, and **`ftParamMakeEffect` now dispatches the
particle-only kinds to their source makers**. **Retract "a source effect costs ~5 DObjs".**
**The quad sheet is A5I3, not RGB555+A1** — one alpha bit made every soft particle a hard blob. One
byte per texel keeps the proven 8,192-byte allocation at 128x64: admission 14 → 23 of 47, misses
1,343 → 528, heap low-water +3,920. **Thirteen textures still off the sheet** — decision on the board.
**A finished FGM note now RELEASES instead of `soundKill`** — five cues were cut mid-waveform.
**Read `BUG_FIXING_PROCESS.md` v2 first**: source is the oracle, the owner is confirmation only, and a
build is spent to confirm a written prediction — never to see whether it looks right. Restart surface
only; durable detail belongs to its owning doc (board, `PERF_LEDGER`, `KNOWN_ISSUES`, `PORTING`).

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02, R2-05 | gated / complete |
| R2-03 | shipped E12/E28/E29/E46/**E32**/**E64b**/**E65**/**E67**/**E69** — 1,228,928 -> 1,096,768, DLDI-off so lower bounds; only the E32 flash residual is open (KNOWN_ISSUES) and it blocks no lever |
| R2-04 | loading + rate clauses done (E5/E6/E57); budget clause closed by E64b+E65 |
| R2-06 | closed — no lever left inside the phase; L6 found the one outside it |
| R2-07 | results flow **MEETS ITS GATE** (0.52x). Successive matches work. **Particle interpreter, quad draw and Dream Land's own bank all DEFAULT ON and measured** — NO-FREEZE, stage builds 2, 110,976 of 110,976 particles drawn, reject ring empty. **Audio closed: 282/282, miss ring 0, 88 cues.** **Owed: the owner's eye and ear.** **L7 refuted — the gate has no named lever.** R2-08 needs the owner's retail play test |

## OPEN P1 #1 — the gate. Over by 137,280, and **there is no named lever**

**Uncapped baseline (`2236532`, DLDI-on, particles on, GObj cap gone): `WORK-H` P50 924,928,
P95 1,257,280**, `VBI 2:457 3:93 4:13 5+:4`, max 19, slips 0
(`artifacts/performance/r207-baseline-2026-08-01-nocap-128.json`). **+17,152 on the previous
1,240,128, all `SRC` +15,296 / `MISC` +2,496 — CONTENT, not regression**: the cap was refusing four
of eleven fireballs and Dream Land's bank drew nothing. Every earlier figure was measured with the
pool capped; do not optimize against them. **The gate is a RANK, so the comparator is the over-gate
COUNT: 18 of 128 frames exceed 1.12M** and P95 ≤ 1.12M needs six or fewer. Ranked per frame, `FTR`
and `STG` are **flat on the worst frames** (`FTR` is 1,536 *below* its clean median on the worst);
the excursion is `SRC` (+250K…+440K) and `MISC` (+77K…+120K), co-occurring. Counterfactuals: `SRC`
to its clean median → 18 → **1**; `MISC` → 18 → **12**. **`SRC` is the gate, `MISC` second,
`FTR`/`STG` not on the critical path.**
**`MISC` is not miscellaneous** — it is `DrawTicks − (FTR+STG+BG+HUD)` plus the flush
(`taskman_seam.c:5008`), i.e. the transient weapon/effect/particle DObj draw, which is still the
generic `ndsRendererScanList` route. Full per-frame table on the board.

**L7 IS REFUTED. Do not re-attempt it as "convert `gmCollisionSetInvertMatrix`".** Wired, measured and
reverted (board: *R2-07 L7 WIRED, MEASURED, REVERTED*): the kernel won **534 cycles/frame** in `SRC` and
lost **6,481** in `FTR`+`STG`, which contain no collision code, and the loss scaled with the code rather
than the work — **1.85 cycles/frame per byte, measured twice**. Engagement was proven (691 fills,
0 declines), so this is a real result and not a wiring failure.

Three findings that constrain whatever comes next. **(1) Hot ARM text costs ~1.85 cycles/frame of `FTR`
mean per byte** — a lever must beat its own size. **(2) Soft float here is cheap**: 61 `__aeabi_*` calls
became 36 SMULLs, a hardware divide and 24 conversions for **99 cycles per call**, not the ~800 assumed.
**(3) Wrapping is additive, only replacing is subtractive** — the float version stayed linked.
So the next lever must **delete work, not relocate it**. If collision is tried again it has to be the
whole subsystem — `func_ovl2_800ED490` is the bigger half (228 instructions, 63 soft-float calls,
40x/frame) — with the decomp versions dropped rather than bypassed. A cheaper inverse exists on the
measured domain (row-scaled rotation; board has the derivation). **Kept:**
`include/nds/nds_r2_collision_mtx.h` and `scripts/check-r2-collision-mtx.ps1`.

**The L7 oracle outlived the lever, and its answer is already recorded** in `nds_r2_collision_mtx.h`
(460 samples): **joint scale 1.1138–1.1199, spanning 0.006**. **Do not re-run it** — rebuilding it
aborted the ROM at the GO countdown (`GENERALFREE 20272`, `MALLOCOVF 0`): its `.text` alone is more
arena than the tree has spare. **Arena margin is the budget line for anything that adds code** — read
`gNdsTaskmanGeneralHeapFreeMin` (29,328 today, latch at 25,600) before adding any. Naming an unused
global in a list KEEPS IT LINKED: referencing all 50 EFDescs cost 8,192 arena bytes and latched the cap.

## OPEN P1 #2 — `BUGS.md` is now entirely particles, VFX and audio cues, and ALL of it is P1

Owner's phase clause: **"All rows in `BUGS.md` fixed. this is a P1 Bugs list and are required to be fixed
for P1"**, and (2026-07-31) **do the missing SFX/VFX before diagnosing the random freezes**.

**SFX IS DONE for a both-CPU match: 282 play calls, 282 supported, MISS RING 0.**
`render-audio-fgm-phase-pack.py --derive <ids>` prints every selector field straight from
`fgm_ucd -> fgm_tbl -> B1_sounds2_ctl`, so authoring a cue is transcription. 56 → **88 cues** (the
board lists which). **A cue can be internally source-exact and still wrong in the mix** — that is the
open escape-roll row, and it is the contract dimension the first pass missed.
**85's `source_rate_above_u16` was never a hardware limit** — 90,510 Hz is fine for the channel timer,
too big only for the `u16` in *our* pack entry, so it renders full-program AOT at 32,000. Modulator
rules, from the decomp: **target 24+ is cross-mod ANOTHER voice**, and **shapes 6/7 are 2/3 with the
phase CLAMPED at the period** (`n_env.c:4158`/`:4172`); 4/5/8 call `randFloat*` and stay unsupported.
**The crowd TRIGGER side BUILDS, RUNS and SHIPS ON** — `ft/ftpublic.c` compiled in place
(`NDS_IMPORT_BATTLESHIP_FT_PUBLIC`), 3,332 B paid for by the weapon pool, `ActorMakeCount 1`. **Five of
its seven counters CANNOT fire**: the `#define` seam renames intra-TU references, so the actor registers
the inner proc and the counter-carrying wrappers are gc'd — read the source's own statics instead. That
is the "wrap a decomp function to count its INTERNAL callers" refutation, hit again.

**VFX — the interpreter is PROVEN CLEAN and the DRAW now WORKS.** **8,192 BYTES is the measured hard
bound** on the atlas ALLOCATION, not on its texels — 16,384, 32,768 and a second page each made
`ndsRendererHardwareResolveStageSourceFrameTexture` fail ~1 frame in 10 → 197 stage rebuilds. Every
pinned number and the argument for it is in `check-nds-particle-banks.ps1`, which `verify-all.ps1` now
runs itself (5.8 s) because the A5I3 conversion shipped with all of them stale.
**Dream Land's bank is packed and drawing** — reject ring empty, 3,741 strided draws. `efParticleInitAll`
resets `sEFParticleBanksNum`, so `EFCommonID` and `PupupuID` both read **0** and every common particle
took Dream Land's stride — key on `sEFParticleScriptBanks[slot]`, never a latched id.
**Fireball FIXED, and it is a MEMORY fix**: 704-byte `WPStruct` × 32 held `GENERALFREE` at 14,796, under
the 25,600 `ifCommonSetMaxNumGObj` threshold all match — cap latched at 47, four spawns refused. The pool
is **3** now (high-water one, measured twice), returning 20,416 B. **Oversized source pools are where the
arena margin is paid from.** **Do not read `sGCCommonsMaxNum` at end of run to decide whether that cap
latched** — not sticky across the scene change, so the Results sample always says -1. Read
`gNdsTaskmanGeneralHeapFreeMin` against 25,600 and the spawn-fail counters.

**Traps:** `--gc-sections` had already discarded the particle textures, so the board's named arena lever
freed zero — **check the `.map` before believing a size claim about linked data nothing reads**;
**`__excpt_entry`'s park is a self-branch too**, so a CPU abort reads like the allocator's
`while (TRUE);`; **a latch is not a counter**; **an allocator index something else can reset is not an
identity**; and **`pwsh`, never `powershell`** — `lib/melonds.ps1:349` is a PS7 ternary, so 5.1 makes
every script that sources it a parse error naming the innocent caller (`VERIFYING.md`).

## SUCCESSIVE MATCHES and the ANNOUNCEMENTS: both FIXED (full write-ups in `docs/PORTING.md` + board)

**Successive matches** were four defects with one law: state that outlived a scene boundary the arena
rewinds — prepared-run cache keyed on a config pointer; texture VRAM with no owner; the source DL heads
never rewound (48 B past a 60 KiB buffer → `while (TRUE);` ~8 s into match two);
`sMNVSResultsFighterGObjs` trusted across a re-entry. Four entries, three Results. **Owed: the
owner's eye check.**

**TIME UP and GAME SET** were three defects in series, each hiding the next: `sIFCommonBattlePlace` never
initialised so **no VS match had ever announced GAME SET** (which also withheld Results); the nine blue
letters had no sprite descriptors; the update proc dereferences `gEFParticleStructsGObj` with no NULL
check (`ifcommon.c:2609`). **A halted-core screenshot can show a stale buffer** — break on the
constructor (`-CaptureAnnounce`). **GAME SET's pitch:** the pack took a cue's rate from `notes[0]` and
pitch code 0 is a REST, so 488 played thirteen semitones low; the guard rejects anything under 12,000 Hz.

## Freeze classes — TWO, with different fixes. Never say "the allocator" without the counter

`sys/malloc.c:30` is `while (TRUE);` (heap exhaustion, `MALLOCOVF`); `sys/taskman.c:338`/`:344` are two
more (a DL buffer past its end, and the graphics heap — `DLBUF0..3`/`GFXHEAP`). The soak prints all of
them. A third shape is not a spin at all: a jump into low memory (`0x00000e9a` as caller frame), seen
twice today. **Do NOT make `syTaskmanMalloc` return NULL globally** — callers do not check.
**Image size competes with the arena**: the boot `calloc` search steps down 4,096 at a time from
`0x150000` to a `0x130000` FLOOR that is a contract with the Task 36 replay guard
(`nds_renderer.h:124-134`) — **do not lower it**; `.text` costs arena as surely as `.bss`.
**`gNdsTaskmanArenaAllocFailCount` is that search's step count, a BOOT CONSTANT** (26, or 32 with
particles on) — not failed match allocations. It nearly bought a revert of a −49,408 P95 chain.

## Measurement traps that cost days — all now instrumented rather than remembered

- **Adjacent stores are NOT an atomic publish.** Four volatile stores, VBlank IRQ between them, halted
  reader sees half of two samples (`FPS_HUD=299,14,15,17421760` twice). `REG_IME = 0` around the group.
  And **never run `verify-battle-mariofox-gcrunall-loop-harness.ps1` directly** — with `-NoBuild` it
  hangs (40 min, no emulator, no timeout). Go through `verify-all.ps1`.
- **A counter with no writer reads 0, which looks clean.** `gNdsRendererProfileTextureRejectReasonMask`
  is written only at profile level ≥2 or with the route probe; a shipping-build 0 is uninstrumented.
- **A value that prints `0.000000` but tests `!= 0` is a DENORMAL, i.e. a missed byte swap** — print
  `%e` or the raw bits. That is three days of "the effect is invisible but every counter is clean".
- **gdb aborts a command file on the first missing symbol, silently** — reads as an emulator hang. Both
  `capture-sudden-death-entry.ps1` and `soak-freeze-watch.ps1` now ask `nm` and strip every read the ELF
  does not define (the soak puts ~80 counters in ONE printf, so one absent symbol cost all of them).
- **A run that ends at the moment under investigation proves nothing.** Two rematch soaks read NO-FREEZE
  with the GAME SET zoom as their final frame (ceiling 7 min, `-PressStartEverySeconds` drives successive
  entries). Break on the event and step frames (`-CaptureAnnounce`).
- **Identical source is not an identical binary** — run the matched control. But the harness is deterministic: two control runs on one ROM came back **bit-identical in every bucket**, so a cross-BUILD delta is real signal. **DLDI-on costs ~29,696 P95 and is the honest config.**
- **Never compare an anim-cache pair frame-by-frame** (order stats only); **the census window is COMPILE-TIME**, so `-NoBuild` measures the last build's window.
- **`-Os` emits `blx __udivsi3` for a CONSTANT divisor**; `sinf`/`cosf` drag in `__ieee754_rem_pio2f` — SSB64 uses a table (L9), never reach for libm. **A lab ROM can differ in CODEGEN**: the `-marm` rule keys on harness ID, so add new lab IDs to `NDS_ARM_RENDERER_HARNESS_IDS`.
- **Compare captures with `scripts/compare-capture-pair.ps1`**; **`addr2line` names deleted AND inlined functions**; **read the HUD before the picture**; **`-MatchedCapture` is broken** (own BUGS row); **`check-decomp-header-mirror.py` is RED on two pre-existing constants** — not caused by new work.

## Refuted — do not re-derive (all by measurement; derivations on the board)

**E51** `line_id`; **E53** `{base,size}` mirror; **the flash as vertex data** (E48-E58); **the pose table**
(E61); **`.text.hot`** (E66); **E57** hitboxes walk the live joint chain; **R2-06 E7** fighter fallback +
Task 39; **R2-06 E6** the Horner fold; **pointer arrays as index arithmetic** (E11); **an AObj pool**
(E12); **R2b's transform double-apply**; **R1's loader AND arena framings**; **the OOM spin and the
DL-buffer overflow as the *Sudden Death* freeze** (they are the *rematch* freezes); **L8's unroll**; **the
atlas allocation-order theory**; **"115,277 B of arena spare"**; **wrapping a decomp function to count
its INTERNAL callers** (the rename bypasses the wrapper and `--gc-sections` deletes it — read the
source's state instead); **L7 as "convert `gmCollisionSetInvertMatrix`"** (wired,
measured, reverted: +6,481 cycles/frame of placement against a 534 win); **`census.SUBSTITUTES` as the
particle seam list**; **"a live match draws two textures"** (SINGLE-CPU mask; both-CPU is five); **"the
Whispy dust spawns in the wrong place"** (measured source-exact at both table entries, `-715`/`rotY π`
and `-205`/`rotY 0`, matching `lr_players` each time — whatever the owner is seeing is downstream).
**STOP ACCUMULATING SMALL LOAD-FRAME CUTS — E11 proves they cannot be banked**: real work removed,
negative bytes added, bit-identical load-frame set, yet P95 +15,744. Only a change clearing ~16,000, or
one that moves work off the frame, counts.

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary `battle_playable_realtime`, mode `163`.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List; git status --short
```

**Do not rebuild `smash64ds.nds`** (owner, 2026-07-28); **do rebuild the tick-HUD ROM whenever the
published one is** (owner, 2026-07-22), flag-identical. `-j`/`MAKEFLAGS` rules are in AGENTS.md
`## Builds`. A clean checkout must build through `build.ps1`, not bare `make`: four of six generated
`.inc` files are gitignored. Preserve canonical mode 163, renderer mode 9, mip 0, static textures,
source countdown, Dream Land water at frame 0, Task 16 `1/1/1`. Do not edit `decomp/`. Successive
matches: `soak-freeze-watch.ps1 -MinutesToRun 7 -PressStartSeconds 165 -PressStartCount 2
-PressStartEverySeconds 145 -PollSeconds 5 -IdenticalFramesToTrip 12`; announcements:
`capture-sudden-death-entry.ps1 -CaptureAnnounce 20` (TIME UP) / `-CaptureGameSet 2`.
