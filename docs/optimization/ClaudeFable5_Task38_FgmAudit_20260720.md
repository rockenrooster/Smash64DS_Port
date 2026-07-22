# TASK 38 — FGM coverage: audit ✔ done, fix RESUMED (no cue stays excluded)

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first — including the content-completeness doctrine (Tyler, 2026-07-21) this
resumption enforces.**

**STATUS (2026-07-21): PHASE E CACHE CANDIDATE BUILT; TYLER LISTEN PENDING.** The audit phases ran and are CLOSED — evidence at
`artifacts/performance/2026-07-21_task38-fgm-audit.md` (manifest inventory,
special-move source census, per-id miss ring landed and verifier-exported).
Consume that evidence; do not redo it. The original Phase C capacity stop-gate is
REMOVED by Tyler's mandate: "I don't want to formally exclude necessary SFX
assets from the game. We need to fix it now." Resume from Phase C below. The
original goal stands: implement the missing/incorrect sounds — audit alone does
not close this task.

Tyler authorized the recommended exact on-demand cache on 2026-07-21. The
runtime now exposes all 49 required IDs through a 204,800-byte, eight-slot
reference-counted NitroFS cache; no cue remains excluded. The regenerated pack
is 415,432 bytes (`8025f6bf...b1e797d`). The seven hit/KO fork repairs are fused
from their simultaneous source programs. The focused generator/cache gate and
profile-0 build pass. A single listen pass remains; AL_FX_CUSTOM cues currently
use their source-program dry render while the wet delay tail is the only named
quality lever still awaiting Tyler's judgment.

The corrected Phase C lower bound is recorded at
`artifacts/performance/2026-07-21_task38-full-coverage-cost.md`. Pack
deduplication makes the seven fused repairs add 99,908 bytes, not 72,260. With
every other missing cue in its cheapest shared dry-source form, the projected
pack floor is 300,540 bytes and post-Task-42 reserve is at most 40,920 bytes
before schedules or exact FX tails. No cap changed; Tyler's representation
decision is required.

**SEQUENCING:** independent of Tasks 39/41/43/44 (no file overlap) and of Task 42
(which owns BGM; do not touch the BGM stream). Own session, one writer.

## Mandate and end state

Every battle-reachable FGM cue plays its EXACT source sound. Substitution stays
banned. An id that still cannot ship at the end must be listed with its reason
and get Tyler's explicit per-item sign-off in the report — silent or unilateral
exclusion is a failed task.

## Inputs you already have (from the closed audit + the tree)

- The excluded set with reasons, source callsites, per-move ids: specials Mario
  217 (Up-B), 218/219 (Down-B), Fox 185 (neutral-B), 186/187 (Up-B start/fly),
  189 (Down-B start), 190 (AttackAirLw); voices 375/429/431/435/440;
  attack/activation 19/41/42/43; omitted fork programs 653-658/685 behind hits
  31/32/34/37/38/40 and 154. (Mario neutral-B 215 already included.)
- Pack state: 27 entries, 128,196 / 131,072 bytes, headroom 2,876. The cap is a
  BUDGET constant, not hardware: `MAX_RESIDENT_BYTES = 128 * 1024`
  (scripts/render-audio-fgm-phase-pack.py:30) with a runtime counterpart sizing
  the resident buffer in src/nds/nds_audio_fgm.c — find it and keep both in
  lockstep; a script/runtime mismatch must fail the build or load loudly, never
  truncate.
