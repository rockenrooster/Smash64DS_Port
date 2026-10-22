# Old machinery deleted (2026-10-20)

Owner 2026-10-20 (docs/OWNER_QUESTIONS.md 6): "Anything not used anymore for completing this
project seems like a good candidate." Each group is its own commit, so any one can be brought back
with `git revert` or `git show <commit>^:<path>`.

## Build targets (9) -- `bdd8704087a`

`smash64ds-p2-1b-scene-walk-hwtri`, the Task 37 and Task 44 device pairs (`task37-on/off`,
`task44-on/off`), the BUGS.md #9 floor-seam pair (`bug9-rigidon/off`),
`smash64ds-battle-playable-bgm-off-hwtri` (with `NDS_BGM_FALSIFIER_OFF`, already refused by the
Makefile) and `smash64ds-task10-hardware-calibration` (with its payload, its flag and two scripts).
The other 19 targets are still called by live scripts (the gate, the results lab, the shell and
battle-playable verifiers).

## Top-level scripts (174)

A script went when all of these held: last changed before 2026-10-01; not called by the Makefile;
not named by any script, tool or doc still in use (README, HANDOFF, STABILITY_AUDIT, P6, P7,
AGENTS), followed transitively -- 41 of the first list of 215 stayed because a kept verifier calls
them (verify-all.ps1, verify-dev-fast.ps1 and their checks).

### Last changed in July (51)

`New-Smash64DSSnapshot.Legacy.ps1`, `benchmark-renderer-cpu-prep-no-gx.ps1`, `benchmark-renderer-triangle-noop.ps1`, `benchmark-renderer-warm-no-upload.ps1`, `build-freeze-diagnostics.ps1`, `capture-nogba.ps1`, `census-aobj-chain-layout.ps1`, `census-fighter-gameplay-joints.ps1`, `census-vsresults-blit.ps1`, `check-battle-playable-texture-census.ps1`, `check-lbcommon-sin-table-dedup.py`, `check-native-light-sidecar.ps1`, `check-printf-integer-only.ps1`, `check-pupupu-water-rgb256.ps1`, `check-snapshot-build-context.ps1`, `check-snapshot-hygiene.ps1`, `check-stage-aot-falsifier.ps1`, `check-task12-renderer-codegen.ps1`, `check-task16-addsub.ps1`, `check-task16-compare-itcm.ps1`, `check-task16-compare.ps1`, `check-task16-i2f.ps1`, `check-task24-evidence-manifest.ps1`, `check-task37-state-hash-determinism.ps1`, `check-task9-phase2-fcmpeq.ps1`, `check_nds_native_stage_arm.ps1`, `compare-renderer-fast-raw.ps1`, `compare-task37-ftstruct-snapshot.ps1`, `debug-nogba.ps1`, `probe-task52-replay-active.ps1`, `probe-task56-fighter-path.ps1`, `sample-runtime-speed.ps1`, `suggest-verification.ps1`, `task65_subsystem_census.py`, `task81_partition_census.py`, `verify-battle-playable-cpu-harness.ps1`, `verify-battle-playable-platform-semantics.ps1`, `verify-battle-playable-throw-release-recovery.ps1`, `verify-nogba-smoke.ps1`, `verify-opening-boundary.ps1`, `verify-opening-movie-speed.ps1`, `verify-opening-skip.ps1`, `verify-stage-aot-device-falsifier.ps1`, `verify-task16-addsub-arm9.ps1`, `verify-task16-compare-arm9.ps1`, `verify-task16-i2f-arm9.ps1`, `verify-task16-i2f-state-hash-ab.ps1`, `verify-task37-itcm-state-hash-ab.ps1`, `verify-task9-phase2-fcmpeq-arm9.ps1`, `verify-task9-state-hash-ab.ps1`, `verify-title-boundary.ps1`

### Last changed in August (64)

`analyze-dcache-stalls.py`, `analyze-fighter-draw-reconciliation.py`, `analyze-io-lane-series.py`, `analyze-load-frame-exclusion.ps1`, `analyze-profile-region-split.py`, `analyze-tick-hud-excursion.ps1`, `audit-p1-vfx-textures.py`, `census-call-frames.py`, `census-call-sites.py`, `census-call-transitions.py`, `census-dcache-working-set.py`, `census-fetch-density.py`, `census-frame-candidates.py`, `census-icache-placement.py`, `census-icache-temporal.py`, `census-literal-pools.py`, `census-softfloat-callers.ps1`, `census-text-owners.py`, `census-texture-demand-postko.ps1`, `census-tick-hud-p95-set.py`, `census-visit-counts.py`, `check-task25r-phase-matrix.ps1`, `check_fcmp_exact.py`, `check_s32tof32_exact.py`, `classify-softfloat-caller-phase.py`, `compare-capture-pair.ps1`, `compare-tick-hud-arms.py`, `compare-tick-hud-buckets.ps1`, `firegrind_bake.py`, `fox_gun_screen_bounds.py`, `placement-layout-model.py`, `probe-arena-overflow.ps1`, `probe-attachment-position.ps1`, `probe-collision-fixed-domain.ps1`, `probe-dk-cargo.ps1`, `probe-effect-texture.ps1`, `probe-fighter-anim-state.ps1`, `probe-fighter-state-delta-census.ps1`, `probe-flame-quad-miss.ps1`, `probe-fox-crouch-collision.ps1`, `probe-fox-gun-matrix.ps1`, `probe-fpshud-publication.ps1`, `probe-ft-pose-oracle.ps1`, `probe-g1-site-cache-liveness.ps1`, `probe-halo-attach.ps1`, `probe-ko-blast.ps1`, `probe-ko-vfx.ps1`, `probe-match-window.ps1`, `probe-objanim-runaway.ps1`, `probe-row6-force-abort.ps1`, `probe-shield-attach.ps1`, `probe-shield-submit.ps1`, `probe-vfx-contracts.ps1`, `probe-whispy-blink-script.ps1`, `probe-whispy-eye-texture.ps1`, `render-particle-quad-atlas.py`, `task37_softfloat_callers.py`, `verify-audio-bgm-results.ps1`, `verify-p2-donkey-charge-presentation.ps1`, `verify-p2-fox-laser-colanim.ps1`, `verify-p2-samus-cpu-determinism.ps1`, `verify-p2-samus-damagefly-tour.ps1`, `verify-p2-samus-tumble-tour.ps1`, `verify-task39-visual-effects.ps1`

