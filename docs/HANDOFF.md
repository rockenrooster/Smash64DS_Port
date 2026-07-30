# Handoff

Updated: 2026-07-29. **Restart surface only, capped at 150 lines.** Anything
durable goes to its owning doc: the board owns the queue and every result,
`PERF_LEDGER.md` measurements, `KNOWN_ISSUES.md` durable gaps and harness traps,
`optimization/TASK_STANDING_RULES.md` how a task is run.

Runtime 2 phase status. **Five gate levers graduated 2026-07-29: E32 (-52,416
P95), E64b (-26,944), E65 (-35,584), E67 (-4,672) and E69 (-12,544). Cumulative
P95 1,228,928 -> 1,096,768, over gate 17/128 -> 6/128.**

| phase | state |
|---|---|
| R2-00a/b/c, R2-01, R2-02 | gated |
| R2-03 | shipped E12/E28/E29/E46/**E32**/**E64b**/**E65**/**E67**/**E69**; only the E32 flash residual is open (KNOWN_ISSUES) |
| R2-04 | loading + rate clauses done (E5/E6/E57); budget clause closed by E64b+E65 |
| R2-05 | **COMPLETE** — reproducibility (E0) and zero fighter special cases (E1) |
| R2-06 | E0 + E1 + E2 done; the soak clause now HAS an instrument (`soak-freeze-watch.ps1`) and is unrun |
| R2-07/08 | not started; R2-08 needs the owner's retail play test |

## Where the gate stands

128-frame ring dump, frames 795..922, `WORK-H`: P50 **966,848**, P95
**1,096,768**, gate 1,120,000, **6/128 over**. Evidence
`artifacts/performance/r203-e69b-mtxcopy-128{.json,-rows.csv}`.

**Margin 23,232 — three times the 5,000-7,000 placement floor.** E65 first landed
the gate at 6,016 (inside the floor); E67 and E69 took it here, so it survives a
relink and finally leaves room for R2-07. Not covered: retail hardware (R2-08), and
the particle work must fit inside 23,232 (switch plan §7 R2-07).

## OPEN P1: random freezes, and the harness could not have seen them

Owner, 2026-07-29: *"lots of freeze bugs that seem random"*, plus a new
`docs/BUGS.md` row *"Sometimes hitting a shielded player causes a freeze"*, plus
*"I can reproduce the bugs in melonDS with dldi checked."*

**`[DLDI] Enable` was pinned by nothing.** The owner's emulator had it true; all
nine runner slots and the attributor had it false, and neither profile function
touched the section — so **no scripted verifier in this campaign ran the owner's
I/O configuration**, which is a sufficient explanation for green gates beside
random freezes. Now forced on in both canonical profiles, asserted per-section by
`check-melonds-policy.ps1` (negative-tested four ways), applied to all eleven
configs, slots sharing the one repo `dldi.bin` read-only. Slot 0 smoke-tested.
DLDI is not cosmetic: it decides whether `nitro:/` is reached through the DLDI SD
driver or the card interface, and this port resolves `nitro:/` **by string path at
runtime** (a GDB attach caught `nitroromResolvePath` on `"FTMarioAnimWai"`).

Instruments: **`scripts/soak-freeze-watch.ps1`** (new — polls two guest counters
that advance on different paths, so it separates a hung main loop from a dead IRQ
path from a merely slow frame, and captures PC/backtrace/`REG_IME`/`IE`/`IF`/
`GXSTAT`/IPC on the first occurrence; exits 2), and `NDS_FREEZE_DIAGNOSTICS`
(`%-on-hwtri`), whose breadcrumbs include a dedicated **FGM enter/return pair with
a channel field** — a prior investigator's fingerprint on the audio path.
`NDS_R2_BOTH_CPU=1` makes Mario a level-3 CPU too: self-driving, hits both ways.

## OPEN P1: the VS Results screen is 21.9M ticks/frame, 1.5 FPS

R2-07's own gate, never measured until now, and 19.5× the battle frame this
campaign has spent weeks on. **89.4% is `tfunc->scene_draw()` alone (19,550,631);
the commit and the source update are ~1.1M each.** Board has the method (a VBlank
timeline over GDB, zero builds) and the two structural suspects: the native OAM
path is gated to `nSCKindVSBattle` only, and Results software-blits two 320×240
layers per frame. The loop at `src/port/taskman_seam.c:6950` carries **no
instrument** — no pacing, no HUD, and it never advances
`gNdsBattlePlayablePacingPresentedFrames`, which is why the on-screen FPS reads
`0.0` there.

## The other half is ANIMATION, not collision (E60/E61 — this replaces the `SRC` row)

**The board carried "float→fixed on the collision path, `gmcollision.c`" for
several cycles. It is wrong by ~20x and the row is deleted.** A leaf helper is
charged to itself, never to its caller, so every float op the animation path
executes was booked to `__aeabi_fadd`/`__aeabi_fmul` and read as a separate,
larger family. Caller-attributed: `gcPlayDObjAnimJoint` **34,022 self + 60,509
helper = 94,531**; the whole animation path **76,047 + 70,895 = 146,942**.

**146,942 ticks/frame, 15.2% of WORK 969,487 — larger than the whole gap.** The
entire collision family is **under 4,000**, below the placement noise floor. The
renderer share is 15,709, inside §3.9's "too small for architecture work" band.

E61 then found **the cubic is 99.6% of that float**: 149.4 cubic nodes/frame at
**405 ticks each**, against 118.7 Step nodes at zero float and 4.5 Linear.
`anim_speed` is `1.0`/`0.5`, **never 0**; `GOBJ_FLAG_NOANIM` skips are **0**.

**Task 78 stopped the animation compiler on a self-vs-inclusive error** — it
compared 82,807 *self* ticks to a 100,000 target while its own §4 listed
`fadd`+`fmul` = 119,912 as a *separate* family. Corrected: 164,236, **1.64x its
target, not 0.85x**. Tasks 95/96 refute only the *layout* route.

**The pose table is REFUTED by size** (E61) — 2.62 MB resident against 4 MB of
main RAM, or 42.6 KB/7–11 ms streamed per transition. Do not propose it again.

## The one open fidelity item

- **E32** — blocked on a **generator gap, not a decision** (E62; the earlier
  "fidelity-budget / visual approval" framing here was wrong). The flash clears
  `G_LIGHTING` and draws vertex colours raw; the owner keeps `POLY_FORMAT_LIGHT0`
  and hardware-lights with stale diffuse/ambient, so it draws Mario *unflashed* —
  not corrupt. E32 is pixel-identical to the generic path on every non-flash
  frame (510/511: 0 px). E49's `NDS_R2_UNLIT_VERTEX_EPOCH` already implements the
  runtime half but is **refuted**: it emits the baked dense `.rgba`, which holds
  packed **normals**, giving rainbow speckle and a *worse* diff (2,199 vs 1,551).
  **Needs the generator to bake the flash variant's vertex colours** as a second
  dense table. E63 sizes it.

**R2-03 E26 — demoted** to 23,844/frame over 136.8 deltas (needs both
`NDS_TASK91_DRAW_PHASE_CENSUS=1` *and* `NDS_R2_SPAN_LEAN_TIMING=1`); bottom of
§3.9's band and not simple. **Must replace the dispatch, not the writes** (E39);
read its spec only with E34/E34-b/E39/E43/E45/E56.

**R2-04 E57 — REFUTED.** `gmCollisionGetFighterPartsWorldPosition`
(`gm/gmcollision.c:489`) places every hitbox by **walking the live joint chain**,
so halving the twice-per-frame evaluation is a gameplay change. With E6 this
closes R2-04's rate clause.

## Refuted this cycle — do not re-derive

- **E51**, a `line_id -> (group, kind)` table for `reloc_backend_mp_collision.c`'s
  three scans: `gNdsStageCollisionLoopYakumonoCount = 1`, so the loop that reads as
  a 64×4 worst case has a trip count of **one**.
- **E53**, an 8-byte `{base,size}` mirror for `ndsRelocFindLoadedFileContaining`:
  exact, still P95 **+11,584** and 92/128 worse.
- **The flash as vertex data** (E48/E49/E50/E55/E58), **the animation pose table**
  (E61, 2.62 MB resident vs 4 MB RAM), **fixed-point collision** (E60, under 4,000
  ticks/frame), and **`.text.hot` placement** (E66, P95 +24,448).

## Restart

Branch `codex/r2-runtime2`, not merged to master. Boundary
`battle_playable_realtime`, mode `163`.

```powershell
$env:DEVKITPRO = 'C:/devkitPro'; $env:DEVKITARM = 'C:/devkitPro/devkitARM'
.\scripts\verify-all.ps1 -Profile Boundary -List
git status --short
```

**Do not rebuild `smash64ds.nds` for P1 work** (owner, 2026-07-28). **Do rebuild
the tick-HUD ROM whenever the published one is** (owner, 2026-07-22) — keep its
Makefile block flag-identical. **Do not pass `-j` to `make`**; the Makefile sets
`MAKEFLAGS += -j$(NDS_JOBS)` from `nproc` and an explicit `-j` overrides it. One
build at a time regardless — generators write to shared paths outside `$(BUILD)`.
A clean checkout must build through `build.ps1`, not bare `make`: four of the six
generated `.inc` files are gitignored and only `build.ps1` regenerates them.

Preserve canonical mode 163, renderer mode 9, mip 0, static texture residency,
source countdown, Dream Land water at source frame 0, Task 16 `1/1/1`. Do not
edit `decomp/`. **Bug #10 is FIXED and folded in** — `06992f10812`,
cherry-picked from `2cbc6189d15` to preserve authorship, with a host fixture, a
structural pin, and the `pause_under20` oracle.
