# Lane 3 follow-up - retire R1 in the P2 ELFs only, P1 family byte-identical

Read-only, 2026-09-28. Tree HEAD `039192a0ab6`; every line number below is HEAD. No repository file was edited: my files are
under `artifacts/performance/2026-09-28_p2-2p8-ram-supply/`, scratch copies live in the session scratchpad. No build, no emulator,
no git write, no agent.
The source patch needs no edit to `renderer_adapter_matrix.c`, `reloc_backend_assets.c` or `scripts/stages/*`; only the
separate Makefile patch touches the Makefile, and the working-tree Makefile already carries another agent's hunks
(+16 lines after :213, +17 after :6957, +65 after :7868), so Makefile lines below are HEAD lines and shift in the tree.

R1 = hierarchy mode 7, the per-root hardware executor, the CPU triangle rasteriser, the resident raw-path tables
(43,437 B in the FP link, `lane3-renderer-retirement.md` section 3).

## 0. Answer

* **No ROM target in the Makefile runs any R1 item in a committed configuration.** They are reachable only by a manual
  gdb poke of `gNdsRendererFastRunMode` to 5, 6 or 7 on an ELF that still links them (four scripts can poke it, none does by
  default, and no script or doc names 5, 6 or 7) and by profile-2 / benchmark builds, which the Makefile has refused for ROMs since `757ff7a494c` (09-08;
  `Makefile:4044`, `:4047`). The frozen P1 ELF (`builds/p1-root-overwritten-0927/smash64ds-battle-playable-hwtri.elf`,
  09-27 15:55) has the same shape as the P2 ELFs: every R1 entry has exactly one referrer, `ndsFighterMarioFoxDLAllDrawForSlot`,
  and cutting those edges frees 52 symbols, 43,314 B (42,186 heap + 1,128 ITCM).
* **The mode constant cannot be the guard**: the published P1 block sets the same `NDS_RENDERER_FAST_RUN_DEFAULT := 9`
  (`Makefile:2593`) as every P2 block, 21 of the 28 named targets are mode 9, and `gNdsRendererFastRunMode` is a `.dtcm`
  word that `capture-melonds.ps1`, `census-results-frame-cost.ps1`, `capture-results-tic.ps1`,
  `verify-battle-mariofox-gcrunall-loop-harness.ps1` poke and `probe-task56-fighter-path.ps1`, `probe-g1-site-cache-liveness.ps1`
  read. The guard is one new flag, `NDS_FIGHTER_LEGACY_EXEC` (default 1, set 0 for exactly six P2 target names).
* **P2 delta (graph model, exact static sums): FP 49,817 B, GATE 47,794 B of heap-section bytes**, plus roughly 0.5-1.1 KB
  of `DrawForSlot` text that constant-folds away (estimate). 43,437 / 41,414 of that is the R1 unit; 6,380 B more is old-path
  residue that only the retired branches kept alive (the static `persistent_renderer_vertices` 5,780 B BSS and the DL-draw
  visit callback 600 B), which moves those bytes out of R3.
* **P1 family and host tools: zero change.** With the flag at its default 1 the patched files expose exactly the same active
  source lines as HEAD under 7 saved build configs (P2 FP and GATE, P1 tick-HUD twice, coarse level 1, forensic level 2,
  task40), so the preprocessed token stream, and therefore the object code, cannot differ.
* Patches (checked with `git apply --check`, exit 0, against the current working tree):
  `lane3_r1_p1safe_sources.patch` (73 added lines, 3 files) and `lane3_r1_p1safe_makefile.patch` (11 added lines). Land the
  sources first (flag defaults to 1, so it is a no-op), then the Makefile hunks (which switch P2 on).
* Not compile-tested: the brief forbids builds. The verification I could do and the commands for the rest are in section 6.

## 1. Every target the Makefile names, and what R1 does in it

28 named targets (`rg -o 'smash64ds[A-Za-z0-9_-]*' Makefile`). `NDS_R2_FIGHTER_NO_ORACLE ?= 1` (`Makefile:1851`) has no override
in any block or script, so `detailed_output` is the constant FALSE in every ROM build (`renderer_adapter_fighter.c:3713`).