### Last changed in September (59)

`analyze-inline-attribution.py`, `analyze-task34-stage-stream.ps1`, `census-fighter-draw-phases.ps1`, `census-profile-pc-per-region.py`, `census-results-frame-cost.ps1`, `census-stage-run-phases.ps1`, `census-texture-key-rebuild.ps1`, `check-alpha-mux-blast-radius.py`, `check-battle-playable-static-textures.ps1`, `check-fighter-draw-global-texture-state.py`, `check-gbi-packets-fixtures.ps1`, `check-r2-light-stretch.py`, `check-r2-shade-twin.py`, `check_kirby_copyfox_colanim_override.py`, `check_mp_damage_proc_matches_source.py`, `check_video_blackout_seam.py`, `probe-battle-progress.ps1`, `probe-fox-gun-overlay.ps1`, `probe-g1-site-cache-census.ps1`, `probe-kirby-copylink-owner.ps1`, `probe-modelpart-chain.ps1`, `probe-ness-upb-real.ps1`, `probe-ness-upb-shell-real.ps1`, `probe-ness-upb-vfx.ps1`, `probe-purin-owner.ps1`, `probe-results-confetti.ps1`, `probe-samus-owner.ps1`, `probe-shell-four-kind.ps1`, `probe-shield-vfx.ps1`, `probe-trace-symbols.ps1`, `test-parallel-diagnostic-isolation.ps1`, `test_check_build_flag_census.py`, `test_diagnostic_reset_words.py`, `test_frontend_overlay.py`, `test_hyrule_particle_bank.py`, `test_ifcommon_announcements.py`, `test_native_failure_verifiers.py`, `test_native_only_rom.py`, `test_particle_packet.py`, `test_particle_view.py`, `test_replay_digest_compare.py`, `test_samus_roll_runtime_normalizer.py`, `test_samus_roll_spline.py`, `test_staffroll_data_width.py`, `validate-smash64ds-skills.py`, `verify-battle-playable-crowd-envelope-timing.ps1`, `verify-battle-playable-damagefall-recovery.ps1`, `verify-p2-falcon-effects.ps1`, `verify-p2-falcon-entry-native.ps1`, `verify-p2-falcon-modelpart.ps1`, `verify-p2-link-specials.ps1`, `verify-p2-ness-specials.ps1`, `verify-p2-samus-attack-tour.ps1`, `verify-p2-samus-entry-native.ps1`, `verify-p2-samus-matrix.ps1`, `verify-p2-samus-native-owner.ps1`, `verify-p2-samus-shield-roll.ps1`, `verify-p2-samus-state-tour.ps1`, `verify-p2-samus-weapon-transforms.ps1`

## The software-triangle and legacy fighter-executor branches -- `1c8636a6632`

Every live build (play/ship, the gate sweep, the profile build, the walk ROM) compiles
`NDS_RENDERER_HW_TRIANGLES=1` and `NDS_FIGHTER_LEGACY_EXEC=0`, so the other branches -- the
CPU triangle rasteriser, fighter hierarchy mode 7, the per-root hardware executor, the raw-path
tables and every `HW_TRIANGLES=0` path -- were resolved out of 47 hand-written sources (1,948
lines) by a partial unifdef that only decides conditions naming those two flags (others kept,
simplified). The Makefile forces both values; the config header still defines them for the
generated stage and item code (`*.exec.inc`) that tests them. Proof: every config's
`make -n -B` compile list preprocessed (`-E -P`) before and after, 483 translation units x 4
configs, byte-identical.

## The P1 battle targets and the Results lab (13 + 1) -- `5da3bb2599b`

`smash64ds-battle-playable-hwtri` (the frozen P1 battle ROM) and its tick-HUD, proof, audio-FGM,
fast, Task 49 differ, freeze-diagnostics on/off, coarse (and its three variants) and forensic
siblings, the `p1`/`p1-tick` goals, and `smash64ds-results-lab-hwtri` (it no longer built: the
P7 menu shell's VS Mode, Options and Record calls fail its frontend-overlay check; the walk ROM
reaches Results). The measurement block is now the gate target's alone. Every live config's
`make -n -B` command list is unchanged. The sampler, the profile census, the soak watch and the
boot-headroom check default to the gate target.

## Build flags every build fixes (267) -- Phase A of the readability pass (2026-10-21)

