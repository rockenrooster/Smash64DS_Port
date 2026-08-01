# Handoff

Updated: 2026-07-31 (evening). **Restart surface only, capped at 200 lines** — durable detail goes to its
owning doc (board: queue + results; `PERF_LEDGER.md`; `KNOWN_ISSUES.md`; `TASK_STANDING_RULES.md`;
`docs/PORTING.md` for closed root causes).

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02, R2-05 | gated / complete |
| R2-03 | shipped E12/E28/E29/E46/**E32**/**E64b**/**E65**/**E67**/**E69** — 1,228,928 -> 1,096,768, DLDI-off so lower bounds; only the E32 flash residual is open (KNOWN_ISSUES) and it blocks no lever |
| R2-04 | loading + rate clauses done (E5/E6/E57); budget clause closed by E64b+E65 |
| R2-06 | closed — no lever left inside the phase; L6 found the one outside it |
| R2-07 | results flow **MEETS ITS GATE** (0.52x). **Successive matches work: four battle entries, zero freezes, owner-confirmed.** Particle/VFX/audio clauses are now the whole of `BUGS.md`. **L7 is the gate lever.** R2-08 needs the owner's retail play test |

## OPEN P1 #1 — the gate. `WORK-H` P95 is over 1,120,000 and L7 is the lever

**Last matched reading (2026-07-31, `git=cc5bc2ff967`, DLDI-on): P95 1,147,200, over by 27,200**, VBlank
2:382 3:76 4:9 5+:4, max 20, slips 0. L9 (SSB64's 1024-entry sine table), L10 (`NDS_R2_FIXED_SQRT` on)
and L9b (deleting L9's duplicate table) took **−85,248** together = 76% of the old gap.
**This reading predates today's three freeze fixes — re-baseline before pricing anything against it.**
The per-frame `func_80004AB0()` added to the battle present loop is ~8 stores plus one scene-light
emission, well under E11's ±5,376 cross-build floor, but it has not been measured.

**L6 named the lever and it is not loading.** An over-gate frame does **+510,390 cycles**, overshoots by
only 295,376 (147,688 ticks), and **66.2% of that premium is soft-float**: `gmCollisionSetInvertMatrix`
runs **34 times on every over frame, 0 on every clean one**, while
`ndsRendererAdapterBuildDObjLocalMatrix` is 1.06x — **render and animation are FLAT**, the relocation
walker 0.5%. **L2 stands: asset loads own only ~18% of over-gate frames** (5 of 28) — keep load
elimination for R2-04 §3.8 as correctness, never as the gate's answer.

**L7 = fixed-point `gm/gmcollision.c`, and it must be ALL of it.** Kernel **GREEN on its falsifier**
(0.016609 vs the 0.0200 bound, from 0.126987, via the `(p - t).R^-1` restructure that stops storing
`-t.R^-1`); ~238,000 cycles/frame ≈ 119,000 ticks against a 27,200 gap. Wiring is the hard half: the
kernel no longer produces a matrix, so `unk_dobjtrans_0x9C` and every consumer change together, and
compose+inverse alone save only ~187,794 of it (float compose 2,652 cyc/call vs 670.8 for the existing
20.12 one — **4.0x, not the 10x an ops model gives**). Needs an E64b-style equivalence bound; collision
decides hits. **The `#define` seam CANNOT reach it** — it renames the decomp definition and its internal
call sites together (why L9 worked and `func_ovl2_800ED490` will not); convert the `FTParts` matrix
cluster (~110 sites) or replace the entry points wholesale. **L8 REFUTED UNBUILT**, folds into L7.

## R2-07 L7 — arithmetic CLEARED on the real domain; only the wiring is left (2026-07-31)

The SwitchPlan's named next action for R2-07, and its blocker was never the code. Measured with
`NDS_R2_COLLISION_L7_ORACLE=1` (read-only, default off, 460 samples over one natural mode-163 match):
**real joint scale 1.1138–1.1199 — a single scale spanning 0.006** — and deviation **0.00049 / 0.00122 /
0.00513** world units at probe offsets of 1/4/16 against the **0.0200** bound, **0 over-bound, 0 singular**.
So the 0.25–2.00 sweep that reads 0.427738 is not a domain SSB64 visits, the gated 0.90–1.10 sweep is
centred slightly low (move it to 1.11–1.12), and the synthetic figure is **pessimistic** by 3×.
**Two traps recorded so the next cycle does not hit them.** The board's named hook,
`sNdsFighterPartsPool`, **is not linked in the shipping-shaped build** — 0 bytes in `build-tickhud`,
33,152 the instant anything references it, because `ndsFighterPartsSyncDObj` is eliminated there too. Fill
it and you fill an array nothing populates. And 33,152 bytes is **eight arena steps**: the first oracle
draft dropped the battle under the same 25 KiB GObj latch the particle runtime hits, *by adding an
instrument*. Walk `gGCCommonLinks[nGCCommonLinkIDFighter]` → `gcGetTreeDObjNext` → `ftGetParts` (+2,260
bytes) and **run `mapdiff` on any new lab flag before running the ROM**.

## OPEN P1 #2 — `BUGS.md` is now entirely particles, VFX and audio cues, and ALL of it is P1

Owner's phase clause: **"All rows in `BUGS.md` fixed. this is a P1 Bugs list and are required to be fixed
for P1"**, and (2026-07-31) **do the missing SFX/VFX before diagnosing the random freezes**. The rows split
in two. **SFX: the announcer is DONE and the generator DOES have a per-cue derivation mode** — the earlier
"extraction job per cue" estimate was wrong. `render-audio-fgm-phase-pack.py --derive <ids>` prints every
selector field straight from `fgm_ucd -> fgm_tbl -> B1_sounds2_ctl`; the walk was already in `build_pack`'s
attack lane and only lacked a flag. Seven lines packed (527 TIME UP, 488 GAME SET, 534 winner-is, 499
Mario, 486 Fox, 472/471 five/four), 56 → 63 cues, `MAX_PACK_BYTES` 512 → 768 KiB (a ROM budget on a
streamed NitroFS payload; the real bound is now `MAX_CUE_IMA_BYTES`). **Proof is the natural-match miss
ring, which the soak now prints by ID**: `96,85,153,472,471,621` → `96,85,153,621`, `PlayFailCount` 0,
NO-FREEZE to Results, Boundary + Latest green. **Every one of the four survivors is a LOOPED cue** and
needs FGM 285's `source_loop_ds_hardware` treatment; 96 `GroundGrind2` additionally has no `pitch` op, so
`validate_articulation` rejects it as written. The crowd row is separately blocked on the TRIGGER side —
`ftPublicMakeActor` only marks bits, so packing its cues today would be dead ROM.
**VFX = one blocker: the particle scripts do not run**, so every effect draws as one of four untextured
primitives. **BOTH PARTICLE BRANCHES ARE MERGED AND IN THE TREE — do not look for
worktrees**: generator + `nds_particle_banks.generated.{inc,h}` (byte-reproducing, 55/119 scripts, 23/47
textures, 82,752 B DS), `nds_particle_banks.c`, `battleship_lbparticle.c`, all in `CFILES`.
**`NDS_R2_PARTICLE_RUNTIME=1` BUILDS AND NOW BOOTS INTO THE BATTLE** — Dream Land renders with both
fighters and a live HUD at TIME 01:00 before it dies. The boot arena freeze is FIXED, and the fix was
**not** the lever the board named: `--gc-sections` had already discarded the 82,752 B of particle
textures (nothing references them), so moving them freed zero. **Check the `.map` before believing a size
claim about linked data nothing reads.** They went to NitroFS anyway — they become live the moment the
quad path references them, and there is no image room for them then. The lever that worked was
`sNdsTask39HitSparkPixels`, 22,528 B of `.rodata` read once into OBJ VRAM: arena
**1,245,184 → 1,269,760** (control 1,269,760 → 1,290,240), `MALLOCOVF=0`, Boundary green.
**Next blocker, sized to the byte and NOT an allocator overflow:** `ifCommonSetMaxNumGObj`
(`ifcommon.c:3156`) latches the GObj pool at the active count once the general heap has <25 KiB free. It
latched at **45/45** with **1,040 B free**; `ifCommonCountdownMakeInterface` asked for the 46th and wrote
through the NULL (`str r0,[r5,#132]`, the inlined `ifSetSObj`). Do not add a NULL check — the seam is the
heap. Levers, all measured: `efParticleInitAll` pools **28,320 B** (112x96 + 24x92 + 80x192, and
`StructsMax` was **0** — untouched); the bank arena copy **10,912 B** (only needed because the normalizer
byte-swaps in place — do it in the generator); float `printf` locale tables **~24,375 B**. Any two clear
it. **Break on `__excpt_entry`, not the frozen PC** — calico's handler double-faults and destroys the
context. Only then price it against the **clean-frame** margin (P50 974,080 / P95 1,056,640), round-robin
a quarter of the generators per frame rather than batching. **The DS quad draw path is still unbuilt** —
`gNdsParticleDrawSeamCount` is the "effects ran, nothing drew" signal.

## SUCCESSIVE MATCHES: FIXED — four defects, one law (write-up in `docs/PORTING.md` + board row)

**Every one was state that outlived a scene boundary the arena rewinds** — prepared-run cache keyed on a
config pointer; texture VRAM with no owner; the source DL heads never rewound in the battle loop (48 bytes
past a 60 KiB buffer → `while (TRUE);` ~8 s into match two); `sMNVSResultsFighterGObjs` trusted across a
Results re-entry (ARM9 data abort at match two's GAME SET).
Evidence: SD lane three bit-identical runs (`STG` 169,536, 28.0 FPS, pond textured); the **same-binary
control arm `-NoTexVramReset` still reproduces it** (BuildCount 92, `STG` 3,148,992, 4.2 FPS, white pond);
rematch lane four entries / three Results / NO-FREEZE, owner-confirmed. **Boundary AND Latest green on
`0e5e8a3`.** Guards: `gNdsRendererSceneTextureVramResetCount` one per battle entry;
`gNdsR2StagePrepareBuildCount` two per entry with ReuseCount rising. **Owed: the owner's eye check.**

## ANNOUNCEMENTS: TIME UP and GAME SET both work — three defects in series (owner-confirmed)

Each one hid the next, which is why the row read as "nothing happens". (1) `sIFCommonBattlePlace` was
never initialised — nothing called the source's `ifCommonBattleInitPlacement` — so `--place == 0` could
never be true and **no VS match had ever announced GAME SET**, which also withheld `game_status = Set`
and therefore Results. (2) The nine blue letters had no sprite descriptors, so they kept the blanket
endian pass's swapped fields (read off `assets/us/relocData/82.vpk0.bin` on the host; the two earlier gdb
attempts were unnecessary). (3) The update proc that announcement installs dereferences
`gEFParticleStructsGObj`/`gEFParticleGeneratorsGObj` with **no NULL check** (`ifcommon.c:2609`), and both
are NULL while the particle runtime is off — a write through address 0, which crashed the game the moment
(1) let the announcement run. TIME UP was spared only because it installs the BONUS update proc.
Fixed with a zeroed placeholder (`ndsEFParticleEnsureGObjPlaceholders`, scoped to the runtime being off).
**Proof:** `sudden-death/2026-07-31_165338-timeup-frame20.png` (TIME UP on screen) and the owner watching
a live run reach GAME SET and Results. The `-CaptureAnnounce`/`-CaptureGameSet` switches break on the
announcement's own constructor and then step frames — a wall-clock watch cannot catch a 90-tick window
and two missed it. **A halted-core screenshot can show a stale buffer**: the GAME SET stills looked empty
while the owner saw the text live, so trust the live watch for announcement pictures.

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
  frame (5-minute cap ended the emulator at match two's hand-off — ceiling is 7 minutes now,
  `-PressStartEverySeconds` drives successive entries); two Sudden Death watches missed the 90-tick
  announcement window entirely. Break on the event and step frames instead (`-CaptureAnnounce`).
- **Identical source is not an identical binary**: two HEAD controls differed by P95 +5,376, ±1
  over-gate frame. Always run the matched control. **DLDI-on costs ~29,696 P95 and is the honest config.**
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
deletes it — read the source's state instead).
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
