# Handoff

Updated: 2026-08-01, `9c30484`. **Boundary AND Latest both green; published battle ROM rebuilt
(11,922,432 B); tick-HUD parity 0 drift; `smash64ds.nds` untouched per the owner.**
**Restart surface only, capped at 200 lines** — durable detail goes to its owning doc (board: queue +
results; `PERF_LEDGER.md`; `KNOWN_ISSUES.md`; `TASK_STANDING_RULES.md`; `PORTING.md` for root causes).

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
1,240,128, all of it `SRC` +15,296 and `MISC` +2,496 — and it is CONTENT, not regression**: the
cap was refusing four of every eleven fireballs and Dream Land's bank drew nothing. Every figure
before this one was measured with the pool capped; do not optimize against them.

**The gate is a RANK, so the comparator is the over-gate COUNT: 18 of 128 frames exceed 1.12M** and
P95 ≤ 1.12M needs six or fewer. Ranked per frame, `FTR` and `STG` are **flat on the worst frames**
(`FTR` is 1,536 *below* its clean median on the worst frame); the excursion is `SRC` (+250K…+440K) and
`MISC` (+77K…+120K), co-occurring. Counterfactuals: `SRC` back to its clean median → 18 → **1**; `MISC`
→ 18 → **12**. **`SRC` is the gate, `MISC` second, `FTR`/`STG` not on the critical path.**
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
**(3) Wrapping is additive, only replacing is subtractive** — the float version stayed linked throughout.

So the next lever must **delete work, not relocate it**. If collision is tried again it has to be the
whole subsystem — `func_ovl2_800ED490` is the bigger half (228 instructions, 63 soft-float calls,
40x/frame) — with the decomp versions dropped rather than bypassed. A cheaper inverse exists on the
measured domain: the joint matrix is a rotation scaled per row, all three scales measure as one value
1.114–1.120, so `R^-1[c][r] = M[r][c] / s_r^2` is three reciprocals and nine multiplies with `vec_scale`
already computed. Needs an orthogonality guard. **Kept:**
`include/nds/nds_r2_collision_mtx.h` and `scripts/check-r2-collision-mtx.ps1`.

**The L7 oracle outlived the lever, and its answer is already recorded** in `nds_r2_collision_mtx.h`
(460 samples): **joint scale 1.1138–1.1199, a single scale spanning 0.006**, which is what makes the
row-scaled inverse worth trying. **Do not re-run it** — rebuilding it aborted the ROM at the GO countdown
(`GENERALFREE 20272` against the 25,600 latch, `MALLOCOVF 0`): its `.text` alone is more arena than the
tree has spare. **Arena margin is the budget line for anything that adds code** — read
`gNdsTaskmanGeneralHeapFreeMin` (26,876 today, latch at 25,600) before adding any.

## OPEN P1 #2 — `BUGS.md` is now entirely particles, VFX and audio cues, and ALL of it is P1

Owner's phase clause: **"All rows in `BUGS.md` fixed. this is a P1 Bugs list and are required to be fixed
for P1"**, and (2026-07-31) **do the missing SFX/VFX before diagnosing the random freezes**.

**SFX IS DONE for a both-CPU match: 282 play calls, 282 supported, MISS RING 0.**
`render-audio-fgm-phase-pack.py --derive <ids>` prints every selector field straight from
`fgm_ucd -> fgm_tbl -> B1_sounds2_ctl`, so authoring a cue is transcription. 56 → **88 cues**: seven
announcer lines, 621 PublicWin, the eleven the crowd actor reaches, 96/153/85, the five **only a
BOTH-CPU match reaches** (11 Escape, 13/14 GuardOn/Off, 278 GamePause, 369 FoxOttotto), and finally
271 Magnify + 368 FoxWin, then 18 LightSwingLw1 + 514 AnnounceSuddenDeath + 365 FoxSelected once every
fireball spawned and the match reached Sudden Death. Each fix uncovers the layer under it.
**85's `source_rate_above_u16` was never a hardware limit** — 90,510 Hz is fine for the channel timer and
too big only for the `u16` in *our* pack entry, so it renders full-program AOT at 32,000. Two modulator
findings, both from the decomp's own source: **target 24+ is cross-mod ANOTHER voice** (skipped before
evaluation, and only for a fork-free cue), and **shapes 6/7 are 2/3 with the phase CLAMPED at the period
rather than wrapped** (`n_env.c:4158`/`:4172`). Shapes 4/5/8 call `randFloat*` and stay unsupported.
**The crowd TRIGGER side BUILDS AND RUNS** — `ft/ftpublic.c` compiled in place
(`NDS_IMPORT_BATTLESHIP_FT_PUBLIC`), first ever build of that flag, 2026-08-01, NO-FREEZE to Results.
"Its whole external surface already existed" was half wrong: five declarations plus
`dFTCommonDataPublicFighterCallFGMs` (`ftcommondata.c` is not compiled here) had to be supplied.
**It RUNS** (`ActorMakeCount 2`, `CommonCheckCount 36`, seven-minute soak NO-FREEZE) and is **still
default 0 on a MEASURED margin**: `gNdsTaskmanGeneralHeapFreeMin` is 26,876 without it and 23,544 with,
so it costs 3,332 B and lands 2,056 under the 25,600 latch. Audio yields to gameplay per the sacrifice
order. **Five of its seven counters CANNOT fire**: the `#define` seam renames intra-TU references,
so the actor registers the inner proc and the counter-carrying wrappers are gc'd. Read the source's own
statics instead; the "wrap a decomp function to count its INTERNAL callers" refutation, hit again.

