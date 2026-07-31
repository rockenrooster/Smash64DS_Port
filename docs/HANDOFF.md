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

## OPEN P1 #2 — `BUGS.md` is now entirely particles, VFX and audio cues, and ALL of it is P1

The owner rewrote the phase clause: **"All rows in `BUGS.md` fixed. this is a P1 Bugs list and are
required to be fixed for P1."** Nine rows, and seven are one blocker: **the particle scripts do not run**,
so every effect draws as one of four untextured 16-vertex primitives. **BOTH PARTICLE BRANCHES ARE MERGED
AND IN THE TREE — do not go looking for worktrees**: `scripts/generate_nds_particle_banks.py` +
`src/nds/generated/nds_particle_banks.generated.{inc,h}` (byte-reproducing: 55/119 reachable scripts,
23/47 textures, 82,752 B DS, 95,043 B linked), `src/nds/nds_particle_banks.c`,
`src/import/battleship_lbparticle.c`, all in `CFILES`.
**`NDS_R2_PARTICLE_RUNTIME=1` NOW BUILDS** — the 790-error wall was 725 enumerators the port's
`include/ft/fighter.h` and the decomp's `ft/ftdef.h` both declare, in nineteen blocks, all with identical
values; `SSB64_NDS_FTDEF_MIRROR` brackets all nineteen and only the seam TU defines it (details on the
board). **But it FREEZES AT BOOT and the reason is a RAM law nothing had written down: `.text` growth
costs taskman arena one-for-one.** The runtime adds **+24,024 `.text`** and only +384 `.bss`, and the
boot-time downward `calloc` search that sizes the arena therefore secures **24,576 fewer bytes** (26 → 32
steps), landing **exactly on the `0x130000` floor** that must not be lowered. Then the scene asks for
**4,896 B** (Dream Land's grpupupu bank, which the stub never allocated) against **4,032 B** of headroom
and `syMallocSet` spins. Deficit ≈ **29 KB**; the **"115,277 B of arena spare" line is REFUTED** — spare
was never the constraint, arena *sizing* is
(`artifacts/verification/freeze-soak/2026-07-31_161212-FROZEN-PICTURE.txt`).
**Lever, with an existing implementation to copy:** the 82,752 B of packed DS particle textures are
linked into `.text`; the battle static pack is 136 KB and lives in a **NitroFS payload** read at match
load and uploaded straight to VRAM (`ndsRendererHardwarePrepareBattleStaticTextures`). Route the particle
textures the same way — the image shrinks by more than the deficit and no arena is spent. Second cut: trim
the linked script set (55 linked, P1 reaches ~26). Only then price it: a 128-frame A/B with the runtime ON
against a matched control, judged against the **clean-frame** margin (P50 974,080 / P95 1,056,640) — the
merge measured byte-identical only because the runtime was off. Round-robin a quarter of the generators
per frame rather than batching quarter-rate work onto one frame in four (that lowers the mean and raises
P95). The flag also arms the `leaves_xf`/`dust_xf` per-scene null that is inert today, and **the DS quad
draw path is still unbuilt** — `gNdsParticleDrawSeamCount` is the "effects ran, nothing drew" signal.

## SUCCESSIVE MATCHES: FIXED — four defects, one law (full write-up in `docs/PORTING.md`, board row)

**Every one was state that outlived a scene boundary the taskman arena rewinds** — prepared-run cache
keyed on a config pointer; texture VRAM with no owner; the source display-list heads never rewound in the
battle loop (48 bytes past a 60 KiB buffer → BattleShip's own `while (TRUE);` ~8 s into match two);
`sMNVSResultsFighterGObjs` trusted across a Results re-entry (ARM9 data abort at match two's GAME SET).
Evidence: SD lane three consecutive bit-identical runs (`STG` 169,536, 28.0 FPS, pond textured); the
**same-binary control arm `-NoTexVramReset` still reproduces it** (BuildCount 92, `STG` 3,148,992, 4.2
FPS, white pond); rematch lane four entries / three Results / NO-FREEZE, owner-confirmed.
**Boundary AND Latest both green on the kept commit `0e5e8a3`.**
Guards: `gNdsRendererSceneTextureVramResetCount` must read **one per battle entry**;
`gNdsR2StagePrepareBuildCount` two per entry with ReuseCount rising.
**Still owed: the owner's eye check on a natural tie + rematch.** New owner row on the same flow: **no
GAME SET after winning Sudden Death** (`BUGS.md`) — same scene-flow surface, use the SD lane for it.

## Freeze classes — TWO, with different fixes. Never say "the allocator" without the counter

`sys/malloc.c:30` is `while (TRUE);` (heap exhaustion, `MALLOCOVF` names it); `sys/taskman.c:338`/`:344`
are two more (a DL buffer past its end, and the graphics heap — `DLBUF0..3`/`GFXHEAP` name those). The
soak prints all of them. **Do NOT make `syTaskmanMalloc` return NULL globally** — callers do not check.
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
- **`scripts/check-decomp-header-mirror.py` is RED on two constants** (`FTSTAT_OPENING1_START`,
  `nSYAudioBGMExplain`) — pre-existing, decomp is the specification, and both edits change shipping TUs
  so they want their own Latest run. Not caused by the enum-mirror guard.

## Refuted — do not re-derive (all by measurement; derivations on the board)

**E51** `line_id`; **E53** `{base,size}` mirror; **the flash as vertex data** (E48-E58); **the pose
table** (E61); **`.text.hot`** (E66); **E57** hitboxes walk the live joint chain; **R2-06 E7** fighter
fallback + Task 39; **R2-06 E6** the Horner fold; **pointer arrays as index arithmetic** (E11); **an AObj
pool** (E12); **R2b's transform double-apply**; **R1's loader AND arena framings**; **the OOM spin and
the DL-buffer overflow as the *Sudden Death* freeze** (they are the *rematch* freezes); **L8's unroll**;
**the atlas allocation-order theory**; **"115,277 B of arena spare" as the particle memory answer**.
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