- Machinery already in embryo (extend, don't reinvent): loop encode + repeat
  oracle (`ima_encode_loop_body`, `ima_ds_repeat_cycles`, `ima_repeat_oracle`,
  script :1788-1900); AOT-baked modulation (`render_modulated_voice_aot` :2256);
  fused-fork form already sized for id 34 (audit capacity note); per-call
  playback rate (soundPlaySample takes a rate — BGM precedent
  nds_audio_bgm.c:419-426) vs the script's single `FGM_OUTPUT_RATE = 32000`
  (:29); runtime stop/handle API (`ndsAudioFgmStop`/`StopAll`, bounded handles).
- RAM context: net reserve 166,672 bytes on the current ROM; the adaptive
  taskman arena (src/port/diagnostics.c:7306-7350) must always reach full size.

## Phase C — cost the whole problem before touching the cap

1. For EVERY excluded battle-reachable id: compute the exact resident byte cost
   in its cheapest EXACT form (hardware-loop form where the source loops — loop
   body + loop point is usually SMALLER than a baked tail; fused-dry fork where
   forks are simultaneous; per-entry output rate where the source rate demands
   it) plus its representation class (loop / schedule / fork / rate / overlap).
2. Deliver a costed coverage table and the total: proposed new
   `MAX_RESIDENT_BYTES`, projected pack bytes, projected net reserve.
3. STOP-AND-ASK line: if full exact coverage would push net reserve below
   96 KiB or shrink the arena, stop and present Tyler the numbers with fallback
   levers (per-entry output-rate reduction on named low-salience entries; other
   options he might weigh). Do not pick a fidelity tradeoff for him.

## Phase D — representation extensions (each oracle-gated like the existing ones)

Priority order; extend the script's validator/oracle culture to each — an
extension without its oracle is not done:

1. **Per-entry output rate** — lift the single-rate assumption; store rate per
   entry; runtime passes it per channel start.
2. **Hardware-looped entries** — encode loop body + predictor-exact loop point
   (the repeat oracle proves the seam); runtime plays looped channels and STOPS
   them on the owning move/status end. Cite the source stop condition
   (decomp file:line) per looped cue — Fox Up-B fly (187) and similar must end
   exactly when the source ends them, wired through the existing handle API.
3. **AOT-baked pitch/volume schedules** — extend `render_modulated_voice_aot`
   coverage to the excluded schedule classes with terminating schedules.
4. **Fused forks** — productionize the id-34 fused-dry path for simultaneous
   forks (this also retires the hit-sound fork debt: 653-658, 685). Offset
   forks → multi-channel only with a worst-case channel-budget table (16
   hardware channels; BGM holds one; show the concurrent worst case) — never
   spend channels silently.
5. **Overlap/handles** — multi-instance handles where the source overlaps the
   same cue; cite the source overlap behavior per id.

## Phase E — enable everything, regenerate, prove engagement

1. Flip every battle-reachable exclusion to included, specials first (Tyler's
   reported symptom), then voices, then attack/activation, then the fork-debt
   hits. Raise the cap constants in lockstep per Phase C's number.
2. Regenerate the pack. HARD GATES: boot with the arena at FULL size (a degraded
   arena fails the task even if the ROM boots); report final pack bytes, cap,
   and net reserve.
3. Re-run the scripted census (both fighters: all specials, jumps, taunt, hits
   at low/mid/high damage, shield, KO, star KO): the miss ring must be EMPTY
   end-to-end this time. Flip the Boundary harness expectation that the audit
   deliberately left permissive
   (`verify-battle-mariofox-gcrunall-loop-harness.ps1`); per standing rules,
   grep scripts/ for every counter/mask whose semantics you touch and update
   expectations in the same commit, cited.

## Phase F — Tyler's listen pass and sign-off

1. ONE melonDS-ready ROM + one checklist covering every new/changed cue: each
   special per fighter, the six fork-fixed hit sounds, the damage/jump voices.
   Batch everything into a single pass; device confirmation folds into the next
   batched retail checkpoint per standing rules.
2. Final report table: every formerly excluded id → shipped-exact | shipped with
   named fidelity lever (Tyler-approved in Phase C) | still excluded with
   Tyler's explicit sign-off. No fourth category exists.

## Gates

- Standing verify chain green; publish identity pins (README expected SHA-256,
  DECOMP_PIN output fields, publish manifest) updated in the SAME commit as the
  pack regeneration.
- Full-match soak: no new `gNdsAudioFgmPlayFailCount`, no ARM7 ACK stalls
  (NDS_AUDIO_FGM_ARM7_ACK_DIAGNOSTICS probe build if one appears), BGM
  untouched.
- Commit the Phase C costed table and Phase E census as artifacts next to the
  existing audit evidence.

## Closeout — 2026-07-21 (Tyler sign-off)

Tyler's listen pass on ROM `3D5C18FB...` (post cache-init fix `5c278710d`)
APPROVED all cues — formerly missing specials, fused-fork layered hits, KO
sequence — **including explicit sign-off on the dry AL_FX_CUSTOM wet tails**
(the one named fidelity lever; debt mask bit stays documented, no further
approval needed). The production silence bug (cache slot table initialized
only on the diagnostics path) is fixed at the load fence. Runtime verifier
fully green: 64 supported plays / 0 failures / phase mask 0x1f / exact KO trio.
Shipped by Tyler's override with the frozen-water diagnostic regression
tracked separately (it pre-dates and is unrelated to the audio work).
