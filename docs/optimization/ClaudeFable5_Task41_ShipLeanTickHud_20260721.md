# TASK 41 — True ship configuration + profile-0 per-owner tick HUD

**Standing rules apply in full: read `docs/optimization/TASK_STANDING_RULES.md`
first.** Run only after the Task 36 session is fully closed (branch settled).

## Why (review-validated)

`docs/optimization/reviews/gpt56.txt` P0-2/P0-3 + P1-2/P1-4/P1-5/P1-6 and
`docs/optimization/reviews/gpt56_2.txt` §4/§7/§8/§11-P0: the shipping profile-0
frame still pays every frame for renderer telemetry reset/publication, EndFrame
GX status/usage register reads, native-OAM semantic hashing and timing,
lower-screen HUD state fingerprinting, repeated zeroing of large
`NDSRendererStats` objects, and 64-bit rate-marker math in
`ndsAudioBgmUpdateRateMarkers` (src/nds/nds_audio_bgm.c:308) on every update.
It ALSO pays at UPDATE cadence (twice per present — planner-verified 7/21) for:
- the Fox AI recorder: `gNdsFTComputerProcessCount++; ndsFTComputerRecord(fp);`
  unconditional at src/import/battleship_ftcomputer.c:165-166 (~130-line
  observer: float conversions, collision-entry loop, volatile stores);
- fighter status/anim-event diagnostic wrappers: the production
  `ftMainSetStatus` IS the wrapper (src/port/reloc_backend_diagnostic_recorders.c:4463)
  and `ftMainPlayAnimEventsAll` has an always-on recorder;
- battle-update wrapper proof counters and harness-mode dispatch around
  `scVSBattleFuncUpdate` (src/port/taskman_seam.c:4353; verify which dispatch
  chain actually executes per update in mode 163);
- taskman per-update proof bookkeeping and FGM proof counters.
None of it changes pixels. Tyler independently requested the same thing plus a
realtime per-owner tick HUD that does not require profile 1 (profile-1 ROMs
distort timing and cannot run affine without the arena OOM).

## Deliverable: two profile-0 variants

- **LEAN (published):** zero diagnostics except (a) the FPS/UP HUD, (b) the
  pacing interval histogram increments and shared engagement counters — these
  are the device-evidence backbone and MUST survive (they are cheap RAM adds).
- **TICKHUD (new target, e.g. `smash64ds-battle-playable-tickhud-hwtri`):** LEAN
  plus a lightweight per-owner tick HUD (below). Profile 1/2 remain unchanged
  for forensic work.

## Steps

1. **Inventory first.** Table of every profile-0-active diagnostic: symbol/site,
   what it writes/reads (volatile publications, GX status reads, hashes,
   memsets, 64-bit divides), and an estimated per-frame cost. Cite file:line for
   each. This table is the checklist for step 2 and the report's core.
2. **Gate them.** New `NDS_SHIP_TELEMETRY` (default 1 = today's behavior; LEAN
   builds set 0; gpt56_2 calls the same concept NDS_RUNTIME_PROOF_DIAGNOSTICS —
   ONE switch, use the NDS_SHIP_TELEMETRY name). Behind it: telemetry
   publication/reset, GX status/usage reads, OAM semantic hash+timing, HUD
   fingerprinting, stats zeroing beyond what the active profile consumes,
   BGM/FGM rate-marker publication, the Fox AI recorder, the fighter
   status/anim-event recorders, and battle-update proof counters.
   **SAFETY GATE for `ftMainSetStatus`:** its before-hook can SUPPRESS the
   original call — before bypassing the wrapper in LEAN, prove with one
   instrumented soak (counter on the suppress path) that the hook never fires
   in profile-0 gameplay; if it ever fires, keep the hook's functional branch
   and strip only the recording. Same proof discipline for any hook that can
   alter control flow. NOT behind it:
   pacing histogram, engagement counters, FPS sampler, freeze-diagnostics entry
   points that are already compile-time-off. Every verifier script that
   GDB-reads a gated counter must keep a build that still has it (diagnostic
   targets keep NDS_SHIP_TELEMETRY=1) — list which scripts read what.
3. **Tick HUD.** `NDS_TICK_HUD` flag (LEAN=0, TICKHUD=1): cpuGetTiming() reads
   at the existing phase boundaries, accumulated into Tyler's nine buckets —
   ALL, Mario+Fox, Stage, Background(wallpaper), Audio, HUD(OAM/interface),
   SourceUpd, MiscDraw(residual+gxFlush), Other(=ALL − sum, so nothing hides).
   Present as rows on the lower screen at the existing half-second HUD cadence
   (current | rolling mean, same format as the phase HUD rows). Measured
   overhead budget: ≤3K ticks/frame — measure and report it honestly.
4. **Build matrix + identity.** Published target becomes LEAN; add TICKHUD
   target to the standard build set. The published ROM bytes change, so update
   the publish identity expectations (DECOMP_PIN OUTPUT_* / README expected
   SHA-256) in the same change — otherwise stranger builds fail their identity
   gate.
5. **Measure the win.** melonDS synchronized A/B, LEAN vs current published
   config: whole-loop and typed owner deltas + calibration-predicted device
   delta. Queue the retail pair in `builds/device-queue/`.

## Gates

- Gameplay verifiers green; synchronized screenshots pixel-identical (stripping
  telemetry must not change rendering).
- Pacing histogram + engagement counters verified still functional in LEAN
  (GDB-read them in one melonDS run — this is a hard gate, not optional).
- TICKHUD bucket sum-check: named buckets + Other ≈ ALL within ~1%.
- verify chain green; full-match soak on both variants.

**Kill criterion:** if the inventory's realistic aggregate is < ~10K/frame,
report the table and stop before building the flag plumbing — the tick HUD
half still proceeds (it is wanted regardless of the telemetry win).