Owner 2026-10-21: "human readability for code and refactoring". 267 of the 394 `NDS_*` flags the
config header defines held one value in every build directory made in the five days before
(160 of them: play/ship, the gate sweep, profile, walk, multiplayer and lab builds). Their
`#if`/`#elif`/`#ifdef` conditions are resolved out of 251 clean hand-written `.c`/`.h` sources
(34,043 lines gone): the profile, census, benchmark, ledger and per-task probe branches of the
flags that are always 0, and the always-true gates (`NDS_P4`, `NDS_P2_<FIGHTER>`,
`NDS_P2_ITEM_CORE`, ...) of the flags that are always 1. Conditions that also test a flag that
varies (the tick HUD, the profile and walk words, the lab rosters, the multiplayer labs) are
kept, simplified. Untouched: generated sources, every `*.inc` (the stage/item executors their
generators and tests read), and the files with uncommitted work in progress. The config header
still defines every flag, so the generated code that tests them is unchanged.

Proof: each live config's `make -n -B` compile list -- the gate sweep `s56`, the profile build
`s52`, the walk ROM `u18`, the play ROM `p7w` and the multiplayer lab `net-rep3b` -- preprocessed
(`-E -P`) before and after: 483 translation units x 5 configs, byte-identical; the rebuilt gate ROM
(`s57`) has every code and data section byte-identical to `s56`. The Python tests that read these
sources pass as before (8 failures are older than the change: stale harness stubs and drifted
reference hashes). Tool: a partial unifdef (`unifdef_known.py`, folding `x && 0` and `x || 1`). 2026-10-22: the
six files git listed as modified at the time only for their line endings (the Fox blaster and
gun, the status callback aliases, the renderer census, the diagnostics state and the ftmain
runtime) are resolved the same way, 570 lines, all 483 units again preprocessing identically.
Then the comparisons Phase A left half-folded, `#if 0 == NDS_RENDERER_BENCHMARK_NONE` and its
kin: the benchmark modes are fixed constants (`nds_renderer.h`: NONE 0, TRIANGLE_NOOP 1,
CPU_PREP_NO_GX 2, WARM_NO_UPLOAD 4), so the 86 conditions in nine renderer files resolve,
1,046 lines, preprocessing identical again.

Then the switches Phase A could not see, because no build sets them: macros the sources define
for themselves. Seven derived ones are built only from resolved flags (`NDS_FIGHTER_PACKET_LIVE`,
the battle FPS/lower/phase HUD gates, `NDS_R2_DELTA_CENSUS`, `NDS_RENDERER_GX_RECORD_FIGHTER`,
`NDS_TASK36_REJECT_TRACE`; then `NDS_FTR_LEAN_KERNEL_ITCM` from those); thirteen are hard-coded
0/1 toggles (`NDS_FTR_LEAN_LIVE`, `NDS_FTR_LEAN_ATTR_LIVE`, `NDS_FTR_LEAN_KINDS_BUILD`,
`NDS_PLAYERS_VS_COMPACT_PREVIEW`, `NDS_ENTRY_PACKET_LIVE`, `NDS_R2_ANIM_ZERO_COPY`,
`NDS_RENDERER_HW_TEXTURE_KEY_SHADOW`, `NDS_P2_NESS_SPECIAL_TOUR`, `NDS_P2_YOSHI_BUG_TOUR`,
`NDS_NATIVE_HAS_IMAGE_BACKED_FIGHTER_OWNERS`, `NDS_R2_SHADE_SKIP_SOFT_LIGHT`,
`NDS_R2_CFX_RING_WRAP`, `NDS_FTR_CARVE_LIVE`); and seven are `#ifndef` defaults nothing overrides
(`NDS_TRANSITION_HOLD`, `NDS_RENDERER_FRAME_SUMMARY_COUNTERS`, `NDS_P2_SOBJ_FAIL_CENSUS`,
`NDS_FTR_LEAN_EVICT_PRODUCTION`, `NDS_R2_ANIM_PARSER`, and the lean kernel's sine-table pair
`NDS_FTR_LEAN_SIN_ITCM`/`_DTCM`). Their conditions resolve and the macros go: 4,466 lines in 22
files, among them the old fighter executor (`#if !NDS_FTR_LEAN_LIVE`, 980 lines), the
non-compact VS preview, the black transition cover and the Ness/Yoshi proof tours. Every unit
preprocesses identically in the five configs but one, which loses a single `((void)0);`. Kept:
`NDS_FTR_LEAN_LIVE` itself (the generated owner images still test it), the playtest stage
knobs `NDS_1P_PLAYTEST_START_STAGE`/`NDS_1P_WALK_START_STAGE` (owner tools, -1 by default), and
the generated headers' own toggles.

