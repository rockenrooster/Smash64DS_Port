# Handoff

Updated: 2026-08-01. **Restart surface only, capped at 200 lines** — durable detail goes to its
owning doc (board: queue + results; `PERF_LEDGER.md`; `KNOWN_ISSUES.md`; `TASK_STANDING_RULES.md`;
`docs/PORTING.md` for closed root causes).

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02, R2-05 | gated / complete |
| R2-03 | shipped E12/E28/E29/E46/**E32**/**E64b**/**E65**/**E67**/**E69** — 1,228,928 -> 1,096,768, DLDI-off so lower bounds; only the E32 flash residual is open (KNOWN_ISSUES) and it blocks no lever |
| R2-04 | loading + rate clauses done (E5/E6/E57); budget clause closed by E64b+E65 |
| R2-06 | closed — no lever left inside the phase; L6 found the one outside it |
| R2-07 | results flow **MEETS ITS GATE** (0.52x). Successive matches work. **The particle interpreter is PROVEN CLEAN on the tick-HUD target** (NO-FREEZE, Violation 0, stage builds 2, 14 scripts, 138,274 visible particles, pools 41/64 and 8/12); the quad DRAW is the remaining half and six `BUGS.md` VFX rows wait on it. Crowd actor + 12 crowd/Results cues implemented. **L7 refuted — the gate has no named lever.** R2-08 needs the owner's retail play test |

## OPEN P1 #1 — the gate. Over by 88,960, and **there is no named lever**

**Measured at HEAD (`800a934`, DLDI-on): `WORK-H` P95 1,208,960, mean 973,484.** Two control runs came
back **bit-identical in every bucket**, so this harness has no run-to-run noise and the number is a
number, not a range. VBlank 2:461 3:91 4:10 5+:4, max 19, slips 0.

**The 1,147,200 / "over by 27,200" this file used to carry has no artifact behind it.** Every
128-sample run in `artifacts/performance` was read end to end: L9 control `2a53c061cd1` 1,281,856,
L10 `b06f16567dc` 1,232,448, HEAD 1,208,960. HEAD is 23,488 **better** than the last attested figure.
The real gap is **88,960**. (`r207-particles-128.json` reports P95 and mean identical to
`r207-L10-sqrt` to the digit under a different sha — same binary or same run relabelled, not an
independent point.)

**L7 IS REFUTED. Do not re-attempt it as "convert `gmCollisionSetInvertMatrix`".** It was wired,
measured and reverted (board: *R2-07 L7 WIRED, MEASURED, REVERTED*). The kernel won **534 cycles/frame**
in `SRC` and lost **6,481** in `FTR`+`STG`, which contain no collision code, and the loss scaled with the
code rather than the work: 2,332 bytes of ARM text cost +4,264 `FTR` mean, 1,840 cost +3,434 — **1.85
cycles/frame per byte, twice**. Engagement was proven (691 fills, 0 declines), so this is a real result
and not a wiring failure.

Three findings that constrain whatever comes next. **(1) Hot ARM text costs ~1.85 cycles/frame of `FTR`
mean per byte** — a lever must beat its own size. **(2) Soft float here is cheap**: 61 `__aeabi_*` calls
became 36 SMULLs, a hardware divide and 24 bit-twiddled conversions for **99 cycles per call**, not the
~800 assumed; "66.2% of the premium is soft-float" says where cycles ARE, not that fixed point is cheaper.
**(3) Wrapping is additive, only replacing is subtractive** — the float version stayed linked throughout.

So the next lever must **delete work, not relocate it**. If collision is tried again it has to be the
whole subsystem — `func_ovl2_800ED490` is the bigger half (228 instructions, 63 soft-float calls,
40x/frame, no divide and no cofactors) — with the decomp versions dropped rather than bypassed. There is
also a cheaper inverse on the measured domain: the joint matrix is a rotation scaled per row and the live
oracle measured all three scales as one value 1.114–1.120, so `R^-1[c][r] = M[r][c] / s_r^2` is three
reciprocals and nine multiplies, with `vec_scale` already computed. Needs an orthogonality guard.
**Kept:** `include/nds/nds_r2_collision_mtx.h` (0.000283 worst on the gated domain against 0.0200 — 68x,
and it beats the frame form everywhere because it reads the rotation block at 6.26 rather than 20.12) and
`scripts/check-r2-collision-mtx.ps1`, which did not exist despite the header claiming it for a week, and
which immediately caught a 160-world-unit overflow.

**The L7 oracle outlived the lever.** `NDS_R2_COLLISION_L7_ORACLE=1` (read-only, default off, 460 samples,
one natural match) measured **joint scale 1.1138–1.1199 — a single scale spanning 0.006**, which is what
makes the row-scaled-rotation inverse worth trying and says the 0.25–2.00 sweep is not a domain SSB64
visits. Two traps from building it: `sNdsFighterPartsPool` **is not linked in the shipping-shaped build**
(0 bytes, 33,152 the instant anything references it — fill it and you fill an array nothing populates),
and 33,152 bytes is **eight arena steps**, so the first draft dropped the battle under the 25 KiB GObj
latch *by adding an instrument*. Walk the live DObj tree instead (+2,260 bytes), and **run `mapdiff` on
any new lab flag before running the ROM**.

## OPEN P1 #2 — `BUGS.md` is now entirely particles, VFX and audio cues, and ALL of it is P1

Owner's phase clause: **"All rows in `BUGS.md` fixed. this is a P1 Bugs list and are required to be fixed
for P1"**, and (2026-07-31) **do the missing SFX/VFX before diagnosing the random freezes**.

**SFX is essentially done.** `render-audio-fgm-phase-pack.py --derive <ids>` prints every selector field
straight from `fgm_ucd -> fgm_tbl -> B1_sounds2_ctl`, and now also `source_pcm_samples`, the fork program
hashes and `render_program_sha256` — the three fields that used to be obtainable only by pasting a
placeholder and reading the generator's error. 56 → **75 cues**: seven announcer lines, then 621 PublicWin
(second cue on 626's wave, so 626's AOT loop-and-ramp render unchanged — a hardware repeat can never serve
either, because their articulation ramps volume *across* the loop) and the eleven the crowd actor reaches.
**"Eight of the twelve crowd cues are LOOPED" was wrong** — `--derive` says none of the eleven loops.
Three cues still miss on a natural run and each has a named obstacle (`BUGS.md`): 96 has no `pitch` op,
153 `AltitudeWarn` needs 285's hardware repeat, 85 derives ~90,510 Hz.
**The crowd TRIGGER side is implemented**: `ft/ftpublic.c` compiled in place
(`NDS_IMPORT_BATTLESHIP_FT_PUBLIC`, default 0) — its whole external surface already existed, so the
thresholds/cooldowns/repeats are the source's by construction. Owed: a build and an ear check.

**VFX — the interpreter is PROVEN CLEAN; the DRAW draws correctly and cannot ship yet.** Three tick-HUD
ROMs differing only in the particle flags, one soak each. Control and `RUNTIME=1` are indistinguishable
(NO-FREEZE, Violation 0, stage builds 2), and `RUNTIME=1` runs 14 scripts / 138,274 visible particles
inside its fixed pools. `DRAW=1` emits **90,165 quads with zero atlas misses**, NO-FREEZE, full match.
**Its tick cost is only ~10,000 (`MISC` 45,120 → 54,656 P50) and `WORK-H` P95 is 1,221,760 — better than
the control it was built from. The pacing is destroyed anyway: 196 of 566 frames at five or more VBlanks
against 4.** Read the histogram; a P95 alone calls this a win because the 128-sample window sits in a
quiet stretch.

**Fully attributed, both ends measured:** the atlas takes 32,768 B of texture VRAM →
`ndsRendererHardwareResolveStageSourceFrameTexture` fails about one frame in ten (reject **site 2, 196
times**) → `PrepareRun` FALSE → the native stage owner rejects → `r2_prepared_valid = 0` → 197 rebuilds,
each drawing that frame through the generic renderer. Every other key component and refusal site is 0.
**The fix is sized by the same run:** `gNdsParticleTextureUseMask` = `0x08400000`, so a live match draws
**three** source textures (22/23/26) while the atlas is built for the 16-texture static reachability set.
Admit by the measured set, and/or A3I5 at 8 bpp (16,384 B + a 32-entry palette).

**Two earlier `DRAW=1` failures are closed and are worth not repeating.** It aborted at the GO countdown
because `ifCommonSetMaxNumGObj` caps the GObj pool under 25 KiB free and the countdown dereferences the
NULL — the runtime alone leaves **1,176 bytes** of margin (`PORTING.md`, second occurrence). And its first
build wedged the geometry engine on a `glEnd()` the stream must not carry.
**Traps:** `--gc-sections` had already discarded the particle textures, so the board's named arena lever
freed zero — **check the `.map` before believing a size claim about linked data nothing reads**; and
**`__excpt_entry`'s park is a self-branch too**, so a CPU abort reads exactly like the allocator's
`while (TRUE);` — the soak separates the two verdicts now.
**A latch is not a counter.** `gNdsRendererTask36*RejectReason` are reset per prepare, so both read 0 on
the run that rebuilt 197 times; the counting versions found it in one soak.

## SUCCESSIVE MATCHES and the ANNOUNCEMENTS: both FIXED (full write-ups in `docs/PORTING.md` + board)

**Successive matches** were four defects with one law: state that outlived a scene boundary the arena
rewinds — prepared-run cache keyed on a config pointer; texture VRAM with no owner; the source DL heads
never rewound (48 bytes past a 60 KiB buffer → `while (TRUE);` ~8 s into match two); and
`sMNVSResultsFighterGObjs` trusted across a Results re-entry (data abort at match two's GAME SET). Four
entries, three Results, NO-FREEZE, owner-confirmed; **Boundary AND Latest green on `0e5e8a3`.** Guards:
`gNdsRendererSceneTextureVramResetCount` one per battle entry, `gNdsR2StagePrepareBuildCount` two.
**Owed: the owner's eye check.**

**TIME UP and GAME SET** were three defects in series, each hiding the next: `sIFCommonBattlePlace` was
never initialised so **no VS match had ever announced GAME SET** (which also withheld Results); the nine
blue letters had no sprite descriptors; and the announcement's update proc dereferences
`gEFParticleStructsGObj` with no NULL check (`ifcommon.c:2609`), so fixing the first crashed the game
until a zeroed placeholder landed. **A halted-core screenshot can show a stale buffer** — trust the live
watch, and break on the constructor (`-CaptureAnnounce`), because a wall-clock watch cannot catch a
90-tick window and two missed it.
**GAME SET's pitch (2026-08-01):** the pack took a cue's rate from `notes[0]` and pitch code 0 is a REST,
so FGM 488 played thirteen semitones low. The guard has to be external — the derivation had the same bug
the pack self-checks against — and now rejects any entry under 12,000 Hz.

## Freeze classes — TWO, with different fixes. Never say "the allocator" without the counter

`sys/malloc.c:30` is `while (TRUE);` (heap exhaustion, `MALLOCOVF`); `sys/taskman.c:338`/`:344` are two
more (a DL buffer past its end, and the graphics heap — `DLBUF0..3`/`GFXHEAP`). The soak prints all of
them. A third shape is not a spin at all: a jump into low memory (`0x00000e9a` as caller frame), seen
twice today. **Do NOT make `syTaskmanMalloc` return NULL globally** — callers do not check.
**Image size competes with the arena**: the boot `calloc` search steps down 4,096 at a time from
`0x150000` to a `0x130000` FLOOR that is a contract with the Task 36 replay guard
(`nds_renderer.h:124-134`) — **do not lower it**; `.text` costs arena as surely as `.bss` (see the
particle row). **`gNdsTaskmanArenaAllocFailCount` is that search's step count, a BOOT CONSTANT** (26,
or 32 with particles on) — not failed match allocations. It nearly bought a revert of a −49,408 P95 chain.

## Measurement traps that cost days — all now instrumented rather than remembered

- **A counter with no writer reads 0, which looks clean.** `gNdsRendererProfileTextureRejectReasonMask`
  is written only at profile level ≥2 or with the route probe; a shipping-build 0 is uninstrumented.
- **gdb aborts a command file on the first missing symbol, silently** — reads as an emulator hang. Both
  `capture-sudden-death-entry.ps1` and `soak-freeze-watch.ps1` now ask `nm` and strip every read the ELF
  does not define (the soak puts ~80 counters in ONE printf, so one absent symbol cost all of them).
- **A run that ends at the moment under investigation proves nothing**, and neither does a wall-clock
  watch aimed at a short window. Two rematch soaks read NO-FREEZE with the GAME SET zoom as their final
  frame (ceiling is 7 minutes now, `-PressStartEverySeconds` drives successive entries); two Sudden Death
  watches missed the 90-tick window. Break on the event and step frames (`-CaptureAnnounce`).
- **Identical source is not an identical binary** — always run the matched control. But the harness itself
  is deterministic: two control runs on the same ROM came back **bit-identical in every bucket**, so a
  cross-BUILD delta is real signal, not noise. **DLDI-on costs ~29,696 P95 and is the honest config.**
- **Never compare an anim-cache pair frame-by-frame** — it shifts load timing; order stats only.
- **The census window is a COMPILE-TIME constant**, so `-NoBuild` measures the last build's window.
- **`-Os` emits `blx __udivsi3` for a CONSTANT divisor**; `sinf`/`cosf` drag in `__ieee754_rem_pio2f` —
  SSB64 uses a table (L9), never reach for libm. **A lab ROM can differ in CODEGEN**: the `-marm` rule
  keys on harness ID, so add new lab IDs to `NDS_ARM_RENDERER_HARNESS_IDS`.
- **Compare captures with `scripts/compare-capture-pair.ps1`** (it crops past melonDS's title bar);
  **`addr2line` names deleted AND inlined functions**; **read the HUD before the picture**;
  **`-MatchedCapture` is broken** (own BUGS row); **`check-decomp-header-mirror.py` is RED on two
  pre-existing constants** — decomp is the specification and both edits want their own Latest run.

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
particle seam list** (it is the set Task 39 REPLACES); **the particle VRAM budget as a blocker** (a live
match draws two textures and 1,280 bytes).
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