**VFX — the interpreter is PROVEN CLEAN and the DRAW now WORKS.** Four tick-HUD ROMs differing only in
the particle flags, one soak each; control and `RUNTIME=1` are indistinguishable. `DRAW=1` with the 32 KB
atlas put **196 of 566 frames at five or more VBlanks** against 4 while `WORK-H` P95 came back *better*
than its control — read the histogram, not the P95 alone.

**Attributed, all counted:** atlas resident →
`ndsRendererHardwareResolveStageSourceFrameTexture` fails ~1 frame in 10 (reject **site 2, 196 times**,
mask **4096 = TEXIMAGE**) → `PrepareRun` FALSE → owner rejects → 197 rebuilds, each frame generic.
**FIXED by one generator constant — the sheet is 64x64 = 8,192 B**, every symptom back to control
(`StagePrepareBuildCount` 2, reuse 2,041, reject site 2 **1**). **The draw costs ~10,100 ticks
(`MISC` P50) and five extra three-VBlank frames, and nothing else.**
**8,192 is a measured HARD BOUND, not a budget** (16,384 rejected exactly as 32,768 did).
**Dream Land's bank is packed and drawing (2026-08-01)** — reject ring empty, 3,741 strided draws —
and landing it broke the common draw twice over (board has the four-soak table). `efParticleInitAll`
resets `sEFParticleBanksNum`, so `EFCommonID` and `PupupuID` both read **0** and every common particle
took Dream Land's stride — key on `sEFParticleScriptBanks[slot]`, never a latched id. And
`QUAD_MEASURED_LIVE` was graded from a **single-CPU** mask; both-CPU is `0x08400007`. Admitted
`{0, 2, 22, 27, 64, 65, 66}`, 6,400 B, cells capped 16×16, **109,560 of 111,644 drawn**.
**Audio: 88 cues.** **Fireball FIXED, and it is a MEMORY fix**: 704-byte `WPStruct` × 32 = 22,528 B for
a pool whose P1 high-water is **one**, holding `GENERALFREE` at 14,796 under the 25,600
`ifCommonSetMaxNumGObj` threshold all match — cap latched at 47, four of eleven spawns refused.
`NDS_R2_WEAPON_POOL = 12` returns **14,080 B** → `SpawnCall 16 / SpawnSuccess 16`, both fail counters 0,
and that match reached **Sudden Death**, a first. **The arena margin is payable.**

**Traps:** `--gc-sections` had already discarded the particle textures, so the board's named arena lever
freed zero — **check the `.map` before believing a size claim about linked data nothing reads**;
**`__excpt_entry`'s park is a self-branch too**, so a CPU abort reads like the allocator's
`while (TRUE);`; **a latch is not a counter** (`gNdsRendererTask36*RejectReason` read 0 on the run that
rebuilt 197 times); and **an allocator index something else can reset is not an identity.**

## SUCCESSIVE MATCHES and the ANNOUNCEMENTS: both FIXED (full write-ups in `docs/PORTING.md` + board)

**Successive matches** were four defects with one law: state that outlived a scene boundary the arena
rewinds — prepared-run cache keyed on a config pointer; texture VRAM with no owner; the source DL heads
never rewound (48 B past a 60 KiB buffer → `while (TRUE);` ~8 s into match two); `sMNVSResultsFighterGObjs`
trusted across a Results re-entry. Four entries, three Results, NO-FREEZE, owner-confirmed. **Owed: the
owner's eye check.**

**TIME UP and GAME SET** were three defects in series, each hiding the next: `sIFCommonBattlePlace` never
initialised so **no VS match had ever announced GAME SET** (which also withheld Results); the nine blue
letters had no sprite descriptors; the update proc dereferences `gEFParticleStructsGObj` with no NULL
check (`ifcommon.c:2609`). **A halted-core screenshot can show a stale buffer** — break on the
constructor (`-CaptureAnnounce`). **GAME SET's pitch:** the pack took a cue's rate from `notes[0]` and
pitch code 0 is a REST, so 488 played thirteen semitones low; the guard is external now and rejects any
entry under 12,000 Hz.

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
**Oversized source pools are the payable half** — the weapon pool alone returned 14,080 B.

## Measurement traps that cost days — all now instrumented rather than remembered

- **Adjacent stores are NOT an atomic publish.** The FPS-HUD assertion sat on the board as "intermittent
  and unexplained" from R2-04 E2 until it went deterministic (`FPS_HUD=299,14,15,17421760` twice): four
  volatile stores, VBlank IRQ between them, halted reader sees half of two samples. `REG_IME = 0` around
  the group; two failures, then Boundary green. And **never run
  `verify-battle-mariofox-gcrunall-loop-harness.ps1` directly** — with `-NoBuild` it hangs (40 min,
  0.64 s CPU, no emulator, no timeout). Go through `verify-all.ps1`.
- **A counter with no writer reads 0, which looks clean.** `gNdsRendererProfileTextureRejectReasonMask`
  is written only at profile level ≥2 or with the route probe; a shipping-build 0 is uninstrumented.
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
atlas allocation-order theory**; **"115,277 B of arena spare" as the particle memory answer**; **wrapping
a decomp function to count its INTERNAL callers** (the rename bypasses the wrapper; `--gc-sections` then
deletes it — read the source's state instead); **L7 as "convert `gmCollisionSetInvertMatrix`"** (wired,
measured, reverted: +6,481 cycles/frame of placement against a 534 win); **`census.SUBSTITUTES` as the
particle seam list**; **"a live match draws two textures"** (that was a SINGLE-CPU mask; both-CPU is five).
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