Resolved to 0 (125): `NDS_ANIM_JOINT_AUDIT`, `NDS_AOBJ_EVENT32_HASH_ORACLE`, `NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS`, `NDS_CAMERA_FRAME_WITNESS`, `NDS_DEBUG_HUD`, `NDS_DREAMLAND_CARD_CULL`, `NDS_DREAMLAND_CARD_CULL_MASK0`, `NDS_DREAMLAND_CARD_CULL_MASK1`, `NDS_DREAMLAND_DS_MESH`, `NDS_ENTRY_EFFECT_DIAG`, `NDS_FIGHTER_ANIM_AUDIT`, `NDS_FOX_BLASTER_BORE_OFFSET_Y`, `NDS_FTR_LEAN_KTIME`, `NDS_FTR_LEAN_PHASE_TICKS`, `NDS_FTR_PRE_WALK_CENSUS`, `NDS_FT_POSE_ORACLE`, `NDS_HARNESS_FAST_LOGIC`, `NDS_HARNESS_FAST_PRESENT_ON_REQUEST`, `NDS_HARNESS_ITEMS_ON`, `NDS_IFCOMMON_HYBRID_OAM`, `NDS_LAB_CULL_PROBE`, `NDS_LAB_JOINT_CAP`, `NDS_LAB_NO_CULL`, `NDS_LAB_NO_DCACHE`, `NDS_LAB_POSE_JOINT_CAP`, `NDS_LAB_STAGE_BINDING_CENSUS`, `NDS_LAB_TINT_SHIFT`, `NDS_LAB_UNCACHED_SINTABLE`, `NDS_NATIVE_OWNER_IMAGE_VERIFY`, `NDS_NET_LAB_GATE_RADIO`, `NDS_NET_LAB_HANG`, `NDS_NET_LAB_LEAVE`, `NDS_NET_LAB_MATCH`, `NDS_NET_LAB_PING`, `NDS_NET_LAB_SOAK`, `NDS_NET_LAB_STARTLOSS`, `NDS_NET_LAB_TEAMS`, `NDS_P2_EFFECT_CENSUS`, `NDS_P2_KIRBY_COPYLINK_PROOF`, `NDS_P2_LINK_BOMB_TOUR`, `NDS_P2_LINK_SPECIAL_TOUR`, `NDS_P2_MISC_SPLIT`, `NDS_P2_NESS_VFX_PROOF`, `NDS_P2_SAMUS_ATTACK_TOUR`, `NDS_P2_SAMUS_DAMAGEFLY_TOUR`, `NDS_P2_SAMUS_STATE_TOUR`, `NDS_P2_SAMUS_TUMBLE_TOUR`, `NDS_P2_SHELL_ARGMAX_ROSTER`, `NDS_P2_YOSHI_BUG_PROOF`, `NDS_PREVIEW_HALT_NONFATAL`, `NDS_R204_FPSHUD_SHADOW`, `NDS_R2_ANIM_CENSUS`, `NDS_R2_ANIM_CUT_ROUTE`, `NDS_R2_ANIM_Q_ITCM_ON`, `NDS_R2_BOTH_CPU`, `NDS_R2_CFX_HWMATH`, `NDS_R2_COLLISION_FIXED`, `NDS_R2_COLLISION_FIXED_DISPATCH`, `NDS_R2_COLLISION_FIXED_NARROW`, `NDS_R2_COLLISION_FIXED_NARROW_DISPATCH`, `NDS_R2_COLLISION_L7_ORACLE`, `NDS_R2_DRAW_SUPPRESS_MASK`, `NDS_R2_FIGHTER_EPOCH_STATE_PROOF`, `NDS_R2_FIGHTER_RUN_PROOF`, `NDS_R2_FIGHTER_SHADE_PROOF`, `NDS_R2_FIGHTER_SHADE_SKIP`, `NDS_R2_FIGHTER_SOFT_LIGHT_KEEP`, `NDS_R2_FIGHTER_STATESPAN_SKIP`, `NDS_R2_FIREBALL_MAP_COLL_DEBUG`, `NDS_R2_FLASH_PROBE`, `NDS_R2_FTANIM_TRACK`, `NDS_R2_FTANIM_TRACK_ORACLE`, `NDS_R2_FTR_CONTRACT_CENSUS`, `NDS_R2_HWMATH_BENCH`, `NDS_R2_HWMATH_ROUTE`, `NDS_R2_KO_STRESS`, `NDS_R2_LOADFRAME_TIMING`, `NDS_R2_MATERIAL_DYNAMIC`, `NDS_R2_POSITION_PROBE`, `NDS_R2_REBIRTH_HALO_PACKED_PROJECTED`, `NDS_R2_REBIRTH_HALO_PHASE_PROFILE`, `NDS_R2_REBIRTH_HALO_SPLIT_NOZ`, `NDS_R2_RELOC_FIXUP_TIMING`, `NDS_R2_SCENE_LOOP_WALK`, `NDS_R2_SECOND_ENTRY_DIAG`, `NDS_R2_SIM_MAC_SHADOW`, `NDS_R2_SPAN_LEAN_TIMING`, `NDS_R2_STAGE_ACTORS_PROOF`, `NDS_R2_STAGE_ROUTE_PROBE`, `NDS_R2_STRIP_ROUTE`, `NDS_R2_UNLIT_VERTEX_EPOCH`, `NDS_RENDERER_BENCHMARK_MODE`, `NDS_RENDERER_HW_DEBUG_TEXTURE_ONLY`, `NDS_RENDERER_M2_DETAILED_LEDGER`, `NDS_RENDERER_M3_PHASE0_PROFILE`, `NDS_RENDERER_PROFILE_LEVEL`, `NDS_RENDERER_SCREEN_SPACE_CENSUS`, `NDS_RENDER_ECONOMY`, `NDS_SCENE_MIP_CACHE_LAB`, `NDS_TASK103_STAGE_RUN_PHASE`, `NDS_TASK107_RENDER_STATE_CENSUS`, `NDS_TASK108_SITR_CALLBACK_CENSUS`, `NDS_TASK20_STACK_PROFILE`, `NDS_TASK22_WALLPAPER_RUN_LAB`, `NDS_TASK29_GX_CENSUS`, `NDS_TASK34_STAGE_STREAM_CENSUS`, `NDS_TASK37_LAYOUT_PROBE`, `NDS_TASK37_LAYOUT_PROBE_ITCM`, `NDS_TASK37_PROFILE_RESULTS`, `NDS_TASK49_GX_DIFFER`, `NDS_TASK51_STAGE_NATIVE`, `NDS_TASK55_STAGE_GEOM`, `NDS_TASK68_FALLBACK_CENSUS`, `NDS_TASK75_LOAD_CENSUS`, `NDS_TASK90_SHADE_CENSUS`, `NDS_TASK91_DRAW_PHASE_CENSUS`, `NDS_TASK93_TEXKEY_CENSUS`, `NDS_TASK9_FLOAT_CENSUS`, `NDS_TASK9_FTSTRUCT_SNAPSHOT`, `NDS_TASK9_FTSTRUCT_SNAPSHOT_UPDATE`, `NDS_TASK9_STATE_HASH`, `NDS_TASK9_STATE_HASH_SKIP_CONTROLLERS`, `NDS_TEX_IDENT_SHADOW`, `NDS_TICK_HUD_SRC_SPLIT`, `NDS_WHISPY_AOT_TICKS`.