| class | targets (Makefile block) | mode / profile | R1 at runtime | flag |
|---|---|---|---|---|
| P2 (6) | `smash64ds` and `-p2-shell-freeplay-hwtri` (3346-3456), `-p2-shell-hwtri` (3243-3345), `-p2-shell-loop-hwtri` (3457-3527), `-p2-1b-scene-walk-hwtri` (3161-3242), `-p2-fourcpu-tickhud-hwtri` (2810-2964 + 2965-3076) | 9 / 0 | none | **0** |
| P1 published, frozen | `smash64ds-battle-playable-hwtri` (2581-2595; published list `Makefile:66`) | 9 / 0 | none | 1 |
| P1 family, profile 0 (13) | `-tickhud-hwtri`, `-proof-hwtri`, `-audio-fgm-hwtri`, `smash64ds-results-lab-hwtri` (2810-2964), `-fast-hwtri` (3080-3160), `-task37-{on,off}-hwtri` (3580), `-task44-{on,off}-hwtri` (3617), `-freeze-diagnostics-{on,off}-hwtri` (3654), `smash64ds-bug9-{rigidon,rigidoff}-hwtri` (3685) | 9 / 0 | none | 1 |
| profile 1 (3) | `-task49-differ-hwtri` (3528), `-coarse-hwtri` (3755), `-bgm-off-hwtri` (3767) | 9 / 1, 8 / 1, 8 / 1 | none (production route); per-root only by a poke of 5/6/7 | 1 |
| standalone payload | `smash64ds-task10-hardware-calibration` (3783) | 0 / 0, no fighter scene | none | 1 |
| refused for ROMs | `-forensic-hwtri` (3793, level 2), `-coarse-triangle-noop-hwtri`, `-coarse-cpu-prep-no-gx-hwtri`, `-coarse-warm-no-upload-hwtri` (3797-3810, benchmark modes 1/2/4) | - | cannot build: `$(error ...)` at `Makefile:4044`, `:4047` | n/a |
| ad-hoc names | scripts pass flags for `smash64ds-task9-state-hash-*-lab`, `-task16-combined-state-*`, `-m3-stage-owner-lab`, `-crowd-ack-hwtri`, ... | level must be 0 or 1 (default `?= 2` at `:138` is refused) | same as their flags | 1 unless one of the six |

Why the runtime column reads "none" (all lines `src/port/renderer_adapter_fighter.c` at HEAD):

* Hierarchy mode 7 needs `gNdsRendererFastRunMode == 7` (`:3789-3791`). No Makefile block sets 7, no script names 7
  (`rg` over `scripts/`, `tools/`, `docs/`), the poke scripts default to -1 or 0.
* The per-root executor needs `native_owner_started`, set only at `:4516-4520` when the native owner is enabled and
  `native_owner_production_attempted == FALSE`. Production is attempted whenever the owner is enabled in mode 8 or 9 with
  `detailed_output == FALSE` (constant) and `no_oracle != FALSE` (`:4363-4366`); both wrappers that reach `DrawForSlot` call
  `ndsRendererHardwareSetNoOracle(TRUE)` first (`:5306`, `:5506`); the third caller (`:5629`) returns at once because
  `sNdsFighterDLAllDrawPixels` is never assigned (`rg` finds no store). So only modes 5, 6, 7 (a poke) reach it, and the
  executor itself refuses `slot > 1` (`src/nds/nds_renderer_native_common.c:15781`).
* The CPU rasteriser has one ROM call chain: `ndsRendererExecuteNativeFighterRootHardware` (`:15751`) -> `ndsRendererNativeSubmitRun`
  (`:15526`, call `:15905`) -> `ndsRendererNativeSubmitGenericTriangle` (`:3326`) -> `ndsRendererSubmitHardwareTriangle`
  (`src/nds/nds_renderer_textures_effects.c:16578`), plus `ndsRendererNativeSubmitRunDirect` (`:15395`, call `:15901`) for the
  vertex helpers. The only other callers are host-only (`src/host/graphics_reference/nds_renderer_reference.c:231,257,262`).
