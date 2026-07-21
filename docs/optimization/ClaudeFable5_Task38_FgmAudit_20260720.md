# TASK 38 — FGM audit: missing Up-B/Down-B sounds + incorrect hit sounds

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.**

**SEQUENCING:** independent of Tasks 36/37 (audio side only: the pack script,
src/nds/nds_audio_fgm.c, and its seam callsites — no linker or renderer overlap).
Run it in its own session; do not run concurrently with the active Task 36 session
in the same worktree (one writer).

## Reported symptoms (Tyler, 2026-07-20)

1. Up-B and Down-B specials play NO sound (verify per fighter, and cover neutral-B
   in the census too).
2. Hurt/hit sounds play, but some are wrong / sound different from the original.

This is a correctness task, not perf. BGM is out of scope (owned elsewhere). Audio
verdicts: counters and manifests prove a sound was REQUESTED and a channel STARTED;
per standing rules only Tyler's listen pass proves it sounds right — never report
symptom 2 fixed from counters alone. melonDS audio output is audible to Tyler, so
the listen loop runs in melonDS; device confirmation folds into the next batched
retail checkpoint, not its own session.

## Architecture facts (verify, then lean on them)

- FGM effects are AOT-rendered by scripts/render-audio-fgm-phase-pack.py into a
  resident IMA-ADPCM pack played on hardware channels
  (src/nds/nds_audio_fgm.c:926, SoundFormat_ADPCM). The script is
  articulation-faithful (UCD programs, tick schedules, pitch cents, volume ramps)
  and heavily oracle-checked.
- The script has a FORMAL EXCLUSION mechanism: `excluded_hit_source_audit`
  (script :1629) emits entries with `runtime_included: False` +
  `runtime_excluded_reasons`; included entries can carry `omitted_fork_programs` —
  multi-voice FGMs where only the root voice was rendered and the other layered
  voices were dropped, with pinned hashes (:1640-1649).
- Request-side counters already exist: `gNdsAudioFgmPlayCalls`
  (nds_audio_fgm.c:77, ticks at :879 on every request) vs
  `gNdsAudioFgmSupportedPlayCount` (:78, ticks at :1018 only when the id resolved
  to a pack entry). The delta is requested-but-unsupported — but anonymous.
- Seam callers: src/port/taskman_seam.c and src/port/reloc_backend_compat_shims.c.

Working hypotheses to confirm or kill: symptom 1 = special-move FGM ids
runtime-excluded / absent from the selector list / requests never reaching the
seam; symptom 2 = layered hit FGMs playing only the root voice (omitted forks)
and/or articulation parameter deltas.

## Phase A — inventory (static, no build)

1. Locate the selector manifest the script consumes. Commit an artifact table:
   every FGM id → name, included/excluded, `runtime_excluded_reasons`,
   `omitted_fork_programs`, `source_callsites`.
2. From `source_callsites` + the decomp (read-only), name which engine path
   requests each id — especially Mario/Fox Up-B and Down-B sound events (these may
   come from animation-command streams rather than C callsites; say which).

## Phase B — runtime request census (decisive for symptom 1)

1. Add a per-id miss ring (last ~16 unsupported fgm_ids + counts; volatile,
   GDB-readable, profile-agnostic like the pacing histogram) so the anonymous
   PlayCalls/Supported delta names its ids.
2. Scripted melonDS run driving BOTH fighters through neutral-B, Up-B, Down-B,
   jumps, taunt, hits at low/mid/high damage, shield, KO, star KO (reuse the
   natural-motion input infra). For each expected sound event, record which case
   holds:
   (a) request arrived, id missing from pack → Phase C;
   (b) request never arrived (PlayCalls didn't tick) → upstream seam gap: find the
       original call path and wire it, citing decomp file:line for the source
       behavior (uncited seam changes get reverted);
   (c) request arrived and played → symptom-2 candidates for Phase D.
3. Commit the census table next to the Phase A artifact.

## Phase C — add the missing sounds

1. Flip stale exclusions to included; add selectors for never-listed ids following
   the script's existing validation contracts. Do not weaken or bypass a validator
   to make an entry fit; cite source for any expectation you touch.
2. Regenerate the pack; report the size delta (~128 KiB resident today).
   HARD GATE: after boot the adaptive taskman arena
   (src/port/diagnostics.c:7306-7350) must still reach full size — a degraded
   arena is a failed gate even if the ROM boots (the affine-OOM lesson). If the
   grown pack threatens the margin, stop and report numbers.
3. Re-run the Phase B census: miss ring EMPTY across the full scripted scenario;
   Up-B/Down-B show request→supported→channel-start for both fighters.

## Phase D — the "sounds different" set (symptom 2)

1. For every hit/hurt id Tyler flags plus every id with `omitted_fork_programs`:
   use the script's own audit output to state WHAT differs from source (omitted
   fork voices? pitch/envelope approximation? resample to FGM_OUTPUT_RATE?).
   Commit that table — the honest menu of what "different" means.
2. Fix the fork-omission class first (most likely audible): if fork voices start
   simultaneously with the root (check tick schedules), pre-MIX them into one
   rendered sample at AOT time — exact, zero runtime cost, no extra channels. If
   offset, report channel-budget math (BGM + worst-case concurrent FGM vs 16
   hardware channels) before choosing multi-channel playback; never spend channels
   silently.
3. Tyler's listen kit: ONE melonDS-ready ROM + a one-page checklist
   (move → what changed → expected difference), one pass. Batch every audio change
   into that single pass.

## Gates

- Standing verify chain green. BEFORE changing any FGM counter/mask semantics,
  grep scripts/ for every counter touched (`gNdsAudioFgmPhasePlayMask`, KO masks,
  etc. are verifier-read) and update expectations in the same commit, cited.
- Phase A/B/D artifacts committed; miss ring empty on the final ROM.
- Pack regeneration changes ROM bytes: update the expected-output identity
  everywhere the publish pipeline pins it (README expected SHA-256, DECOMP_PIN
  output fields, publish manifest) in the SAME commit.
- Full-match soak: no new `gNdsAudioFgmPlayFailCount`, no ARM7 ACK stalls
  (NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS available for a probe build if one appears).
- Final report: symptom 1 closed by counters + Tyler confirmation; symptom 2 items
  listed fixed/deferred per Tyler's listen pass — his ear is the oracle, say so.