Resolved to a nonzero value (142): `NDS_AOBJ_EVENT32_LEDGER_INDEX=1`, `NDS_BATTLE_PROFILE=1`, `NDS_DEV_LIVE_INPUT_PREVIEW=1`, `NDS_FAST_MEM=1`, `NDS_FIGHTER_ANIM_CYCLER_KIND=-1`, `NDS_FTPARTS_FLAT_MAX=96u`, `NDS_FTPARTS_FLAT_SLOTS=4u`, `NDS_FTR_LEAN_RELAXED=1`, `NDS_FT_POSE=1`, `NDS_FT_POSE_HOLD=1`, `NDS_IF_GAMESTATUS_COMPACT=1`, `NDS_IMPORT_BATTLESHIP_AUDIO_ASSETS=1`, `NDS_IMPORT_BATTLESHIP_AUDIO_BGM=1`, `NDS_IMPORT_BATTLESHIP_AUDIO_FGM=1`, `NDS_IMPORT_BATTLESHIP_BATTLE_PLAYABLE=1`, `NDS_IMPORT_BATTLESHIP_EFFECT_MANAGER=1`, `NDS_IMPORT_BATTLESHIP_FOX_BLASTER=1`, `NDS_IMPORT_BATTLESHIP_FOX_REFLECTOR=1`, `NDS_IMPORT_BATTLESHIP_FOX_SPECIAL_HI=1`, `NDS_IMPORT_BATTLESHIP_FTCOMPUTER=1`, `NDS_IMPORT_BATTLESHIP_FTMAIN=1`, `NDS_IMPORT_BATTLESHIP_FTMANAGER=1`, `NDS_IMPORT_BATTLESHIP_FT_PUBLIC=1`, `NDS_IMPORT_BATTLESHIP_IFCOMMON=1`, `NDS_IMPORT_BATTLESHIP_MARIO_FIREBALL=1`, `NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_HI=1`, `NDS_IMPORT_BATTLESHIP_MARIO_SPECIAL_LW=1`, `NDS_IMPORT_BATTLESHIP_NORMAL_MOVESET=1`, `NDS_IMPORT_BATTLESHIP_VS_RESULTS=1`, `NDS_IMPORT_BATTLESHIP_WEAPON_MANAGER=1`, `NDS_NATIVE_OWNER_IMAGE_KIRBY=1`, `NDS_NATIVE_OWNER_IMAGE_NESS=1`, `NDS_NATIVE_OWNER_IMAGE_PIKACHU=1`, `NDS_NATIVE_OWNER_IMAGE_PURIN=1`, `NDS_NATIVE_OWNER_IMAGE_SAMUS=1`, `NDS_NATIVE_OWNER_IMAGE_YOSHI=1`, `NDS_NATIVE_STAGE_GENERATED_SEGMENT0_ENABLE=1`, `NDS_NET_LAB_PING_FRAMES=600`, `NDS_NET_LAB_STARTLOSS_ROUNDS=2`, `NDS_P2_ARM9_WRAM=1`, `NDS_P2_HURTBOX_REJECT=1`, `NDS_P2_ITEM_CORE=1`, `NDS_P2_KIRBY=1`, `NDS_P2_NESS=1`, `NDS_P2_PIKACHU=1`, `NDS_P2_PROOF_FIGHTER0=-1`, `NDS_P2_PURIN=1`, `NDS_P2_SAMUS=1`, `NDS_P2_YOSHI=1`, `NDS_P4=1`, `NDS_P4_BOWSER=1`, `NDS_P4_CRASH=1`, `NDS_P4_DEDEDE=1`, `NDS_P4_FALCO=1`, `NDS_P4_GANONDORF=1`, `NDS_P4_IKE=1`, `NDS_P4_LANKY=1`, `NDS_P4_MARTH=1`, `NDS_P4_METAKNIGHT=1`, `NDS_P4_MRGAW=1`, `NDS_P4_PALUTENA=1`, `NDS_P4_PEACH=1`, `NDS_P4_PIANO=1`, `NDS_P4_ROY=1`, `NDS_P4_SHEIK=1`, `NDS_P4_SNAKE=1`, `NDS_P4_SONIC=1`, `NDS_P4_WARIO=1`, `NDS_P4_WOLF=1`, `NDS_R2_ANIM_CACHE=1`, `NDS_R2_AOBJ16_PREBAKE=1`, `NDS_R2_BATTLEPACK=1`, `NDS_R2_BATTLEPACK_BLOB_BYTES=287904u`, `NDS_R2_BATTLEPACK_KEEP_CACHE=1`, `NDS_R2_CUBIC_FIXED=1`, `NDS_R2_DELTA_PATH_ITCM=1`, `NDS_R2_FIGHTER_GX_COMPOSE=1`, `NDS_R2_FIGHTER_HW_LIGHT=1`, `NDS_R2_FIGHTER_HW_MTX=1`, `NDS_R2_FIGHTER_MTX_DIRECT=1`, `NDS_R2_FIGHTER_NO_ORACLE=1`, `NDS_R2_FIGHTER_PACKET=1`, `NDS_R2_FIGHTER_RUN_MEMO=1`, `NDS_R2_FIGHTER_SHUFFLE_FOLD=1`, `NDS_R2_FIREBALL_NATIVE_MAP_COLL=1`, `NDS_R2_FIREBALL_QUAD=1`, `NDS_R2_FIREGRIND_NATIVE=1`, `NDS_R2_FIXED_SQRT=1`, `NDS_R2_FOX_BLASTER_GLOW_AOT=1`, `NDS_R2_FOX_BLASTER_QUAD=1`, `NDS_R2_FOX_CPU_DEFAULT=1`, `NDS_R2_FOX_GUN_OVERLAY=1`, `NDS_R2_FTANIM_STREAM=1`, `NDS_R2_FTANIM_TRACK_DISPATCH=1`, `NDS_R2_FTR_DRAW_MEMO=1u`, `NDS_R2_IMPACT_WAVE_NATIVE=1`, `NDS_R2_MAIN_PRESENT_GUARD=1`, `NDS_R2_PARTICLE_DRAW=1`, `NDS_R2_PARTICLE_RUNTIME=1`, `NDS_R2_REBIRTH_HALO_FAST_ADAPTER=1`, `NDS_R2_REBIRTH_HALO_FULL_OFFLOAD=1`, `NDS_R2_REBIRTH_HALO_GX_GROUP_MASK=0x3f`, `NDS_R2_REBIRTH_HALO_HW_LIGHT=1`, `NDS_R2_REBIRTH_HALO_NATIVE=1`, `NDS_R2_REBIRTH_HALO_PACKED_FIFO=2`, `NDS_R2_REBIRTH_HALO_SPLIT_MTX=1`, `NDS_R2_RESULTS_AFFINE=1`, `NDS_R2_RESULTS_LAYER_MEMO=1`, `NDS_R2_SHIELD_QUAD=1`, `NDS_R2_SOURCE_EFFECTS_PARTICLE=1`, `NDS_R2_STAGE_DIRECT=1`, `NDS_R2_STAGE_DMA=1`, `NDS_R2_STAGE_PREFLIGHT=1`, `NDS_R2_STAGE_VALIDATE_STRIDE=8`, `NDS_R2_STAGE_VIEWPROJ=1`, `NDS_R2_WHISPY_NATIVE_AOT=1`, `NDS_R2_WHISPY_NATIVE_TEXTURES=1`, `NDS_RENDERER_BATTLE_STATIC_TEXTURE_DEFAULT=1`, `NDS_RENDERER_FAST_RUN_DEFAULT=9`, `NDS_RENDERER_HW_TRIANGLES=1`, `NDS_RENDER_ECONOMY_OWNER_MASK=32`, `NDS_TASK104_STAGE_STATS_ELISION=1`, `NDS_TASK106_UPDATES_PER_PRESENT=2u`, `NDS_TASK16_FLOAT_ADDSUB=1`, `NDS_TASK16_FLOAT_COMPARE=1`, `NDS_TASK16_FLOAT_I2F=1`, `NDS_TASK32_DRAW_HOT_TEXT=1`, `NDS_TASK36_HW_COMPOSE=2`, `NDS_TASK37_ITCM_LEAVES=5`, `NDS_TASK37_ITCM_PORT=1`, `NDS_TASK39_FX_FLASH=1`, `NDS_TASK39_FX_SPRITES=1`, `NDS_TASK44_STAGE_STEADY=1`, `NDS_TASK53_REPLAY_ARENA_FIX=1`, `NDS_TASK56_FIGHTER_PRIMITIVES=2`, `NDS_TASK82_ITCM_REPACK=1`, `NDS_TASK85_ALIGNED_NATIVE_ACCESS=1`, `NDS_TASK86_MATRIX_COPY=1`, `NDS_TASK9_FLOAT_ITCM=1`, `NDS_TASK9_FLOAT_PHASE2=1`, `NDS_TASK9_STATE_HASH_REGION_MASK=0xFFFFFFFFu`, `NDS_TASK9_STATE_HASH_STRIDE=1u`.