* The raw-path tables: `PackedCorners` / `RunFirstCorner` are read by `EmitProductionRaw{Textured,Untextured}Run` (`:9197`,
  `:9222`), which the production emitter reaches only when `NDS_TASK56_FIGHTER_PRIMITIVES == 0` or the strip route is off
  (`:15238-15253`; P2 has primitives 2, `NDS_R2_STRIP_ROUTE 0`), and by the hierarchy executor (`:17616`, `:17621`); the
  validation read (`nds_renderer_native_fighter_production.c:837-853`) is compiled only when
  `NDS_NATIVE_FIGHTER_IMAGE_HAS_PACKED_CORNERS` (primitives 0, strip route or the screen-space census). `DenseCorners` is read by
  `EmitDenseRawRun` (`:15310`, only from `SubmitRunDirect`). The 22 `*JointSchedule` + 22 `*BindingJoints` tables (1,122 + 309 B)
  are read by the hierarchy executor and, for Mario/Fox only, by the profile-1 census
  (`nds_renderer_native_fighter_production.c:906`, `PROFILE_LEVEL == 1 && NDS_RENDERER_M2_DETAILED_LEDGER`).

Link-level evidence (`tools/lane3_r1_edges.py`, exact `--emit-relocs` graph): in the FP, GATE and frozen-P1 ELFs each entry has one
referrer and the DrawForSlot-edge cut frees the same set as deleting the entry nodes.

| ELF | members | heap B | text / rodata / bss | entry edit alone | held only by the corner-table initialisers |
|---|---:|---:|---|---:|---:|
| FP (`builds/build-fp-argmax`, relinked 15:02, same unit as the 12:19 snapshot) | 80 | 43,437 | 26,292 / 12,949 / 4,196 | 36,179 | 7,258 |
| GATE (`builds/build-yos-gatechk`, 14:55) | 51 | 41,414 | 25,236 / 11,970 / 4,208 | 34,156 | 7,258 |
| frozen P1 (09-27 15:55) | 52 | 42,186 (+1,128 ITCM) | 26,124 + 1,128 ITCM / 11,654 / 4,408 | 36,056 | 7,258 |

The frozen P1 build needs its R1 code for one more reason: `scripts/check-renderer-itcm-placement.ps1` requires
`ndsRendererNativeShadeProductionActions` in ITCM for `smash64ds-battle-playable-hwtri.elf` (`$nativeFighterFunctions`, lines 73-77, applied by `$requiresNativeFighter` at :86),
and in that ELF the only referrer of the function is `ndsRendererExecuteNativeFighterOwnerHierarchy`
(`lane3_refgraph.py callers`). Removing R1 from P1 would fail that check; the flag leaves P1 alone.

## 2. Guard design

`NDS_FIGHTER_LEGACY_EXEC`: 1 compiles the tree exactly as before, 0 folds the R1 entries away. `include/nds/nds_renderer.h` gets the
default (`#ifndef ... 1`) and one `#error` that refuses 0 unless the build is the production fighter configuration
(HW triangles, profile 0, `NDS_R2_FIGHTER_NO_ORACLE`, `NDS_R2_FIGHTER_HW_LIGHT`, `NDS_TASK56_FIGHTER_PRIMITIVES >= 1`, no strip
route, no screen-space census, no benchmark mode, fast-run default 8 or 9). Evaluated with `tools/lane3_r1_pp.py guard`: no error for
FP and GATE at flag 0; the error fires for the coarse (level 1) and forensic (level 2) configs at flag 0; never at flag 1.

The edit strategy is deliberately **fold the entries, do not delete the definitions**:

* every R1 entry has a single caller in `DrawForSlot` (or the 4-byte `ndsRendererExecuteNativeFighterRoot` wrapper), so making
  those calls dead lets `-ffunction-sections -fdata-sections` + `--gc-sections` drop all 80 members (the FP result above);
* the definitions are pinned by text tools: `scripts/fighters/generate_nds_native_owners.py:56-242` (`SOURCE_CLOSURE_POLICIES` names six
  hierarchy closures and the build fails closed if the text is gone), `scripts/fighters/test_native_actor_tarucann.py:140`
  (compiles `nds_renderer_native_owners.c` up to `s32 ndsRendererExecuteNativeFighterOwnerHierarchy(`, so no `#if` may open before
  it), and `scripts/check-gbi-decode-fixtures.ps1` (2090, 2351-2352, 2719, 2841, 2871-2872);
