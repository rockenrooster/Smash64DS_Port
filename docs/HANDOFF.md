# Handoff

Updated: 2026-07-31 (evening). **Restart surface only, capped at 150 lines** — durable detail goes to its
owning doc (board: queue + results; `PERF_LEDGER.md`; `KNOWN_ISSUES.md`; `TASK_STANDING_RULES.md`;
`docs/PORTING.md` for closed root causes).

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02 | gated |
| R2-03 | shipped E12/E28/E29/E46/**E32**/**E64b**/**E65**/**E67**/**E69** — 1,228,928 -> 1,096,768, but DLDI-off and so lower bounds; only the E32 flash residual is open (KNOWN_ISSUES) and it blocks no lever |
| R2-04 | loading + rate clauses done (E5/E6/E57); budget clause closed by E64b+E65. R2-05 **COMPLETE** |
| R2-06 | closed — no lever left inside the phase; L6 found the one outside it |
| R2-07 | results flow **MEETS ITS GATE** (581,197 ticks/tic = 0.52x). **Successive matches now work: four battle entries, zero freezes (2026-07-31, owner-confirmed).** Particle/audio/HUD clauses untouched — they are now the whole of `BUGS.md`. **L7 (float collision) is the gate lever.** R2-08 needs the owner's retail play test |

## OPEN P1 #1 — the gate. `WORK-H` P95 is over 1,120,000 and L7 is the lever

**Last matched reading (2026-07-31, `git=cc5bc2ff967`, DLDI-on): P95 1,147,200, over by 27,200**, VBlank
2:382 3:76 4:9 5+:4, max 20, slips 0. L9 (SSB64's 1024-entry sine table), L10 (`NDS_R2_FIXED_SQRT` on)
and L9b (deleting L9's duplicate table) took **−85,248** together = 76% of the old gap.
**This reading predates today's three freeze fixes — re-baseline before pricing anything against it.**
The per-frame `func_80004AB0()` added to the battle present loop is ~8 stores plus one scene-light
emission, well under E11's ±5,376 cross-build floor, but it has not been measured.

**L6 named the lever and it is not loading.** An over-gate frame does **+510,390 cycles** of work,
overshoots by only 295,376 (147,688 ticks), and **66.2% of that premium is soft-float**.
`gmCollisionSetInvertMatrix` runs **34 times on every over frame and 0 on every clean one**;
`ndsRendererAdapterBuildDObjLocalMatrix` is 1.06x, so **render and animation are FLAT** and the
relocation walker is 0.5%. **L2 stands: asset loads own only ~18% of over-gate frames** (5 of 28) — keep
load elimination for R2-04 §3.8 as correctness, never as the gate's answer.

**L7 = fixed-point `gm/gmcollision.c`, and it must be ALL of it.** The kernel is **GREEN on its
falsifier** (0.016609 against the 0.0200 bound, from 0.126987, via the `(p - t).R^-1` restructure that
stops storing `-t.R^-1`); at ~238,000 cycles/frame it is worth ~119,000 ticks against a 27,200 gap.
Wiring is the hard half: the kernel no longer produces a matrix, so `unk_dobjtrans_0x9C` and every
consumer change together. Compose+inverse alone save only ~187,794 of it (the float compose is 2,652
cyc/call vs 670.8 for the existing 20.12 one — **4.0x, not the 10x an ops model gives**). Needs an
E64b-style equivalence bound; collision decides hits. **The `#define` seam CANNOT reach it** — it renames
the decomp definition and its internal call sites together (why L9 worked and `func_ovl2_800ED490` will
not); convert the `FTParts` matrix cluster (~110 sites) or replace the entry points wholesale.
**L8 REFUTED UNBUILT** (~12,000/frame, under E11's bar); folds into L7.

## OPEN P1 #2 — `BUGS.md` is now entirely particles, VFX and audio cues, and ALL of it is P1

The owner rewrote the phase clause: **"All rows in `BUGS.md` fixed. this is a P1 Bugs list and are
required to be fixed for P1."** Eight rows remain and seven are the same blocker: **the particle banks
are not ported**, so `lbParticleMakeScriptID` returns NULL and every effect draws as one of four
untextured 16-vertex primitives. Sized and it fits: 26 of 119 efcommon scripts naming 18 of 47 textures
= 129,768 B, plus 4,896 B for Dream Land's grpupupu bank, against 210,320 B measured arena headroom.
**PARTICLE-BANK WIP IS ON TWO BRANCHES, NOT IN ANY WORKTREE** — `worktree-agent-a15dedc9b2cf19349`
(generator + checker) and `worktree-agent-a8c9ad131bc0073b0` (`battleship_lbparticle.c`, runtime header,
`gbi.h`/shim). **Unreviewed, unbuilt; start from these, do not rewrite.** Price them against the
**clean-frame** margin (P50 974,080 / P95 1,056,640), round-robin a quarter of the generators per frame
rather than batching quarter-rate work onto one frame in four (that lowers the mean and raises P95), and
remember `NDS_R2_PARTICLE_RUNTIME=1` arms the `leaves_xf`/`dust_xf` per-scene null that is inert today.

## SUCCESSIVE MATCHES: FIXED — four defects, one law (full write-up in `docs/PORTING.md`)

**Every one was state that outlived a scene boundary the taskman arena rewinds.** (1) the stage
prepared-run cache keyed on a config POINTER; (2) texture VRAM had no owner, so entry two allocated into
entry one's pool shape and `glTexImage2D` refused a 4 KiB upload against 268,800 free bytes; (3) the
source display-list heads were never rewound in the battle loop, so `gSYTaskmanDLHeads[0]` ran 48 bytes
past a 60 KiB buffer and BattleShip's own `while (TRUE);` stopped the game ~8 s into match two; (4)
`sMNVSResultsFighterGObjs` was trusted across a Results re-entry and a dead GObj reached `gcMoveGObjDL`
(ARM9 in ABORT mode, `lr_usr` at `ftParamMoveDLLink+18`).
Evidence: SD lane three consecutive runs bit-identical (`STG` 169,536, 28.0 FPS, pond textured,
`artifacts/verification/sudden-death/2026-07-31_14{2938,3102,3140}`); **same-binary control arm
`-NoTexVramReset` still reproduces it** (BuildCount 92, `STG` 3,148,992, 4.2 FPS, white pond); rematch
lane four entries, reset count 4, violations 0, `STG` 169,408
(`artifacts/verification/freeze-soak/2026-07-31_151642-NO-FREEZE.png`).
Guards: `gNdsRendererSceneTextureVramResetCount` must read **one per battle entry**;
`gNdsR2StagePrepareBuildCount` two per entry with ReuseCount rising.
**Still owed on this row: one Latest run and the owner's eye check on a natural tie + rematch.**

## Freeze classes — TWO, with different fixes. Never say "the allocator" without the counter

`decomp/src/sys/malloc.c:30` is `while (TRUE);` (heap exhaustion, `MALLOCOVF` names it) and
`sys/taskman.c:338`/`:344` are two more (a display-list buffer past its end, and the graphics heap —
`DLBUF0..3` and `GFXHEAP` name those). `soak-freeze-watch.ps1` prints all of them and its verdict text
now says so. **Do NOT make `syTaskmanMalloc` return NULL globally** — callers do not check. The anim
cache arena lives on the taskman heap (92,160 B, using ~92,112) precisely because BSS competes with the
runtime `calloc` that sizes the heap: crossing the `0x130000` search floor costs 196,608 B in one step.
**Do not lower that floor** — it is a contract with the Task 36 replay guard (`nds_renderer.h:124-134`).
**`gNdsTaskmanArenaAllocFailCount` is a BOOT CONSTANT** (26), the step count of that downward search —
not failed match allocations. It nearly bought a revert of a −49,408 P95 chain.

## Measurement traps that cost days — all now instrumented rather than remembered

- **A counter with no writer reads 0, which looks clean.** `gNdsRendererProfileTextureRejectReasonMask`
  is written only at profile level ≥2 or with the route probe; a shipping-build 0 is uninstrumented.
- **gdb aborts a command file on the first missing symbol, silently** — reads as an emulator hang.
  `capture-sudden-death-entry.ps1` now asks `nm` and strips every read the ELF does not define.
- **A run that ends at the moment under investigation proves nothing.** Two rematch soaks read
  NO-FREEZE with the GAME SET zoom as their final frame; the 5-minute cap was terminating the emulator
  at match two's hand-off. Ceiling is 7 minutes now; `-PressStartEverySeconds` drives successive entries.
- **Identical source is not an identical binary**: two HEAD controls differed by P95 +5,376, ±1
  over-gate frame. Always run the matched control. **DLDI-on costs ~29,696 P95 and is the honest config.**
- **Never compare an anim-cache pair frame-by-frame** — it shifts load timing; order stats only.
- **The census window is a COMPILE-TIME constant**, so `-NoBuild` measures the last build's window.
- **`-Os` emits `blx __udivsi3` for a CONSTANT divisor**; `sinf`/`cosf` drag in `__ieee754_rem_pio2f` —
  SSB64 uses a table (L9), never reach for libm. **A lab ROM can differ in CODEGEN**: the `-marm` rule
  keys on harness ID, so add new lab IDs to `NDS_ARM_RENDERER_HARNESS_IDS`.
- **Compare captures with `scripts/compare-capture-pair.ps1`** — it crops past melonDS's host-FPS title
  bar, which otherwise reads as a visual regression whenever a candidate is faster.
- **`addr2line` names deleted AND inlined functions**; **read the HUD before the picture** (`TIME`/`DMG`
  say which match a capture belongs to); **`-MatchedCapture` is broken** (own BUGS row).

## Refuted — do not re-derive (all by measurement; derivations on the board)

**E51** `line_id`; **E53** `{base,size}` mirror; **the flash as vertex data** (E48-E58); **the pose
table** (E61); **`.text.hot`** (E66); **E57** hitboxes walk the live joint chain; **R2-06 E7** fighter
fallback + Task 39; **R2-06 E6** the Horner fold; **pointer arrays as index arithmetic** (E11); **an AObj
pool** (E12); **R2b's transform double-apply**; **R1's loader AND arena framings**; **the OOM spin and
the DL-buffer overflow as the *Sudden Death* freeze** (they are the *rematch* freezes); **L8's unroll**;
**the atlas allocation-order theory** (releasing in entry-one order is not enough; resetting is).
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
source countdown, Dream Land water at frame 0, Task 16 `1/1/1`. Do not edit `decomp/`.
Successive-match runs: `soak-freeze-watch.ps1 -MinutesToRun 7 -PressStartSeconds 165 -PressStartCount 2
-PressStartEverySeconds 145 -PollSeconds 5 -IdenticalFramesToTrip 12`.