## Definitions nothing names (25) -- Phase B of the readability pass (2026-10-21)

25 `static` functions and tables that nothing names outside their own definition (`git grep`
over `src/`, `include/` and `scripts/`, so no other config, host build, test or script can
reach them), 744 lines in 9 files: 12 superseded cliff, damage, jump and thrown shims in
`reloc_backend_compat_shims.c`; the opening action-preview and title-sprite normalizers and
their caches in `reloc_backend_assets.c`; the Mario/Fox proof slot lookup
(`reloc_backend_fighter_model.c`); a preview-material recorder (`opening_movie_backend.c`);
the menu shell's centre and CSS portrait helpers; the platform perf-counter update; two
particle camera helpers and the halo's no-Z matrix load. The compiler had already dropped
each one: the changed objects' code and data sections are byte-identical before and after in
all five live configs, and the rebuilt gate ROM (`s58`) has every symbol at its `s57` size,
byte-identical except the MRGAW tables (a staging regeneration renumbered his files; replay
digest identical).

## Functions every link discards (97) -- Phase B, second batch (2026-10-22)

97 extern functions that every live link discards (their `.text.<name>` sections in the linker
maps) and that nothing names beyond their definition and prototypes, 3,948 lines in 34 files:
the Mario/Fox proof era's 66 loop prepare/finalize and display hooks (`reloc_backend_mp_collision.c`
39, `reloc_backend_cliff_ledge.c` 25, `reloc_backend_movement.c` 2), nine platform helpers (the
scene mip-cache handshake, a rectangle draw, a DL-preview clear, a frame-boundary GX sample), the
renderer profile's state hash and recorders, the P2 hurtbox reject test, two Q-format animation
helpers, the Results OAM bake, and single helpers in the effect manager, the battle pack, the UI
kit, the net session, the preview pack, Source 2D and the reloc assets. Kept by rule: the source's
own functions in imported files (`func_ovl3_*`, `itLizardonFallUnusedSetStatus`) and the compat
stubs named after them (`ftPublicDefeatedAddID`, `lbCommonTan`), the libultra shims
(`osGetThreadId`, `osGetThreadPri`, `osYieldThread`, `osInvalICache`) and the two `__wrap_`
functions the Makefile's `--wrap` flags bind.