* `renderer_adapter_matrix.c` (another agent's file) holds `ndsRendererAdapterPrepareNativeOwnerHierarchy` and 456 B of hierarchy-only
  workspace members; neither needs an edit for the link result.

Physical deletion of the definitions is a later step with prerequisites (generator policy edit + regeneration + re-pin, the tarucann
prefix test, the fixture regexes, and the matrix file), and it would still have to keep the definitions for the P1 family.

## 3. Exact edit list (HEAD lines; `lane3_r1_p1safe_sources.patch` carries the text)

| id | file:line | edit | unlinks |
|---|---|---|---|
| E0 | `include/nds/nds_renderer.h` after :289 | default flag = 1, `must be 0 or 1`, `#error` for illegal 0 (29 lines) | - |
| E1a | `src/port/renderer_adapter_fighter.c:3478` | `native_owner_started` declared only when the flag is 1 | - |
| E1b | `:3482` | `native_owner_hierarchy_mode` becomes `const sb32 ... = FALSE` when the flag is 0 | every mode-7 test folds; `ndsRendererExecuteNativeFighterOwnerHierarchy` (7,092 B), `ndsRendererAdapterPrepareNativeOwnerHierarchy`, the inlined `BuildNativeHierarchyInputs` (418 B in DrawForSlot), `PrepareHierarchyRun`, the joint tables |
| E2 | `:3759-3771` | flag 0: `native_owner_enabled` true only for modes 8 and 9 (a poked 5/6/7 falls to the existing fail-closed reject) | mode 5/6/7 admission |
| E3 | `:3789-3791` | hierarchy-mode assignment only when the flag is 1 | - |
| E4 | `:4512-4536` | `ndsRendererBeginNativeFighterOwner` block only when the flag is 1 | `Begin` (352 B), `native_owner_started = TRUE` |
| E5 | `:4885-4910` | `else if (native_owner_started != FALSE)` per-root branch only when the flag is 1 | `ExecuteNativeFighterRoot` -> `RootHardware` (6,576 B) -> SubmitRun/SubmitRunDirect/PrepareDirectRun/VisitSourceCommand/Matrix3Mul20p12, the CPU rasteriser and its leaves, `LightShadeCache` (4,192 B), `DenseCorners` (4,260 B), the raw emitters; the callback pointer `ndsFighterMarioFoxVisitDLDrawCommand` (448 B) and `ndsFighterDLDrawAppendTriangle` (152 B) |
| E6 | `:4981-4994` | `ndsRendererEndNativeFighterOwner` / `Abort` block only when the flag is 1 | `End` (172 B), `Abort` (52 B) |
| E7 | `:4666-4674` | flag 0: `(void)native_materials; (void)native_material_count;` instead of the assignments only the per-root call read | keeps `-Wunused-but-set-variable` quiet |
| E8a, E8b | `:3431`, `:3739` | the function-static `persistent_renderer_vertices` and its `ndsRendererInitVertexCache` call only when the flag is 1 | 5,780 B BSS (only Begin, the per-root call and End used it); check `check-gbi-decode-fixtures.ps1:2871-2872` still finds both strings (they stay inside the `#if`) |
| A1 | `src/nds/nds_renderer_assets.c:769-771`, `:806-809` | flag 0: the Mario/Fox tables initialiser gets `NULL, 0u, NULL, 0u` for `packed_corners`, `packed_corner_count`, `run_first_corner`, `run_first_corner_count` | `PackedCorners` 4,260, `PackedCornersLow` 2,742, `RunFirstCorner` 142, `RunFirstCornerLow` 114 (the images already leave `packed_corners` NULL, `nds_renderer_assets.c:3757`) |
| M1 | `Makefile:2580` (after the fast-run default) | `NDS_FIGHTER_LEGACY_EXEC ?= $(if $(filter <six names>,$(TARGET)),0,1)` plus a 0/1 validation like `:2543-2562` | - |
| M2 | `Makefile:7172` (after the `NDS_R2_FIGHTER_NO_ORACLE` echo) | `echo '#define NDS_FIGHTER_LEGACY_EXEC ...'` into `nds_build_config.h` | - |
| M3 | `Makefile:8554` (after `BENCH_MAKE_FAST_RUN_DEFAULT`) | `BENCH_MAKE_FIGHTER_LEGACY_EXEC` in `print-benchmark-flags` (optional; `check-tickhud-parity.ps1` compares the P1 pair, both 1) | - |

The six names in M1: `smash64ds smash64ds-p2-shell-hwtri smash64ds-p2-shell-freeplay-hwtri smash64ds-p2-shell-loop-hwtri
smash64ds-p2-1b-scene-walk-hwtri smash64ds-p2-fourcpu-tickhud-hwtri`. `?=` keeps a command-line `NDS_FIGHTER_LEGACY_EXEC=1` as the
A/B control (same program as today). A forgotten new P2 name simply keeps the flag at 1. All six P2 blocks already force HW_MTX, GX_COMPOSE and
HW_LIGHT to 1 (`Makefile:2867` shared proof block for the fourcpu-tickhud gate, `:3185` 1b-walk, `:3260` shell, `:3370`
freeplay and `smash64ds`, `:3476` loop); the FP and GATE resolved configs show primitives 2, strip route 0, census 0,
benchmark 0, mode 9, level 0, NO_ORACLE 1, HW triangles 1, so the `#error` passes for all six.

Edit positions after the patch (source line numbers in the patched `renderer_adapter_fighter.c`): the pinned line 4962 becomes 4995.

## 4. Expected P2 byte delta (static sums from the reloc graph; the link delta is that minus at most one 32 B alignment step per removed section)

| step | FP heap B | GATE heap B | sections (FP) |
|---|---:|---:|---|
| E1-E6 + A1: R1 unit (`lane3_r1_edges.py`) | 43,437 | 41,414 | `.main` text 26,292 + rodata 12,949; `.main.bss` 4,196 |
| E5 side effect: DL-draw visit callback + `AppendTriangle` | 600 | 600 | `.main` text |
| E8: `persistent_renderer_vertices` | 5,780 | 5,780 | `.main.bss` |
| **total (`tools/lane3_r1_total.py`)** | **49,817** | **47,794** | text 26,892, rodata 12,949, bss 9,976 |
| `DrawForSlot` shrink from folded inline code | ~0.5-1.1 KB | same | estimate: line attribution puts 1,142 B on the hierarchy/per-root ranges (builder 418, mode 82, anim-lock 72, matrix prep 102, inputs call 44, executor ternary 72, Begin 48, per-root 94, End 210) |

ITCM and DTCM: 0 change in both P2 ELFs (no R1 member lives there). Every freed heap-section byte grows the battle arena one for
one (`2026-09-28_p2-2p8-stage-heap-cliff/README.md`). Against the earlier report: R1 becomes 49,817 B and R3 falls from 81,920 to
75,540 B (the moved 6,380 B were counted in R3 before). P1 family: 0 B.

Other P2 targets (shell, loop, 1b-walk) use the same code family; I measured only FP and GATE.

## 5. Checkers and tests

Must change:

1. `scripts/probe-yoshi-tour-full.ps1:224` - `tbreak src/port/renderer_adapter_fighter.c:4962 ...` is a line pin (the
   `if (detailed_output != FALSE)` after `ndsFighterDLDrawCopyPersistentRendererState`); it moves to `:4995`. Better: resolve it by
   text as `verify-battle-mariofox-gcrunall-loop-harness.ps1:2264-2312` does.
2. `scripts/probe-p2-fourcpu-sparse.ps1:886-892` (`-FirstPacketFault`) - two gdb `break`s on `ndsRendererNativeEmitProductionRawTexturedRun` /
   `...UntexturedRun`; those functions are not linked in a P2 ELF after this change, and `break` on a missing function aborts a gdb
   script. Drop them (a raw emitter fault can no longer happen). The line pins at `:137-147` (`nds_renderer_native_common.c:7869/7875`,
   `nds_renderer_preamble.c:3648/3675/3504`, `nds_renderer_native_fighter_production.c:180`, `nds_renderer_textures_effects.c:11198`) are in files
   this patch does not edit (7869/7875 already point into unrelated run-memo code).

Add:

3. `scripts/check-gbi-decode-fixtures.ps1`: assert the header default and `#error` text, that `Makefile` carries the six-name list with default 1,
   and that the published P1 block (`ifeq ($(TARGET),smash64ds-battle-playable-hwtri)`) does not mention the flag.
4. An ELF check per family: `tools/lane3_r1_symcheck.py <elf> absent` for the P2 ELFs, `present` for P1-family ELFs (36 names; 43,437 B
   in today's FP, 41,402 GATE, 40,394 frozen P1).

Verified unaffected (read, not run):

* `scripts/check-renderer-itcm-placement.ps1` - the P2 ELF names do not require the native-fighter set; the evicted list accepts absent symbols; the P1
  names keep `ShadeProductionActions` because the P1 flag stays 1.
* `scripts/check-tickhud-parity.ps1` (P1 pair, both 1), `check_native_only_rom.py` / `test_native_only_rom.py` (`FORBIDDEN` names none of R1).
* `scripts/verify-battle-mariofox-gcrunall-loop-harness.ps1:2264-2312` text anchors: after the patch `native_owner_production_attempted = TRUE;` is unique (4440),
  `runtime_hardware_triangle_count =` unique in the next 100 lines (4462), `native_owner_failed = TRUE;` unique in the next 150 (4493),
  `if (native_owner_production_attempted == FALSE)` unique (4563).
* `scripts/check-gbi-decode-fixtures.ps1:1471,1475` (DrawForSlot text regexes), `test_native_failure_record.py:16`, `test_intro_transient_context.py`,
  `test_kirby_trio_body.py`, `check_native_owner_geometry_closure.py` (Kirby accept text): none of the pinned text is inside an edited range.
* `scripts/fighters/generate_nds_native_owners.py`, `test_native_actor_tarucann.py`, `task56_fighter_topology_census.py` (parses the generated `.inc`
  tables, which stay): no source or generator text was removed.
* Poke scripts (`capture-melonds.ps1:410`, `census-results-frame-cost.ps1:167`, `capture-results-tic.ps1:99`,
  `verify-battle-mariofox-gcrunall-loop-harness.ps1:2001,2021`): still write the word; on a P2 ELF a poke of 5/6/7 now means "not native" (reject), poke 8/9 unchanged.
* Documentation only: the survey comment in `include/nds/nds_r2_hwmath_unit.h:14-15` lists `ndsRendererHardwareSubmitVertex` and
  `ndsRendererHardwareClipVertexNdcDepth` as holders of the divide unit; both are gone from P2 ELFs.

## 6. Verification

Done here (no compiler, no build):

* Exact reference graphs of three ELFs (`lane3_r1_edges.py`): single referrer per entry, edge cut == node cut, sets above.
* `git apply --check` on both patches against the working tree: exit 0.
* `tools/lane3_r1_pp.py equal`: with the flag at 1 the patched `renderer_adapter_fighter.c` and `nds_renderer_assets.c` expose the same active source lines as HEAD
  under 7 configs (3,588 / 2,509 active lines for FP; 3,278 / 1,991 GATE; 3,069 / 1,214 P1 tick-HUD; 2,800 / 1,139 coarse; 1,856 / 160 forensic).
* `tools/lane3_r1_pp.py grep ... 0`: with the flag at 0 and the FP or GATE config, no active line in `renderer_adapter_fighter.c` references `Begin/End/Abort/ExecuteNativeFighterRoot`,
  `native_owner_started`, `ndsFighterMarioFoxVisitDLDrawCommand` or `persistent_renderer_vertices`; the remaining hierarchy references are all behind
  `native_owner_hierarchy_mode != FALSE` on a `const FALSE` (lines 4189, 4391, 4438 in the patched file) and fold at -O2 and -Os.
* Text anchors of the P1 harness (section 5) and the `#if`/`#endif` balance of all three files under both flag values.

To do (needs a build; commands):

```
git apply artifacts/performance/2026-09-28_p2-2p8-ram-supply/lane3_r1_p1safe_sources.patch   # flag defaults to 1: a no-op
# then, after the Makefile owner has landed their hunks:
git apply artifacts/performance/2026-09-28_p2-2p8-ram-supply/lane3_r1_p1safe_makefile.patch
make TARGET=smash64ds-p2-shell-freeplay-hwtri BUILD=build-r1-off NDS_FIGHTER_LEGACY_EXEC=1   # control, equals today
make TARGET=smash64ds-p2-shell-freeplay-hwtri BUILD=build-r1-on                              # candidate
python .../tools/lane3_r1_symcheck.py builds/build-r1-on/<elf> absent    # and `present` for build-r1-off
arm-none-eabi-size -A <both elves>                                       # expect ~ -49.8 KB in .main + .main.bss
make p1-tick                       # P1 sibling, does not publish; .main/.main.rw/.itcm/.rodata must equal the pre-patch tick-HUD ELF
python scripts/compare-replay-digest.py --sequence ...                   # P2 gate ROM, both arms: digests identical
```

Never build the published names (`smash64ds`, `smash64ds-battle-playable-hwtri`) to test this: they publish into the project root and would overwrite the frozen P1 ROM.
`smash64ds-p2-shell-freeplay-hwtri` and the tick-HUD siblings do not publish.

## 7. What is still a dual path in P2 after R1

1. Lean list vs the old production route, per fighter draw: `ndsFighterDisplayContractSubmit` (`renderer_adapter_fighter.c:5508-5546`) still falls back to `DrawForSlot`
   on a lean decline, for owner slots >= 12 (polygon team, Master Hand; 1P is compiled in) and for the 1P intro transient (`:5263-5324`). That is R3, 75,540 B after this patch.
2. Inside the old route: packet replay / record / direct execution in `ndsRendererExecuteNativeFighterOwnerProduction`, the plan and validate machinery, the per-run texture memo.
3. `gNdsRendererFastRunMode` stays a runtime word with readers outside the fighter draw: `nds_renderer_dispatch_profile.c:173-208` (stage texture sites for modes 4/7/8, fast-owner
   enable for 1-4/7/8), `renderer_adapter_stage.c:4530`, `nds_renderer_native_owners.c:4225,4243` (stage-native needs 9), `renderer_fighter_lean.c:808-811` (lean needs 8 or 9),
   `reloc_backend_movement.c:15913,16000` (profile 1 only). With the mode pinned at 9 these are constant in P2 but the compiler cannot fold a volatile word. Mode 8 still makes fighters native and the stage fail closed.
4. `native_owner_production_attempted == FALSE` loop (`:4538`): after the patch it only rejects (the `production_hardware_started == FALSE` path and `no_oracle == FALSE`); keep it, it is the fail-closed tail.
5. Compile-time dead but source-present: the raw and strip arms of the production emitters (`nds_renderer_native_common.c:9401-9407`, `9573-9579`, `15238-15253`). The `#error` keeps
   them dead by refusing `NDS_TASK56_FIGHTER_PRIMITIVES == 0` and `NDS_R2_STRIP_ROUTE` with the flag at 0.

## 8. Risks and unproven

* Not compiled. The fold depends on GCC treating `const sb32 x = FALSE` as a constant, which it does at -O2 and -Os (the only levels any ROM target uses); if a reference survived, the P2 gain would shrink by
  at most the 7,092 B hierarchy executor, and `lane3_r1_symcheck.py absent` would say so.
* Expect one `-Wunused-function` for the static `ndsFighterMarioFoxVisitDLDrawCommand` (`renderer_adapter_legacy_dl_probes.c:441`) in P2 builds; no gate treats warnings as errors
  (no `-Werror`, no warning-count check in `Makefile`, `build.ps1`, `scripts/lib`).
* A gdb poke of `gNdsRendererFastRunMode` to 5, 6 or 7 on a P2 ELF now rejects instead of drawing Mario/Fox through the per-root executor.
* The FP ELF in `builds/build-fp-argmax` was relinked at 15:02 (my earlier snapshot was 12:19); the R1 unit is identical in both, but the rest of that link changed.
* Layout shift: removing 50 KB moves `.main` and cache phase; the 09-28 heap-cliff note says two ROMs 1,888 B apart read STG 169K vs 230K until the arena chooser was fixed.
  Re-measure with the fixed chooser.
* The Makefile hunks were checked against the working tree only for `git apply` offsets; the other agent's Makefile edits may still move or touch the anchors
  (`NDS_RENDERER_FAST_RUN_DEFAULT ?=`, the `NDS_R2_FIGHTER_NO_ORACLE` echo, `BENCH_MAKE_FAST_RUN_DEFAULT`).

## 9. Files

`lane3-r1-p1safe.md` (this), `lane3_r1_p1safe_sources.patch`, `lane3_r1_p1safe_makefile.patch`, `lane3_members_r1_p1frozen.csv` (the 52 R1 members of the frozen P1 ELF),
tools: `lane3_r1_edges.py` (referrers, edge-cut vs node-cut, freed set), `lane3_r1_total.py` (freed bytes step by step for the whole patch), `lane3_r1_patch.py` (writes both patches from HEAD blobs; asserts every anchor matches once),
`lane3_r1_pp.py` (conditional-directive evaluator: `equal`, `grep`, `guard`), `lane3_r1_lineattr.py` (DrawForSlot bytes per source range), `lane3_r1_symcheck.py` (R1 symbols absent or present in an ELF).
Reused from the first report: `lane3_symtab.py`, `lane3_srcmap.py`, `lane3_relocgraph.py`, `lane3_refgraph.py`.
