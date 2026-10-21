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

## Still to do

- The P1-family targets (`smash64ds-battle-playable-*`) and the scripts only they use: with the
  branches above gone they build the production renderer anyway. The results lab
  (`smash64ds-results-lab-hwtri`) is being checked on the production path.
