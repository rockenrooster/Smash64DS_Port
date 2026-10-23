# P8 status -- stability, RAM, CPU and readability

Live state for [the P8 plan](P8_PLAN.md). One row per item; the cursor is the
first row that is not done.

## Cursor

**Next action:** loop V first (owner 2026-10-22, high priority: the particle
effects that hold frame 0, plan section 2b): V0's ledger -- each texture's
scripted frames and costs, and texture VRAM per stage (`gNdsVramCensus` on
the gate ROM) -- then V1-V3 one effect family a batch. R1 (the renderer's
cold code as Thumb) goes on in V's gaps: its profiles and the `s67` baseline
gate are building. F, G2, H-2 and I interleave.

**Before the first edit:** when this plan was written (2026-10-22 08:20) a
session was running the owner's goal "finish Todo list: readability, RAM
hunting, crash hunting": it committed `7f0dca28600` at 08:18, had a `make`
running, a Phase C2 plan ready for `reloc_backend_assets.c` and its own map
census (`map_census.py`, memory `ram-hunt-census-2026-10-22`). One integrator
only: this goal continues that work, it does not run beside it. Check for a
running build and that session's uncommitted work first. The worktree started
with 56 dirty paths (owner docs, test scripts, a few sources): leave them,
commit with explicit paths only.

## Baseline (2026-10-22)

| what | identity | numbers |
|---|---|---|
| HEAD | `7f0dca28600` | readability A, B-1..B-4b and C's first split landed |
| ship play ROM | `builds/play-p7z` | `.main` 1,865,184; `.main.rw` 175,580; `.main.bss` 875,208 (image 2,915,972); overlays 423,040; ITCM 32,728 / 32,768; DTCM 12,244 / 16,384. Battle-resident: R0 computes it |
| gate ROM, worst-case search | `build-sweep-s56` | cast <= 1,250,000 / 95% on 16 of 16; heavy roster over 1,250,000 / 90% on 11 of 16 (Hyrule Temple 1,609,344, 80.7%); mixes 24 of 64 over; lowest heap 25,604 |
| last gate ROM | `build-sweep-s61` | digests identical to `s60` on three configs |
| stability | `docs/STABILITY_AUDIT.md`, 2026-10-20 | every mode on every fighter, 0 halts (before ~47,000 lines of A/B deletions) |

## Items