Proof: every changed translation unit (315 a config, the headers' prototypes included) compiles
in all five live configs with implicit declarations as errors. With the real flags, 301 objects
are byte-identical and 12 differ only by the deleted functions' sections (the 66 hooks were empty
in these configs). Two re-decided inlining: the Results OAM draw now inlines its one remaining
helper (`ndsResultsPrepareSObj`, 8 bytes net), and the reloc AObj16/AObj32 asset tests swapped
which is out of line (the same calls on the clip admission path). The rebuilt gate ROM (`s60`)
has every other symbol at its `s59` size, replay digests identical to `s59` over 1,960 samples
(the originals on Dream Land, Mr. Game & Watch x4 on Remix stage 41), WORK P95 within noise;
the ship-config play ROM links.

## Helpers the deletions left unused (81) -- Phase B, third batch (2026-10-22)

81 `static` helpers gcc reports unused in all five live configs once the two batches above were
gone, and that nothing else names, 5,233 lines in 7 files: the Mario/Fox hooks' loop reset and
pause helpers and the live-preview pause and diagnostics helpers they called
(`reloc_backend_mp_collision.c` 42, `reloc_backend_cliff_ledge.c` 26, `reloc_backend_movement.c`
6), the title-sprite normalizer and its table (`reloc_backend_assets.c`), a cliff collision
bridge, a DL-preview material store and three platform perf helpers. A second scan of the
changed units finds no more: the 22 statics still unused there are named by code that only
other builds compile, by comments or by scripts, and stay. The compiler had dropped each
already: both changed translation units (the `scene_backend.c` unity build and
`nds_platform.c`) are byte-identical before and after, section for section.

## The P1 proof harnesses' conditions (3,433 lines) -- Phase B, fourth batch (2026-10-22)

Every build directory selects scene harness 163 (`NDS_DEV_SCENE_HARNESS_BATTLE_PLAYABLE`: the
gate sweep, walk, play, net and profile builds) or 164 (the Results lab). The Mario/Fox proof
harnesses (walk, dash, jump, landing, the scheduler, controller and preview loops, the stage
collision and cliff tours, ...) were only selected by the P1 targets deleted in `5da3bb2599b`, and
the 64 `NDS_MARIOFOX_*_HARNESS` gates derived from them are 0 for both values. Every `#if` that
decides the same way for 163 and 164 is resolved (a partial unifdef with a two-valued flag), in 8
files: `reloc_backend_cliff_ledge.c` 2,387 lines, `taskman_seam_harness.c` 499,
`reloc_backend_fighter_model.c` 414 (its proof predicates now plain `return FALSE;`), and small
parts of the movement, compat-shim, asset, backup and scene-harness files. Proof: all 483
translation units preprocess byte-identically before and after in the five live configs.
The code those predicates still guard at run time goes in the next batch.

## The P1 proof code those predicates guarded (33,230 lines) -- Phase B, fifth batch (2026-10-22)

With the harnesses resolved, 79 proof predicates return FALSE in every build: the 73
`ndsFighterMarioFox*ProofEnabled` and the VS battle loop's six `*UpdateEnabled` gates. Every braced
`if`/`else if`/`else` chain and `(cond) ? TRUE : FALSE` return they decide is folded (a condition is
decided only when nothing else in it is a call, cast, member access or assignment, so no side
effect is dropped), each function's code after a now-unconditional top-level `return` is cut, and
whatever nothing names any more is deleted to a fixpoint: about 360 functions and 250 variables,
the proof tours' probes, recorders, loop runners and counters. `reloc_backend_cliff_ledge.c` goes
from 12,896 lines to 204, `reloc_backend_ftmain_status_compat.c` from 4,489 to 659, the
collision, damage and hit-pipeline proof slices lose 3,490, 735 and 648, mp_collision 6,887,
movement 4,094, compat shims 2,204. The VS battle update no longer calls six proof-loop gates
every frame (`scVSBattleFuncUpdate` 280 to 188 bytes) and ITCM gets 8 bytes back. Kept: the
statics the host reference renderers (`src/host/graphics_reference/`) use, the FGM table
`check-fgm-pack-coverage.py` parses, and the six predicates that depend on the build (the battle
playable and natural-motion proofs are the live path under harness 163).

Proof: every changed translation unit (88 a config) compiles in all five live configs; no new
`-Wreturn-type` warning (a first run's tail cutter took `return FALSE;` for a declaration of a
variable named FALSE; the warning diff found it and the batch was redone from the pre-batch tree);
with the real flags 85 objects are byte-identical and the other three differ only by the deleted
code, the shorter VS battle update and the unity build's re-decided inlining (one stage-load
helper, `ndsMPBuildTopologyCache`, 4 bytes longer). The gate ROM (`s61`) keeps every other symbol
at its `s60` size; replay digests are identical to `s60` over 1,960 samples on three configs (the
originals on Dream Land and on Remix stage 41, Mr. Game & Watch x4 on stage 41), WORK P95 within
noise (all three slightly lower); the ship-config play ROM links.

## Giant files split into topic slices -- Phase C of the readability pass (2026-10-22)

Nothing deleted here: the largest sources become consecutive slices of the same translation
unit, so the compiler sees the same text. `nds_renderer_native_common.c` (19,218 lines) keeps its
first 1,460 and continues in eight files the unity parent `nds_renderer.c` includes right after
it, in order: `_rebirth_halo`, `_dl_walk`, `_entry`, `_production_runs`, `_fighter_packet`,
`_lean_lists`, `_lean_entries` and `_hierarchy` (`nds_renderer_native_*.c`, 953 to 3,960 lines,
each headed by what it holds). The renderer unit preprocesses byte-identically in the five configs.
Host tests read a split file whole through `source_test_helpers.source()`, whose `SPLIT_SOURCES`
lists each file's slices and checks the parent still includes them in that order; the entry-effect
checkers read `nds_renderer_native_entry.c`, and `admit_fighter.py` writes its dense-normal rows to
`_dl_walk` and its owner tables to `_hierarchy`.

`reloc_backend_assets.c` (18,246 lines) keeps its first 1,269 (the loaded-file registry, the
scene file buffer, the zero-copy motion hits) and continues in seven slices `reloc_backend.c`
includes after it: `reloc_backend_symbol_tables.c` (probes, Sprite manifests, known symbols),
`_asset_ids.c`, `_resolve.c`, `_normalize.c`, `_extern_trees.c`, `_motion_cache.c` and
`_force_load.c` (1,378 to 3,545 lines). `scene_backend.o` preprocesses byte-identically in the five
configs. Tools that edit or scan the backend read it whole: `stage_reloc_file.py` through
`read_for_edit()` / `write_for_edit()` (a marker line before each slice; each slice is written back
to its own file), `mf_corpus.py` and `audit_mn_screen_coverage.py` through `source()`. The defines
`generate_fighter_production_manifest.py` checks during make stay in the first file.

## The P1/P2 verifier framework -- Phase E of the readability pass (2026-10-22)

Dormant since September, owner OK: 33 scripts go. The drivers (`verify-all.ps1`,
`verify-dev-fast.ps1`, `verify-boundary.ps1`, `verify-current.ps1`), their checks of the tree
(`check-docs.ps1`, `check-architecture.ps1`, `check-harness-registry.ps1` and the registry
`lib/harness-registry.ps1`), the nine `verify-battle-playable-*` verifiers, and what only they
ran: `assert-melonds-horizontal-detail.ps1`, `check-generator-staleness.ps1`,
`check-mn-screen-coverage.ps1` (a wrapper of `test_mn_screen_coverage.py`'s audit),
`check-one-minute-match-verifier.ps1`, the four `check-p2-*` source checks and
`check_p2_link_weapon_attributes.py`, `check-published-roms.ps1`, the renderer parity corpus
(its check, microbench, generator and generated header), `check_preview_pack_owner_sizes.py` and
the obsolete `lib/snapshot-hygiene.ps1`. Found by a reference graph: a script went when every
script, Makefile rule, source or live doc that names it was itself going.

Kept, because they check live code and pass: `check-fighter-matrix-angle-index.ps1`,
`check-fighter-production-manifest.ps1`, `check-mp-topology-fixtures.ps1`,
`check-r2-collision-mtx.ps1`, `check-pupupu-water-aot.ps1`,
`check-toolchain-path-normalization.ps1` and `check-untracked-dependencies.py`. Kept but
failing, for H-2 to fix or retire: `check-mp-floor-crossing-fixtures.ps1` (its kernel was
renamed) and `check-ft-hitstatus-fixtures.ps1` (Fox's animation token table). The generators the
staleness sweep named stay. `make clean-generated` loses `-KeepLatestBuilds` (the registry's
Boundary plan); `admit_fighter.py` no longer patches the architecture allowlist.
`docs/VERIFYING.md` keeps its live procedure; `docs/HARNESSES.md` goes.

## Still to do

- `check-gbi-decode-fixtures.ps1`, the framework's last file: it reads the deleted verifiers'
  text and fails, but the owner's worktree holds it modified, so it stays until that is clean.
- The legacy fighter executor's last trace: its code went in `1c8636a6632`; the
  `NDS_FIGHTER_LEGACY_EXEC` knob (Makefile override, config header row, bench line) and the two
  constant `#error` guards it left in `nds_renderer.h` go with the Makefile's dead knobs (G).