| item | state | evidence / next |
|---|---|---|
| Q1-Q3 owner decisions (plan section 7) | answered 2026-10-22 | Q1 yes (AGENTS.md and HANDOFF.md route here); Q2 yes (R10 if R1-R9 leave the heavy mixes short); Q3 P7 only as bugs |
| H-1 tools S1 and the gate need, into `scripts/tools/` | done | 22 files, `scripts/tools/README.md`: play/walk/gate/soak builds, the probe session and halt probes, the six audit drivers, the wireless lag driver, the worst-case sweep, the heap census, the boot check; no session paths (`common.ps1`); `boot_check.ps1` boots `play-ram1` clean |
| S1 regression net (full matrix) on HEAD | in progress | ROMs `walk-u19`, `build-soak-s32`, `net-p8a`, `build-sweep-s62` on HEAD `8b9d2a52f9f`; slot images regenerate per slot now (`5075604365a`), five audits side by side. Soak m1-m8 (every fighter, all items, four players): 0 halts, 0 native failures, heap low-water 41,696 at the least. Bonus: 38 practices clean. Training: 29 of 32 to the select; the 3 timeouts (DK, Kirby, Mad Piano) are the probe's: the walk took RESET, the game went on to the Training select at 60 fps, but the scene-entry breakpoint no longer fired (gdb), so no END line. Wireless 2 consoles: 3 matches, 0 desyncs, 0 aborts. VS walk: 20 laps, 87 scene entries, 0 native failures, battle heap low-water 100,304. Metal Mario: the campaign stage and both bonus boards clean. Campaign: all 32 fighters (12, Metal Mario and the 19 contents) through the ladder to the ending, 0 native failures, heap low-water 28,388 at the least (content 10). Wireless 4 consoles (`net-p8a`, `wireless_lag.ps1 -Consoles 4`; melonDS-mp's instances 2 and 3 needed their own gdb ports): 3 matches on every console, 0 desyncs, 0 aborts; one console waited up to 20 VBlanks a gate under the four in-process instances' host load. The matrix reruns on the final HEAD |
| C split the 13 port files over 4,000 lines | done | C1-C12 and the counter purge: every plan file is under 4,000 lines (largest port file now `nds_renderer_native_entry.c`, 3,965). `renderer_adapter_stage.c`'s 5,677-line body keeps its text: its claim and submit phases are fragments it includes in place (`split_function.py`). `include/ft/fighter.h` (4,933) is the one file left over: a decomp-shaped mirror of BattleShip's ft headers that `scripts/p4/ft_layout.py` and nine other tools parse, so it keeps the source's layout (section 3) |
| S3 static analysis (`-fanalyzer`, allocations, stacks) | in progress | `-fanalyzer` over the 80 port compiles of the play config: 21 distinct warnings, all triaged false or benign (NULL-checked helpers read as maybe-NULL in `nds_p4.c`; the ARM7 snapshot copy; a zero-count loop; linker-symbol pointer differences; the joint table's parent-first order). `-fstack-usage`: largest frames `ndsAudioParseCtl` 12,384 B (boot, main stack), `ndsRendererAdapterSubmitStageDLBody` 5,192 B, `ndsRendererAdapterTryNativeEntryEffect` 2,888 B; next the allocation audit and the coroutine stack paint |
| D purpose headers and invariant comments | done | `fade4d1e094` (40 src/port files), `cab524db6f9` (103 in src/nds, include and src/host): every port file opens with what it owns, who calls it, its contracts and generated inputs; nine one-line headers remain (complete statements). Before them the husks went: D0 `1c20ddf249d` (11 empty files), D0b `454132e40a6` (885 macros nothing names; `nds_audio_bgm.h`'s stream pins kept) |
| E dormant verifier framework, legacy fighter executor | done | `1371dee1858`: 33 scripts and `HARNESSES.md` (-9,593 lines); seven passing host checks kept, two failing ones left for H-2; `check-gbi-decode-fixtures.ps1` stays while the owner's worktree holds it modified. The executor itself went in `1c8636a6632`; its knob goes with G |
| F history names | open | |
| G Makefile cleanup and split | open | proof: `make -n` identity |
| H-2 the rest of the tools, `quick_check.ps1` | in progress | the ten host areas collect again (`1780ca40d43`: the three tests that stopped collection follow the code). Past collection they ran 117 failed / 43 errors, all stale harnesses (production functions grew seams, moved or went): `2710bbac5a3` fixed the wallpaper runtime, menu handoff, stage blob loader and crash residency suites (41 pass; `nds_r2_hwmath_unit.h` gained a host arm, ARM/Thumb code identical) -- now 113 failed / 6 errors over 47 files. Rule per test: the code still lives, the harness follows it (stubs for new seams, re-pinned counts with the reason); the code is gone, the case goes with it. `d81e43c5667`: the wallpaper table and Zebes capture. Next: the anim-cache group (`test_anim_cache_budget`, `test_css_cache_sizing`), the CSS preview transaction, the pack estimator pins |
| I docs routing, P7 close-out, archive | in progress | `AGENTS.md`, `HANDOFF.md` and `docs/README.md` route to P8 (the owner's pending README rows kept); P7's rows closed with their evidence (`ce376f1be3e`); next the `PERF_LEDGER.md` archive (`BUG_NOTES.md` waits: the owner's move from `docs/p2/` is uncommitted) |
| V0 particle ledger: scripted frames and costs per texture; texture VRAM per stage | done | owner 2026-10-22 high priority (plan 2b); the item explosion (`a45a48ffd2a`) and SparkleWhite (`cd0c687f4d5`) already play all frames. (a) `scripts/2d_vfx/particle_frame_ledger.py`: 118 reachable efcommon scripts over 46 textures; 28 textures hold one frame while their scripts step every source frame (textures 2, 5, 10, 12-18, 20, 23, 25, 26, 28, 30-36, 38, 40-42, 44, 46), and three fighter-bank ones (Ness PK Fire 160: 6, Yoshi 176: 2, Kirby 209: 3); Dream Land's leaf (Whispy native texture 2, 4 frames) is held on frame 0 by its own native path, and the native FireGrind draw caches texture 5's frame-0 row. Every scripted frame at 0.75x is 155,104 texels and packs into 24 sheets (192 KiB, +15 sheets): the 24x24 cells fill 5,760 of a sheet's 8,192 (ten a sheet). The sheets load from NitroFS into texture VRAM at battle prepare; main RAM grows about 100 B a sheet (palette, binding, name). Gate DTCM was 4 B over its ceiling with the ninth sheet: the name table moved to main RAM (`97b9cbc89a4`). (b) texture VRAM per stage (census lab ROMs `build-vram-v1` / `build-vram-b0`, `NDS_DIAG_COUNTERS=1`, heavy roster, `vram_census.ps1`): the 24-sheet bake fills VRAM (free at GO 192-1,280 B of 393,216) and refuses stage textures on every measured stage but Dream Land (198-5,872 rejects a match; Hyrule Castle rebuilds its stage prepare 2,395 times). The 9-sheet baseline refuses nothing; free at GO 23,040 (Hyrule Castle, already 196 prepare rebuilds) to 88,832 (stage 41), 1,500 frames in 22,528-67,840. Hyrule Castle at GO: stage warm textures 71,808, entry-effect textures kept after GO 55,744 (49 names), particle sheets 73,728, the countdown's traffic-light atlases 57,344 (`ndsIFCommonPrepareCloudAtlases` 49,152 + `ndsIFCommonPrepareTrafficAtlas` 8,192, drawn ~7 s and resident all match), battle scene textures 29,696, halo/shield 8,192. So: (A) release the countdown atlases when its thread ends (+57 KB, ~7 sheets, nothing visible), (B) upload the animation frames as extra sheets after that, frame 0 until then. Census lab builds need `-EvictSyms ndsMPFindLineYakumonoID -EvictSections` five diagnostics DTCM words (`build_gate_rom.ps1`) |
| V1-V3 particle animations | in progress | `1c2a1f870ad`: (A) the countdown's traffic/cloud atlases (57,344 B) released when its process ends; (B) the frames the base cells lack in seven animation sheets after the nine base ones (the countdown's bytes; 17 sheets starved the texture cache: Hyrule Temple heavy +146K WORK P95, two native failures), uploaded one a frame into A+B's holes (A+B are locked after setup), textures whole in `QUAD_ANIMATION_PRIORITY` order, every second 1-tick frame left out. 25 textures animate (92 frames): the whole owner list but the two 64x64 dusts (DustDash, DustHeavy; 0.5x or more VRAM: the owner's call). `8032b2b2a5a`: Dream Land's leaf, four frames. Gate `s71` short sweep: 16 sheets resident, 0 refusals, 0 texture rejects, 0 quad misses, digests identical (one resync). Play `play-p8i` boots clean. Open: the two dusts (owner), Ness PK Fire / Yoshi egg frames (fighter banks, past the budget), captures beside N64 |
| S2 fault injection (squeeze, slow storage, long soak, laps, fuzz) | open | lab flags only |
| S4 stalls (load ledger, storage waits, menu spikes) | open | |
| S5 freeze report gaps | open | menus, loads, before the first frame; identity fields |
| S6 open bug rows | in progress | Yoshi-win Results freeze, Master Hand stiff, FD backgrounds, Palutena's Results dress, wireless start on a slow card. Dream Land with the heavy roster (one native failure a match since `s67`): every shield fell back -- the shield quad's texture (`ndsEFManagerShieldTexture`) is made at the first shield, after the admission locked VRAM A+B, and the stage left nothing else free (names 0, 0 quad draws); the fallback then walked the shield's DL-link list as a display list (FTManagerCommon 0x2F0, NO_PROGRAM, 159 records). Fix built as `s73`: the upload takes A+B's holes once, the fallback is the source proc (native entry shield, 0x248). The lean decline is still open |
| R0 RAM ledger and buffer peaks | open | after D (owner order) |
| R1 cold renderer code as Thumb -Os | done | `aad10df7436`: 69 setup functions with no cycle in 40 battle profiles (`build-prof-s70`) get `NDS_RENDERER_COLD`; cold submit/draw paths (fighter or item absent from the profiled rosters) stay ARM. Image -21,504 B (s69 -> s71); short sweep P95 against s67: HT heavy flat (program 1 segment again), HT mix A -125K, Congo heavy -50K / mix A -69K, Saffron heavy -85K / mix A -137K, cast flat; digests identical after one resync. Next: the rest of the renderer's cold set needs profiles with every fighter and items on (76 KB of cold draw code), and scene_backend's cold parts at -Os |
| R2-R9 RAM levers | open | ranked in plan section 4; R4's second printf engine already gone (`41636f74b0f`, 7 KB) |
| R10 per-fighter code overlays | allowed (owner Q2) | only if R1-R9 leave the heavy mixes short |
| P0 CPU ledger (band vs median census) | in progress | 49 per-function profiles on `build-prof-s70` (`artifacts/performance/2026-10-23_s70[pe]-*-profile`: cast, heavy and mixes on every VS stage, every original fighter with items). Heavy roster, share of all cycles: idle VBlank wait 19.1%, `ndsFtrLeanKernelCompose` 4.9%, `ndsRendererCommitNativeStageSegment` 2.4%, pose parse/play/update 5.0%, soft float (`__aeabi_fadd`/`fmul`) 2.3%, `ndsStageGxCommitFast` 1.3%, floor/wall sweeps 2.4%, `ndsMObjStepScaled` 0.75%, `ndsRendererMtxMulAffine20p12` 0.7%. Next: band vs median (the P95 frames' mix) |
| P1-P11 CPU levers | open | ranked in plan section 5; after D, alternate with R by gain over risk |
| P12 lower update rates | gated | owner's yes, only after P1-P11 |
| Spends (R's bytes into stage programs, lean spares, the motion arena) | open | Hyrule Temple's whole program with the heavy roster and the mixes first |
| Final: play ROM, S1 matrix, worst-case search on the final HEAD | open | |

## Log

Newest first. One line a batch: date, item, commit, proof, numbers.

- 2026-10-23 -- owner row (BUGS.md, Wario): Master Hand "glitching around" was my S1 campaign audit's runner
  window (`au-c7`, Wario, 17:44-17:52; the video 17:49:40, melonDS at 120+ fps), not the play ROM. The
  walk-only KO helper skipped Master Hand by the battle-state slot's kind; after a whole ladder slot 2 still
  read a polygon (fkind 19) with Master Hand's live GObj, so Master Hand dropped below the stage every 45
  ticks and the fight went to Continue. `ee33ad992cd` reads the live FTStruct (kind and port); `walk-u20`,
  the same ladder: Master Hand cycles its statuses, dies, Stage Clear, the ending.
- 2026-10-23 -- H-2: the ten host areas collect again (`1780ca40d43`); four more suites follow the code
  (`2710bbac5a3`, 41 tests; the divider header's host arm leaves ARM and Thumb code identical).
- 2026-10-22 -- D1: purpose headers, `fade4d1e094` (src/port) and `cab524db6f9` (src/nds, include, src/host);
  comments only, every unit preprocess-identical in the five configs.
- 2026-10-22 -- D0b: 885 macros nothing names (`454132e40a6`, 971 lines in 45 files, three rounds to a
  fixpoint; the scan reads tracked sources, generated sources, overlay patches, scripts, the Makefile and
  linker scripts). Every unit preprocess-identical in the five configs; the play ROM rebuilt after
  (`play-p8f`) has ARM9/ARM7 byte-identical to `play-p8e` and every NitroFS path on identical data; host
  tests 0 new failures.
- 2026-10-22 -- D0: 11 husk files (`1c20ddf249d`); scene_backend.o and nds_renderer.o non-debug sections
  identical in the five configs.
- 2026-10-22 -- owner: Ike's and Palutena's congratulations art (`assets/congra/c19.png`, `c20.png`) built
  into `play-p8e` (plates 0x1F66-0x1F69 in NitroFS, regenerated from the new PNGs).
- 2026-10-22 -- S1 campaign: all 32 fighters to the ending, 0 native failures, lowest heap 28,388.
- 2026-10-22 -- E1: the P1/P2 verifier framework (`1371dee1858`; 33 scripts and `HARNESSES.md`, chosen by a
  reference graph); `VERIFYING.md` keeps its live procedure; three comment edits preprocess identically.
- 2026-10-22 -- C12: `renderer_adapter_stage.c` (14,929 lines) into 4 slices (1,764 to 3,152) and
  its body's two phases into in-place fragments (3,166 and 1,825; `ca7f94b6c01`); `scene_backend`
  preprocess-identical in the five configs. The body is 27,680 B of code with ~50 submit arms of
  one shape: an R lever (a table-driven submit) once D is done.
- 2026-10-22 -- C11: `nds_startup.h` into topic headers it includes in place (`4f08be8dfa2`);
  every translation unit of the full `make -n -B` listings identical.
- 2026-10-22 -- C/R4: 3,224 dead diagnostic counters out (`556e7c2d1dd`; 7,247 lines from 21 files:
  `diagnostics_renderer_census.c` 4,129 -> 905, `nds_startup.h` 8,601 -> 5,379,
  `taskman_seam_core.c` 4,587 -> 3,862). Ship play ROM `play-p8c` -> `play-p8d`: every function's
  size unchanged, `.main.bss` -64 B.
- 2026-10-22 -- C4-C10: `reloc_backend_compat_shims.c` (`b63827ac393`), `reloc_backend_mp_collision.c`
  (`920b26e62d4`), `nds_menu_shell_css.c` (`21cdea01e1f`), `nds_ifcommon_oam.c` (`e0d3153f3e3`),
  `reloc_backend_movement.c` (`cfc9ff8bc80`), `renderer_adapter_matrix.c` (`8e6cc8a43a0`),
  `nds_renderer_assets.c` (`58bb68a3649`), each into topic slices, preprocess-identical in the five
  configs; the split tools in `scripts/tools/` (`ce72f56faef`).
- 2026-10-22 -- owner rows: Link's entry wave repeats its streaks around the ring (`71dbc74efe4`);
  Palutena's and Ike's congratulations screens checked on the current code (their own art, not
  Fox's).
- 2026-10-22 -- C3: `nds_renderer_textures_effects.c` (16,294 lines) into the kept file and 7 slices
  (1,276 to 3,025 lines; `9baf0db6e5f`); `nds_renderer.o` preprocess-identical in the five configs;
  the 5 affected test files fail exactly as before (2 pre-existing).
- 2026-10-22 -- owner rows: the off-screen magnifier (`49170d5e19d`: bubbles at the screen edge, the
  culled fighter redrawn inside by its lean list, rim and arrow; ship ITCM -8 B) and Link's entry
  wave (`afad0c94ab2` own polygon ID, `d1165f25cbc` its green texture); witnessed on
  `build-sweep-s63`/`s64`; play ROM `play-p8b` boots clean.
- 2026-10-22 -- I: `PERF_LEDGER.md`'s July-September rows to `docs/archive/` (`327946d2392`).
- 2026-10-22 -- I: P7's in-progress rows closed with their evidence (`ce376f1be3e`).
- 2026-10-22 -- S1 tools: a slot's DLDI image regenerates while other slots run (`5075604365a`;
  the Training audit had logged 32 empty runs); the shield palette test's C1 shadowing fixed
  (`8d16bb7b50d`).
- 2026-10-22 -- H-1: the S1 and gate tools in `scripts/tools/` (22 files, README); parse-clean,
  `boot_check.ps1` on `play-ram1`: scene 1, 0 storage failures.
- 2026-10-22 -- C2: `reloc_backend_assets.c` (18,246 lines) into 8 slices of the
  `scene_backend` unit (1,269 to 3,545 lines); `scene_backend.o` preprocess-identical in the
  five configs; the 14 affected test files fail exactly as on HEAD (18 pre-existing), the
  four script tests pass; one stale Phase A check fixed (`test_results_demo_submotion_routes.py`).
- 2026-10-22 -- R4 part: newlib's stream printf engine out of the link (`41636f74b0f`),
  `.main` -7,168 B on `play-ram1`; boots clean.

- 2026-10-22 -- owner: CPU hunting beside the RAM hunting (plan section 5, loop P).
- 2026-10-22 -- owner answers Q1-Q3; I: `AGENTS.md` and `docs/HANDOFF.md` route to P8.
